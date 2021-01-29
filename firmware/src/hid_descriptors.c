#include <stdlib.h>
#include <stdint.h>
#include "hid_descriptors.h"

const uint8_t hid_keyboard_report_descriptor[63] = {
    0x05, 0x01,             // Usage Page (Generic Desktop),
    0x09, 0x06,             // Usage (Keyboard),
    0xA1, 0x01,             // Collection (Application),
    0x75, 0x01,             // Report Size (1),
    0x95, 0x08,             // Report Count (8),
    0x05, 0x07,             // Usage Page (Key Codes),
    0x19, 0xE0,             // Usage Minimum (224),
    0x29, 0xE7,             // Usage Maximum (231),
    0x15, 0x00,             // Logical Minimum (0),
    0x25, 0x01,             // Logical Maximum (1),
    0x81, 0x02,             // Input (Data, Variable, Absolute), ;Modifier byte
    //  byte 0: modifier
    0x95, 0x01,             // Report Count (1),
    0x75, 0x08,             // Report Size (8),
    0x81, 0x03,             // Input (Constant), ;Reserved byte
    // byte 1: reserved
    0x95, 0x05,             // Report Count (5),
    0x75, 0x01,             // Report Size (1),
    0x05, 0x08,             // Usage Page (LEDs),
    0x19, 0x01,             // Usage Minimum (1),
    0x29, 0x05,             // Usage Maximum (5),
    0x91, 0x02,             // Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,             // Report Count (1),
    0x75, 0x03,             // Report Size (3),
    0x91, 0x03,             // Output (Constant), ;LED report padding
    // byte 2: leds
    0x95, 0x06,             // Report Count (6),
    0x75, 0x08,             // Report Size (8),
    0x15, 0x00,             // Logical Minimum (0),
    //0x25, 0x7F,             // Logical Maximum(104),
    0x25, 0xFF,             // Logical Maximum(255),
    0x05, 0x07,             // Usage Page (Key Codes),
    0x19, 0x00,             // Usage Minimum (0),
    //0x29, 0x7F,             // Usage Maximum (104),
    0x29, 0xFF,             // Usage Maximum (255),
    0x81, 0x00,             // Input (Data, Array), ;Normal keys
    // bytes 3-9: keys
    0xc0                    // End Collection
};

const uint8_t hid_mediakey_report_descriptor[37] = {
    0x05, 0x0C,         // Usage Page (Consumer)
    0x09, 0x01,         // Usage (Consumer Controls)
    0xA1, 0x01,         // Collection (Application)

    0x15, 0x00,         // Logical Minimum (0)
    0x25, 0x01,         // Logical Maximum (1)
    0x75, 0x01,         // Report Size (1)

    0x95, 0x07,         // Report Count (5)
    0x09, 0xB5,         // Usage (Next Track)
    0x09, 0xB6,         // Usage (Previous Track)
    0x09, 0xB7,         // Usage (Stop)
    0x09, 0xCD,         // Usage (PlayPause)
    0x09, 0xE2,         // Usage (Mute)
    0x81, 0x06,         // Input (Data,Var,Rel)

    0x09, 0xE9,         // Usage (Volume +)
    0x09, 0xEA,         // Usage (Volume -)
    0x81, 0x02,         // Usage (Data,Var,Abs)
    // 7 bit keys
    0x95, 0x01,         // Report Count (1),
    0x81, 0x01,         // Input (Constant,Ary,Abs),
    // 1 bit padding
    0xC0                // End Collection
};
