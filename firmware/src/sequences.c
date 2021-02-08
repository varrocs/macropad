#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "key_press.h"
#include "key_press_seq.h"
#include "sequences.h"

#define KEY_COUNT 9
#define BUFFER_SIZE 256

extern keypress_buffer EMPTY_KEYPRESS_BUFFER = { .sequence=NULL, .length=0 };

static keypress_t sequence_buffer_1[BUFFER_SIZE];
static keypress_t sequence_buffer_2[BUFFER_SIZE];

static keypress_buffer keys[KEY_COUNT];

static keypress_t* current_sequence_buffer = sequence_buffer_1;

static keypress_t* other_buffer(void) {
    if (current_sequence_buffer == sequence_buffer_1) {
        return sequence_buffer_2;
    } else {
        return sequence_buffer_1;
    }
}

bool sq_set_sequence(int key, keypress_buffer keypresses) {
	const keypress_t* sequence = keypresses.sequence;
	uint16_t length = keypresses.length;

	if (0 > key || key >= KEY_COUNT) return false;

	const keypress_t* source=NULL;
	keypress_t* target = other_buffer();
	for (int i=0; i < KEY_COUNT; ++i) {

		if (i==key && sequence == NULL) {
			keys[i] = EMPTY_KEYPRESS_BUFFER;
			source = NULL;
		} else if (i==key && sequence != NULL) {
			keys[i] = target;
			source = sequence;
		} else if (i!=key && keys[i] == EMPTY_KEYPRESS_BUFFER) {
			keys[i] = EMPTY_KEYPRESS_BUFFER;
			source = NULL;
		} else if (i!=key && keys[i] != EMPTY_KEYPRESS_BUFFER) {
			source = keys[i];
			keys[i].sequence = target;
			keys[i].length = length;
		}

		if (source != NULL) {
			const size_t buffer_usage = (target - other_buffer());
			const size_t remaining_buffer = BUFFER_SIZE - buffer_usage;
			if (remaining_buffer < length) {
				keys[i] = EMPTY_KEYPRESS_BUFFER;
				continue;
			}
			
			memcpy(target, source, length * sizeof keypress_t);
			target += length;
		}
	}
	current_sequence_buffer = other_buffer();
}

keypress_buffer sq_get_sequence(int key) {
	if (key >= KEY_COUNT) return NULL;
	return keys[key];
}
