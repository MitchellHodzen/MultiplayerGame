#include "ecdb_handler.h"
#include <stdlib.h>
#include <stdbool.h>
#include <SDL3/SDL.H>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"

bool ECDB_Handler_Init(struct ECDB_Handler** ecdb_handler, unsigned int maxEntities)
{
    *ecdb_handler = (struct ECDB_Handler*) malloc(sizeof(struct ECDB_Handler));
    if (*ecdb_handler == NULL)
    {
        // Couldnt instantiate an ecdb handler
        return false;
    }

    (*ecdb_handler)->ecdb = (struct ECDB*) malloc(sizeof(struct ECDB));

    if ((*ecdb_handler)->ecdb == NULL)
    {
        // Couldnt instantiate an ecdb
        ECDB_Handler_Free(ecdb_handler);
        return false;
    }

    if (!ECDB_Init((*ecdb_handler)->ecdb, maxEntities, 3))
    {
        // Couldn't initialize the ecdb
        ECDB_Handler_Free(ecdb_handler);
        return false;
    }

    if (!ECDB_RegisterComponent((*ecdb_handler)->ecdb, sizeof(struct Vector2), &((*ecdb_handler)->positions_handle)))
    {
        // Couldn't initialize positions component
        ECDB_Handler_Free(ecdb_handler);
        return false;
    }
    if (!ECDB_RegisterComponent((*ecdb_handler)->ecdb, sizeof(SDL_FColor), &((*ecdb_handler)->colors_handle)))
    {
        // Couldn't initialize colors component
        ECDB_Handler_Free(ecdb_handler);
        return false;
    }
    if (!ECDB_RegisterComponent((*ecdb_handler)->ecdb, sizeof(struct C_Input), &((*ecdb_handler)->inputs_handle)))
    {
        // Couldn't initialize input component
        ECDB_Handler_Free(ecdb_handler);
        return false;
    }
    return true;
}

void ECDB_Handler_Free(struct ECDB_Handler** ecdb_handler)
{
    ECDB_Free((*ecdb_handler)->ecdb);
    free(*ecdb_handler);
    *ecdb_handler = NULL;
}

struct Vector2* ECDB_Handler_Get_Positions(struct ECDB_Handler const *const ecdb_handler)
{
    return (struct Vector2*) ecdb_handler->ecdb->_componentArrays[ecdb_handler->positions_handle];
}

struct C_Input* ECDB_Handler_Get_Inputs(struct ECDB_Handler const *const ecdb_handler)
{
    return (struct C_Input*) ecdb_handler->ecdb->_componentArrays[ecdb_handler->inputs_handle];
}

SDL_FColor* ECDB_Handler_Get_Colors(struct ECDB_Handler const *const ecdb_handler)
{
    return (struct SDL_FColor*) ecdb_handler->ecdb->_componentArrays[ecdb_handler->colors_handle];
}

bool ECDB_Handler_EntityHasPosition(struct ECDB_Handler const *const ecdb_handler, int entityId)
{
    return ECDB_EntityHasComponent(ecdb_handler->ecdb, entityId, ecdb_handler->positions_handle);
}

bool ECDB_Handler_EntityHasInput(struct ECDB_Handler const *const ecdb_handler, int entityId)
{
    return ECDB_EntityHasComponent(ecdb_handler->ecdb, entityId, ecdb_handler->inputs_handle);
}

bool ECDB_Handler_EntityHasColor(struct ECDB_Handler const *const ecdb_handler, int entityId)
{
    return ECDB_EntityHasComponent(ecdb_handler->ecdb, entityId, ecdb_handler->colors_handle);
}
