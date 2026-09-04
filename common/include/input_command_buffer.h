#ifndef INPUT_COMMAND_BUF_DEF
#define INPUT_COMMAND_BUF_DEF
#include "stdbool.h"

#define EVENT_BUF_SIZE 100

enum COMMAND
{
    UNDETERMINED,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    ATTACK,
    TOGGLE_INTERPOLATION,
    TOGGLE_PREDICTION,
    START_CHAT
};

struct Command_Entry
{
    enum COMMAND command;
    bool pressed; // if not pressed then released
};

struct Command_Buffer
{
    struct Command_Entry command_queue[EVENT_BUF_SIZE];
    unsigned int command_cnt;
};

#endif /* INPUT_COMMAND_BUF_DEF */