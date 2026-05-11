#ifndef SYSTEM_APPLY_INPUT
#define SYSTEM_APPLY_INPUT

struct ECDB;
struct Vector2;

void s_apply_input(struct ECDB const *const ecdb, int input_handle, struct Vector2 input);

#endif /* SYSTEM_APPLY_INPUT */