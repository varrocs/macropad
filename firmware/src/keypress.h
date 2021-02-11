typedef uint16_t keypress_t ;

typedef struct keypress_buffer {
	keypress_t* sequence;
	uint16_t length;
} keypress_buffer;


#define MOD_BYTE(x) (((x) & 0xFF00)>>8)
#define SCANCODE(x) ( (x) & 0x00FF)

#define IS_ZERO_KEYPRESS(k) ((k)==0)

#define IS_EMPTY_KEYPRESS_BUFFER(b) ((b).sequence==NULL && (b).length==0)

