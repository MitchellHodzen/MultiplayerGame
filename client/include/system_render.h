#ifndef SYSTEM_RENDER
#define SYSTEM_RENDER
#include <stddef.h>

struct ECDB;
struct SDL_Renderer;
struct TTF_Font;
struct TTF_TextEngine;
struct Component_Handles;
struct SDL_Texture;
struct Animation;

void s_render(struct ECDB const *const ec, unsigned int camera_id, struct Component_Handles* component_handles, struct Animation* animations, struct SDL_Texture* spritesheet, size_t chat_buffer_size, struct TTF_Font* font, struct TTF_TextEngine* textEngine, struct SDL_Renderer* renderer);

void s_render_server_ghost(struct ECDB const *const ec, unsigned int camera_id, int transforms_handle, int last_server_position_handle, int animation_instance_handle, struct Animation* animations, struct SDL_Texture* spritesheet, struct SDL_Renderer* renderer);

#endif /* SYSTEM_RENDER */