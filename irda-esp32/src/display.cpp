#ifdef ENABLE_SSD1306

#include "display.h"

#include "Adafruit_SSD1306.h"
#include "_1980v23P04_16.h"
#include "config.h"
#include "log.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

static const unsigned char PROGMEM image_RX_bits[] = {0x40, 0x90, 0xa0, 0xa8, 0xa0, 0x90, 0x40};

static const unsigned char PROGMEM image_TX_bits[] = {0x10, 0x48, 0x28, 0xa8, 0x28, 0x48, 0x10};

namespace Display {

const char* TAG = "Display";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool init() {
    if (!Wire.setPins(PIN_I2C_SDA, PIN_I2C_SCL) || !Wire.begin()) {
        LOGE(TAG, "Can't use these pins for I2C: SDA=%d SCL=%d", PIN_I2C_SDA, PIN_I2C_SCL);
        return false;
    }
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        LOGE(TAG, "SSD1306 allocation failed");
        return false;
    }

    display.clearDisplay();
    display.display();

    return true;
}

void showIdleScreen() {
    display.clearDisplay();

    display.drawRect(50, 50, 24, 11, 1);

    display.drawCircle(55, 55, 3, 1);

    display.drawCircle(68, 55, 3, 1);

    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&_1980v23P04_16);
    display.setCursor(26, 43);
    display.print("AND AIM HERE");

    display.setCursor(24, 32);
    display.print("IR > COM > PC");

    display.setCursor(47, 21);
    display.print("PRESS");

    display.drawLine(0, 9, 127, 9, 1);

    display.setCursor(93, 7);
    display.print("WQV-1");

    display.setCursor(1, 7);
    display.print("SEARCHING");

    display.display();
}

void indicate(Indicator i) {
    display.fillRect(86, 1, 5, 7, 0);
    switch (i) {
        case Indicator::TX:
            display.drawBitmap(86, 1, image_TX_bits, 5, 7, 1);
            break;
        case Indicator::RX:
            display.drawBitmap(86, 1, image_RX_bits, 5, 7, 1);
            break;
    }
    display.display();
}

size_t printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    size_t result = display.printf(format, args);
    va_end(args);
    return result;
}

}  // namespace Display

#endif