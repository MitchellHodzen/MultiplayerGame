#include "system_movement.h"
#include <stdio.h>
#include "vector2.h"
#include "ecdb.h"
#include "component_input.h"

void s_move(struct ECDB const *const ec, int positions_handle, int inputs_handle, float deltaTimeS)
{
    int counter = 0;
    Vector2* positions = (Vector2*) ec->_componentArrays[positions_handle];
    C_Input* inputs = (C_Input*) ec->_componentArrays[inputs_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, positions_handle) && ECDB_EntityHasComponent(ec, i, inputs_handle))
        {
            positions[i].x += inputs[i].direction.x * inputs[i].speed * deltaTimeS;
            positions[i].y += inputs[i].direction.y * inputs[i].speed * deltaTimeS;
            counter++;
        }
    }
    printf("entities moved: %i\n", counter);
}