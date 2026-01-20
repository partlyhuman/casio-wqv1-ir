#include "FFat.h"
#include "config.h"
#include "esp32-hal-psram.h"
#include "frame.h"
#include "image.h"
#include "log.h"
#include "memstream.h"
#include "msc.h"

static const char *TAG = "Main";
static const char *DUMP_PATH = "/dump.bin";

// Packets seem to be up to 192 bytes
constexpr size_t BUFFER_SIZE = 256;
static uint8_t readBuffer[BUFFER_SIZE];
void *psramBuffer;
size_t psramBufferLen;
uint8_t sessionId = 0xff;
size_t len;
size_t dataLen;
uint8_t addr;
uint8_t ctrl;

void onButton();

void setup() {
    Serial.begin(115200);
    delay(100);
    // Doing this means it doesn't start until serial connected?
    // while (!Serial);

    psramInit();

    MassStorage::init();
    Image::init();

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LED_OFF);

    // Probably remove this for production
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(PIN_BUTTON, onButton, FALLING);

    // Shutdown pin, tied to ground now
    // pinMode(PIN_SD, OUTPUT);
    // digitalWrite(PIN_SD, LOW);
    // delay(1);

    IRDA.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    IRDA.setMode(UART_MODE_IRDA);
    while (!IRDA);

    LOGI(TAG, "Setup complete");
}

static inline bool expect(uint8_t expectedAddr, uint8_t expectedCtrl, int expectedMinLength = -1) {
    if (addr != expectedAddr) {
        LOGD(TAG, "Expected addr=%02x got addr=%02x\n", expectedAddr, addr);
        return false;
    }
    if (ctrl != expectedCtrl) {
        LOGD(TAG, "Expected ctrl=%02x got ctrl=%02x\n", expectedCtrl, ctrl);
        return false;
    }
    if (expectedMinLength >= 0 && len < expectedMinLength) {
        LOGD(TAG, "Expected at least %d bytes of data, got %d\n", expectedMinLength, len);
        return false;
    }
    return true;
}

bool sendRetry(uint8_t a, uint8_t c, const uint8_t *d = nullptr, size_t l = 0, int retries = 5) {
    for (int retry = 0; retry < retries; retry++) {
        Frame::writeFrame(a, c, d, l);

        digitalWrite(PIN_LED, LED_ON);
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
        digitalWrite(PIN_LED, LED_OFF);

        if (len <= 0) {
            LOGW(TAG, "Timeout, retrying %d...", retry);
            continue;
        }
        if (len == BUFFER_SIZE) {
            LOGE(TAG, "Filled buffer up all the way, probably dropping content");
            return false;
        }

        if (!Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl)) {
            LOGW(TAG, "Malformed, retrying %d...", retry);
            continue;
        }
        return true;
    }
    return false;
}

bool openSession() {
    LOGI(TAG, "\n\n--- HANDSHAKING ---");

    sessionId = 0xff;

    // >	FFh	B3h	(possibly repeated)
    sendRetry(0xff, 0xb3, nullptr, 0, 1);
    // Frame::writeFrame(0xff, 0xb3);
    // if (!readFrame()) return false;
    // <	FFh	A3h	<hh> <mm> <ss> <ff>
    if (!expect(0xff, 0xa3, 4)) return false;

    // Generate session ID
    sessionId = rand() % 0xff;
    // echo back data adding the session ID to the end <hh> <mm> <ss> <ff><assigned address>
    readBuffer[4] = sessionId;

    do {
        // >	FFh	93h	<hh> <mm> <ss> <ff><assigned address>
        sendRetry(0xff, 0x93, readBuffer, 5, 1);
        // <	<adr>	63h	(possibly repeated)
    } while (!expect(sessionId, 0x63));

    do {
        // >	<adr>	11h
        sendRetry(sessionId, 0x11);
        // <	<adr>	01h
    } while (!expect(sessionId, 0x01));

    LOGI(TAG, "Handshake established!");
    return true;
}

