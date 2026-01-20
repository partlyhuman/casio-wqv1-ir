#include "image.h"

#include <string.h>

#include "FFat.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../lib/stb_image_write.h"

namespace Image {

static const char *TAG = "Image";

void debug(const Image &i) {
    Serial.printf("Image '%s' taken %d/%d/%d %02d:%02d", i.name, i.month, i.day, (2000 + i.year_minus_2000), i.hour,
                  i.minute);
}

static uint8_t expanded[W * H];

bool init() {
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

void stbi_write_cb(void *context, void *data, int size) {
    ((File *)context)->write((uint8_t *)data, size);
}

bool save(const Image &img) {
    static uint count = 0;
    // char name[25];
    // sanitizedName(img, name, 25);

    // Filenames need to start with / and the base path of /ffat is transparently handled by FFat
    char filename[128];
    // snprintf(filename, 128, "/%s_%d-%d-%d_%02d-%02d_%02d.bmp", name, img.month, img.day, (2000 +
    // img.year_minus_2000),
    //  img.hour, img.minute, ++count);
    snprintf(filename, 128, "/%02d_%d-%d-%d_%02d-%02d.bmp", ++count, img.month, img.day, (2000 + img.year_minus_2000),
             img.hour, img.minute);

    // Convert Casio pixel data to 8bpp 1-channel grayscale image
    for (int i = 0; i < W * H; i++) {
        // two pixels stored per byte, in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b & 0xf : b >> 4;
        // 0 is white (reverse of normal), expand 4 bits to 0-255 (15 * 17 = 255)
        expanded[i] = 255 - pixel * 17;
    }

    Serial.printf("Writing image to %s...\n", filename);
    File f = FFat.open(filename, FILE_WRITE);
    stbi_write_bmp_to_func(stbi_write_cb, &f, W, H, 1, expanded);
    f.close();
    Serial.printf("After writing: %d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());

    return true;
}

void exportImagesFromDump(const char *path) {
    Image *img = new Image{};

    File dump = FFat.open(path, FILE_READ);
    size_t count = dump.size() / sizeof(Image);
    dump.seek(0);
    for (size_t i = 0; i < count; i++) {
        Serial.printf("Reading out image %d/%d\n", i, count);
        dump.readBytes(reinterpret_cast<char *>(img), sizeof(Image));
        save(*img);
    }
    dump.close();

    delete img;
}

}  // namespace Image