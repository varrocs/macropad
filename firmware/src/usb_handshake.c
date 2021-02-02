#include <stdlib.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>

#include "shell.h"
#include "usb_descriptors.h"
#include "hid_descriptors.h"

#include "usb_handshake.h"

#define UNUSED(x) ((void)x)

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
    UNUSED(buf);

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
            *len = sizeof hid_keyboard_report_descriptor;
        }
        else if(req->wIndex==IFACE_NUMBER_MKEYS)
        {
            //printf("Got request for mediakey report descriptor\n");
            *buf = (uint8_t*)hid_mediakey_report_descriptor;
            *len = sizeof hid_mediakey_report_descriptor;
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
		for (int i = 0; i < len; i++) {
			shell_add_char(buf[i]);
		}

		buf[len] = 0;
	}
}

void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
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