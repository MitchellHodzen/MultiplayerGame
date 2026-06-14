#include <SDL3/SDL.H>
#include <SDL3_ttf/SDL_ttf.h>
#include "system_movement.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"

void s_render(struct ECDB const *const ec, int transforms_handle, int colors_handle, int texts_handle, TTF_Font* font, TTF_TextEngine* textEngine, SDL_Renderer* renderer)
{
    // Clear previous render before drawing
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); // Black
    SDL_RenderClear(renderer);

    // Draw the squares
    float length = 75;
    struct C_Transform* transforms = (struct C_Transform*) ec->_componentArrays[transforms_handle];
    SDL_FColor* colors = (SDL_FColor*) ec->_componentArrays[colors_handle];
    char (*texts)[100 + 1] = (char (*)[100 + 1]) ec->_componentArrays[texts_handle];
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
                    if (!TTF_DrawRendererText(text, newXPos, global_position.y))
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