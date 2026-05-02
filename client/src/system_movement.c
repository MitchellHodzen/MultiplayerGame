#include "system_movement.h"
#include "vector2.h"
#include "ecdb_handler.h"
#include "ecdb.h"
#include "component_input.h"

void s_move(struct ECDB_Handler const *const ecdb_handler, float deltaTimeS)
{
    int counter = 0;
    struct Vector2* positions = ECDB_Handler_Get_Positions(ecdb_handler);
    struct C_Input* inputs = ECDB_Handler_Get_Inputs(ecdb_handler);
    for(unsigned int i = 0; i < ecdb_handler->ecdb->_maxEntities; ++i)
    {
        if(ECDB_Handler_EntityHasPosition(ecdb_handler, i) && ECDB_Handler_EntityHasInput(ecdb_handler, i))
        {
            positions[i].x += inputs[i].direction.x * inputs[i].speed * deltaTimeS;
            positions[i].y += inputs[i].direction.y * inputs[i].speed * deltaTimeS;
            counter++;
        }
    }
}