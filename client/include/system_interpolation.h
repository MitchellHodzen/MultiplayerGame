#ifndef SYSTEM_INTERPOLATION_DEF
#define SYSTEM_INTERPOLATION_DEF

struct ECDB;
struct N_C_Transform_Interpolation_Buffer;

void s_interpolate_position(struct ECDB const *const ec, int transforms_handle, int transforms_buffer_handle, struct N_C_Transform_Interpolation_Buffer* network_trans_buffers, unsigned long estimated_server_time, unsigned int interpolation_delay_ms);

#endif /* SYSTEM_INTERPOLATION_DEF */