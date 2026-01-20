#include "image.h"

#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../lib/stb_image_write.h"
#include "FFat.h"

namespace Image {

static const char *TAG = "Image";

void debug(const Image &i) {
    Serial.printf("Image '%s' taken %d/%d/%d %02d:%02d", i.name, i.month, i.day, (2000 + i.year_minus_2000), i.hour,
                  i.minute);
}

static uint8_t expanded[W * H];

bool init() {
    // FFat.begin(true);

    // Serial.println("Listing files in /ffat:");
    // File root = FFat.open("/");
    // File file = root.openNextFile();
    // while (file) {
    //     Serial.println(String(file.name()));
    //     file.close();
    //     file = root.openNextFile();
    // }
    // file.close();
    // root.close();
    // Serial.println("End");
    // Serial.printf("%d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());
    // FFat.end();
    return true;
}

void sanitizedName(const Image &img, char *dst, size_t dst_size) {
    // copy at most dst_size-1 chars
    strncpy(dst, img.name, dst_size - 1);
    dst[dst_size - 1] = '\0';

    // find last non-space character
    int last = -1;
    for (int i = 0; dst[i]; i++) {
        if (dst[i] != ' ') last = i;
    }
    if (last == -1) {
        strncpy(dst, "img", dst_size);
        dst[dst_size - 1] = '\0';
    } else {
        dst[last + 1] = '\0';
    }

    // replace all remaining spaces with underscores
    for (int i = 0; dst[i]; i++) {
        if (dst[i] == ' ') dst[i] = '_';
    }
}

void stbi_write_callback(void *context, void *data, int size) {
    static_cast<File *>(context)->write((uint8_t *)data, size);
}

bool save(Image &img) {
    char name[25];
    static uint count = 0;
    sanitizedName(img, name, 25);

    char filename[128];
    snprintf(filename, 128, "/%s_%d-%d-%d_%02d-%02d_%02d.bmp", name, img.month, img.day, (2000 + img.year_minus_2000),
             img.hour, img.minute, ++count);

    for (int i = 0; i < W * H; i++) {
        // two pixels stored in a byte, 2bpp in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b & 0xf : b >> 4;
        expanded[i] = 255 - pixel * 17;  // max value 0xf * 17 = 255
    }

    Serial.printf("Writing image to %s...", filename);

    // if (!FFat.begin()) {
    //     ESP_LOGE(TAG, "Failed to open filesystem");
    //     return false;
    // }
    File f = FFat.open(filename, FILE_WRITE);
    stbi_write_bmp_to_func(stbi_write_callback, &f, W, H, 1, expanded);
    f.close();

    ESP_LOGD(TAG, "After writing: %d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());
    // FFat.end();

    return true;
}

}  // namespace Image