#include "FFat.h"
#include "PSRamFS.h"
#include "config.h"
#include "display.h"
#include "frame.h"
#include "image.h"
#include "log.h"
#include "msc.h"

// Implementation of https://www.mgroeber.de/wqvprot.html by @partlyhuman

static const char *TAG = "Main";
static const char *DUMP_PATH = "/dump.bin";

constexpr size_t MAX_IMAGES = 100;
// Packets seem to be up to 192 bytes
constexpr size_t BUFFER_SIZE = 256;
static uint8_t readBuffer[BUFFER_SIZE];
// Whether we're streaming the transferred data into PSRAM or FAT
bool usePsram;
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

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LED_OFF);

    // Should be a little under 1mb. PSRamFS will use heap if not available, we want to prevent that though
    size_t psramSize = MAX_IMAGES * sizeof(Image::Image) + 1024;
    if (psramInit() && psramSize < ESP.getMaxAllocPsram() && psramSize < ESP.getFreePsram()) {
        LOGD(TAG, "Initializing PSRAM...");
        usePsram = PSRamFS.setPartitionSize(psramSize) && PSRamFS.begin(true);
    }
    if (!usePsram) {
        LOGD(TAG, "Using FFAT instead of PSRAM...");
    }

    MassStorage::init();
    Image::init();
    Display::init();

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
        digitalWrite(PIN_LED, LED_OFF);
        Frame::writeFrame(a, c, d, l);
        // Important: don't do any heavy work here or you'll miss the beginning of the buffer
        digitalWrite(PIN_LED, LED_ON);
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);

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
    Display::showIdleScreen();

    sessionId = 0xff;

    // >	FFh	B3h	(possibly repeated)
    sendRetry(0xff, 0xb3, nullptr, 0, 1);
    // Frame::writeFrame(0xff, 0xb3);
    // if (!readFrame()) return false;
    // <	FFh	A3h	<hh> <mm> <ss> <ff>
    if (!expect(0xff, 0xa3, 4)) return false;

    // Display::print().printf(STR_HANDSHAKING);
    // Display::update();

    // Generate session ID
    sessionId = rand() % 0x0f;
    // echo back data adding the session ID to the end <hh> <mm> <ss> <ff><assigned address>
    readBuffer[4] = sessionId;

    Display::showConnectingScreen(0);

    do {
        // >	FFh	93h	<hh> <mm> <ss> <ff><assigned address>
        sendRetry(0xff, 0x93, readBuffer, 5, 1);
        // <	<adr>	63h	(possibly repeated)
    } while (!expect(sessionId, 0x63));

    Display::showConnectingScreen(1);

    do {
        // >	<adr>	11h
        sendRetry(sessionId, 0x11);
        // <	<adr>	01h
    } while (!expect(sessionId, 0x01));

    Display::showConnectingScreen(2);

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

        // TODO abort after N failures

        // skip the initial 0x05
        size_t packetLen = dataLen - 1;
        stream.write(readBuffer + 1, packetLen);
        offset += packetLen;

        // increment packet ids
        getPacketNum = getPacketNum + 0x20;  // natural wraparound 0xF1 + 0x20 = 0x11
        retPacketNum += 0x2;
        if (retPacketNum >= 0x50) retPacketNum = 0x40;  // cycles 40-4E,40-4E...

        // int curImg = offset / IMAGE_SIZE;
        // LOGI(TAG, "Progress: image %d/%d\t| %d bytes\t| %0.0f%%", curImg + 1, imgCount, offset,
        //      100.0f * offset / imgCount / IMAGE_SIZE);
        Display::showProgressScreen(offset, IMAGE_SIZE * imgCount, IMAGE_SIZE);
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

    Display::showConnectingScreen(3);

    // >	<adr>	11h
    sendRetry(sessionId, 0x11);
    // <	<adr>	20h	07h FAh 1Ch 3Dh <num_images>
    if (!expect(sessionId, 0x20, 5)) return false;

    Display::showConnectingScreen(4);

    size_t imgCount = readBuffer[4];
    LOGI(TAG, "Watch says %d images available", imgCount);
    // >	<adr>	32h	06h
    static const uint8_t args_6[] = {0x6};
    sendRetry(sessionId, 0x32, args_6, sizeof(args_6));
    // <	<adr>	41h
    if (!expect(sessionId, 0x41)) return false;

    Display::showConnectingScreen(5);

    LOGI(TAG, "Upload preamble completed!");

    // Start every session with a fresh disk,
    // but don't wipe until we are just about to save, in order to allow access to previous export
    FFat.end();
    FFat.format(false);
    FFat.begin();

    size_t size = imgCount * sizeof(Image::Image);
    File dump;
    if (usePsram) {
        LOGD(TAG, "Using psram");
        dump = PSRamFS.open(DUMP_PATH, FILE_WRITE);
    } else {
        dump = FFat.open(DUMP_PATH, FILE_WRITE);
    }
    if (!dump) {
        LOGE(TAG, "Failed to allocate file %d bytes", size);
        return false;
    }
    downloadToFile(imgCount, dump);
    dump.close();

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
                File dump = usePsram ? PSRamFS.open(DUMP_PATH, FILE_READ) : FFat.open(DUMP_PATH, FILE_READ);
                Image::exportImagesFromDump(dump);
                dump.close();
                delay(250);  // Give the 100% screen some time to register

                Display::showMountedScreen();
                Serial.println("\n\nAttaching mass storage device, go look for your images!");
                Serial.println("Don't forget to eject when done!");

                Serial.flush();
                mscMode = true;
                MassStorage::begin();
                return;
            }
        }

        LOGE(TAG, "Failure, restarting from handshake");
        delay(1000);
    }
}
