#pragma once

#include <stdint.h>

/* Buttons come from channels 5 .. 4 + HID_BUTTON_COUNT (AUX1 .. AUXn).
 * ExpressLRS keeps the top channels busy with arming and service data
 * (untransmitted channels arrive as 2047), so only 8 buttons are exposed by
 * default. Raise it up to 12 if your radio really drives channels 13..16. */
#ifndef HID_BUTTON_COUNT
#define HID_BUTTON_COUNT 8
#endif

#if HID_BUTTON_COUNT <= 8
typedef uint8_t hid_buttons_t;
#else
typedef uint16_t hid_buttons_t;
#endif

typedef struct __attribute__((packed)) {
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t rx;
    hid_buttons_t buttons;
} hid_report_t;

#define HID_REPORT_BYTES (4 + (int)sizeof(hid_buttons_t))
