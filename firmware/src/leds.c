#include <libopencm3/stm32/gpio.h>
#include "leds.h"

void led_up() {
	gpio_clear(GPIOC, GPIO13);
}

void led_down() {
	gpio_set(GPIOC, GPIO13);
}
