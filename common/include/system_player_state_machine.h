#ifndef SYSTEM_PLAYER_STATE_MACHINE_DEF
#define SYSTEM_PLAYER_STATE_MACHINE_DEF

struct ECDB;

void s_player_state_machine(struct ECDB const *const ec, int inputs_handle, int player_states_handle);

#endif /* SYSTEM_PLAYER_STATE_MACHINE_DEF */