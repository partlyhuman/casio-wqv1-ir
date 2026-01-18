uint8_t sessionId;

static uint8_t readBuffer[1024];

void connect() {
  sessionId = 0xff;

  size_t len;

  do {
    IRDA.flush();
    // Possibly repeated
    // delay(100);

    writeFrame(0xff, 0xb3, nullptr, 0);

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