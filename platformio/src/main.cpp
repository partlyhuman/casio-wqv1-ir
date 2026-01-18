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

uint8_t sessionId;

static uint8_t readBuffer[1024];

void connect() {
    sessionId = 0xff;

    size_t len;

    do {
        IRDA.flush();
        // Possibly repeated
        // delay(100);

        Frame::writeFrame(0xff, 0xb3, nullptr, 0);

        // wait for: FFh,A3h,<hh> <mm> <ss> <ff>
        len = IRDA.readBytes(readBuffer, 1024);
    } while (len < 2);

    Serial.printf("Received %d bytes!\n< ", len);
    // Echo to serial
    for (size_t i = 0; i < len; i++) {
        Serial.printf("%02x", readBuffer[i]);
    }
    Serial.println();

    // sessionId = 4;  // randomize?
    // static const uint8_t payload[] = { 0, 0, 0, 0, sessionId };
    // writeFrame(0xff, 0x93, payload, sizeof(payload));
}

void loop() {
    connect();
    delay(10000);
}
