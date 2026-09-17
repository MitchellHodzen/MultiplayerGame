#ifndef COMPONENT_INPUT
#define COMPONENT_INPUT
#include "vector2.h"
#include "input_command_buffer.h"

struct C_Input
{
    struct Vector2 direction;
    float speed;
    struct Command_Buffer commands;
};

#endif /* COMPONENT_INPUT */