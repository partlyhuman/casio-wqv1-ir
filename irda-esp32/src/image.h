#pragma once
#include <Arduino.h>

#include "FS.h"

namespace Image {

constexpr size_t W = 120;
constexpr size_t H = 120;

struct __attribute__((packed)) Image {
    char name[24];  // space padded, not null-terminated
    uint8_t year_minus_2000;
    uint8_t month;
    uint8_t day;
    uint8_t hour;              // Reversed from spec
    uint8_t minute;            // Reversed from spec
    uint8_t pixel[W * H / 2];  // one nibble per pixel
};
static_assert(sizeof(Image) == 0x1C3D, "Image size mismatch");

bool init();
bool save(const Image &img);
void exportImagesFromDump(File &dump);

}  // namespace Image
