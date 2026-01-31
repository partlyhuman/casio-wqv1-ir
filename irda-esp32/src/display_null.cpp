#ifndef ENABLE_SSD1306
#include "display.h"

namespace Display {

bool init() {
    return false;
}
void showIdleScreen() {
}
void showConnectingScreen(int offset) {
}
void showProgressScreen(size_t bytes, size_t totalBytes, size_t bytesPerImage, const char* label) {
}
void showMountedScreen() {
}

}  // namespace Display
#endif