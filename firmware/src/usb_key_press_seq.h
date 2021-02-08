typedef struct {
	bool need_send;
	keypress_t keypress;
} key_msg;

size_t key_sequence_len(const keypress_t*);

void enqueue_init(void);
void enqueue_key_presses(keypress_buffer keypresses);
key_msg enqueue_tick(void);
