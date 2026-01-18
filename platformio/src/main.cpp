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

    // pinMode(PIN_RX, INPUT);
    // pinMode(PIN_TX, OUTPUT);

    // Begin, setPins, setMode ordering?
    IRDA.begin(115200, SERIAL_8N1, PIN_RX, PIN_TX);
    if (!IRDA.setPins(PIN_RX, PIN_TX)) {
        Serial.println("Failed to set pins");
        digitalWrite(LED_BUILTIN, LOW);
    }
    if (!IRDA.setMode(UART_MODE_IRDA)) {
        Serial.println("Failed setting IRDA mode");
        digitalWrite(LED_BUILTIN, LOW);
    }

    while (!IRDA);

    Serial.write("Done setup!");
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
        Serial.printf("Expected addr=%02x got addr=%02x\n", expectedAddr, addr);
        return false;
    }
    if (ctrl != expectedCtrl) {
        Serial.printf("Expected ctrl=%02x got ctrl=%02x\n", expectedCtrl, ctrl);
        return false;
    }
    if (expectedMinLength >= 0 && len < expectedMinLength) {
        Serial.printf("Expected at least %d bytes of data, got %d\n", expectedMinLength, len);
        return false;
    }
    return true;
}

void connect() {
    log_i("\n\n--- HANDSHAKING ---");
    do {
        Frame::writeFrame(0xff, 0xb3);
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
    } while (len < Frame::FRAME_SIZE);
    Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl);
    if (!expect(0xff, 0xa3, 4)) {
        log_e("FAIL: Expected reply of FFh A3h <hh> <mm> <ss> <ff>");
        return;
    }

    // Generate session ID
    sessionId = 4;
    // echo back data adding the session ID to the end <hh> <mm> <ss> <ff><assigned address>
    readBuffer[4] = sessionId;

    IRDA.flush();
    Frame::writeFrame(0xff, 0x93, readBuffer, 5);
    do {
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
    } while (len == 0);
    Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl);
    if (!expect(sessionId, 0x63)) {
        log_e("Expected reply of <adr> 63h");
        return;
    }

    IRDA.flush();
    Frame::writeFrame(sessionId, 0x11);
    do {
        len = IRDA.readBytesUntil(Frame::FRAME_EOF, readBuffer, BUFFER_SIZE);
    } while (len == 0);
    Frame::parseFrame(readBuffer, len, dataLen, addr, ctrl);
    if (!expect(sessionId, 0x01)) {
        Serial.println("Expected reply of <adr> 01h");
    }

    Serial.println("Handshake established!");

    // sessionId = 4;  // randomize?
    // static const uint8_t payload[] = { 0, 0, 0, 0, sessionId };
    // writeFrame(0xff, 0x93, payload, sizeof(payload));
}

void loop() {
    connect();
    delay(10000);
}
