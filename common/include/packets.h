#ifndef PACKETS
#define PACKETS
#include "vector2.h"

enum Packet_Type
{
    REQUEST_JOIN,
    ADD_SQUARE,
    INPUT_DIRECTION,
    UPDATE,
    UNUSED,
};

struct P_Add_Square
{
    enum Packet_Type type;
    unsigned int networkId;
    struct Vector2 position;
};

struct P_Input_Direction
{
    enum Packet_Type type;
    unsigned int networkId;
    struct Vector2 direction;
};

struct P_Update
{
    enum Packet_Type type;
    unsigned int networkId;
    struct Vector2 position;
};

struct P_Chat_Header
{
    unsigned int networkId;
};


#endif /* PACKETS */