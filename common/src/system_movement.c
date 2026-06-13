#include "system_movement.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"
#include "component_input.h"

void s_move(struct ECDB const *const ec, int transforms_handle, int inputs_handle, float deltaTimeS)
{
    struct C_Transform* transforms = (struct C_Transform*) ec->_componentArrays[transforms_handle];
    struct C_Input* inputs = (struct C_Input*) ec->_componentArrays[inputs_handle];
    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, transforms_handle))
        {
            // if input is being done, apply it
            if(ECDB_EntityHasComponent(ec, i, inputs_handle))
            {
                transforms[i].position.x += inputs[i].direction.x * inputs[i].speed * deltaTimeS;
                transforms[i].position.y += inputs[i].direction.y * inputs[i].speed * deltaTimeS;
            }

            // If parent has input, apply it as well
            // TODO: This is temporary, only works for one parent, and is cache thrashing
            unsigned int parentId = transforms[i].parent_id;
            if(ECDB_EntityHasComponent(ec, parentId, inputs_handle))
            {
                transforms[i].position.x += inputs[parentId].direction.x * inputs[parentId].speed * deltaTimeS;
                transforms[i].position.y += inputs[parentId].direction.y * inputs[parentId].speed * deltaTimeS;
            }
        }
    }
}