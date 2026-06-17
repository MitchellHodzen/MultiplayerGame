#ifndef SYSTEM_PHYSICS_DEF
#define SYSTEM_PHYSICS_DEF

struct ECDB;
void s_update_physics(struct ECDB const *const ecdb, int physics_2d_handle, int inputs_handle, float deltaTimeS);
void s_apply_physics(struct ECDB const *const ecdb, int physics_2d_handle, int transforms_handle, float deltaTimeS);

#endif /* SYSTEM_PHYSICS_DEF */