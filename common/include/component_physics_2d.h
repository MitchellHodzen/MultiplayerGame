#ifndef PHYSICS_2D_DEF
#define PHYSICS_2D_DEF
#include "vector2.h"

struct C_Physics_2d
{
    struct Vector2 velocity;
	float max_speed;
	float friction;
};

#endif /* PHYSICS_2D_DEF */