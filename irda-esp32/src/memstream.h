#pragma once
#include <Arduino.h>

#include "Print.h"
#include "log.h"
#include "math.h"

class MemStream : public Print {
   private:
    uint8_t *head;
    size_t offset = 0;
    size_t len;

   public:
    MemStream(size_t length) : len(length), offset(0) {
        head = (uint8_t *)ps_malloc(len);
        if (!head) {
            LOGE("MemStream", "Failed to allocate %d bytes in psram", len);
            len = 0;
        }
    }
    ~MemStream() {
        free(head);
    }
    size_t write(uint8_t b) {
        if (availableForWrite() > 0) {
            head[offset++] = b;
            return 1;
        }
        return 0;
    }
    size_t write(const uint8_t *buffer, size_t size) {
        size_t s = min((size_t)availableForWrite(), size);
        if (s > 0) {
            memcpy(head + offset, buffer, s);
            offset += s;
        }
        return s;
    }
    int availableForWrite() {
        return len - offset;
    }
    void flush() {
    }
};
