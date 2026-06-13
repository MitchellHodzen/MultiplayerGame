#include <SDL3/SDL.H>
#include "system_movement.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"

void s_render(struct ECDB const *const ec, int transforms_handle, int colors_handle, SDL_Renderer* renderer)
{
    // Clear previous render before drawing
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE ); // Black
    SDL_RenderClear(renderer);

    // Draw the squares
    float length = 200;
    struct C_Transform* transforms = (struct C_Transform*) ec->_componentArrays[transforms_handle];
    SDL_FColor* colors = (SDL_FColor*) ec->_componentArrays[colors_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, colors_handle) && ECDB_EntityHasComponent(ec, i, transforms_handle))
        {
            float halfLength = length / 2;
            SDL_FColor color = colors[i];
            SDL_FRect rect = { .x = transforms[i].position.x - halfLength, .y = transforms[i].position.y - halfLength, .w = length, .h = length};
            SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}