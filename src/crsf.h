#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "hardware/uart.h"

#define CRSF_CHANNEL_COUNT 16
#define CRSF_FRAME_MAX 64
#define CRSF_CHANNEL_MIN 172
#define CRSF_CHANNEL_MAX 1811
#define CRSF_CHANNEL_CENTER 992

typedef struct {
    uint16_t channels[CRSF_CHANNEL_COUNT];
    bool failsafe;
    uint32_t last_frame_ms;
} crsf_state_t;

void crsf_init(uart_inst_t *uart, unsigned rx_pin, unsigned tx_pin);
void crsf_task(void);
const crsf_state_t *crsf_get_state(void);
