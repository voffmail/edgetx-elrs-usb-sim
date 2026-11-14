#pragma once

#include <stdint.h>

typedef struct __attribute__((packed)) {
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t rx;
    uint16_t buttons;
} hid_report_t;

#define HID_REPORT_BYTES 6
#define HID_BUTTON_COUNT 12
