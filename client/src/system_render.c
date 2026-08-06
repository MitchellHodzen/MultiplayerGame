#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include "system_render.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"
#include "component_player_state.h"
#include "component_handles.h"
#include "camera.h"
#include "component_animation.h"

struct y_order_entry
{
    struct C_Animation_Instance animation_instance;
    struct Vector2 position;
    int y_offset;
};

void render_sprite(struct C_Animation_Instance* animation_instance, struct Vector2 position, struct Animation* animations, struct SDL_Texture* spritesheet, struct SDL_Renderer* renderer)
{
    struct Animation_Frame current_frame = animations[animation_instance->animation_index].frames[animation_instance->current_frame];
    SDL_FRect sprite_rect = { .x = current_frame.spritesheet_clip_x, .y = current_frame.spritesheet_clip_y, .w = current_frame.spritesheet_clip_width, .h = current_frame.spritesheet_clip_height};

    float scale = 5;
    float width = current_frame.spritesheet_clip_width * scale;
    float height = current_frame.spritesheet_clip_height * scale;
    SDL_FRect pos_rect = { .x = position.x - (width / 2), .y = position.y - (height / 2), .w = width, .h = height};

    // reposition the position rect based on the camera
    SDL_RenderTexture(renderer, spritesheet, &sprite_rect, &pos_rect);
}

void s_render(struct ECDB const *const ec, unsigned int camera_id, struct Component_Handles* component_handles, struct Animation* animations, struct SDL_Texture* spritesheet, size_t chat_buffer_size, struct TTF_Font* font, struct TTF_TextEngine* textEngine, struct SDL_Renderer* renderer)
{
    // TODO: move buffer elsewhere and dont hardcode size
    struct y_order_entry y_order_buffer[100];
    memset(y_order_buffer, 0, sizeof(struct y_order_entry) * 100);
    unsigned int y_buffer_cnt = 0;

    unsigned int transforms_handle = component_handles->transforms_handle;
    unsigned int colors_handle = component_handles->colors_handle;
    unsigned int player_states_handle = component_handles->player_states_handle;
    unsigned int texts_handle = component_handles->text_handle;
    unsigned int animation_instance_handle = component_handles->animation_instance_handle;
    unsigned int y_order_handle = component_handles->y_order_handle;

    // Draw the squares
    float length = 75;
    struct C_Transform* transforms = (struct C_Transform*) ec->componentArrays[transforms_handle];
    SDL_FColor* colors = (SDL_FColor*) ec->componentArrays[colors_handle];
    struct C_Player_State* states = (struct C_Player_State*) ec->componentArrays[player_states_handle];
    struct C_Animation_Instance* animation_instances = (struct C_Animation_Instance*) ec->componentArrays[animation_instance_handle];
    int* y_order_offsets = (int*) ec->componentArrays[y_order_handle];

    struct Vector2 camera_position = transforms[camera_id].position;

    char* texts = (char*) ec->componentArrays[texts_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        // Only draw if there is a position
        if (ECDB_EntityHasComponent(ec, i, transforms_handle))
        {
            // Calculate the position in global space
            struct Vector2 global_position = { .x = transforms[i].position.x, .y = transforms[i].position.y };

            // TODO: This is temporary, only works for one parent, and is cache thrashing
            unsigned int parentId = transforms[i].parent_id;
            if(ECDB_EntityHasComponent(ec, parentId, transforms_handle))
            {
                global_position.x += transforms[parentId].position.x;
                global_position.y += transforms[parentId].position.y;
            }
            
            // Offset for the camera
            global_position.x -= camera_position.x;
            global_position.y -= camera_position.y;

            /*// draw squares
            if(ECDB_EntityHasComponent(ec, i, colors_handle))
            {
                float halfLength = length / 2;
                SDL_FColor color = colors[i];
                if (states[i].state == ATTACKING)
                {
                    color.g -= 100;
                    color.b -= 100;
                }
                else if (states[i].state == RUNNING)
                {
                    color.r -= 100;
                    color.b -= 100;
                }

                SDL_FRect rect = { .x = global_position.x - halfLength, .y = global_position.y - halfLength, .w = length, .h = length};
                SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
                SDL_RenderFillRect(renderer, &rect);
            }*/

            // draw sprites
            if (ECDB_EntityHasComponent(ec, i, animation_instance_handle))
            {
                if (ECDB_EntityHasComponent(ec, i, y_order_handle))
                {
                    // add to the Y order buffer to be drawn later
                    struct y_order_entry entry = {.animation_instance = animation_instances[i], .position = global_position, .y_offset = y_order_offsets[i]};

                    float y_pos_to_check = entry.position.y + entry.y_offset;

                    // todo: Replace with better sort that doesn't take o(n)
                    for(unsigned int j = 0; j <= y_buffer_cnt; ++j)
                    {
                        // If the time on the snapshot is more recent than the current index, insert the snapshot and shift everything down
                        if (entry.position.y < y_order_buffer[j].position.y || j == y_buffer_cnt)
                        {
                            // move everything after the current index over one, starting from the back
                            for(int k = y_buffer_cnt; k > j; --k)
                            {
                                y_order_buffer[k] = y_order_buffer[k - 1];
                            }

                            // Insert the new snapshot into the buffer
                            y_order_buffer[j] = entry;

                            // Increase buffer size if it isnt maxed out
                            y_buffer_cnt++;

                            break;
                        }
                    }
                }
                else
                {
                    // If no y order component, draw now
                    render_sprite(&animation_instances[i], global_position, animations, spritesheet, renderer);
                }
            }
        }
    }

    // draw the buffered sprites
    for(unsigned int i = 0; i < y_buffer_cnt; ++i)
    {
        render_sprite(&y_order_buffer[i].animation_instance, y_order_buffer[i].position, animations, spritesheet, renderer);
    }

    // Draw text above everything else
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        // Only draw if there is a position
        if (ECDB_EntityHasComponent(ec, i, transforms_handle) && ECDB_EntityHasComponent(ec, i, texts_handle))
        {
            // Calculate the position in global space
            struct Vector2 global_position = { .x = transforms[i].position.x, .y = transforms[i].position.y };

            // TODO: This is temporary, only works for one parent, and is cache thrashing
            unsigned int parentId = transforms[i].parent_id;
            if(ECDB_EntityHasComponent(ec, parentId, transforms_handle))
            {
                global_position.x += transforms[parentId].position.x;
                global_position.y += transforms[parentId].position.y;
            }
            
            // Offset for the camera
            global_position.x -= camera_position.x;
            global_position.y -= camera_position.y;

            char* entity_text = texts + (chat_buffer_size * i);
            // TODO: Terrible, do not create a ttf text each loop entry
            TTF_Text* text = TTF_CreateText(textEngine, font, entity_text, 0); // 0 length for null terminated

            // Offset the position based on the text size
            int textWidth, textHeight;
            if (TTF_GetTextSize(text, &textWidth, &textHeight))
            {
                // transform position is the center of the text, so offset the x by half
                float newXPos = global_position.x - (textWidth / 2);
                float newYPos = global_position.y - (textHeight / 2);
                if (!TTF_DrawRendererText(text, newXPos, newYPos))
                {
                    SDL_Log("Failed to render text: %s", SDL_GetError());
                }
            }
            else
            {
                SDL_Log("Failed to get text dimsensions: %s", SDL_GetError());

            }
            TTF_DestroyText(text);
        }
    }
}

