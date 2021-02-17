#include <libopencm3/stm32/gpio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "keypress.h"
#include "keypress_sequence.h"
#include "sequences.h"
#include "layout.h"
#include "matrix.h"

#define ROW_DELAY 30
#define DEBOUNCE_COUNT 5

#define DEF_MASK(x) ((1 << x) - 1)
#define MASK 	DEF_MASK(DEBOUNCE_COUNT)

#define TIMES8(x) x; x; x; x; x; x; x; x
#define TIMES9(x) x; x; x; x; x; x; x; x; x
#define TIMES72(x) TIMES8(TIMES9(x))


int rows[ROW_COUNT] = {GPIO3,GPIO4,GPIO5};
int cols[COL_COUNT] = {GPIO12,GPIO13,GPIO14};

debounce_t g_debouce_state;
key_state_t g_keystates[KEY_COUNT];

static void delay_us(int delay) {
	(void)delay;
	for (int i = 0; i< delay; i++) {
		TIMES72(__asm__("nop"));
	}
}

static void press_key(int key) {
	keypress_buffer seq = sq_get_sequence(key);
	if (! IS_EMPTY_KEYPRESS_BUFFER(seq)) {
		enqueue_key_presses(seq);
	}
}

static void do_matrix_scan(bool states[KEY_COUNT]) {
	for (int row = 0; row < ROW_COUNT; row++) {
		gpio_set(GPIOB, rows[row]);
		delay_us(ROW_DELAY);
		const int16_t value = gpio_port_read(GPIOB);
		for (int col = 0; col < COL_COUNT; col++) {
			const bool s = (bool)(value & cols[col]);
			states[row*COL_COUNT + col] = s;
		}
		gpio_clear(GPIOB, rows[row]);
		delay_us(ROW_DELAY);
	}
}

static void compare_key_states(key_state_t prev_state[], debounce_result_t scan_result) {
	for (int i = 0; i<KEY_COUNT; ++i) {
		key_state_t current_state = scan_result.result[i];
		if (current_state == KEY_UNDEFINED) continue;
		if (current_state == KEY_HIGH && prev_state[i] == KEY_LOW) {
		    press_key(i);
		}
		prev_state[i] = current_state;
	}
}

static void debounce_init(debounce_t* d) {
	for (int i = 0; i<KEY_COUNT; ++i) {
		d->debounce_buffer[i] = 0;
	}
}

static void init_keystates(key_state_t k[]) {
	for (int i = 0; i<KEY_COUNT; ++i) {
		k[i] = KEY_LOW;
	}
}

static debounce_result_t debounce_scan(debounce_t* d, bool scan[KEY_COUNT]) {
	debounce_result_t result;
	for (int i = 0; i<KEY_COUNT; ++i) {
		uint8_t b = d->debounce_buffer[i];
		d->debounce_buffer[i] = (MASK & (b << 1)) | scan[i];
		if (d->debounce_buffer[i] == 0) {
			result.result[i] = KEY_LOW;
		} else if (d->debounce_buffer[i] == MASK) {
			result.result[i] = KEY_HIGH;
		} else {
			result.result[i] = KEY_UNDEFINED;
		}
	}
	return result;
}

static void scan_with_debounce(debounce_t* t) {
	bool scan[KEY_COUNT];
	do_matrix_scan(scan);

	debounce_result_t debounced_result = debounce_scan(t, scan);
	compare_key_states(g_keystates, debounced_result);
}

void init_matrix_scan() {
	debounce_init(&g_debouce_state);
	init_keystates(g_keystates);
}

void matrix_scan() {
	scan_with_debounce(&g_debouce_state);
}

