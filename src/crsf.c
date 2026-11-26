#include "crsf.h"

#include <stddef.h>

#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/uart.h"
#include "pico/time.h"

#define CRSF_BAUD 420000
#define CRSF_RING_SIZE 256
#define CRSF_LINK_TIMEOUT_MS 500
#define CRSF_TYPE_RC_CHANNELS 0x16
#define CRSF_RC_PAYLOAD_LEN 22

static const uint8_t valid_addresses[] = {0xC8, 0xEE, 0xEA, 0xEC};

static uart_inst_t *s_uart;
static uint8_t s_ring[CRSF_RING_SIZE];
static volatile uint16_t s_head;
static volatile uint16_t s_tail;
static uint8_t s_frame[CRSF_FRAME_MAX];
static uint16_t s_frame_len;
static crsf_state_t s_state;

static uint8_t crc8_dvb_s2(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xD5) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static void unpack_channels(const uint8_t *payload, uint16_t *channels)
{
    uint32_t bits = 0;
    int available = 0;
    int index = 0;

    for (int i = 0; i < CRSF_RC_PAYLOAD_LEN && index < CRSF_CHANNEL_COUNT; i++) {
        bits |= (uint32_t)payload[i] << available;
        available += 8;
        while (available >= 11 && index < CRSF_CHANNEL_COUNT) {
            channels[index++] = (uint16_t)(bits & 0x7FFu);
            bits >>= 11;
            available -= 11;
        }
    }
}

static bool address_is_valid(uint8_t address)
{
    for (size_t i = 0; i < sizeof(valid_addresses); i++) {
        if (valid_addresses[i] == address) {
            return true;
        }
    }
    return false;
}

/* ExpressLRS uses 0x7FF (2047) for channels the radio does not transmit:
 * its CRSF_CHANNEL_VALUE_UNSET (0xFFFF) marker truncated to 11 bits. Any
 * value outside the CRSF range is treated as "no channel" and centered,
 * otherwise such a channel would look like a permanently pressed button. */
static void sanitize_channels(uint16_t *channels)
{
    for (int i = 0; i < CRSF_CHANNEL_COUNT; i++) {
        if (channels[i] < CRSF_CHANNEL_MIN || channels[i] > CRSF_CHANNEL_MAX) {
            channels[i] = CRSF_CHANNEL_CENTER;
        }
    }
}

static void handle_frame(void)
{
    const uint8_t length = s_frame[1];
    const size_t body_len = (size_t)length - 1u;
    const uint8_t *body = &s_frame[2];

    if (crc8_dvb_s2(body, body_len) != s_frame[2 + body_len]) {
        return;
    }

    if (body[0] != CRSF_TYPE_RC_CHANNELS || body_len < 1u + CRSF_RC_PAYLOAD_LEN) {
        return;
    }

    unpack_channels(&body[1], s_state.channels);
    sanitize_channels(s_state.channels);
    s_state.last_frame_ms = to_ms_since_boot(get_absolute_time());
    s_state.failsafe = false;
}

static void feed(uint8_t byte)
{
    if (s_frame_len == 0 && !address_is_valid(byte)) {
        return;
    }

    if (s_frame_len >= CRSF_FRAME_MAX) {
        s_frame_len = 0;
        return;
    }

    s_frame[s_frame_len++] = byte;

    if (s_frame_len == 2) {
        const uint8_t declared = s_frame[1];
        if (declared < 2 || (uint16_t)(declared + 2) > CRSF_FRAME_MAX) {
            s_frame_len = 0;
            return;
        }
    }

    if (s_frame_len >= 2 && s_frame_len == (uint16_t)(s_frame[1] + 2)) {
        handle_frame();
        s_frame_len = 0;
    }
}

static void on_uart_rx(void)
{
    while (uart_is_readable(s_uart)) {
        const uint8_t byte = (uint8_t)uart_getc(s_uart);
        const uint16_t next = (uint16_t)((s_head + 1u) % CRSF_RING_SIZE);
        if (next != s_tail) {
            s_ring[s_head] = byte;
            s_head = next;
        }
    }
}

void crsf_init(uart_inst_t *uart, unsigned rx_pin, unsigned tx_pin)
{
    s_uart = uart;

    for (int i = 0; i < CRSF_CHANNEL_COUNT; i++) {
        s_state.channels[i] = CRSF_CHANNEL_CENTER;
    }
    s_state.failsafe = true;

    uart_init(uart, CRSF_BAUD);
    gpio_set_function(rx_pin, UART_FUNCSEL_NUM(uart, rx_pin));
    gpio_set_function(tx_pin, UART_FUNCSEL_NUM(uart, tx_pin));
    uart_set_format(uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(uart, true);

    const uint irq_number = (uart == uart0) ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq_number, on_uart_rx);
    uart_set_irq_enables(uart, true, false);
    irq_set_enabled(irq_number, true);
}

void crsf_task(void)
{
    while (s_tail != s_head) {
        const uint8_t byte = s_ring[s_tail];
        s_tail = (uint16_t)((s_tail + 1u) % CRSF_RING_SIZE);
        feed(byte);
    }

    const uint32_t now = to_ms_since_boot(get_absolute_time());
    if (s_state.last_frame_ms == 0 || (now - s_state.last_frame_ms) > CRSF_LINK_TIMEOUT_MS) {
        s_state.failsafe = true;
    }
}

const crsf_state_t *crsf_get_state(void)
{
    return &s_state;
}
