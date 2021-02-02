#include <stdlib.h>
#include <stdbool.h>
#include "microrl/microrl.h"
#include "output_buffer.h"
#include "shell.h"

microrl_t shell;
output_buffer_t output_buffer;

static void shell_print_callback (const char * str) {
	const size_t len = strlen(str);
	ob_add_data(&output_buffer, str, len);
	//usbd_ep_write_packet(global_usb_dev, ENDP_ADDR_SRL_DATA_IN, str, len);
}

int shell_execute_callback(int argc, const char* const* argv) {
	if (argc == 0) return 0;

	if (strcmp(argv[0], "up") == 0) {
		led_up();
	} else if (strcmp(argv[0], "down") == 0) {
		led_down();
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
