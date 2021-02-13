#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "microrl/microrl.h"
#include "output_buffer.h"
#include "keypress.h"
#include "shell.h"
#include "sequences.h"
#include "leds.h"
#include "keypress_sequence.h"
#include "tables.h"

microrl_t shell;
output_buffer_t output_buffer;

const char ascii_to_scan_code_table[] = {ASCII2SCANCODE_TABLE};
const char scancode_to_ascii[] = {SCANCODE2ASCII_TABLE};

static void shell_print_callback (const char * str) {
	const size_t len = strlen(str);
	ob_add_data(&output_buffer, str, len);
}

char MSG_ERR[] = "\r\nERR\r\n";
char MSG_OK[] = "\r\nOK\r\n";
char MSG_WTF[] = "\r\n?\r\n";
char MSG_NL[] = "\r\n";

keypress_t keypress_scratchpad[128];
char       ascii_scratchpad[128];

static int ascii_to_keypresses(const char* input_buffer, keypress_t* target_buffer) {
	int i=0;
	for (const char* c = input_buffer; *c != '\0'; c++) {
		if (*c>=128) continue; // Skip it
		uint8_t scancode = ascii_to_scan_code_table[(uint8_t)*c];
		target_buffer[i++] = scancode;
	}
	return i;
}

static int keypresses_to_ascii(const keypress_buffer input_buffer, char* target_buffer) {
	int i=0;
	for (int j = 0; j < input_buffer.length; j++) {
		const keypress_t kp = input_buffer.sequence[j];
		const uint8_t scancode = SCANCODE(kp);	
		if (scancode>=128) continue; // Skip it
		const char ascii = scancode_to_ascii[scancode];
		target_buffer[i++] = ascii;
	}
	return i;
}

static void set_string_sequence(int argc, const char* const* argv) {
	if (argc < 2) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);
	int len = ascii_to_keypresses(argv[1], keypress_scratchpad);
	keypress_buffer b = { .length = len, .sequence=keypress_scratchpad};
	int ok = sq_set_sequence(key, b);
	if (ok) {
		ob_add_data(&output_buffer, MSG_OK, sizeof MSG_OK);
	} else {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
	}
}

static void get_string_sequence(int argc, const char* const* argv) {
	if (argc < 1) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);
	keypress_buffer b = sq_get_sequence(key);
	int len = keypresses_to_ascii(b, ascii_scratchpad);
	if (!IS_EMPTY_KEYPRESS_BUFFER(b)) {
		ob_add_data(&output_buffer, MSG_NL, sizeof MSG_NL);
		ob_add_data(&output_buffer, ascii_scratchpad, len);
		ob_add_data(&output_buffer, MSG_NL, sizeof MSG_NL);
	} else {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
	}
}

static void set_keypress_sequence(int argc, const char* const* argv) {
	if (argc < 2) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);

	int si=0;
	for (int i = 1; i < argc; i++) {
		unsigned long input_scancode = strtoul(argv[i], NULL, 16);
		keypress_scratchpad[si++] = (keypress_t)input_scancode;
	}

	keypress_buffer b = { .length = si, .sequence=keypress_scratchpad};
	int ok = sq_set_sequence(key, b);
	if (ok) {
		ob_add_data(&output_buffer, MSG_OK, sizeof MSG_OK);
	} else {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
	}
}

static void get_keypress_sequence(int argc, const char* const* argv) {
	if (argc < 1) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}
	const int key = atoi(argv[0]);
	keypress_buffer b = sq_get_sequence(key);
	int c = 0;
	for (int i = 0; i < b.length; ++i) {
		const int remaining = (sizeof ascii_scratchpad) - c;
		if (remaining <= 0) {
			break;
		}
		int printed = snprintf(ascii_scratchpad+c, remaining, "%x ", b.sequence[i]);
		c += printed;
	}
	if (!IS_EMPTY_KEYPRESS_BUFFER(b)) {
		ob_add_data(&output_buffer, MSG_NL, sizeof MSG_NL);
		ob_add_data(&output_buffer, ascii_scratchpad, c);
		ob_add_data(&output_buffer, MSG_NL, sizeof MSG_NL);
	} else {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
	}
}

static void press_key(int argc, const char* const* argv) {
	if (argc < 1) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);
	keypress_buffer seq = sq_get_sequence(key);
	if (! IS_EMPTY_KEYPRESS_BUFFER(seq)) {
		enqueue_key_presses(seq);
		ob_add_data(&output_buffer, MSG_OK, sizeof MSG_OK);
	}
}

int shell_execute_callback(int argc, const char* const* argv) {
	if (argc == 0) return 0;

	if (strcmp(argv[0], "up") == 0) {
		led_up();
	} else if (strcmp(argv[0], "down") == 0) {
		led_down();
	} else if (strcmp(argv[0], "sets") == 0) {
		set_string_sequence(argc-1, argv+1);
	} else if (strcmp(argv[0], "gets") == 0) {
		get_string_sequence(argc-1, argv+1);
	} else if (strcmp(argv[0], "set") == 0) {
		set_keypress_sequence(argc-1, argv+1);
	} else if (strcmp(argv[0], "get") == 0) {
		get_keypress_sequence(argc-1, argv+1);
	} else if (strcmp(argv[0], "press") == 0) {
		press_key(argc-1, argv+1);
	} else {
		ob_add_data(&output_buffer, MSG_WTF, sizeof MSG_WTF);
	}
	return 0;
}

void shell_add_char(char c) {
    microrl_insert_char (&shell, c);
}

bool shell_get_data(char** data, size_t* len) {
    return ob_read_data(&output_buffer, data, len);
}

void init_shell() {
	ob_init(&output_buffer);
	microrl_init(&shell, shell_print_callback);
	microrl_set_execute_callback (&shell, shell_execute_callback);
}

