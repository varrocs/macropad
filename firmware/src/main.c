#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/usb/usbd.h>

#include "usb_descriptors.h"

#include "usb_handshake.h"
#include "shell.h"

#define KEY_MEDIA_VOLUMEUP 0xed
#define KEY_MEDIA_VOLUMEDOWN 0xee

#define KEY_VOLUMEUP 0x80 // Keyboard Volume Up
#define KEY_VOLUMEDOWN 0x81 // Keyboard Volume Down

usbd_device *global_usb_dev=NULL;


static uint8_t key_to_write = 0;
static uint8_t key_pressed = 0;

// ----------------------------------------------------------------------- MISC
uint16_t usb_write_key(uint8_t key);

void led_up(void) {
	gpio_clear(GPIOC, GPIO13);
   // usb_write_key(KEY_VOLUMEUP);
	//key_to_write = KEY_MEDIA_VOLUMEUP;
	key_to_write = KEY_VOLUMEUP;
}

void led_down(void) {
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


void sys_tick_handler(void);
void sys_tick_handler(void) {

	char* str;
	size_t len;
	if (shell_get_data(&str, &len)) {
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

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

int main(void)
{
	int i;

	usbd_device *usbd_dev;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	init_shell();

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

	while (1)
		usbd_poll(usbd_dev);
}

