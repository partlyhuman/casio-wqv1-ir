#include "config.h"
#include "esp_log.h"
#include "frame.h"
#include "image.h"
#include "msc.h"

static const char *TAG = "Main";

void onButton();

void setup() {
    Serial.begin(115200);
    delay(1000);
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    while (!Serial);

    ESP_LOGI(TAG, "Setup ----");
    MassStorage::init();
    Image::init();

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LED_OFF);

    pinMode(PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(PIN_BUTTON, onButton, FALLING);

    // Shutdown pin, could also tie to ground
    pinMode(PIN_SD, OUTPUT);
    digitalWrite(PIN_SD, LOW);
    delay(1);

    IRDA.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    IRDA.setMode(UART_MODE_IRDA);
    while (!IRDA);

    ESP_LOGI(TAG, "Setup complete");
}

constexpr size_t BUFFER_SIZE = 1024;
static uint8_t readBuffer[BUFFER_SIZE];
uint8_t sessionId = 0xff;
size_t len;
size_t dataLen;
uint8_t addr;
uint8_t ctrl;

static inline bool expect(uint8_t expectedAddr, uint8_t expectedCtrl, int expectedMinLength = -1) {
    if (addr != expectedAddr) {
        ESP_LOGD(TAG, "Expected addr=%02x got addr=%02x\n", expectedAddr, addr);
        return false;
    }
    if (ctrl != expectedCtrl) {
        ESP_LOGD(TAG, "Expected ctrl=%02x got ctrl=%02x\n", expectedCtrl, ctrl);
        return false;
    }
    if (expectedMinLength >= 0 && len < expectedMinLength) {
        ESP_LOGD(TAG, "Expected at least %d bytes of data, got %d\n", expectedMinLength, len);
        return false;
    }
    return true;
}

bool readFrame() {
    len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
    if (len == 0) {
        ESP_LOGW(TAG, "Timeout");
        return false;
    }
    return Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl);
}

bool sendRetry(uint8_t a, uint8_t c, const uint8_t *d = nullptr, size_t l = 0, int retries = 5) {
    for (int retry = 0; retry < retries; retry++) {
        Frame::writeFrame(a, c, d, l);
        digitalWrite(PIN_LED, LED_ON);
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
        digitalWrite(PIN_LED, LED_OFF);
        if (len <= 0) {
            ESP_LOGW(TAG, "Timeout, retrying %d...", retry);
            continue;
        }
        if (!Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl)) {
            ESP_LOGW(TAG, "Malformed, retrying %d...", retry);
            continue;
        }
        return true;
    }
    return false;
}

bool openSession() {
    ESP_LOGI(TAG, "\n\n--- HANDSHAKING ---");

    sessionId = 0xff;

    // >	FFh	B3h	(possibly repeated)
    Frame::writeFrame(0xff, 0xb3);
    // if (!readFrame()) continue;
    if (!readFrame()) return false;
    // <	FFh	A3h	<hh> <mm> <ss> <ff>
    if (!expect(0xff, 0xa3, 4)) return false;

    // Generate session ID
    sessionId = rand() % 0xff;
    // echo back data adding the session ID to the end <hh> <mm> <ss> <ff><assigned address>
    readBuffer[4] = sessionId;

    IRDA.flush();
    do {
        // >	FFh	93h	<hh> <mm> <ss> <ff><assigned address>
        Frame::writeFrame(0xff, 0x93, readBuffer, 5);
        if (!readFrame()) return false;
        // <	<adr>	63h	(possibly repeated)
    } while (!expect(sessionId, 0x63));

    IRDA.flush();
    do {
        // >	<adr>	11h
        Frame::writeFrame(sessionId, 0x11);
        if (!readFrame()) return false;
        // <	<adr>	01h
    } while (!expect(sessionId, 0x01));

    ESP_LOGI(TAG, "Handshake established!");
    return true;
}

