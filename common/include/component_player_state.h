#ifndef PLAYER_STATES_DEF
#define PLAYER_STATES_DEF

enum Player_State_Type
{
    IDLE,
    RUNNING,
    ATTACKING
};

enum Player_Direction
{
    PLAYER_UP,
    PLAYER_DOWN,
    PLAYER_LEFT,
    PLAYER_RIGHT
};

struct C_Player_State
{
    enum Player_State_Type state;
    enum Player_Direction direction;
};

#endif /* PLAYER_STATES_DEF */