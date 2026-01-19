#include "image.h"

#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../lib/stb_image_write.h"
#include "FFat.h"

namespace Image {

bool debug(const Image &i) {
    log_i("Image '%s' taken %d/%d/%d %02d:%02d", i.name, i.month, i.day, (2000 + i.year_minus_2000), i.hour, i.minute);
}

static uint8_t expanded[W * H];

bool init() {
    return true;
    // bool ret = !FFat.begin(true);
    // if (!ret) log_e("Error starting FFAT");
    // return ret;
}

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
    snprintf(filename, 128, "/ffat/%s_%d-%d-%d_%02d-%02d.bmp", strlen(name) > 0 ? name : "img", img.month, img.day,
             (2000 + img.year_minus_2000), img.hour, img.minute);

    for (int i = 0; i < W * H; i++) {
        // two pixels stored in a byte, 2bpp in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b >> 4 : b & 0xf;
        expanded[i] = pixel * 17;  // max value 0xf * 17 = 255
                                   // Grayscale can be used with stb_image_write
    }

    log_i("Writing image to %s...", filename);
    if (!stbi_write_bmp(filename, W, H, 1, expanded)) {
        log_e("Failed to write %s", filename);
        return false;
    }
    log_i("Wrote image to %s!", filename);
    return true;
}

}  // namespace Image