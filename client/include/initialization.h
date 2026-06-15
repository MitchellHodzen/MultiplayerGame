#ifndef INIT_DEF
#define INIT_DEF
#include <stdbool.h>
#include "component_handles.h"
#include <clay.h>

struct ECDB;
struct SDL_Window;
struct SDL_Renderer;
struct TTF_TextEngine;
struct TTF_Font;
struct Net_Manager;

struct Init_Vars
{
    unsigned int screen_width;
    unsigned int screen_height;
    unsigned int max_entities;
    unsigned int max_chat_size;
};

struct Game_Data
{
    struct SDL_Window* window;
    struct SDL_Renderer* renderer;
    struct TTF_TextEngine* textEngine;
    struct TTF_Font* font;
    void* clayArena;
    struct ECDB* ec;
    struct Component_Handles componentHandles;
    unsigned int* entityNetworkIdMap;
    unsigned int* networkIdEntityMap;
};

bool Game_Data_Init(struct Game_Data** gameData, struct Init_Vars* init_vars, Clay_ErrorHandler errorHandler, Clay_Dimensions (*measureTextFunction)(Clay_StringSlice text, Clay_TextElementConfig *config));
void Game_Data_Free(struct Game_Data** gameData);

#endif /* INIT_DEF */