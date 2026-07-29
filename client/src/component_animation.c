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

void s_animation_iterate(struct ECDB* ecdb, int animation_instance_handle, unsigned int delta_time_ms, struct Animation* test_animation)
{
    struct C_Animation_Instance* animations = (struct C_Animation_Instance*) ecdb->componentArrays[animation_instance_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if (ECDB_EntityHasComponent(ecdb, i, animation_instance_handle))
        {
            animations[i].frame_time_accumulator_ms += delta_time_ms;
            while (animations[i].frame_time_accumulator_ms > test_animation->miliseconds_per_frame)
            {
                animations[i].current_frame++;
                if (animations[i].current_frame >= test_animation->frame_count)
                {
                    animations[i].current_frame = 0;
                }

                animations[i].frame_time_accumulator_ms -= test_animation->miliseconds_per_frame;
            }
        }
    }
}