void s_render_server_ghost(struct ECDB const *const ec, unsigned int camera_id, int transforms_handle, int last_server_position_handle, int animation_instance_handle, struct Animation* animations, struct SDL_Texture* spritesheet, struct SDL_Renderer* renderer)
{
    // Render a ghost square at the most recently received server position
    float length = 75;
    struct Vector2* last_server_position_buffer = (struct Vector2*) ec->componentArrays[last_server_position_handle];
    struct C_Animation_Instance* animation_instances = (struct C_Animation_Instance*) ec->componentArrays[animation_instance_handle];
    struct Vector2 camera_position = ((struct C_Transform*) ECDB_GetEntityComponent(ec, camera_id, transforms_handle))->position;

    // Make texture transparent
    SDL_SetTextureAlphaMod(spritesheet, 60);

    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if (ECDB_EntityHasComponent(ec, i, last_server_position_handle))
        {
            // Get the most recent transform from the server
            struct Vector2 last_server_position = last_server_position_buffer[i];
            last_server_position.x -= camera_position.x;
            last_server_position.y -= camera_position.y;

            // draw sprites
            if (ECDB_EntityHasComponent(ec, i, animation_instance_handle))
            {
                struct Animation_Frame current_frame = animations[animation_instances[i].animation_index].frames[animation_instances[i].current_frame];
                SDL_FRect sprite_rect = { .x = current_frame.spritesheet_clip_x, .y = current_frame.spritesheet_clip_y, .w = current_frame.spritesheet_clip_width, .h = current_frame.spritesheet_clip_height};

                float scale = 5;
                float width = current_frame.spritesheet_clip_width * scale;
                float height = current_frame.spritesheet_clip_height * scale;
                SDL_FRect pos_rect = { .x = last_server_position.x - (width / 2), .y = last_server_position.y - (height / 2), .w = width, .h = height};

                // reposition the position rect based on the camera
                SDL_RenderTexture(renderer, spritesheet, &sprite_rect, &pos_rect);
            }
            else
            {
                // draw ghost square
                float halfLength = length / 2;
                SDL_FColor color = { .r = 1, .g = 1, .b = 1, .a = 0.2};
                SDL_FRect rect = { .x = last_server_position.x - halfLength, .y = last_server_position.y - halfLength, .w = length, .h = length};
                SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // Undo transparency to the base texture
    SDL_SetTextureAlphaMod(spritesheet, 255);
}