bool downloadImages() {
    ESP_LOGI(TAG, "--- UPLOAD ---");

    // >	<adr>	10h	01h
    static const uint8_t args_1[] = {0x1};
    IRDA.flush();
    sendRetry(sessionId, 0x10, args_1, sizeof(args_1));
    // <	<adr>	21h
    if (!expect(sessionId, 0x21)) return false;
    // >	<adr>	11h
    sendRetry(sessionId, 0x11);
    // <	<adr>	20h	07h FAh 1Ch 3Dh <num_images>
    if (!expect(sessionId, 0x20, 5)) return false;
    size_t numImages = readBuffer[4];
    ESP_LOGI(TAG, "Watch says %d images available", numImages);
    // >	<adr>	32h	06h
    static const uint8_t args_6[] = {0x6};
    sendRetry(sessionId, 0x32, args_6, sizeof(args_6));
    // <	<adr>	41h
    if (!expect(sessionId, 0x41)) return false;

    ESP_LOGI(TAG, "Upload preamble completed!");

    size_t numBytes = sizeof(Image::Image) * numImages;
    uint8_t *imageBytes = new uint8_t[numBytes];

    uint8_t getPacketNum = 0x31;
    uint8_t retPacketNum = 0x42;
    for (size_t writeOffset = 0; writeOffset < numBytes;) {
        sendRetry(sessionId, getPacketNum);
        if (!expect(sessionId, retPacketNum)) {
            ESP_LOGI(TAG, "Mismatched Send %02x expect ret %02x, retrying", getPacketNum, retPacketNum);
            continue;
        }
        if (readBuffer[0] != 0x05) {
            ESP_LOGW(TAG, "Expected data to start with 0x05, retrying");
            continue;
        }

        size_t copyLen = dataLen - 1;  // skip the initial 0x05
        if (writeOffset + copyLen > numBytes) {
            ESP_LOGE(TAG, "Overflow: offset=%u len=%u size=%u", writeOffset, dataLen, numBytes);
            break;
        }
        memcpy(imageBytes + writeOffset, readBuffer + 1, copyLen);
        writeOffset += copyLen;

        getPacketNum = getPacketNum + 0x20;  // natural wraparound 0xF1 + 0x20 = 0x11
        retPacketNum += 0x2;
        if (retPacketNum >= 0x50) retPacketNum = 0x40;  // cycles 40-4E,40-4E...

        // Debug - exit early
        // if (writeOffset / sizeof(Image::Image) >= 1) {
        //     ESP_LOGI(TAG, "Got 1 image, Exiting early.");
        //     numImages = 1;
        //     break;
        // }

        ESP_LOGI(TAG, "Read %d bytes, total %d bytes / %d images", dataLen, writeOffset,
                 writeOffset / sizeof(Image::Image));
    }

    ESP_LOGI(TAG, "Done reading data!");

    // TODO this should go after finishing sync but that isn't working yet
    Image::Image *images = reinterpret_cast<Image::Image *>(imageBytes);
    for (int i = 0; i < numImages; i++) {
        // Null terminate strings
        // images[i].name[23] = '\0';
        Image::debug(images[i]);
        Image::save(images[i]);
    }

    ESP_LOGI(TAG, "Completed!");

    delete[] imageBytes;
    return true;
}

bool closeSession() {
    ESP_LOGI(TAG, "Closing session...");
    // >	<adr>	54h	06h
    static const uint8_t args_6[] = {0x6};
    sendRetry(sessionId, 0x54, args_6, sizeof(args_6));
    // <	<adr>	61h
    if (!expect(sessionId, 0x43)) return false;
    // >	<adr>	53h
    sendRetry(sessionId, 0x53);
    // <	<adr>	63h
    if (!expect(sessionId, 0x63)) return false;

    sessionId = 0xff;
    return true;
}

volatile bool mscMode = false;

void onButton() {
    mscMode = !mscMode;
    Serial.printf("Changed mode -> %s\n", mscMode ? "MSC" : "IRDA");
    if (mscMode) {
        MassStorage::begin();
    } else {
        MassStorage::end();
    }
}

void loop() {
    if (mscMode) {
        yield();
    } else {
        digitalWrite(PIN_LED, LED_ON);
        if (openSession()) {
            if (downloadImages()) {
                closeSession();
                mscMode = true;
                MassStorage::begin();
            }
        } else {
            delay(1000);
        }
        digitalWrite(PIN_LED, LED_OFF);
    }
}
