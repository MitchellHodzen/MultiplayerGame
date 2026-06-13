#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER

struct ECDB;
struct SDL_Renderer;

void s_render(struct ECDB const *const ec, int positions_handle, int colors_handle, int texts_handle, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */