#include "component_animation.h"
#include "ecdb.h"

bool Animation_Add_Frame(struct Animation* animation, struct Animation_Frame frame)
{
    if (animation->frame_count >= 50)
    {
        return false;
    }

    animation->frames[animation->frame_count] = frame;
    animation->frame_count++;
    return true;
}

void s_animation_iterate(struct ECDB* ecdb, int animation_instance_handle, unsigned int delta_time_ms, struct Animation* animations)
{
    struct C_Animation_Instance* animation_instances = (struct C_Animation_Instance*) ecdb->componentArrays[animation_instance_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if (ECDB_EntityHasComponent(ecdb, i, animation_instance_handle))
        {
            struct C_Animation_Instance* animation_instance = &(animation_instances[i]);
            struct Animation* animation = &(animations[animation_instance->animation_index]);
            animation_instance->frame_time_accumulator_ms += delta_time_ms;
            while (animation_instance->frame_time_accumulator_ms > animation->miliseconds_per_frame)
            {
                animation_instance->current_frame++;
                if (animation_instance->current_frame >= animation->frame_count)
                {
                    animation_instance->current_frame = 0;
                }

                animation_instance->frame_time_accumulator_ms -= animation->miliseconds_per_frame;
            }
        }
    }
}