#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/cm3/systick.h>

//#include "shell.h"
#include "microrl/microrl.h"
#include "usb_descriptors.h"
#include "hid_descriptors.h"
#include "output_buffer.h"

#define UNUSED(x) ((void)x)

#define KEY_MEDIA_VOLUMEUP 0xed
#define KEY_MEDIA_VOLUMEDOWN 0xee

#define KEY_VOLUMEUP 0x80 // Keyboard Volume Up
#define KEY_VOLUMEDOWN 0x81 // Keyboard Volume Down

usbd_device *global_usb_dev=NULL;
static microrl_t shell;

static output_buffer_t output_buffer;

static uint8_t key_to_write = 0;
static uint8_t key_pressed = 0;

// ----------------------------------------------------------------------- MISC
uint16_t usb_write_key(uint8_t key);

static void led_up(void) {
	gpio_clear(GPIOC, GPIO13);
   // usb_write_key(KEY_VOLUMEUP);
	//key_to_write = KEY_MEDIA_VOLUMEUP;
	key_to_write = KEY_VOLUMEUP;
}

static void led_down(void) {
	gpio_set(GPIOC, GPIO13);
	//usb_write_key(KEY_VOLUMEDOWN);
	// key_to_write = KEY_MEDIA_VOLUMEDOWN;
	key_to_write = KEY_VOLUMEDOWN;
}

uint16_t usb_write_key(uint8_t key)
{
	uint8_t addr = ENDP_ADDR_HID_IN;
	// uint8_t addr = ENDP_ADDR_MKEYS_IN;
	uint8_t buf[8]={
		0, // modifiers
		0, // reserverd
		0, // leds,
		key,
		0,//ps2keyboard.usb_keys[1],
		0,//ps2keyboard.usb_keys[2],
		0,//ps2keyboard.usb_keys[3],
		0,//ps2keyboard.usb_keys[4],
	};

	return usbd_ep_write_packet(global_usb_dev, addr, buf, sizeof buf);
}

// ----------------------------------------------------------------------- SHELL

static void shell_print_callback (const char * str) {
	const size_t len = strlen(str);
	ob_add_data(&output_buffer, str, len);
	//usbd_ep_write_packet(global_usb_dev, ENDP_ADDR_SRL_DATA_IN, str, len);
}

static int shell_execute_callback(int argc, const char* const* argv) {
	if (argc == 0) return 0;

	if (strcmp(argv[0], "up") == 0) {
		led_up();
	} else if (strcmp(argv[0], "down") == 0) {
		led_down();
	}
	return 0;
}


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

		//usbd_ep_write_packet(usbd_dev, ENDP_ADDR_SRL_DATA_IN, buf, len);


		//char* response;
		//size_t response_length;
		//on_incoming_data(buf, len, &response, &response_length);
		//usbd_ep_write_packet(usbd_dev, ENDP_ADDR_SRL_DATA_IN, response, response_length);
		for (int i = 0; i < len; i++) {
			microrl_insert_char (&shell, buf[i]);
		}

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


void sys_tick_handler(void) {

	char* str;
	size_t len;
	if (ob_read_data(&output_buffer, &str, &len)) {
		usbd_ep_write_packet(global_usb_dev, ENDP_ADDR_SRL_DATA_IN, str, len);
	}

	if (key_to_write != 0) {
		int16_t written = usb_write_key(key_to_write);
		if (written) {
			key_pressed = key_to_write;
			key_to_write=0;
		}
		return ;
	}

	if (key_pressed != 0) {
		int16_t written = usb_write_key(0);
		if (written) {
			key_pressed = 0;
		}
	}

}

int main(void)
{
	int i;

	usbd_device *usbd_dev;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	ob_init(&output_buffer);

	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(99999);
	systick_interrupt_enable();
	systick_counter_enable();

	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);

	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &usb_device_desc, &usb_config, usb_strings, USB_STRINGS_NUM, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	for (i = 0; i < 0x800000; i++)
		__asm__("nop");
	gpio_clear(GPIOA, GPIO12);


	global_usb_dev = usbd_dev;
	microrl_init(&shell, shell_print_callback);
	microrl_set_execute_callback (&shell, shell_execute_callback);

	while (1)
		usbd_poll(usbd_dev);
}

