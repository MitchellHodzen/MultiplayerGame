#ifndef COMPONENT_TRANSFORM
#define COMPONENT_TRANSFORM
#include "vector2.h"

struct C_Transform
{
    unsigned int parent_id;
    struct Vector2 position;
};

#endif /* COMPONENT_TRANSFORM */