#include "camera.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"

void s_camera_reposition(const struct ECDB* ecdb, unsigned int camera_id, unsigned int transform_handle, unsigned int camera_component_handle, unsigned int level_width, unsigned int level_height)
{
    struct C_Transform* camera_transform = (struct C_Transform*) ECDB_GetEntityComponent(ecdb, camera_id, transform_handle);
    struct C_Camera* camera_component = (struct C_Camera*) ECDB_GetEntityComponent(ecdb, camera_id, camera_component_handle);

    // If the target is valid and has a position, reposition the camera accordingly
    if (ecdb->validEntities[camera_component->target_id] && ECDB_EntityHasComponent(ecdb, camera_component->target_id, transform_handle))
    {
        // TODO: account for entities with parents
        struct C_Transform* target_transform = (struct C_Transform*) ECDB_GetEntityComponent(ecdb, camera_component->target_id, transform_handle);

        // Center it
        camera_transform->position.x = target_transform->position.x - camera_component->width / 2;
        camera_transform->position.y = target_transform->position.y - camera_component->height / 2;
    }

    // If the camera is bigger than the level, center on the level. If the camera is smaller than the level, and is out of bounds, reposition it so it is in bounds
    // TODO: Skip target tracking if this is the case
    if (camera_component->width > level_width)
    {
        camera_transform->position.x = (int)(level_width / 2) - (camera_component->width / 2);
    }
    else
    {
        if (camera_transform->position.x < 0)
        {
            // Too far to the left of the level
            camera_transform->position.x = 0;
        }
        else if (camera_transform->position.x + camera_component->width > level_width)
        {
            // Too far to the right of the level
            camera_transform->position.x = level_width - camera_component->width;
        }
    }
    
    if (camera_component->height > level_height)
    {
        camera_transform->position.y = (int)(level_height / 2) - (camera_component->height / 2);
    }
    else
    {
        if (camera_transform->position.y < 0)
        {
            // Too far above the level
            camera_transform->position.y = 0;
        }
        else if (camera_transform->position.y + camera_component->height > level_height)
        {
            // Too far below the level
            camera_transform->position.y = level_height - camera_component->height;
        }
    }
}