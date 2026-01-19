#include "image.h"

#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../lib/stb_image_write.h"
#include "FFat.h"

namespace Image {

void debug(const Image &i) {
    log_i("Image '%s' taken %d/%d/%d %02d:%02d", i.name, i.month, i.day, (2000 + i.year_minus_2000), i.hour, i.minute);
}

static uint8_t expanded[W * H];

bool init() {
    // bool ret = !FFat.begin(true);
    // if (!ret) log_e("Error starting FFAT");
    // return ret;

    FFat.begin(true);

    Serial.println("Listing files in /ffat:");
    File root = FFat.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.println(String(file.name()));
        file.close();
        file = root.openNextFile();
    }
    file.close();
    root.close();
    Serial.println("End");
    Serial.printf("%d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());
    FFat.end();
    return true;
}

// context will be the pointer to our FFat file
void stbi_ffat_write_func(void *context, void *data, int size) {
    File *f = static_cast<File *>(context);
    f->write((uint8_t *)data, size);
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

bool save(Image &img) {
    char name[25];
    sanitizedName(img, name, 25);
    log_i("name: %s", name);

    char filename[128];
    snprintf(filename, 128, "/%s_%d-%d-%d_%02d-%02d.bmp", name, img.month, img.day, (2000 + img.year_minus_2000),
             img.hour, img.minute);
    log_i("filename: %s", filename);

    for (int i = 0; i < W * H; i++) {
        // two pixels stored in a byte, 2bpp in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b >> 4 : b & 0xf;
        expanded[i] = pixel * 17;  // max value 0xf * 17 = 255
                                   // Grayscale can be used with stb_image_write
    }

    log_i("Writing image to %s...", filename);

    if (!FFat.begin()) {
        log_e("Failed to open filesystem");
        return false;
    }
    File f = FFat.open(filename, FILE_WRITE);
    stbi_write_bmp_to_func(stbi_ffat_write_func, &f, W, H, 1, expanded);
    f.close();

    log_d("After writing: %d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());
    FFat.end();

    return true;
}

}  // namespace Image