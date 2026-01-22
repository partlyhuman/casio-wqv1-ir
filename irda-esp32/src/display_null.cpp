#ifndef ENABLE_SSD1306
#include <Arduino.h>
namespace Display {

bool init() {
    return false;
}

void showIdleScreen() {
}

size_t printf(const char *format, ...) {
    return 0;
}

}  // namespace Display
#endif