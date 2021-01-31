/*typedef struct buffer_segment
{
    char* start;
    size_t length;
} buffer_segment;
*/

/*
typedef struct output_buffer_t
{
    char buffer[256];
    struct buffer_segment segments[8];
    // int cursor;
    char* cursor;
    int segment_write_cursor;
    int segment_read_cursor;
    
} output_buffer_t;
*/
typedef struct output_buffer_t
{
    char buffer[256];
    // char* cursor;
    int i;
    
} output_buffer_t;

void ob_init(output_buffer_t* b);
void ob_add_data(output_buffer_t *b, const char* data, size_t len);
bool ob_read_data(output_buffer_t *b, char** data, size_t* len);
