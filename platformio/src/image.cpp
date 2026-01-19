#include "image.h"

#include <string.h>

#include "../lib/stb_image_write.h"

namespace Image {

bool debug(const Image &i) {
    log_i("Image '%s' taken %d/%d/%d %02d:%02d", i.name, i.month, i.day, (2000 + i.year_minus_2000), i.hour, i.minute);
}

static uint32_t expanded[W * H];

bool save(Image &img) {
    char name[25];
    memcpy(name, img.name, 24);
    for (int i = 23; i >= 0; i--) {
        if (!isspace(name[i])) {
            name[i + 1] = '\0';
            break;
        }
    }

    char filename[128];
    snprintf(filename, 128, "%s_%d-%d-%d_%02d-%02d.png", strlen(name) > 0 ? name : "img", img.month, img.day,
             (2000 + img.year_minus_2000), img.hour, img.minute);

    for (int i = 0; i < W * H; i++) {
        // two pixels stored in a byte, 2bpp in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b >> 4 : b & 0xf;
        uint8_t gray = pixel * 17;
        // RGBA, A = 255
        expanded[i] = (gray << 24) | (gray << 16) | (gray << 8) | 0xff;
        // AGBR?
        // expanded[i] = (0xff << 24) | (gray << 16) | (gray << 8) | gray;
    }
}

}  // namespace Image