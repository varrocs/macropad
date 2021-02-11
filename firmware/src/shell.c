#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "microrl/microrl.h"
#include "output_buffer.h"
#include "keypress.h"
#include "shell.h"
#include "sequences.h"
#include "leds.h"
#include "keypress_sequence.h"

microrl_t shell;
output_buffer_t output_buffer;

// See at the bottom
const char ascii_to_scan_code_table[];
const char scancode_to_ascii[];

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

static int keypresses_to_ascii(const keypress_t* input_buffer, char* target_buffer) {
	int i=0;
	for (const keypress_t* k = input_buffer; !IS_ZERO_KEYPRESS(*k); k++) {
		uint8_t scancode = SCANCODE(*k);	
		if (scancode>=128) continue; // Skip it
		char ascii = scancode_to_ascii[scancode];
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
	int len = keypresses_to_ascii(b.sequence, ascii_scratchpad); // TODO
	if (!IS_EMPTY_KEYPRESS_BUFFER(b)) {
		ob_add_data(&output_buffer, MSG_NL, sizeof MSG_NL);
		//ob_add_data(&output_buffer, b.sequence, b.length);
		ob_add_data(&output_buffer, ascii_scratchpad, len);
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


// Lookup table to convert ascii characters in to keyboard scan codes
// Format: most signifficant bit indicates if scan code should be sent with shift modifier
// remaining 7 bits are to be used as scan code number.

const char ascii_to_scan_code_table[] = {
  // /* ASCII:   0 */ 0,
  // /* ASCII:   1 */ 0,
  // /* ASCII:   2 */ 0,
  // /* ASCII:   3 */ 0,
  // /* ASCII:   4 */ 0,
  // /* ASCII:   5 */ 0,
  // /* ASCII:   6 */ 0,
  // /* ASCII:   7 */ 0,
  /* ASCII:   8 */ 42,
  /* ASCII:   9 */ 43,
  /* ASCII:  10 */ 40,
  /* ASCII:  11 */ 0,
  /* ASCII:  12 */ 0,
  /* ASCII:  13 */ 0,
  /* ASCII:  14 */ 0,
  /* ASCII:  15 */ 0,
  /* ASCII:  16 */ 0,
  /* ASCII:  17 */ 0,
  /* ASCII:  18 */ 0,
  /* ASCII:  19 */ 0,
  /* ASCII:  20 */ 0,
  /* ASCII:  21 */ 0,
  /* ASCII:  22 */ 0,
  /* ASCII:  23 */ 0,
  /* ASCII:  24 */ 0,
  /* ASCII:  25 */ 0,
  /* ASCII:  26 */ 0,
  /* ASCII:  27 */ 41,
  /* ASCII:  28 */ 0,
  /* ASCII:  29 */ 0,
  /* ASCII:  30 */ 0,
  /* ASCII:  31 */ 0,
  /* ASCII:  32 */ 44,
  /* ASCII:  33 */ 158,
  /* ASCII:  34 */ 159,
  /* ASCII:  35 */ 49, /* # - somewhat special SC:160 */
  /* ASCII:  36 */ 161,
  /* ASCII:  37 */ 162,
  /* ASCII:  38 */ 163,
  /* ASCII:  39 */ 177,
  /* ASCII:  40 */ 165,
  /* ASCII:  41 */ 166,
  /* ASCII:  42 */ 176,
  /* ASCII:  43 */ 48,
  /* ASCII:  44 */ 54,
  /* ASCII:  45 */ 56,
  /* ASCII:  46 */ 55,
  /* ASCII:  47 */ 164,
  /* ASCII:  48 */ 39,
  /* ASCII:  49 */ 30,
  /* ASCII:  50 */ 31,
  /* ASCII:  51 */ 32,
  /* ASCII:  52 */ 33,
  /* ASCII:  53 */ 34,
  /* ASCII:  54 */ 35,
  /* ASCII:  55 */ 36,
  /* ASCII:  56 */ 37,
  /* ASCII:  57 */ 38,
  /* ASCII:  58 */ 183,
  /* ASCII:  59 */ 182,
  /* ASCII:  60 182*/ 0,
  /* ASCII:  61 */ 167,
  /* ASCII:  62 183*/ 0,
  /* ASCII:  63 */ 173,
  /* ASCII:  64 */ 84,
  /* ASCII:  65 */ 132,
  /* ASCII:  66 */ 133,
  /* ASCII:  67 */ 134,
  /* ASCII:  68 */ 135,
  /* ASCII:  69 */ 136,
  /* ASCII:  70 */ 137,
  /* ASCII:  71 */ 138,
  /* ASCII:  72 */ 139,
  /* ASCII:  73 */ 140,
  /* ASCII:  74 */ 141,
  /* ASCII:  75 */ 142,
  /* ASCII:  76 */ 143,
  /* ASCII:  77 */ 144,
  /* ASCII:  78 */ 145,
  /* ASCII:  79 */ 146,
  /* ASCII:  80 */ 147,
  /* ASCII:  81 */ 148,
  /* ASCII:  82 */ 149,
  /* ASCII:  83 */ 150,
  /* ASCII:  84 */ 151,
  /* ASCII:  85 */ 152,
  /* ASCII:  86 */ 153,
  /* ASCII:  87 */ 154,
  /* ASCII:  88 */ 155,
  /* ASCII:  89 */ 157,
  /* ASCII:  90 */ 156,
  /* ASCII:  91 */ 101,
  /* ASCII:  92 */ 109,
  /* ASCII:  93 */ 102,
  /* ASCII:  94 */ 102,
  /* ASCII:  95 */ 184,
  /* ASCII:  96 */ 100,
  /* ASCII:  97 */ 4,
  /* ASCII:  98 */ 5,
  /* ASCII:  99 */ 6,
  /* ASCII: 100 */ 7,
  /* ASCII: 101 */ 8,
  /* ASCII: 102 */ 9,
  /* ASCII: 103 */ 10,
  /* ASCII: 104 */ 11,
  /* ASCII: 105 */ 12,
  /* ASCII: 106 */ 13,
  /* ASCII: 107 */ 14,
  /* ASCII: 108 */ 15,
  /* ASCII: 109 */ 16,
  /* ASCII: 110 */ 17,
  /* ASCII: 111 */ 18,
  /* ASCII: 112 */ 19,
  /* ASCII: 113 */ 20,
  /* ASCII: 114 */ 21,
  /* ASCII: 115 */ 22,
  /* ASCII: 116 */ 23,
  /* ASCII: 117 */ 24,
  /* ASCII: 118 */ 25,
  /* ASCII: 119 */ 26,
  /* ASCII: 120 */ 27,
  /* ASCII: 121 */ 29,
  /* ASCII: 122 */ 28,
  /* ASCII: 123 */ 100,
  /* ASCII: 124 */ 99,
  /* ASCII: 125 */ 103,
  /* ASCII: 126 */ 112
};


const char scancode_to_ascii[] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
  '\t', /* <-- Tab */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',     
    0, /* <-- control key */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0,  /* All other keys are undefined */
};
