// https://gist.githubusercontent.com/frank26080115/82e2b4256883604016e177bdd78790c2/raw/2c962bcb94932355c75bc30ba39484443a750226/usbmsc_fat_demo.ino
/*
This demonstrates how to get a ESP32-S3 (my board is a lilygo-t-display-s3) to:
 * read/write files with the FFat library
 * access the files through USB

(warning: you cannot do both at the same time)

You must configure the ESP32 partitions to have a FAT partition
Either by setting 'Partition Scheme' in the 'Tools' menu
Or by specifying the appropriate *.CSV file (such as 'default_ffat.csv')

The FFat library must be modified, see comments below
*/

#include "FFat.h"
#include "FS.h"
#include "USB.h"
#include "USBMSC.h"

USBMSC MSC;

wl_handle_t flash_handle;
uint32_t sect_size;
uint32_t total_size;
uint32_t sect_cnt;

void usbEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id,
                      void* event_data) {
  log_d("EVENT base=%d id=%d\n", event_base, event_id);
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t* data = (arduino_usb_event_data_t*)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT:
      case ARDUINO_USB_RESUME_EVENT:
        log_d("START");
        FFat.begin();
        MSC.mediaPresent(true);
        MSC.begin(sect_cnt, sect_size);
        break;
      case ARDUINO_USB_STOPPED_EVENT:
      case ARDUINO_USB_SUSPEND_EVENT:
        log_d("STOP");
        MSC.mediaPresent(false);
        MSC.end();
        FFat.end();
        digitalWrite(LED_BUILTIN, LOW);
        break;

      default:
        break;
    }
  }
}

int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer,
                uint32_t bufsize) {
  wl_write(flash_handle, (size_t)((lba * sect_size) + offset),
           (const void*)buffer, (size_t)bufsize);
  // flash on writes
  static uint8_t f = HIGH;
  digitalWrite(LED_BUILTIN, f);
  f = (f == HIGH) ? LOW : HIGH;

  return bufsize;
}

int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  wl_read(flash_handle, (size_t)((lba * sect_size) + offset), (void*)buffer,
          (size_t)bufsize);
  return bufsize;
}

void flash(int count = 100) {
  uint8_t f = HIGH;
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, f);
    f = (f == HIGH) ? LOW : HIGH;
    delay(500);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Serial.begin(9600);
  // delay(250);
  // Serial.println("Starting setup");

  if (!FFat.begin(false)) {
    flash();
    return;
  }

  char filename[128];
  sprintf(filename, "/test_%d.txt", rand());
  auto f = FFat.open(filename, "w", true);
  f.write('H');
  f.flush();
  f.close();

  flash_handle = FFat._wl_handle;  // made this public
  sect_size = wl_sector_size(flash_handle);
  total_size = wl_size(flash_handle);
  sect_cnt = total_size / sect_size;

  USB.onEvent(usbEventCallback);
  MSC.vendorID("ESP32");
  MSC.productID("USB_MSC");
  MSC.productRevision("1.0");
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.mediaPresent(true);
  MSC.begin(sect_cnt, sect_size);
  USB.begin();
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {}
