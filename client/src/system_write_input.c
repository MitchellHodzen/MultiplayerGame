#include "system_write_input.h"
#include "vector2.h"
#include "ecdb.h"
#include "component_input.h"

void s_write_input(struct ECDB const *const ecdb, int input_handle, struct Vector2 input)
{
    struct C_Input* inputs = (struct C_Input*) ecdb->_componentArrays[input_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ecdb, i, input_handle))
        {
            inputs[i].direction.x = input.x;
            inputs[i].direction.y = input.y;
        }
    }
}