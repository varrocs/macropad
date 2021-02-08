typedef uint16_t keypress_t ;

typedef struct keypress_buffer {
	keypress_t* sequence;
	uint16_t length;
} keypress_buffer;

#define MOD_BYTE(x) (((x) & 0xFF00)>>8)
#define SCANCODE(x) ( (x) & 0xFF)

