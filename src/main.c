#include "pico/stdlib.h"
#include "tusb.h"

#include "crsf.h"
#include "led.h"
#include "usb_descriptors.h"

#define CRSF_UART uart0
#define CRSF_RX_PIN 29
#define CRSF_TX_PIN 28

#define CH_ROLL 0
#define CH_PITCH 1
#define CH_THROTTLE 2
#define CH_YAW 3
#define CH_BUTTON_FIRST 4

#define AXIS_MAX 127
#define BUTTON_PRESS_OFFSET 150
#define BUTTON_RELEASE_OFFSET 50

static uint16_t s_buttons;

static int8_t channel_to_axis(uint16_t value)
{
    if (value < CRSF_CHANNEL_MIN) {
        value = CRSF_CHANNEL_MIN;
    }
    if (value > CRSF_CHANNEL_MAX) {
        value = CRSF_CHANNEL_MAX;
    }

    const int32_t delta = (int32_t)value - CRSF_CHANNEL_CENTER;
    const int32_t span = (delta >= 0)
        ? (CRSF_CHANNEL_MAX - CRSF_CHANNEL_CENTER)
        : (CRSF_CHANNEL_CENTER - CRSF_CHANNEL_MIN);

    int32_t scaled = (delta * AXIS_MAX) / span;
    if (scaled > AXIS_MAX) {
        scaled = AXIS_MAX;
    }
    if (scaled < -AXIS_MAX) {
        scaled = -AXIS_MAX;
    }
    return (int8_t)scaled;
}

static void build_report(hid_report_t *report, const crsf_state_t *state)
{
    if (state->failsafe) {
        *report = (hid_report_t){0};
        s_buttons = 0;
        return;
    }

    report->x = channel_to_axis(state->channels[CH_ROLL]);
    report->y = (int8_t)(-channel_to_axis(state->channels[CH_PITCH]));
    report->z = channel_to_axis(state->channels[CH_THROTTLE]);
    report->rx = channel_to_axis(state->channels[CH_YAW]);

    uint16_t buttons = 0;
    for (int i = 0; i < HID_BUTTON_COUNT; i++) {
        const uint16_t value = state->channels[CH_BUTTON_FIRST + i];
        const bool was_pressed = (s_buttons & (1u << i)) != 0;
        const uint16_t threshold = (uint16_t)(CRSF_CHANNEL_CENTER
            + (was_pressed ? BUTTON_RELEASE_OFFSET : BUTTON_PRESS_OFFSET));
        if (value > threshold) {
            buttons |= (uint16_t)(1u << i);
        }
    }
    s_buttons = buttons;
    report->buttons = (hid_buttons_t)buttons;
}

int main(void)
{
    crsf_init(CRSF_UART, CRSF_RX_PIN, CRSF_TX_PIN);
    tud_init(BOARD_TUD_RHPORT);
    led_init();

    absolute_time_t next_send = make_timeout_time_ms(1);

    while (true) {
        tud_task();
        crsf_task();
        led_task(crsf_get_state());

        if (time_reached(next_send)) {
            next_send = make_timeout_time_ms(1);

            hid_report_t report;
            build_report(&report, crsf_get_state());

            if (tud_hid_ready()) {
                tud_hid_report(0, &report, sizeof(report));
            }
        }
    }
}
