#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>

#include "usb_descriptors.h"
#include "usb_handshake.h"
#include "keypress.h"
#include "shell.h"
#include "keypress_sequence.h"
#include "layout.h"
#include "matrix.h"

usbd_device *global_usb_dev=NULL;

// ----------------------------------------------------------------------- MISC

static uint16_t usb_write_key(keypress_t key)
{
	const uint8_t mod_byte = MOD_BYTE(key);
	const uint8_t scan_code = SCANCODE(key);
	const uint8_t addr = ENDP_ADDR_HID_IN;
	// uint8_t addr = ENDP_ADDR_MKEYS_IN;
	uint8_t buf[8]={
		mod_byte, // modifiers
		0, // reserverd
		0, // leds,
		scan_code,
		0,//ps2keyboard.usb_keys[1],
		0,//ps2keyboard.usb_keys[2],
		0,//ps2keyboard.usb_keys[3],
		0,//ps2keyboard.usb_keys[4],
	};

	return usbd_ep_write_packet(global_usb_dev, addr, buf, sizeof buf);
}

void tim2_isr(void)
{
	matrix_scan();
	TIM_SR(TIM2) &= ~TIM_SR_UIF; /* Clear interrrupt flag. */
}

void sys_tick_handler(void) {

	char* str;
	size_t len;
	if (shell_get_data(&str, &len)) {
		usbd_ep_write_packet(global_usb_dev, ENDP_ADDR_SRL_DATA_IN, str, len);
	}

	keypress_msg msg = enqueue_get();
	if (msg.need_send) {
		uint16_t written = usb_write_key(msg.keypress);
		if (written > 0) {
			enqueue_next();
		}
	}
}

static void nvic_setup(void)
{
	/* Without this the timer interrupt routine will never be called. */
	nvic_enable_irq(NVIC_TIM2_IRQ);
	nvic_set_priority(NVIC_TIM2_IRQ, 1);
}

static void timer_setup(void)
{
	rcc_periph_clock_enable(RCC_TIM2);

	/*
	 * The goal is to let the LED2 glow for a second and then be
	 * off for a second.
	 */

	/* Set timer start value. */
	TIM_CNT(TIM2) = 1;

	/* Set timer prescaler. 72MHz/1440 => 50000 counts per second. */
	TIM_PSC(TIM2) = 1440;

	/* End timer value. If this is reached an interrupt is generated. */
	TIM_ARR(TIM2) = 50000/100; // scan 100 times a second

	/* Update interrupt enable. */
	TIM_DIER(TIM2) |= TIM_DIER_UIE;

	/* Start timer. */
	TIM_CR1(TIM2) |= TIM_CR1_CEN;

}

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

int main(void)
{
	int i;

	usbd_device *usbd_dev;

	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	nvic_setup();

	init_shell();
	enqueue_init();

	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
	/* SysTick interrupt every N clock pulses: set reload to N-1 */
	systick_set_reload(99999);
	systick_interrupt_enable();
	systick_counter_enable();

	// to make pb3 and pb4 usable
	rcc_periph_clock_enable(RCC_AFIO);
	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB); // For the matrix scanning
	gpio_set(GPIOA, GPIO12);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);

	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ,
		      GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);

	// rows
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO3 | GPIO4 | GPIO5);

	// col
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,  GPIO12 | GPIO13 | GPIO14);
	gpio_clear(GPIOB, GPIO12 | GPIO13 | GPIO14);

	// For the matrix scanning
	init_matrix_scan();
	timer_setup();

	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &usb_device_desc, &usb_config, usb_strings, USB_STRINGS_NUM, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

	for (i = 0; i < 0x800000; i++)
		__asm__("nop");
	gpio_clear(GPIOA, GPIO12);


	global_usb_dev = usbd_dev;

	while (1)
		usbd_poll(usbd_dev);
}

