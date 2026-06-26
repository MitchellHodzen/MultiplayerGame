#ifndef SYSTEM_INTERPOLATION_DEF
#define SYSTEM_INTERPOLATION_DEF

struct ECDB;

void s_interpolate_position(struct ECDB const *const ec, int transforms_handle, int transforms_buffer_handle, unsigned long estimated_server_time, unsigned int interpolation_delay_ms);

#endif /* SYSTEM_INTERPOLATION_DEF */