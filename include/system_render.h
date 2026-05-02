#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER

struct ECDB_Handler;
struct SDL_Renderer;

void s_render(struct ECDB_Handler const *const ecdb_handler, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */