#pragma once
#include <Arduino.h>

namespace Frame {

// smallest frame size in bytes
constexpr size_t FRAME_SIZE = 6;
extern const char *lastError;
void writeFrame(uint8_t addr, uint8_t control, const uint8_t *data = nullptr, size_t len = 0);
bool parseFrame(uint8_t *buf, size_t len, size_t &outLen, uint8_t &addr, uint8_t &control);

}  // namespace Frame