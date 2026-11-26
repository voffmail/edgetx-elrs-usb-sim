#include "led.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/time.h"

#include "ws2812.pio.h"

#define LED_PIO pio0
#define LED_SM 0
#define LED_FREQ_HZ 800000.0f
#define LED_UPDATE_MS 5

/* No link: smooth fade in and out, 2 s per full cycle. */
#define FADE_PERIOD_MS 2000
#define FADE_MAX_LEVEL 96

/* Link up: solid green. */
#define LINK_R 0
#define LINK_G 64
#define LINK_B 0

/* Axes moving: fast blink. */
#define BLINK_HALF_PERIOD_MS 80
#define BLINK_R 0
#define BLINK_G 96
#define BLINK_B 96

/* Keep blinking this long after the last stick movement. */
#define MOVE_HOLD_MS 300

/* Channel delta (in CRSF ticks) that counts as stick movement. */
#define MOVE_DELTA 6

/* Roll, pitch, throttle, yaw. */
#define AXIS_COUNT 4

static bool s_initialized;
static bool s_sent;
static uint8_t s_r, s_g, s_b;

static uint16_t s_prev[AXIS_COUNT];
static bool s_prev_valid;
static uint32_t s_move_until_ms;
static uint32_t s_next_update_ms;

static void put_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_sent && r == s_r && g == s_g && b == s_b) {
        return;
    }

    /* WS2812 expects GRB, MSB first, 24 bits. */
    const uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | (uint32_t)b;
    pio_sm_put_blocking(LED_PIO, LED_SM, grb << 8u);

    s_r = r;
    s_g = g;
    s_b = b;
    s_sent = true;
}

void led_init(void)
{
    const uint offset = pio_add_program(LED_PIO, &ws2812_program);
    ws2812_program_init(LED_PIO, LED_SM, offset, LED_PIN, LED_FREQ_HZ, false);
    s_initialized = true;
    put_rgb(0, 0, 0);
}

static bool axes_moved(const crsf_state_t *state, uint32_t now)
{
    bool moved = false;

    for (int i = 0; i < AXIS_COUNT; i++) {
        const uint16_t value = state->channels[i];
        if (s_prev_valid) {
            const int diff = (int)value - (int)s_prev[i];
            if (diff >= MOVE_DELTA || diff <= -MOVE_DELTA) {
                moved = true;
            }
        }
        s_prev[i] = value;
    }
    s_prev_valid = true;

    if (moved) {
        s_move_until_ms = now + MOVE_HOLD_MS;
    }
    return moved;
}

void led_task(const crsf_state_t *state)
{
    if (!s_initialized) {
        return;
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if ((int32_t)(now - s_next_update_ms) < 0) {
        return;
    }
    s_next_update_ms = now + LED_UPDATE_MS;

    if (state->failsafe) {
        /* Do not treat the first frame after link recovery as movement. */
        s_prev_valid = false;
    }

    const bool moving = axes_moved(state, now);

    if (state->failsafe) {
        const uint32_t half = FADE_PERIOD_MS / 2;
        const uint32_t phase = now % FADE_PERIOD_MS;
        const uint32_t level = (phase < half)
            ? (phase * FADE_MAX_LEVEL / half)
            : ((FADE_PERIOD_MS - phase) * FADE_MAX_LEVEL / half);

        put_rgb(0, (uint8_t)(level / 6), (uint8_t)level);
    } else if (moving || (int32_t)(s_move_until_ms - now) > 0) {
        const bool on = ((now / BLINK_HALF_PERIOD_MS) & 1u) == 0u;
        put_rgb(on ? BLINK_R : 0, on ? BLINK_G : 0, on ? BLINK_B : 0);
    } else {
        put_rgb(LINK_R, LINK_G, LINK_B);
    }
}
