#include "system_player_state_machine.h"
#include "ecdb.h"
#include "component_input.h"
#include "component_player_state.h"

void s_player_state_machine(struct ECDB const *const ecdb, int inputs_handle, int player_states_handle)
{
    enum Player_State* states = (enum Player_State*) ecdb->componentArrays[player_states_handle];
    struct C_Input* inputs = (struct C_Input*) ecdb->componentArrays[inputs_handle];
    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ecdb, i, player_states_handle) && ECDB_EntityHasComponent(ecdb, i, inputs_handle))
        {
            switch(states[i])
            {
                case IDLE:
                    if (inputs[i].direction.x != 0 || inputs[i].direction.y != 0)
                    {
                        states[i] = RUNNING;
                    }
                    break;
                case RUNNING:
                    if (inputs[i].direction.x == 0.0f && inputs[i].direction.y == 0.0f)
                    {
                        states[i] = IDLE;
                    }
                    break;
                default:
                    break;
            }
        }
    }
}