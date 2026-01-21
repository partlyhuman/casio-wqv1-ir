#include "display.h"

#include "Adafruit_SSD1306.h"
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
    Serial.printf("Initializing display... Default SDA=%d SCL=%d Configured SDA=%d SCL=%d\n", SDA, SCL, PIN_I2C_SDA,
                  PIN_I2C_SCL);
    Serial.flush();
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

Print& print() {
    display.clearDisplay();
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    return display;
}

void update() {
    // display.drawCircle(10, 10, 20, WHITE);
    // display.drawCircle(10, 10, 10, BLACK);
    display.display();
}

}  // namespace Display