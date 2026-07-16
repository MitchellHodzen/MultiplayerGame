#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include "system_render.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"
#include "component_player_state.h"

void s_render(struct ECDB const *const ec, int transforms_handle, int colors_handle, int texts_handle, int player_states_handle, TTF_Font* font, TTF_TextEngine* textEngine, SDL_Renderer* renderer)
{
    // Draw the squares
    float length = 75;
    struct C_Transform* transforms = (struct C_Transform*) ec->componentArrays[transforms_handle];
    SDL_FColor* colors = (SDL_FColor*) ec->componentArrays[colors_handle];
    enum Player_State* states = (enum Player_State*) ec->componentArrays[player_states_handle];

    char (*texts)[100 + 1] = (char (*)[100 + 1]) ec->componentArrays[texts_handle];
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

            // draw squares
            if(ECDB_EntityHasComponent(ec, i, colors_handle))
            {
                float halfLength = length / 2;
                SDL_FColor color = colors[i];
                if (states[i] == RUNNING)
                {
                    // if running make it a little more red
                    color.g -= 100;
                    color.b -= 100;
                }

                SDL_FRect rect = { .x = global_position.x - halfLength, .y = global_position.y - halfLength, .w = length, .h = length};
                SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
                SDL_RenderFillRect(renderer, &rect);
            }

            // draw text
            if(ECDB_EntityHasComponent(ec, i, texts_handle))
            {
                // TODO: Terrible, do not create a ttf text each loop entry
                TTF_Text* text = TTF_CreateText(textEngine, font, texts[i], 0); // 0 length for null terminated

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
}

void s_render_server_ghost(struct ECDB const *const ec, int last_server_position_handle, struct SDL_Renderer* renderer)
{
    // Render a ghost square at the most recently received server position
    float length = 75;
    struct Vector2* last_server_position_buffer = (struct Vector2*) ec->componentArrays[last_server_position_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if (ECDB_EntityHasComponent(ec, i, last_server_position_handle))
        {
            // Get the most recent transform from the server
            struct Vector2 last_server_position = last_server_position_buffer[i];

            // draw ghost square
            float halfLength = length / 2;
            SDL_FColor color = { .r = 1, .g = 1, .b = 1, .a = 0.2};
            SDL_FRect rect = { .x = last_server_position.x - halfLength, .y = last_server_position.y - halfLength, .w = length, .h = length};
            SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}