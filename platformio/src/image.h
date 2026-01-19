#pragma once
#include <Arduino.h>

namespace Image {

constexpr size_t W = 120;
constexpr size_t H = 120;

struct __attribute__((packed)) Image {
    char name[24];  // space padded, not null-terminated
    uint8_t year_minus_2000;
    uint8_t month;
    uint8_t day;
    uint8_t minute;
    uint8_t hour;
    uint8_t pixel[W * H / 2];  // one nibble per pixel
};
static_assert(sizeof(Image) == 0x1C3D, "Image size mismatch");

bool save(Image &img);
bool debug(const Image &i);

}  // namespace Image
