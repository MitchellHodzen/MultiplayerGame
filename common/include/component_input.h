#ifndef COMPONENT_INPUT
#define COMPONENT_INPUT
#include "vector2.h"
#include "input_command_buffer.h"

struct C_Input
{
    float speed;
    struct Command_Snapshot cmnd_snapshot
};

#endif /* COMPONENT_INPUT */