#ifndef SYSTEM_WRITE_INPUT
#define SYSTEM_WRITE_INPUT

struct ECDB;
struct Vector2;

void s_write_input(struct ECDB const *const ecdb, int input_handle, struct Vector2 input);

#endif /* SYSTEM_WRITE_INPUT */