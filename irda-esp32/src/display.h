#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Display {

enum class Indicator : uint8_t { NONE, TX, RX };

bool init();
void showIdleScreen();
void showConnectingScreen(int offset = 0);
void showProgressScreen(size_t bytes, size_t totalBytes, size_t bytesPerImage, const char* label = "DOWNLOADING");
void showMountedScreen();
void indicate(Indicator);

}  // namespace Display