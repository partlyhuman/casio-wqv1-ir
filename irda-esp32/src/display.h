#pragma once
#include <stddef.h>
#include <stdint.h>

namespace Display {

bool init();
void showIdleScreen();
size_t printf(const char* format, ...);

}  // namespace Display