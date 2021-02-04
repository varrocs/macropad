#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "microrl/microrl.h"
#include "output_buffer.h"
#include "shell.h"
#include "sequences.h"

microrl_t shell;
output_buffer_t output_buffer;

static void shell_print_callback (const char * str) {
	const size_t len = strlen(str);
	ob_add_data(&output_buffer, str, len);
}

char MSG_ERR[] = "\r\nERR\r\n";
char MSG_OK[] = "\r\nOK\r\n";

static void set_string_sequence(int argc, const char* const* argv) {
	if (argc < 2) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);
	set_sequence(key, argv[1]);

	ob_add_data(&output_buffer, MSG_OK, sizeof MSG_OK);
}

static void get_string_sequence(int argc, const char* const* argv) {
	if (argc < 1) {
		ob_add_data(&output_buffer, MSG_ERR, sizeof MSG_ERR);
		return;
	}

	int key = atoi(argv[0]);
	char * seq = get_sequence(key);
	int len = strlen(seq);

	ob_add_data(&output_buffer, "\r\n", 2);
	ob_add_data(&output_buffer, seq, len);
	ob_add_data(&output_buffer, "\r\n", 2);
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
	} else {
		ob_add_data(&output_buffer, "\r\n?\r\n", 5);
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
