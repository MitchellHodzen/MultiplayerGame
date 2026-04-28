#ifndef SYSTEM_MOVEMENT
#define SYSTEM_MOVEMENT

struct ECDB;

void s_move(struct ECDB const *const ec, int positions_handle, int inputs_handle, float deltaTimeS);

#endif /* SYSTEM_MOVEMENT */