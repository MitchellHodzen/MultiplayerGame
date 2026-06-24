#include "system_interpolation.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"

void s_interpolate_position(struct ECDB const *const ec, int transforms_handle, int transforms_buffer_handle)
{
    struct C_Transform* transforms = (struct C_Transform*) ec->_componentArrays[transforms_handle];
    struct N_C_Transform_Interpolation_Buffer* trans_buf = (struct N_C_Transform_Interpolation_Buffer*) ec->_componentArrays[transforms_buffer_handle];

    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, transforms_handle) && ECDB_EntityHasComponent(ec, i, transforms_buffer_handle))
        {
            // TODO: We're faking interpolation by just shifting back 3 entries - update this with clock sync to calculate actual offset
            transforms[i].position = trans_buf[i]._buffer[3].transform.position;
        }
    }
}