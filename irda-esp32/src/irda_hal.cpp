#include "irda_hal.h"

#include "config.h"
#include "hal/uart_hal.h"

HardwareSerial IRDA = HardwareSerial(IRDA_UART_NUM);
static uart_dev_t *IRDA_UART = UART_LL_GET_HW(IRDA_UART_NUM);

void IRDA_tx(bool b) {
    IRDA_UART->conf0.irda_tx_en = b;
}