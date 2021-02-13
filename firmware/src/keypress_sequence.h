typedef struct {
	bool need_send;
	keypress_t keypress;
} keypress_msg;

size_t keypress_sequence_len(const keypress_t*);

void 			enqueue_init(void);
void 			enqueue_key_presses(keypress_buffer keypresses);
keypress_msg 	enqueue_get(void);
void 			enqueue_next(void);
