#ifndef SYSTEM_PLAYER_STATE_MACHINE_DEF
#define SYSTEM_PLAYER_STATE_MACHINE_DEF

struct ECDB;

void s_player_state_machine(struct ECDB const *const ecdb, int inputs_handle, int player_states_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s);

#endif /* SYSTEM_PLAYER_STATE_MACHINE_DEF */