bool downloadToFile(size_t imgCount, Stream &stream) {
    // NOTE - multi image downloads DO cross image boundaries, so doing it one at a time is no good
    constexpr size_t IMAGE_SIZE = sizeof(Image::Image);

    uint8_t getPacketNum = 0x31;
    uint8_t retPacketNum = 0x42;
    for (size_t offset = 0; offset < IMAGE_SIZE * imgCount;) {
        sendRetry(sessionId, getPacketNum);
        if (!expect(sessionId, retPacketNum)) {
            LOGI(TAG, "Mismatched Send %02x expect ret %02x, retrying", getPacketNum, retPacketNum);
            continue;
        }
        if (readBuffer[0] != 0x05) {
            LOGW(TAG, "Expected data to start with 0x05, retrying");
            continue;
        }

        // skip the initial 0x05
        size_t packetLen = dataLen - 1;
        stream.write(readBuffer + 1, packetLen);
        offset += packetLen;

        // increment packet ids
        getPacketNum = getPacketNum + 0x20;  // natural wraparound 0xF1 + 0x20 = 0x11
        retPacketNum += 0x2;
        if (retPacketNum >= 0x50) retPacketNum = 0x40;  // cycles 40-4E,40-4E...

        int curImg = offset / IMAGE_SIZE;
        Serial.printf("Progress: image %d/%d\t| %d bytes\t| %0.0f%%\n", curImg + 1, imgCount, offset,
                      100.0f * offset / imgCount / IMAGE_SIZE);
    }

    LOGI(TAG, "Done reading all images!");
    return true;
}

bool downloadImages() {
    LOGI(TAG, "--- UPLOAD ---");

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
    size_t imgCount = readBuffer[4];
    LOGI(TAG, "Watch says %d images available", imgCount);
    // >	<adr>	32h	06h
    static const uint8_t args_6[] = {0x6};
    sendRetry(sessionId, 0x32, args_6, sizeof(args_6));
    // <	<adr>	41h
    if (!expect(sessionId, 0x41)) return false;

    LOGI(TAG, "Upload preamble completed!");

    // Start every session with a fresh disk,
    // but don't wipe until we are just about to save, in order to allow access to previous export
    FFat.end();
    FFat.format(false);
    FFat.begin();

    if (psramFound() && psramFound) {
        LOGD(TAG, "Using psram");
        psramBufferLen = imgCount * sizeof(Image::Image);
        psramBuffer = ps_malloc(psramBufferLen);
        if (psramBuffer == nullptr) {
            LOGE(TAG, "Failed to allocate %d bytes of PSRAM", psramBufferLen);
            return false;
        }
        WriteBufferStream stream(psramBuffer, psramBufferLen);
        downloadToFile(imgCount, stream);
    } else {
        File dump = FFat.open(DUMP_PATH, FILE_WRITE, true);
        if (!dump) return false;
        downloadToFile(imgCount, dump);
        dump.close();
    }

    return true;
}

bool closeSession() {
    LOGI(TAG, "Closing session...");
    // >	<adr>	54h	06h
    static const uint8_t args_6[] = {0x6};
    sendRetry(sessionId, 0x54, args_6, sizeof(args_6));
    // <	<adr>	61h // NOTE this seems incorrect, using 0x43 matching observed behaviour
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
        if (openSession()) {
            if (downloadImages()) {
                closeSession();
                if (psramBuffer != nullptr) {
                    ReadBufferStream file(psramBuffer, psramBufferLen);
                }
                Image::exportImagesFromDump(DUMP_PATH);

                Serial.println("\n\nAttaching mass storage device, go look for your images!");
                Serial.println("Don't forget to eject when done!");

                Serial.flush();
                mscMode = true;
                MassStorage::begin();
            }
        } else {
            delay(1000);
        }
    }
}
