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

void showConnectingScreen(int offset) {
    static const unsigned char PROGMEM image_arrow_left_bits[] = {0x20, 0x40, 0xfe, 0x40, 0x20};

    static const unsigned char PROGMEM image_arrow_right_bits[] = {0x08, 0x04, 0xfe, 0x04, 0x08};

    display.clearDisplay();

    display.drawLine(0, 9, 127, 9, 1);

    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&_1980v23P04_16);
    display.setCursor(93, 7);
    display.print("WQV-1");

    display.setCursor(1, 7);
    display.print("CONNECTING");

    display.drawBitmap(48 + offset, 34, image_arrow_right_bits, 7, 5, 1);

    display.drawBitmap(69 - offset, 34, image_arrow_left_bits, 7, 5, 1);

    display.display();
}

void showProgressScreen(size_t bytes, size_t totalBytes, size_t bytesPerImage, const char* step) {
    static const unsigned char PROGMEM image_frame0_bits[] = {
        0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04,
        0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0xff, 0xfc};

    static const unsigned char PROGMEM image_frame1_bits[] = {
        0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04,
        0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0xf0, 0x04, 0x50, 0x04, 0x30, 0x04, 0x1f, 0xfc};

    static const unsigned char PROGMEM image_frame2_bits[] = {
        0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04,
        0xfe, 0x04, 0x42, 0x04, 0x22, 0x04, 0x12, 0x04, 0x0a, 0x04, 0x06, 0x04, 0x03, 0xfc};

    static const unsigned char PROGMEM image_frame3_bits[] = {
        0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0x80, 0x04, 0xff, 0x84, 0x40, 0x84,
        0x20, 0x84, 0x10, 0x84, 0x08, 0x84, 0x04, 0x84, 0x02, 0x84, 0x01, 0x84, 0x00, 0xfc};

    static const unsigned char PROGMEM image_frame4_bits[] = {
        0xff, 0xfc, 0x80, 0x04, 0x80, 0x04, 0xff, 0xe4, 0x40, 0x24, 0x20, 0x24, 0x10, 0x24,
        0x08, 0x24, 0x04, 0x24, 0x02, 0x24, 0x01, 0x24, 0x00, 0xa4, 0x00, 0x64, 0x00, 0x3c};

    static const unsigned char* const frames[] PROGMEM = {image_frame0_bits, image_frame1_bits, image_frame2_bits,
                                                          image_frame3_bits, image_frame4_bits};

    static uint8_t frame = 0;

    display.clearDisplay();

    display.drawRect(1, 53, 125, 9, 1);

    display.fillRect(3, 55, 1, 5, 1);

    // Progress bar, from w=0 to w=121
    display.fillRect(3, 55, bytes * 122 / totalBytes, 5, 1);

    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&_1980v23P04_16);
    display.setCursor(38, 34);
    display.printf("Photo %d/%d", 1 + bytes / bytesPerImage, totalBytes / bytesPerImage);

    display.setCursor(1, 50);
    display.printf("%0.0f%%", 100.0f * bytes / totalBytes);

    display.drawLine(0, 9, 127, 9, 1);

    display.setCursor(93, 7);
    display.print("WQV-1");

    display.setCursor(1, 7);
    display.print(step);

    display.drawBitmap(22, 21, frames[(frame++ / 10) % 5], 14, 14, 1);

    display.display();
}

void indicate(Indicator i) {
    static const unsigned char PROGMEM image_RX_bits[] = {0x40, 0x90, 0xa0, 0xa8, 0xa0, 0x90, 0x40};
    static const unsigned char PROGMEM image_TX_bits[] = {0x10, 0x48, 0x28, 0xa8, 0x28, 0x48, 0x10};
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

void showMountedScreen() {
    static const unsigned char PROGMEM image_file_save_bits[] = {
        0x7f, 0xfc, 0x90, 0xaa, 0x90, 0xa9, 0x90, 0xe9, 0x90, 0x09, 0x8f, 0xf1, 0x80, 0x01, 0x80, 0x01,
        0x80, 0x01, 0x9f, 0xf9, 0x90, 0x09, 0x97, 0xe9, 0x90, 0x09, 0xd7, 0xeb, 0x90, 0x09, 0x7f, 0xfe};

    display.clearDisplay();

    display.drawBitmap(56, 19, image_file_save_bits, 16, 16, 1);

    display.setTextColor(1);
    display.setTextWrap(false);
    display.setFont(&_1980v23P04_16);
    display.setCursor(8, 51);
    display.print("USB drive mounted");

    display.setCursor(13, 60);
    display.print("Eject when done!");

    display.drawLine(0, 9, 127, 9, 1);

    display.setCursor(93, 7);
    display.print("WQV-1");

    display.setCursor(1, 7);
    display.print("MOUNTED");

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