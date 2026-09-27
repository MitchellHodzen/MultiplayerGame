#ifndef COMPONENT_INPUT
#define COMPONENT_INPUT
#include "vector2.h"
#include "input_command_buffer.h"

struct C_Input
{
    float speed;
    struct Command_Buffer commands;
    bool cmnd_states[CMND_MAX_CNT];
};

#endif /* COMPONENT_INPUT */