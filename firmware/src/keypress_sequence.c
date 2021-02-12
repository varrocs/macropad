#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "keypress.h"
#include "keypress_sequence.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/*size_t key_sequence_len(const keypress_t* b) {
	const keypress_t* c;
	for (c = b; *c != 0; c++);
	return c-b;
}*/

#define KEYPRESS_BUFFER_LEN (128)
keypress_t keyPressBuffer[KEYPRESS_BUFFER_LEN]; // upper byte: modifier, lower byte: scancode
keypress_t *write_cursor;
keypress_t *read_cursor;
bool pressed;

void enqueue_init() {
	pressed = false;
	read_cursor = write_cursor = keyPressBuffer;
}

void enqueue_key_presses(keypress_buffer keypresses) {
	const int remaining = KEYPRESS_BUFFER_LEN - (write_cursor - keyPressBuffer);
	const int step = MIN(keypresses.length, remaining);
	memcpy(write_cursor, keypresses.sequence, (step * sizeof(keypress_t)));
	write_cursor += step;
}

keypress_msg enqueue_tick() {
	if (write_cursor == read_cursor) {
		keypress_msg result = { .need_send=false, .keypress=0 };
		return result;
	}

	keypress_t key_to_send;
	if (pressed) {
		pressed = false;
		key_to_send = 0;
		read_cursor++;
		if (write_cursor == read_cursor) {
			enqueue_init();
		}
	} else {
		key_to_send = *read_cursor;
		pressed = true;
	}
	keypress_msg result = { .need_send=true, .keypress=key_to_send };
	return result;
}
