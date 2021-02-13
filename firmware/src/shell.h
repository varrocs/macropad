void shell_add_char(char);
bool shell_get_data(char** data, size_t* len);
void init_shell(void);

int shell_execute_callback(int argc, const char* const* argv);