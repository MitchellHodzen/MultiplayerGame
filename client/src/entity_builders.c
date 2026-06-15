#include "entity_builders.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include "component_lifetime.h"

bool AddParentedText(struct ECDB* ec, struct Component_Handles* componentHandles, unsigned int parentId, struct Vector2 position, char* input_str, int* entityId)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create floating text box");
        return false;
    }

    struct C_Transform* entityTransform = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->transforms_handle);
    entityTransform->position = position;
    entityTransform->parent_id = parentId;
    char* text = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->text_handle);
    strcpy(text, input_str);
    return true;
}

bool AddParentedTextWithLifetime(struct ECDB* ec, struct Component_Handles* componentHandles, unsigned int parentId, struct Vector2 position, char* input_str, float lifetimeS, int* entityId)
{
    if (!AddParentedText(ec, componentHandles, parentId, position, input_str, entityId))
    {
        SDL_Log("Couldn't create floating text box");
        return false;
    }

    // Delete after some time
    struct C_Lifetime* lifetime = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->lifetimes_handle);
    lifetime->lifetimeS = lifetimeS;
    return true;
}

bool AddSquare(struct ECDB* ec, struct Component_Handles* componentHandles, struct Vector2 position, SDL_FColor color, int* entityId, char* playerName)
{
    if (ECDB_CreateEntity(ec, entityId) == false)
    {
        SDL_Log("Couldn't create square");
        return false;
    }

    struct C_Transform* entityTransform = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->transforms_handle);
    entityTransform->position = position;
    SDL_FColor* entityCol = ECDB_EnableEntityComponent(ec, *entityId, componentHandles->colors_handle);
    memcpy(entityCol, &color, sizeof(SDL_FColor));
    
    // If a name was passed in, create a nameplate entity underneath the square
    if (playerName != NULL)
    {
        int nameplate;
        if (!AddParentedText(ec, componentHandles, *entityId, (struct Vector2){ 0, 50}, playerName, &nameplate))
        {
            SDL_Log("Failed to create nameplate for entity %i", *entityId);
        }
    }

    int joinedText;
    if (!AddParentedTextWithLifetime(ec, componentHandles, *entityId, (struct Vector2){ 0, -60}, "Joined", 2, &joinedText))
    {
        SDL_Log("Failed to create joined text for entity %i", *entityId);
    }

    return true;
}
