#include "component_transform.h"

// TODO: Enet unreliable packets are still sequenced, so we will always get the most recent. always push in front?
void Interp_Buf_Add(struct N_C_Transform_Interpolation_Buffer* interp_buf, struct N_C_Transform_Snapshot snapshot)
{
    // Buffer is ordered from most recent to least recent
    for(unsigned int i = 0; i < NET_TRANS_BUF_SIZE; ++i)
    {
        // If the time on the snapshot is more recent than the current index, insert the snapshot and shift everything down
        if (interp_buf->_buffer[i].server_time < snapshot.server_time)
        {
            // move everything after the current index over one, starting from the back
            for(unsigned int j = NET_TRANS_BUF_SIZE - 1; j > i; --j)
            {
                interp_buf->_buffer[j] = interp_buf->_buffer[j - 1];
            }

            // Insert the new snapshot into the buffer
            interp_buf->_buffer[i] = snapshot;

            // Increase buffer size if it isnt maxed out
            interp_buf->buffer_size += 1 * (interp_buf->buffer_size != NET_TRANS_BUF_SIZE);

            break;
        }
    }
}