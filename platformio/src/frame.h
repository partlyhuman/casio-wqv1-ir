#pragma once
#include <Arduino.h>

namespace Frame {

// Smallest frame with no data
constexpr size_t FRAME_SIZE = 6;
constexpr uint8_t FRAME_BOF = 0xc0;
constexpr uint8_t FRAME_EOF = 0xc1;
constexpr uint8_t FRAME_ESC = 0x7d;

void writeFrame(uint8_t addr, uint8_t control, const uint8_t *data = nullptr, size_t len = 0);
bool parseFrame(uint8_t *buf, size_t len, size_t &outLen, uint8_t &addr, uint8_t &control);

}  // namespace Frame