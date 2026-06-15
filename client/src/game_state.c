#include "game_state.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include "component_lifetime.h"

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount, unsigned int max_chat_size)
{
    if (!ECDB_Init(ecdb, entityCount, 5))
    {
        SDL_Log("Couldn't initialize component DB");
        return false;
    }
    
    struct C_Transform defaultTransform = {.parent_id = (*ecdb)->invalidEntityId};
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Transform), &(componentHandles->transforms_handle), &defaultTransform))
    {
        SDL_Log("Couldn't initialize transform component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(SDL_FColor), &(componentHandles->colors_handle), NULL))
    {
        SDL_Log("Couldn't initialize colors component");
        ECDB_Free(ecdb);
        return false;
    }
    
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Input), &(componentHandles->inputs_handle), NULL))
    {
        SDL_Log("Couldn't initialize input component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(char) * (max_chat_size + 1), &(componentHandles->text_handle), NULL))
    {
        SDL_Log("Couldn't initialize text component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Lifetime), &(componentHandles->lifetimes_handle), NULL))
    {
        SDL_Log("Couldn't initialize lifetime component");
        ECDB_Free(ecdb);
        return false;
    }

    return true;
}

bool InitializeNetworkTracking(struct ECDB* ecdb, unsigned int** entityNetworkIdMap, unsigned int** networkIdEntityMap)
{
    unsigned int mapSize = sizeof(unsigned int) * (ecdb->_maxEntities + 1); // Add 1 for the invalid index
    *entityNetworkIdMap = malloc(mapSize);
    *networkIdEntityMap = malloc(mapSize);
    if (*entityNetworkIdMap == NULL || *networkIdEntityMap == NULL)
    {
        printf("Allocation of entity tracking data structures failed\n");
        return false;
    }

    // for each network ID, set the value to the invalid value
    for(unsigned int i = 0; i < ecdb->_maxEntities + 1; i++)
    {
        (*entityNetworkIdMap)[i] = ecdb->invalidEntityId;
        (*networkIdEntityMap)[i] = ecdb->invalidEntityId;
    }
}

bool Game_Data_Init(struct Game_Data** gameData, unsigned int max_entities, unsigned int max_chat_size)
{
    *gameData = (struct Game_Data*) malloc(sizeof(struct Game_Data));
    if (*gameData == NULL)
    {
        SDL_Log("Could not allocate game data struct");
        return false;
    }

    if (InitializeECDB(&(*gameData)->ec, &(*gameData)->componentHandles, max_entities, max_chat_size))
    {
        SDL_Log("ECDB Initialized");
    }
    else
    {
        SDL_Log("ECDB Initialization Failed");
        return false;
    }

    if (InitializeNetworkTracking((*gameData)->ec, &(*gameData)->entityNetworkIdMap, &(*gameData)->networkIdEntityMap))
    {
        SDL_Log("Network Entity Tracking Initialized");
    }
    else
    {
        SDL_Log("Network Entity Tracking Initialization Failed");
        return false;
    }
    return true;
}

void Game_Data_Free(struct Game_Data** gameData)
{
    // Close up
    SDL_Log("free network entity tracking");
    free((*gameData)->entityNetworkIdMap);
    (*gameData)->entityNetworkIdMap = NULL;
    free((*gameData)->networkIdEntityMap);
    (*gameData)->networkIdEntityMap = NULL;

    SDL_Log("free ecdb");
    ECDB_Free(&(*gameData)->ec);
    (*gameData)->ec = NULL;

    *gameData = NULL;
}