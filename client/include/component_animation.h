#ifndef COMOPNENT_ANIMATION_INSTANCE_DEF
#define COMOPNENT_ANIMATION_INSTANCE_DEF
#include <stdbool.h>
#include <SDL3/SDL.H>

struct C_Animation_Instance
{
    unsigned int animation_index;
    unsigned int current_frame;
    unsigned int miliseconds_per_frame;
    unsigned int frame_time_accumulator_ms;
    bool loop;
};

struct Animation
{
    SDL_FRect frames[50];
    unsigned int frame_count;
};

bool Animation_Add_Frame(Animation* animation, SDL_FRect frame)
{
    if (animation->frame_count >= 50)
    {
        return false;
    }

    animation->frames[animation->frame_count] = frame;
    animation->frame_count++;
    return true;
}

#endif /* COMOPNENT_ANIMATION_INSTANCE_DEF */