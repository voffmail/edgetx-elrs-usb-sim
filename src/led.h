#pragma once

#include "crsf.h"

/* Onboard WS2812 data pin. This board revision wires it to GP22; the
 * RP2350-USB-A reference design uses GP16. Probe the pin if your board
 * differs: the LED simply stays dark when the pin is wrong. */
#ifndef LED_PIN
#define LED_PIN 22
#endif

void led_init(void);
void led_task(const crsf_state_t *state);
