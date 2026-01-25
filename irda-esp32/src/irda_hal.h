#pragma once
#include <Arduino.h>

// Hide some of the implementation details of IRDA sending
// ESP32 has builtin IRDA flavour UART, that requires some extra stuff (IRDA_tx)
// Another implementation might use a MCP2120
// Configure in config.h by setting IRDA_UART_NUM

extern HardwareSerial IRDA;

// Must set true before transmitting and false after transmitting
void IRDA_tx(bool b);
