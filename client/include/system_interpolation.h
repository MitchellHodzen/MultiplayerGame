#ifndef SYSTEM_INTERPOLATION_DEF
#define SYSTEM_INTERPOLATION_DEF

struct ECDB;
struct N_C_Transform_Interpolation_Buffer;

void s_interpolate_position(struct ECDB const *const ec, int transforms_handle, int transforms_buffer_handle, unsigned long estimated_server_time, unsigned int server_updates_per_s, unsigned int frame_drop_allowance, unsigned int round_trip_time_ms);
#endif /* SYSTEM_INTERPOLATION_DEF */