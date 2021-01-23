/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>

#define ENDP_ADDR_SRL_DATA_OUT  0x01
#define ENDP_ADDR_SRL_DATA_IN   0x82
#define ENDP_ADDR_SRL_COMM_IN   0x83
#define IFACE_NUMBER_SRL_COMM		0
#define IFACE_NUMBER_SRL_DATA		1
// Prev: 0x83
#define ENDP_ADDR_HID_IN        0x84
#define IFACE_NUMBER_HID		2
// Prev: 0x84 
#define ENDP_ADDR_MKEYS_IN        0x85
#define IFACE_NUMBER_MKEYS		3

#define UNUSED(x) ((void)x)


static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	//.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};


// START SERIAL INTERFACES --------------------------------

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = ENDP_ADDR_SRL_COMM_IN,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = ENDP_ADDR_SRL_DATA_OUT,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = ENDP_ADDR_SRL_DATA_IN,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 },
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = IFACE_NUMBER_SRL_COMM,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = IFACE_NUMBER_SRL_DATA,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

// END SERIAL INTERFACES --------------------------------


// START HID INTERFACES --------------------------------

static const uint8_t hid_keyboard_report_descriptor[] = {
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
    0x25, 0x7F,             // Logical Maximum(104),
    0x05, 0x07,             // Usage Page (Key Codes),
    0x19, 0x00,             // Usage Minimum (0),
    0x29, 0x7F,             // Usage Maximum (104),
    0x81, 0x00,             // Input (Data, Array), ;Normal keys
    // bytes 3-9: keys
    0xc0                    // End Collection
};

static const uint8_t hid_mediakey_report_descriptor[] = {
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

struct hid_function {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed));

static const struct hid_function keyboard_hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(keyboard_hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0100,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_keyboard_report_descriptor),
    }
};

static const struct hid_function mediakey_hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(mediakey_hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0100,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_mediakey_report_descriptor),
    }
};

const struct usb_endpoint_descriptor hid_keyboard_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = ENDP_ADDR_HID_IN,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 8,
    .bInterval = 0x20,
};

const struct usb_interface_descriptor hid_keyboard_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = IFACE_NUMBER_HID,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 1, /* boot */
    .bInterfaceProtocol = 1, /* keyboard */
    .iInterface = 0,

    .endpoint = &hid_keyboard_endpoint,
    .extra = &keyboard_hid_function,
    .extralen = sizeof(keyboard_hid_function),
}};


const struct usb_endpoint_descriptor hid_mediakey_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = ENDP_ADDR_MKEYS_IN,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 8,
    .bInterval = 0x20,
};

const struct usb_interface_descriptor hid_mediakey_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber =  IFACE_NUMBER_MKEYS,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = &hid_mediakey_endpoint,
    .extra = &mediakey_hid_function,
    .extralen = sizeof(mediakey_hid_function),
}};

// END HID INTERFACES --------------------------------


static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}, {
	.num_altsetting = 1,
	.altsetting = hid_keyboard_iface,
}, {
	.num_altsetting = 1,
	.altsetting = hid_mediakey_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 4,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Black Sphere Technologies",
	"CDC-ACM Demo",
	"DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes cdcacm_control_request(
			usbd_device *usbd_dev, 
			struct usb_setup_data *req, 
			uint8_t **buf, uint16_t *len, 
			usbd_control_complete_callback *complete)
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		char local_buf[10];
		struct usb_cdc_notification *notif = (void *)local_buf;

		/* We echo signals back to host as notification. */
		notif->bmRequestType = 0xA1;
		notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
		notif->wValue = 0;
		notif->wIndex = 0;
		notif->wLength = 2;
		local_buf[8] = req->wValue & 3;
		local_buf[9] = 0;
		// usbd_ep_write_packet(0x83, buf, 10);
		return USBD_REQ_HANDLED;
		}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;
		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_control_request_interface(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
            usbd_control_complete_callback *complete)
{
    UNUSED(complete);
    UNUSED(usbd_dev);
    //UNUSED(buf);

    if(req->bmRequestType==(USB_REQ_TYPE_CLASS|USB_REQ_TYPE_INTERFACE) /*0x21*/)
    {
        if((req->bRequest == USB_REQ_SET_CONFIGURATION) &&
           (req->wIndex==0 /* Must be 0 for SET_CONFIGURATION*/))
        {
            if(*len>0)
            {
                //uint8_t data=**buf;

                // LEDs
                //update_leds(data&0b001, data&0b010, data&0b100);

                return USBD_REQ_HANDLED;
            }
            return USBD_REQ_NOTSUPP;
        }
        else if(req->bRequest==0x0A /*SET_IDLE*/)
        {
            return USBD_REQ_HANDLED;
        }
    }

    // TODO COPIED hid_control_request
    if((req->bmRequestType == (USB_REQ_TYPE_IN|USB_REQ_TYPE_STANDARD|USB_REQ_TYPE_INTERFACE) /*0x81*/) &&
       (req->bRequest == USB_REQ_GET_DESCRIPTOR /*0x06*/) &&
       (req->wValue == 0x2200 /*Descriptor Type (high byte) and Index (low byte)*/))
    {
        // req->wIndex is the interface number
        if(req->wIndex==IFACE_NUMBER_HID)
        {
            //printf("Got request for keyboard report descriptor\n");
            *buf = (uint8_t *)hid_keyboard_report_descriptor;
            *len = sizeof(hid_keyboard_report_descriptor);
        }
        else if(req->wIndex==IFACE_NUMBER_MKEYS)
        {
            //printf("Got request for mediakey report descriptor\n");
            *buf = (uint8_t*)hid_mediakey_report_descriptor;
            *len = sizeof(hid_mediakey_report_descriptor);
        }
        else
        {
            return USBD_REQ_NOTSUPP;
        }
        return USBD_REQ_HANDLED;
    }

   // printf("Got USB unsupported request: bmRequestType=%#x\nbRequest=%#x\nwValue=%#x\nwIndex=%#x\nwLenght=%#x\n\n",
   //        req->bmRequestType,
   //        req->bRequest,
   //        req->wValue,
   //        req->wIndex,
   //        req->wLength);

    return USBD_REQ_NOTSUPP;
}

