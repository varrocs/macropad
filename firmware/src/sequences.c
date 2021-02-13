#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "keypress.h"
#include "keypress_sequence.h"
#include "sequences.h"

#define KEY_COUNT 9
#define BUFFER_SIZE 256

keypress_buffer EMPTY_KEYPRESS_BUFFER = { .sequence=NULL, .length=0 };

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
	const keypress_t* new_sequence = keypresses.sequence;
	uint16_t new_length = keypresses.length;

	if (0 > key || key >= KEY_COUNT) return false;

	const keypress_t* source=NULL;
	keypress_t* target = other_buffer();
	for (int i=0; i < KEY_COUNT; ++i) {

		if (i==key && new_sequence == NULL) {
			keys[i] = EMPTY_KEYPRESS_BUFFER;
			source = NULL;
		} else if (i==key && new_sequence != NULL) {
			keys[i].sequence = target;
			keys[i].length = new_length;
			source = new_sequence;
		} else if (i!=key && IS_EMPTY_KEYPRESS_BUFFER(keys[i])) {
			keys[i] = EMPTY_KEYPRESS_BUFFER;
			source = NULL;
		} else if (i!=key && !IS_EMPTY_KEYPRESS_BUFFER(keys[i])) {
			source = keys[i].sequence;
			keys[i].sequence = target;
			keys[i].length = keys[i].length;
			}

		if (source != NULL) {
			const size_t buffer_usage = (target - other_buffer());
			const size_t remaining_buffer = BUFFER_SIZE - buffer_usage;
			if (remaining_buffer < keys[i].length) {
				keys[i] = EMPTY_KEYPRESS_BUFFER;
				continue;
			}
			
			memcpy(target, source, (keys[i].length * sizeof(keypress_t)));
			target += keys[i].length;
		}
	}
	current_sequence_buffer = other_buffer();

	return true;
}

keypress_buffer sq_get_sequence(int key) {
	if (key >= KEY_COUNT || key < 0) return EMPTY_KEYPRESS_BUFFER;
	return keys[key];
}

