#ifndef TICK_INPUT_DEF
#define TICK_INPUT_DEF
#include "string.h"
#include "input_command_buffer.h"
#include "ring_buffer.h"

struct Tick_Input
{
    bool from_client;
    unsigned int tick;
    struct Command_Snapshot command_snapshot;
};

static inline void Tick_Input_Buffer_Add(struct Ring_Buffer* tick_input_buffer, struct Tick_Input* input)
{
    // Most input received will be added to the end, so check that first
    struct Tick_Input* last_frame_input = (struct Tick_Input*)Ring_Buffer_Peek(tick_input_buffer);

    // If the buffer is empty just add the value
    if (last_frame_input == NULL)
    {
        *((struct Tick_Input*)Ring_Buffer_Get_Next(tick_input_buffer)) = *input;
        return;
    }

    // If the value is after the latest tick, add the value at the end + any necessary padding
    if (last_frame_input->tick < input->tick)
    {
        // If the new input is well ahead of existing input, fill in any gaps afterwards with copies of previous input
        unsigned int ticks_ahead = input->tick - last_frame_input->tick;
        if (ticks_ahead > 1)
        {
            struct Tick_Input last_frame_input_copy = {.from_client = false, .tick = last_frame_input->tick};
            memcpy(last_frame_input_copy.command_snapshot.cmnd_states, last_frame_input->command_snapshot.cmnd_states, CMND_MAX_CNT);

            for(unsigned int i = 0; i < ticks_ahead - 1; ++i)
            {
                last_frame_input_copy.tick++;
                *((struct Tick_Input*)Ring_Buffer_Get_Next(tick_input_buffer)) = last_frame_input_copy;
            }
        }

        *((struct Tick_Input*)Ring_Buffer_Get_Next(tick_input_buffer)) = *input;
        return;

    }

    // If the value is equivalent to the last one, replace it
    if (last_frame_input->tick == input->tick)
    {
        *last_frame_input = *input;
        return;
    }

    // If the input received is somewhere in the middle of the buffer, find where to put it. Starting at size - 2 since we already checked the last value
    for(int i = tick_input_buffer->buffer_size - 2; i >= 0; --i)
    {
        struct Tick_Input* existing_frame_input = (struct Tick_Input*)Ring_Buffer_Get_At(tick_input_buffer, i);

        // If this is the frame, replace it
        if (existing_frame_input->tick == input->tick)
        {
            *existing_frame_input = *input;

            // Forward fill input
            struct Tick_Input input_copy = {.from_client = false, .tick = input->tick};
            memcpy(input_copy.command_snapshot.cmnd_states, input->command_snapshot.cmnd_states, CMND_MAX_CNT);
            for(unsigned int j = i+1; j < tick_input_buffer->buffer_size; ++j)
            {
                struct Tick_Input* frame_to_replace = (struct Tick_Input*)Ring_Buffer_Get_At(tick_input_buffer, j);
                // If we received this input from the user, stop forward filling
                if (frame_to_replace->from_client == true)
                {
                    return;
                }

                // If this was a fill frame, fill it with the new value
                input_copy.tick++;
                *frame_to_replace = input_copy;
            }
        }
    }
}

#endif /* TICK_INPUT_DEF */