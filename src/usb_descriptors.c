#include "usb_descriptors.h"

#include <string.h>

#include "tusb.h"

#define USB_VID 0x1209
#define USB_PID 0xC550
#define USB_BCD 0x0100

static const tusb_desc_device_t s_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = USB_BCD,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

static const uint8_t s_report_descriptor[] = {
    0x05, 0x01,
    0x09, 0x04,
    0xA1, 0x01,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x04,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x32,
    0x09, 0x33,
    0x81, 0x02,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x0C,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x0C,
    0x81, 0x02,
    0x75, 0x04,
    0x95, 0x01,
    0x81, 0x03,
    0xC0,
};

static const uint8_t s_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(s_report_descriptor), 0x81, 8, 1),
};

/* Index 0 is reserved for the LANGID descriptor, so string indexes start at 1:
 * 1 = iManufacturer, 2 = iProduct, 3 = iSerialNumber. */
static const char *s_strings[] = {
    "",
    "CRSF",
    "EdgeTX ELRS USB SIM",
    "0001",
};

uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&s_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return s_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    static uint16_t buffer[32];
    uint8_t count;

    if (index == 0) {
        buffer[1] = 0x0409;
        count = 1;
    } else {
        if (index >= sizeof(s_strings) / sizeof(s_strings[0])) {
            return NULL;
        }
        const char *text = s_strings[index];
        count = (uint8_t)strlen(text);
        if (count > 31) {
            count = 31;
        }
        for (uint8_t i = 0; i < count; i++) {
            buffer[1 + i] = (uint16_t)text[i];
        }
    }

    buffer[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2u * count + 2u));
    return buffer;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void)instance;
    return s_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
