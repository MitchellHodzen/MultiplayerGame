#include <SDL3/SDL.H>
#include "system_movement.h"
#include "vector2.h"
#include "ecdb.h"

void s_render(struct ECDB const *const ec, int positions_handle, int colors_handle, SDL_Renderer* renderer)
{
    float length = 200;
    Vector2* positions = (Vector2*) ec->_componentArrays[positions_handle];
    SDL_FColor* colors = (SDL_FColor*) ec->_componentArrays[colors_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, colors_handle) && ECDB_EntityHasComponent(ec, i, positions_handle))
        {
            float halfLength = length / 2;
            SDL_FColor color = colors[i];
            SDL_FRect rect = { .x = positions[i].x - halfLength, .y = positions[i].y - halfLength, .w = length, .h = length};
            SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}