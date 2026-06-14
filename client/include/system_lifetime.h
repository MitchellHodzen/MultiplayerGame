#ifndef SYSTEM_LIFETIME
#define SYSTEM_LIFETIME
#include "component_lifetime.h"
#include "ecdb.h"

void s_lifetime_iterate(struct ECDB* ecdb, int lifetimes_handle, float deltaTimeS)
{
    struct C_Lifetime* lifetimes = (struct C_Lifetime*) ecdb->_componentArrays[lifetimes_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        // If the entity has a lifetime, iterate it
        if(ECDB_EntityHasComponent(ecdb, i, lifetimes_handle))
        {
            lifetimes[i]._elapsed += deltaTimeS;
            if (lifetimes[i]._elapsed >= lifetimes[i].lifetimeS)
            {
                // If the amount of time passed is greater than or equal to the lifetime, mark completed
                lifetimes[i].completed = true;
            }
        }
    }
}

void s_lifetime_remove(struct ECDB* ecdb, int lifetimes_handle)
{
    struct C_Lifetime* lifetimes = (struct C_Lifetime*) ecdb->_componentArrays[lifetimes_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        // If the entity has a completed lifetime, remove the entity
        if(ECDB_EntityHasComponent(ecdb, i, lifetimes_handle) && lifetimes[i].completed == true)
        {
            ECDB_DestroyEntity(ecdb, i);
        }
    }
}

#endif /* SYSTEM_LIFETIME */