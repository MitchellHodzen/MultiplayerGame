#ifndef CAMERA_DEF
#define CAMERA_DEF
#include "vector2.h"

struct ECDB;

struct C_Camera
{
    unsigned int target_id;
    int width;
    int height;
};

void s_camera_reposition(const struct ECDB* ecdb, unsigned int camera_id, unsigned int transform_handle, unsigned int camera_component_handle, unsigned int level_width, unsigned int level_height);

#endif /* CAMERA_DEF */