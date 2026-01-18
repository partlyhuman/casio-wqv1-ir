#pragma once
#include <Arduino.h>

namespace Frame {

void writeFrame(uint8_t addr, uint8_t control, const uint8_t *data, size_t len);

}