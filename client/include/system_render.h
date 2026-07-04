#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER

struct ECDB;
struct SDL_Renderer;
struct TTF_Font;
struct TTF_TextEngine;
struct N_C_Transform_Interpolation_Buffer;

void s_render(struct ECDB const *const ec, int transforms_handle, int colors_handle, int texts_handle, struct TTF_Font* font, struct TTF_TextEngine* textEngine, struct SDL_Renderer* renderer);

void s_render_server_ghost(struct ECDB const *const ec, int trans_buf_handle, struct N_C_Transform_Interpolation_Buffer* network_trans_buffers, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */