#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Display {

enum class Indicator : uint8_t { NONE, TX, RX };

bool init();
void showIdleScreen();
void indicate(Indicator);
size_t printf(const char* format, ...);

}  // namespace Display