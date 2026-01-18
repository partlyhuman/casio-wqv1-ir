#include "esp32-hal-uart.h"
#include "esp32-hal.h"
#include "esp32-hal-periman.h"
#include "driver/uart.h"
#include "hal/uart_ll.h"
#include "soc/soc_caps.h"
#include "soc/uart_struct.h"
#include "soc/uart_periph.h"

constexpr uint8_t FRAME_BOF = 0xc0;
constexpr uint8_t FRAME_EOF = 0xc1;
constexpr uint8_t FRAME_ESC = 0x7d;

static inline void writeEscaped(uint8_t b) {
  if (b == FRAME_BOF || b == FRAME_EOF || b == FRAME_ESC) {
    IRDA.write(FRAME_ESC);
    IRDA.write(b ^ 0x20);
  } else {
    IRDA.write(b);
  }
}

void writeFrame(uint8_t addr, uint8_t control, const uint8_t *data, size_t len) {
  Serial.printf("> %02x %02x ", addr, control);
  for (size_t i = 0; i < len; i++) Serial.printf("%02x ", data[i]);
  Serial.println("");

  UART1.conf0.irda_tx_en = true;
  // UART_LL_GET_HW(1)->conf0.irda_tx_en = true;
  uint16_t checksum = 0;
  IRDA.write(FRAME_BOF);
  writeEscaped(addr);
  checksum += addr;
  writeEscaped(control);
  checksum += control;
  for (size_t i = 0; i < len; i++) {
    uint8_t b = data[i];
    checksum += b;
    writeEscaped(b);
  }
  IRDA.write((uint8_t)((checksum >> 8) & 0xff));
  IRDA.write((uint8_t)(checksum & 0xff));
  // writeEscaped((checksum >> 8) & 0xff);
  // writeEscaped(checksum & 0xff);
  IRDA.write(FRAME_EOF);
  IRDA.flush(true);
  UART1.conf0.irda_tx_en = false;
}

/*
void readFrame(bool wait = true) {
  bool isEscaped = false;
  if wait() {
    while (Serial.available() == 0) {
      // delay?
    }
  }
  uint8_t b = IRDA.read();
  if (b == )
  int checksum = 0;
  while (true) {
    //read one byte
    byte data = (byte)IRDA.read();

    if (isEscaped) {
      if ((SPACE & data) != 0) {
        data = (byte)(data & 0xDF);
      } else {
        data = (byte)(data | SPACE);
      }
      isEscaped = false;
    } else {
      if (data == PACKET_START_BYTE) {
        isEscaped = true;
        continue;
      }
      if (data == ESCAPE_BYTE) {
        isEscaped = true;
        continue;
      }
      if (data == PACKET_END_BYTE) {
        break;
      }
    }
    checksum += data & 0xff;
    baos.write(data);
  }

  //check checksum
  byte[] retval = baos.toByteArray();
  checksum -= retval[retval.length - 1] & 0xff;
  checksum -= retval[retval.length - 2] & 0xff;
  checksum = checksum % 0x10000;

  int value = (retval[retval.length - 2] & 0xff) * 0x100;
  value += retval[retval.length - 1] & 0xff;
  value = value % 0x10000;

  if (checksum != value) {
    log("Wrong checksum");
  }

  log("r: " + bu.byteArrayToString(retval));
  return retval;
}
*/
