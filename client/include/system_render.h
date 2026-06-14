#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER

struct ECDB;
struct SDL_Renderer;
struct TTF_Font;
struct TTF_TextEngine;

void s_render(struct ECDB const *const ec, int positions_handle, int colors_handle, int texts_handle, struct TTF_Font* font, struct TTF_TextEngine* textEngine, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */