#include "system_physics.h"
#include "vector2.h"
#include "ecdb.h"
#include "component_transform.h"
#include "component_input.h"
#include "component_physics_2d.h"

float apply_friction_on_axis(float velocity_leg, float friction, float deltaTimeS)
{
    if (velocity_leg > 0)
    {
        // if velocity is positive, friction is negative
        velocity_leg -= (friction * deltaTimeS);

        // if velocity is now negative, zero out
        if (velocity_leg < 0)
        {
            velocity_leg = 0;
        }
    }
    else if (velocity_leg < 0)
    {
        // if velocity is negative, friction is positive
        velocity_leg += (friction * deltaTimeS);

        // if velocity is now positive, zero out
        if (velocity_leg > 0)
        {
            velocity_leg = 0;
        }
    }

    return velocity_leg;
}

void s_update_physics(struct ECDB const *const ecdb, int physics_2d_handle, int inputs_handle, float deltaTimeS)
{
    struct C_Physics_2d* physics = (struct C_Physics_2d*) ecdb->componentArrays[physics_2d_handle];
    struct C_Input* inputs = (struct C_Input*) ecdb->componentArrays[inputs_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        // if input is being done, apply it to physics
        if(ECDB_EntityHasComponent(ecdb, i, physics_2d_handle) && ECDB_EntityHasComponent(ecdb, i, inputs_handle))
        {
            physics[i].velocity.x += inputs[i].direction.x * inputs[i].speed * deltaTimeS;
            physics[i].velocity.y += inputs[i].direction.y * inputs[i].speed * deltaTimeS;

            // apply friction
            physics[i].velocity.x = apply_friction_on_axis(physics[i].velocity.x, physics[i].friction, deltaTimeS);
            physics[i].velocity.y = apply_friction_on_axis(physics[i].velocity.y, physics[i].friction, deltaTimeS);
        }
    }
}

void s_apply_physics(struct ECDB const *const ecdb, int physics_2d_handle, int transforms_handle, float deltaTimeS)
{
    struct C_Transform* transforms = (struct C_Transform*) ecdb->componentArrays[transforms_handle];
    struct C_Physics_2d* physics = (struct C_Physics_2d*) ecdb->componentArrays[physics_2d_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        // if input is being done, apply it
        if(ECDB_EntityHasComponent(ecdb, i, transforms_handle) && ECDB_EntityHasComponent(ecdb, i, physics_2d_handle))
        {
            transforms[i].position.x += physics[i].velocity.x * deltaTimeS;
            transforms[i].position.y += physics[i].velocity.y * deltaTimeS;
        }
    }
}