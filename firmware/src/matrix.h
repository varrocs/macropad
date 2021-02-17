typedef enum key_state_t {
	KEY_HIGH,
	KEY_LOW,
	KEY_UNDEFINED
} key_state_t;

typedef struct debounce_result_t {
	key_state_t result[KEY_COUNT];
} debounce_result_t;

typedef struct debounce_t {
	uint8_t debounce_buffer[KEY_COUNT];
} debounce_t;


void init_matrix_scan(void);
void matrix_scan(void);

