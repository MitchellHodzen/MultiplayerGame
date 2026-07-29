#ifndef COMOPNENT_ANIMATION_INSTANCE_DEF
#define COMOPNENT_ANIMATION_INSTANCE_DEF
#include <stdbool.h>
#include <SDL3/SDL.H>

struct ECDB;

struct C_Animation_Instance
{
    unsigned int animation_index;
    unsigned int current_frame;
    unsigned int frame_time_accumulator_ms;
};

struct Animation_Frame
{
    SDL_FRect spritesheet_clip_rect;
    int origin_offset_pixels_x;
    int origin_offset_pixels_y;
};

struct Animation
{
    struct Animation_Frame frames[50];
    unsigned int frame_count;
    unsigned int miliseconds_per_frame;
    bool loop;
};

bool Animation_Add_Frame(struct Animation* animation, struct Animation_Frame frame);
void s_animation_iterate(struct ECDB* ecdb, int animation_instance_handle, unsigned int delta_time_ms, struct Animation* test_animation);

#endif /* COMOPNENT_ANIMATION_INSTANCE_DEF */