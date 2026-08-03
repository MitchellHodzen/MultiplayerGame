#include "entity_builders.h"
#include "component_handles.h"
#include <SDL3/SDL.H>
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_transform.h"
#include "component_lifetime.h"
#include "component_player_state.h"
#include "component_animation.h"

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
    ECDB_EnableEntityComponent(ec, *entityId, componentHandles->player_states_handle);
    
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

bool Add_Flower(struct ECDB* ecdb, struct Component_Handles* component_handles, struct Vector2 position)
{
    unsigned int flower_id;
    if (ECDB_CreateEntity(ecdb, &flower_id) == false)
    {
        SDL_Log("Couldn't create flower");
        return false;
    }

    struct C_Transform* entity_transform = ECDB_EnableEntityComponent(ecdb, flower_id, component_handles->transforms_handle);
    entity_transform->position = position;

    struct C_Animation_Instance* anim = ECDB_EnableEntityComponent(ecdb, flower_id, component_handles->animation_instance_handle);
    anim->animation_index = 4;

    return true;
}