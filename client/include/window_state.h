#ifndef WINDOW_STATE_DEF
#define WINDOW_STATE_DEF
#include <stdbool.h>

struct SDL_Window;
struct SDL_Renderer;
struct TTF_TextEngine;
struct TTF_Font;

struct Window_State
{
    struct SDL_Window* window;
    struct SDL_Renderer* renderer;
    struct TTF_TextEngine* textEngine;
    struct TTF_Font* font;
    void* clayArena;
};

bool Window_State_Init(struct Window_State** window_state, unsigned int screen_width, unsigned int screen_height);
void Window_State_Free(struct Window_State** window_state);

#endif /* WINDOW_STATE_DEF */