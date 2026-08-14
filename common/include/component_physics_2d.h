#ifndef PHYSICS_2D_DEF
#define PHYSICS_2D_DEF
#include "vector2.h"

struct C_Physics_2d
{
    struct Vector2 velocity;
	float max_speed;
	float friction;
};

static inline const struct C_Physics_2d DEFAULT_PLAYER_PHYSICS = {.max_speed = 150, .friction = 650};

#endif /* PHYSICS_2D_DEF */