static enum usbd_request_return_codes hid_control_request(
			usbd_device *usbd_dev, 
			struct usb_setup_data *req,
			uint8_t **buf, uint16_t *len,
			usbd_control_complete_callback *complete)
{
    (void)complete;
    (void)usbd_dev;

    if((req->bmRequestType == (USB_REQ_TYPE_IN|USB_REQ_TYPE_STANDARD|USB_REQ_TYPE_INTERFACE) /*0x81*/) &&
       (req->bRequest == USB_REQ_GET_DESCRIPTOR /*0x06*/) &&
       (req->wValue == 0x2200 /*Descriptor Type (high byte) and Index (low byte)*/))
    {
        // req->wIndex is the interface number
        if(req->wIndex==IFACE_NUMBER_HID)
        {
            //printf("Got request for keyboard report descriptor\n");
            *buf = (uint8_t *)hid_keyboard_report_descriptor;
            *len = sizeof(hid_keyboard_report_descriptor);
        }
        else if(req->wIndex==IFACE_NUMBER_MKEYS)
        {
            //printf("Got request for mediakey report descriptor\n");
            *buf = (uint8_t*)hid_mediakey_report_descriptor;
            *len = sizeof(hid_mediakey_report_descriptor);
        }
        else
        {
            return USBD_REQ_NOTSUPP;
        }
        return USBD_REQ_HANDLED;
    }

    //printf("Got USB unsupported request: bmRequestType=%#x\nbRequest=%#x\nwValue=%#x\nwIndex=%#x\nwLenght=%#x\n\n",
        //    req->bmRequestType,
        //    req->bRequest,
        //    req->wValue,
        //    req->wIndex,
        //    req->wLength);

    return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	(void)usbd_dev;

	char buf[64];
	int len = usbd_ep_read_packet(usbd_dev, ENDP_ADDR_SRL_DATA_OUT, buf, 64);

	if (len) {
		usbd_ep_write_packet(usbd_dev, ENDP_ADDR_SRL_DATA_IN, buf, len);
		buf[len] = 0;
	}
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	// Serial
	usbd_ep_setup(usbd_dev, ENDP_ADDR_SRL_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, ENDP_ADDR_SRL_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, ENDP_ADDR_SRL_COMM_IN, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	// HID
	usbd_ep_setup(usbd_dev, ENDP_ADDR_HID_IN, USB_ENDPOINT_ATTR_INTERRUPT, 8, NULL);
	usbd_ep_setup(usbd_dev, ENDP_ADDR_MKEYS_IN, USB_ENDPOINT_ATTR_INTERRUPT, 8, NULL);

	// Serial
	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);

	// HID
	usbd_register_control_callback(
			usbd_dev,
			USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
			USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
			hid_control_request);

	usbd_register_control_callback(
			usbd_dev,
			USB_REQ_TYPE_CLASS|USB_REQ_TYPE_INTERFACE,
			USB_REQ_TYPE_DIRECTION|USB_REQ_TYPE_TYPE|USB_REQ_TYPE_RECIPIENT,
			hid_control_request_interface);
}

int main(void)
{
	int i;

	usbd_device *usbd_dev;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_set(GPIOC, GPIO11);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);

	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	for (i = 0; i < 0x800000; i++)
		__asm__("nop");
	gpio_clear(GPIOC, GPIO11);

	while (1)
		usbd_poll(usbd_dev);
}

