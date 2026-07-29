#ifndef INIT_DEF
#define INIT_DEF
#include <stdbool.h>
#include "component_handles.h"
#include "component_animation.h"

struct ECDB;
struct Chat_Buffers;

struct Game_Data
{
    struct ECDB* ec;
    struct Chat_Buffers* chat_buffers;
    struct Component_Handles componentHandles;
    unsigned int* networkIdEntityMap;
    struct Animation animations[10];
    unsigned int animation_count;
};

bool Game_Data_Init(struct Game_Data** gameData, unsigned int max_entities, unsigned int max_chat_size, unsigned int chat_history_size);
void Game_Data_Free(struct Game_Data** gameData);

#endif /* INIT_DEF */