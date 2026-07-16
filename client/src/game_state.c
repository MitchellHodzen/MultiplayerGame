#include "game_state.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include "ecdb.h"
#include "chat_buffers.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include "component_lifetime.h"
#include "component_physics_2d.h"
#include "component_player_state.h"

bool InitializeECDB(struct ECDB** ecdb, struct Component_Handles* componentHandles, unsigned int entityCount, unsigned int max_chat_size)
{
    if (!ECDB_Init(ecdb, entityCount, 10))
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
    
    struct N_C_Transform_Interpolation_Buffer default_trans_buffer;
    memset(default_trans_buffer._buffer, 0, NET_TRANS_BUF_SIZE * sizeof(struct N_C_Transform_Snapshot));
    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct N_C_Transform_Interpolation_Buffer), &(componentHandles->transforms_interpolation_buffer_handle), &default_trans_buffer))
    {
        SDL_Log("Couldn't initialize networked transform interpolation buffer component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct Vector2), &(componentHandles->last_server_position_handle), NULL))
    {
        SDL_Log("Couldn't initialize last server position component");
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

    if (!ECDB_RegisterComponent(*ecdb, sizeof(unsigned int), &(componentHandles->network_id_handle), NULL))
    {
        SDL_Log("Couldn't initialize network id component");
        ECDB_Free(ecdb);
        return false;
    }

    if (!ECDB_RegisterComponent(*ecdb, sizeof(struct C_Physics_2d), &(componentHandles->physics_2d_handle), NULL))
    {
        SDL_Log("Couldn't initialize physics component");
        ECDB_Free(ecdb);
        return false;
    }

    enum Player_State default_state = IDLE;
    if (!ECDB_RegisterComponent(*ecdb, sizeof(enum Player_State), &(componentHandles->player_states_handle), &default_state))
    {
        SDL_Log("Couldn't initialize player state component");
        ECDB_Free(ecdb);
        return false;
    }

    return true;
}

bool InitializeNetworkTracking(struct ECDB* ecdb, unsigned int** networkIdEntityMap)
{
    unsigned int mapSize = sizeof(unsigned int) * (ecdb->_maxEntities + 1); // Add 1 for the invalid index
    *networkIdEntityMap = malloc(mapSize);
    if (*networkIdEntityMap == NULL)
    {
        SDL_Log("Allocation of entity tracking data structures failed");
        return false;
    }

    // for each network ID, set the value to the invalid value
    for(unsigned int i = 0; i < ecdb->_maxEntities + 1; i++)
    {
        (*networkIdEntityMap)[i] = ecdb->invalidEntityId;
    }

    return true;
}

bool Game_Data_Init(struct Game_Data** gameData, unsigned int max_entities, unsigned int max_chat_size, unsigned int chat_history_size)
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

    if (InitializeNetworkTracking((*gameData)->ec, &(*gameData)->networkIdEntityMap))
    {
        SDL_Log("Network Entity Tracking Initialized");
    }
    else
    {
        SDL_Log("Network Entity Tracking Initialization Failed");
        return false;
    }

    if (Chat_Initialize(&(*gameData)->chat_buffers, max_chat_size, chat_history_size))
    {
        SDL_Log("Chat buffers Initialized");
    }
    else
    {
        SDL_Log("Chat buffers Initialization Failed");
        return false;
    }

    return true;
}

void Game_Data_Free(struct Game_Data** gameData)
{
    SDL_Log("free chat");
    Chat_Free(&(*gameData)->chat_buffers);
    (*gameData)->chat_buffers = NULL;

    SDL_Log("free network entity tracking");
    free((*gameData)->networkIdEntityMap);
    (*gameData)->networkIdEntityMap = NULL;

    SDL_Log("free ecdb");
    ECDB_Free(&(*gameData)->ec);
    (*gameData)->ec = NULL;

    *gameData = NULL;
}