#pragma once
#include <Arduino.h>

#include "soc/uart_periph.h"
#include "soc/uart_struct.h"

#define PIN_LED 15
#define LED_ON LOW
#define LED_OFF HIGH
#define PIN_SD 1
#define PIN_TX 5
#define PIN_RX 3
#define PIN_BUTTON 0
#define PIN_I2C_SDA SDA
#define PIN_I2C_SCL SCL

#define IRDA Serial1
#define IRDA_UART UART1
