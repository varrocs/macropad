#include <string.h> 
#include <stdbool.h>
#include "sequences.h"

#define KEY_COUNT 9
#define BUFFER_SIZE 256

static char sequence_buffer_1[BUFFER_SIZE];
static char sequence_buffer_2[BUFFER_SIZE];

static char* keys[KEY_COUNT];

static char* current_sequence_buffer = sequence_buffer_1;

static char* other_buffer() {
    if (current_sequence_buffer == sequence_buffer_1) {
        return sequence_buffer_2;
    } else {
        return sequence_buffer_1;
    }
}

bool sq_set_sequence(int key, const char* sequence) {
	if (0 > key || key >= KEY_COUNT) return false;
	const char* source=NULL;
	char* target = other_buffer();
	for (int i=0; i < KEY_COUNT; ++i) {

		if (i==key && sequence == NULL) {
			keys[i] = NULL;
			source = NULL;
		} else if (i==key && sequence != NULL) {
			keys[i] = target;
			source = sequence;
		} else if (i!=key && keys[i] == NULL) {
			keys[i] = NULL;
			source = NULL;
		} else if (i!=key && keys[i] != NULL) {
			source = keys[i];
			keys[i] = target;
		}

		if (source != NULL) {
			const int length = strlen(source);
			const int buffer_usage = (target - other_buffer());
			const int remaining_buffer = BUFFER_SIZE - buffer_usage;
			if (remaining_buffer < length+1) {
				keys[i] = NULL;
				continue;
			}
			strcpy(target, source);
			target += length+1; // Skip \0
		}
	}
	current_sequence_buffer = other_buffer();
}

char* sq_get_sequence(int key) {
	if (key >= KEY_COUNT) return NULL;
    return keys[key];
}
