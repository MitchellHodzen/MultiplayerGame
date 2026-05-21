#include "system_reset_input.h"
#include "vector2.h"
#include "ecdb.h"
#include "component_input.h"


void s_reset_input(struct ECDB const *const ecdb, int input_handle)
{
    struct C_Input* inputs = (struct C_Input*) ecdb->_componentArrays[input_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        // Don't check if we have input since we're 0ing it out
        inputs[i].direction.x = 0;
        inputs[i].direction.y = 0;
    }
}