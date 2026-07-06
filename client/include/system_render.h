#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER

struct ECDB;
struct SDL_Renderer;
struct TTF_Font;
struct TTF_TextEngine;

void s_render(struct ECDB const *const ec, int transforms_handle, int colors_handle, int texts_handle, struct TTF_Font* font, struct TTF_TextEngine* textEngine, struct SDL_Renderer* renderer);

void s_render_server_ghost(struct ECDB const *const ec, int last_server_position_handle, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */