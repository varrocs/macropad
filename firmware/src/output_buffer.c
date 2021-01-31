
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "output_buffer.h"

void ob_init(output_buffer_t* b) {
	// b->cursor = b->buffer;
    b->i=0;
}

void ob_add_data(output_buffer_t *b, const char* data, size_t len) {
    // The message is too big
    if (len > sizeof b->buffer) {
        return;
    }
    // The message would overflow the buffer, start from the beginning
    // (and don't care about overwriting the old messages)
    if ((sizeof b->buffer - len) < (size_t)(b->i)) {
        b->i=0;
    }

    memcpy(&b->buffer[b->i], data, len);
    // b->cursor += len;
    b->i += len;
}

bool ob_read_data(output_buffer_t *b, char** data, size_t* len) {
    // if (b->buffer == b->cursor) {
    if (b->i == 0) {
        return false; // No new message
    }
    *data = b->buffer;
    *len = (int)b->i;
    ob_init(b);
    return true;
}

/*
void init_output_buffer(output_buffer_t* b) {
    b->cursor = b->buffer;
    b->segment_write_cursor=0;
    b->segment_read_cursor=0;
}

void add_segment(output_buffer_t *b, const char* data, size_t len) {
    // The message is too big
    if (len > sizeof b->buffer) {
        return;
    }
    // The message would overflow the buffer, start from the beginning
    // (and don't care about overwriting old messages)
    if ((b->cursor - b->buffer) > (int)len) {
        b->cursor = b->buffer;
    }

    buffer_segment s = { .start = b->cursor, .length = len};
    memcpy(s.start, data, s.length);
    b->cursor += s.length;

    b->segments[b->segment_write_cursor % 8] = s;
    b->segment_write_cursor ++;
}

bool readSegment(output_buffer_t *b, char** data, size_t* len) {
    if (b->segment_read_cursor == b->segment_write_cursor) {
        return false; // No new message
    }
    buffer_segment* s = &b->segments[b->segment_read_cursor];
    *data = s->start;
    *len  = s->length;
    b->segment_read_cursor++;
    if (b->segment_read_cursor == b->segment_write_cursor) { // The read cursor reached the write cursor, reset them
        b->segment_read_cursor = b->segment_write_cursor = 0;
    }
    return true;
}
*/
