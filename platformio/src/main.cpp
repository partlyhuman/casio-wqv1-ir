#include "config.h"
#include "frame.h"

void setup() {
    Serial.begin(115200);
    delay(100);
    while (!Serial);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // Shutdown pin, could also tie to ground
    pinMode(PIN_SD, OUTPUT);
    digitalWrite(PIN_SD, LOW);
    delay(1);

    IRDA.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    IRDA.setMode(UART_MODE_IRDA);
    while (!IRDA);

    log_i("Setup complete");
}

struct {
    char name[24];  // space padded
    unsigned char year_minus_2000, month, day;
    unsigned char minute, hour;

    unsigned char pixel[120 * 120 / 2];  // one nibble per pixel
};

constexpr size_t BUFFER_SIZE = 1024;
static uint8_t readBuffer[BUFFER_SIZE];
uint8_t sessionId = 0xff;
size_t len;
size_t dataLen;
uint8_t addr;
uint8_t ctrl;

static inline bool expect(uint8_t expectedAddr, uint8_t expectedCtrl, int expectedMinLength = -1) {
    if (addr != expectedAddr) {
        log_d("Expected addr=%02x got addr=%02x\n", expectedAddr, addr);
        return false;
    }
    if (ctrl != expectedCtrl) {
        log_d("Expected ctrl=%02x got ctrl=%02x\n", expectedCtrl, ctrl);
        return false;
    }
    if (expectedMinLength >= 0 && len < expectedMinLength) {
        log_d("Expected at least %d bytes of data, got %d\n", expectedMinLength, len);
        return false;
    }
    return true;
}

bool readFrame() {
    len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
    if (len == 0) {
        log_w("Timeout");
        return false;
    }
    return Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl);
}

bool sendRetry(uint8_t a, uint8_t c, const uint8_t *d = nullptr, size_t l = 0, int retries = 5) {
    for (int retry = 0; retry < retries; retry++) {
        Frame::writeFrame(a, c, d, l);
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
        if (len <= 0) {
            log_w("Timeout, retrying %d...", retry);
            continue;
        }
        if (!Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl)) {
            log_w("Malformed, retrying %d...", retry);
            continue;
        }
        return true;
    }
    return false;
}

bool handshake() {
    log_i("\n\n--- HANDSHAKING ---");
    do {
        // >	FFh	B3h	(possibly repeated)
        Frame::writeFrame(0xff, 0xb3);
        if (!readFrame()) continue;
        // <	FFh	A3h	<hh> <mm> <ss> <ff>
    } while (!expect(0xff, 0xa3, 4));

    // Generate session ID
    sessionId = 4;
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

    log_i("Handshake established!");
    return true;
}

bool prepareForUpload() {
    log_i("--- UPLOAD ---");

    // >	<adr>	10h	01h
    static const uint8_t cmd0[] = {0x1};
    IRDA.flush();
    sendRetry(sessionId, 0x10, cmd0, sizeof(cmd0));
    // <	<adr>	21h
    if (!expect(sessionId, 0x21)) return false;
    // >	<adr>	11h
    sendRetry(sessionId, 0x11);
    // <	<adr>	20h	07h FAh 1Ch 3Dh 01h
    if (!expect(sessionId, 0x20, 5)) return false;
    // >	<adr>	32h	06h
    static const uint8_t cmd1[] = {0x6};
    sendRetry(sessionId, 0x32, cmd1, sizeof(cmd1));
    // <	<adr>	41h
    if (!expect(sessionId, 0x41)) return false;

    log_i("Upload preamble completed!");
    return true;
}

void loop() {
    if (!handshake()) {
        delay(10000);
        return;
    }
    if (!prepareForUpload()) {
        delay(10000);
        return;
    }
}
