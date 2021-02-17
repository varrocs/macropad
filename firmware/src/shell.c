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

const keypress_t ascii_to_usb_scan_code[] = {ASCII_TO_USB_SCANCODE};

static void shell_print_callback (const char * str) {
	const size_t len = strlen(str);
	ob_add_data(&output_buffer, str, len);
}

char MSG_ERR[] = "\r\nERR\r\n";
char MSG_OK[] = "\r\nOK\r\n";
char MSG_WTF[] = "\r\n?\r\n";
char MSG_NL[] = "\r\n";
char MSG_HELP[] = 
	"set <key> <scancodes...>\r\n"
	"get <key>\r\n"
	"sets <key> <ascii...>\r\n"
	"gets <key>\r\n"
	"press <key>\r\n"
	"ledup\r\n"
	"leddown\r\n"
	"help\r\n"
;

keypress_t keypress_scratchpad[128];
char       ascii_scratchpad[128];

static int ascii_to_keypresses(const char* input_buffer, keypress_t* target_buffer) {
	int i=0;
	for (const char* c = input_buffer; *c != '\0'; c++) {
		if (*c>=128) continue; // Skip it
		const keypress_t keypress = ascii_to_usb_scan_code[(uint8_t)*c];
		target_buffer[i++] = keypress;
	}
	return i;
}

static int find_ascii_to_usb_scancode(keypress_t kp) {
	const int scancode_count = sizeof(ascii_to_usb_scan_code) / sizeof(keypress_t);
	for (int i = 0; i<scancode_count; ++i) {
		if (ascii_to_usb_scan_code[i] == kp) {
			return i;
		}
	}
	return -1;
}

static int keypresses_to_ascii(const keypress_buffer input_buffer, char* target_buffer) {
	int i=0;
	for (int j = 0; j < input_buffer.length; j++) {
		const keypress_t kp = input_buffer.sequence[j];
		const int ascii = find_ascii_to_usb_scancode(kp);
		if (ascii < 0) {
			continue;
		}
		target_buffer[i++] =(char)ascii;
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
		int printed = snprintf(ascii_scratchpad+c, remaining, "0x%x ", b.sequence[i]);
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

static void print_help(void) {
	ob_add_data(&output_buffer, MSG_HELP, sizeof MSG_HELP);
}

int shell_execute_callback(int argc, const char* const* argv) {
	if (argc == 0) return 0;

	if (strcmp(argv[0], "ledup") == 0) {
		led_up();
	} else if (strcmp(argv[0], "leddown") == 0) {
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
	} else if (strcmp(argv[0], "help") == 0) {
		print_help();
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

