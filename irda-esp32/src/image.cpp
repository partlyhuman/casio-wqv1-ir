#include "image.h"

#include "FFat.h"
#include "ctime"
#include "log.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../lib/stb_image_write.h"

namespace Image {

static const char *TAG = "Image";

static uint8_t expanded[W * H];

bool init() {
    return true;
}

void setSystemTime(const Image &img) {
    tm t{};
    t.tm_year = (2000 + img.year_minus_2000) - 1900;
    t.tm_mon = img.month - 1;
    t.tm_mday = img.day;
    t.tm_hour = img.hour;
    t.tm_min = img.minute;
    t.tm_isdst = 0;
    time_t time = mktime(&t);

    timeval epoch = {time, 0};
    settimeofday((const timeval *)&epoch, 0);
}

void stbi_write_cb(void *context, void *data, int size) {
    ((File *)context)->write((uint8_t *)data, size);
}

bool save(const Image &img) {
    static uint count = 0;

    // Convert Casio pixel data to 8bpp 1-channel grayscale image
    for (int i = 0; i < W * H; i++) {
        // two pixels stored per byte, in 2 nybbles
        uint8_t b = img.pixel[i / 2];
        uint8_t pixel = (i % 2 == 0) ? b & 0xf : b >> 4;
        // 0 is white (reverse of normal), expand 4 bits to 0-255 (15 * 17 = 255)
        expanded[i] = 255 - pixel * 17;
    }

    // Filenames need to start with / and the base path of /ffat is transparently handled by FFat
    constexpr size_t PATH_MAX_LEN = 128;
    char basepath[PATH_MAX_LEN];
    char filename[PATH_MAX_LEN];
    snprintf(basepath, PATH_MAX_LEN, "/WQV%02d", ++count);
    // snprintf(basepath, PATH_MAX_LEN, "/%02d_%d-%d-%d_%02d-%02d", ++count, img.month, img.day,
    //          (2000 + img.year_minus_2000), img.hour, img.minute);

    LOGI(TAG, "Writing image to %s...\n", basepath);

    // try to set timestamp when writing file
    setSystemTime(img);

    snprintf(filename, PATH_MAX_LEN, "%s.png", basepath);
    File f = FFat.open(filename, FILE_WRITE, true);
    if (f) {
        stbi_write_png_to_func(stbi_write_cb, &f, W, H, 1, expanded, W);
        // stbi_write_bmp_to_func(stbi_write_cb, &f, W, H, 1, expanded);
        f.close();
    }

    // Write meta info
    snprintf(filename, PATH_MAX_LEN, "%s.meta", basepath);
    f = FFat.open(filename, FILE_WRITE, true);
    if (f) {
        f.printf("date = %04d-%02d-%02dT%02d:%02d\n", 2000 + img.year_minus_2000, img.month, img.day, img.hour,
                 img.minute);
        f.print("title = ");
        f.write((uint8_t *)img.name, sizeof(Image::name));
        f.print("\n\n");
        f.flush();
        f.close();
    }

    LOGI(TAG, "After writing: %d/%d total %d\n", FFat.usedBytes(), FFat.freeBytes(), FFat.totalBytes());

    return true;
}

void exportImagesFromDump(File &dump) {
    Image *img = new Image{};

    if (!dump) {
        LOGE(TAG, "No file!");
        return;
    }
    size_t count = dump.size() / sizeof(Image);
    dump.seek(0);
    for (size_t i = 0; i < count; i++) {
        LOGD(TAG, "Reading out image %d/%d\n", i, count);
        dump.readBytes(reinterpret_cast<char *>(img), sizeof(Image));
        save(*img);
    }

    delete img;
}

}  // namespace Image