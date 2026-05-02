#include <SDL3/SDL.H>
#include "system_movement.h"
#include "vector2.h"
#include "ecdb_handler.h"
#include "ecdb.h"

void s_render(struct ECDB_Handler const *const ecdb_handler, struct SDL_Renderer* renderer)
{
    float length = 200;
    struct Vector2* positions = ECDB_Handler_Get_Positions(ecdb_handler);
    SDL_FColor* colors = ECDB_Handler_Get_Colors(ecdb_handler);
    for(unsigned int i = 0; i < ecdb_handler->ecdb->_maxEntities; ++i)
    {
        if(ECDB_Handler_EntityHasColor(ecdb_handler, i) && ECDB_Handler_EntityHasPosition(ecdb_handler, i))
        {
            float halfLength = length / 2;
            SDL_FColor color = colors[i];
            SDL_FRect rect = { .x = positions[i].x - halfLength, .y = positions[i].y - halfLength, .w = length, .h = length};
            SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a );
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}