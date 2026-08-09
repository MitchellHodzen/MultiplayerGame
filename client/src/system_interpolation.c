#include "system_interpolation.h"
#include "vector2.h"
#include "component_transform.h"
#include "ecdb.h"

static unsigned int calculate_interpolation_delay(unsigned int server_updates_per_s, unsigned int frame_drop_allowance, unsigned int round_trip_time_ms)
{
    // How long it takes for a server to send us a packet is approximately half of round trip time
    float server_delay_ms = (float)round_trip_time_ms / 2;

    // Time between frames will be server updates per ms + server delay
    unsigned int frame_delay = (1000 / server_updates_per_s) + server_delay_ms;

    // Give enough time for frame_drop_allowance frames to drop
    return frame_drop_allowance * frame_delay;
}

static struct Vector2 calculate_interpolated_position(struct N_C_Transform_Interpolation_Buffer* trans_buf, unsigned long interp_time, struct Vector2 current_position)
{
    if (trans_buf->buffer_size == 0)
    {
        // If the buffer is empty, return the entity's actual position
        return current_position;
    }

    // determine which two values to interpolate between
    for (unsigned int i = 0; i < trans_buf->buffer_size; ++i)
    {
        // If the interp time is greater than the snapshot, interpolate between this and the previous snapshot
        if (interp_time >= trans_buf->_buffer[i].server_time)
        {
            if (i == 0)
            {
                // If we're at the start and there are no previous snapshots, we don't have anything to interpolate against, return the most recently received snapshot
                return trans_buf->_buffer[i].transform.position;
            }

            // if we aren't at the start, grab the more recent snapshot and interpolate between the two
            struct N_C_Transform_Snapshot from = trans_buf->_buffer[i];
            struct N_C_Transform_Snapshot to = trans_buf->_buffer[i - 1];

            // noramlize the time
            float interp = (interp_time - from.server_time) / (float)(to.server_time - from.server_time);

            // linearly interpolate the vector: from*interp + to*(1-interp)
            struct Vector2 retval = {.x = (to.transform.position.x * interp) + (from.transform.position.x * (1 - interp)), .y = (to.transform.position.y * interp) + (from.transform.position.y * (1 - interp))};
            return retval;
        }
    }

    // If we've gotten here, the time is before any snapshots, so return oldest snapshot
    return trans_buf->_buffer[trans_buf->buffer_size - 1].transform.position;
}

void s_interpolate_position(struct ECDB const *const ec, int transforms_handle, int transforms_buffer_handle, unsigned long estimated_server_time, unsigned int server_updates_per_s, unsigned int frame_drop_allowance, unsigned int round_trip_time_ms)
{
    unsigned int interpolation_delay_ms = calculate_interpolation_delay(server_updates_per_s, frame_drop_allowance, round_trip_time_ms);
    //printf("Frame drop allownace: %u. Server updates per second: %u. Round trip time: %u. interpolation delay: %u\n", frame_drop_allowance, server_updates_per_s, round_trip_time_ms, interpolation_delay_ms);
    struct C_Transform* transforms = (struct C_Transform*) ec->componentArrays[transforms_handle];
    struct N_C_Transform_Interpolation_Buffer* trans_buf = (struct N_C_Transform_Interpolation_Buffer*) ec->componentArrays[transforms_buffer_handle];

    // We interpolate interpolation_delay_ms in the past based on our estimated server time
    unsigned long interp_time = estimated_server_time - interpolation_delay_ms;

    for(unsigned int i = 0; i < ec->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ec, i, transforms_handle) && ECDB_EntityHasComponent(ec, i, transforms_buffer_handle))
        {
            transforms[i].position = calculate_interpolated_position(&trans_buf[i], interp_time, transforms[i].position);
        }
    }
}