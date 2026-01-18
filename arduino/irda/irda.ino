// #include "driver/uart.h"
// #include "hal/uart_ll.h"
#include "esp32-hal-uart.h"


#define PIN_SD 5
#define PIN_RX 20
#define PIN_TX 21

#define IRDA Serial1

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Shutdown pin, could also tie to ground
  pinMode(PIN_SD, OUTPUT);
  digitalWrite(PIN_SD, LOW);
  delay(1);

  pinMode(PIN_RX, INPUT);
  pinMode(PIN_TX, OUTPUT);

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
  // if (!uartPinSignalInversion(IRDA._uart,  UART_SIGNAL_IRDA_TX_INV|UART_SIGNAL_RXD_INV, true)) {
  // if (!uartPinSignalInversion(IRDA._uart, UART_SIGNAL_RXD_INV, true)) {
  // if (uart_set_line_inverse(1, UART_SIGNAL_IRDA_TX_INV|UART_SIGNAL_RXD_INV) != ESP_OK) {
    // Serial.println("Failed inverting IRDA mode");
    // digitalWrite(LED_BUILTIN, LOW);
  // }
  // if (!IRDA.setRxInvert(true)) {
  //   Serial.println("Failed changing IRDA rx invert");
  //   digitalWrite(LED_BUILTIN, LOW);
  // }
  // if (!IRDA.setTxInvert(true)) {
  //   Serial.println("Failed changing IRDA tx invert");
  //   digitalWrite(LED_BUILTIN, LOW);
  // }

  while (!IRDA) {
    Serial.println("Waiting for IRDA to come up...");
    delay(10);
  }

  Serial.write("Done setup!");
}

void loop() {
  connect();
  delay(10000);
}
