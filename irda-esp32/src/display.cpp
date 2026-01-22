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

bool z = false;
void showIdleScreen() {
    if (z) return;
    z = true;
    display.stopscroll();

    display.clearDisplay();
    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&_1980v23P04_16);
    display.setCursor(2, 10);
    display.print("WAITING FOR WATCH...");

    display.setCursor(24, 39);
    display.print("IR > COM > PC");

    display.setCursor(46, 29);
    display.print("Select");

    display.drawRect(50, 50, 24, 11, 1);

    display.drawCircle(55, 55, 3, 1);

    display.drawCircle(68, 55, 3, 1);

    display.display();

    LOGI(TAG, "Starting scroller");
    display.startscrollright(1, 10);
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