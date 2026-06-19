#ifndef PACKETS
#define PACKETS
#include "vector2.h"

enum Packet_Type
{
    REQUEST_JOIN,
    JOIN_SERVER,
    ADD_SQUARE,
    INPUT_DIRECTION,
    UPDATE,
    UNUSED,
};

struct P_JOIN_SERVER
{
    enum Packet_Type type;
    unsigned int max_entities;
    unsigned int max_chat_length;
    unsigned int network_id;
    unsigned int ticks_per_s;
    struct Vector2 position;
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

enum Message_Importance
{
    MESSAGE_IMPORTANCE_STANDARD,
    MESSAGE_IMPORTANCE_ALERT,
    MESSAGE_IMPORTANCE_LOW
};

struct P_Chat_Header
{
    enum Message_Importance messageImportance;
    bool isServerMessage;
    unsigned int networkId;
};


#endif /* PACKETS */