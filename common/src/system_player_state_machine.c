#include "system_player_state_machine.h"
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_player_state.h"
#include "component_physics_2d.h"
#include "component_animation.h"
#include <math.h>
#include "input_command_buffer.h"

static bool AreSameSign(float a, float b)
{
    return (a >= 0 && b >= 0) || (a < 0 && b < 0);
}

static struct Vector2 Calculate_Input_Direction(struct C_Input* input)
{
    struct Vector2 input_direction = {.x = -(input->cmnd_states[MOVE_LEFT]) + input->cmnd_states[MOVE_RIGHT], .y = -(input->cmnd_states[MOVE_UP]) + input->cmnd_states[MOVE_DOWN]};
    return input_direction;
}

static float CalculateMovementLeg(float input_leg, float current_velocity_leg, float delta_time_s, float acceleration, float friction)
{
    float retval = current_velocity_leg;
    
    if (input_leg != 0)
    {
        //If there is input on this axis, move along the axis
        retval = current_velocity_leg + (acceleration * input_leg) * delta_time_s;
        
        if (!AreSameSign(retval, input_leg))
        {
            //If we are currently traveling in a different direction than input, apply friction as a boost
            retval += (friction * input_leg) * delta_time_s;
        }
    }
    else
    {
        //If there is no input, apply friction on the axis
        if (current_velocity_leg != 0)
        {
            int direction = 1;
            if (current_velocity_leg < 0)
            {
                direction = -1;
            }

            retval = current_velocity_leg - (friction * direction) * delta_time_s;
            
            if (!AreSameSign(retval, current_velocity_leg))
            {
                //If the velocity has switched signs, set to 0
                retval = 0;
            }
        }
    }
    
    return retval;
}

bool Attack_Triggered(const struct Command_Buffer* cmnds)
{
    for (int i = cmnds->command_cnt - 1; i >= 0; --i)
    {
        struct Command_Entry* cmnd = &cmnds->command_queue[i];
        if (cmnd->pressed == true && cmnd->command == ATTACK)
        {
            return true;
        }
    }

    return false;
}

bool Try_Get_Last_Movement_Input(const struct Command_Buffer* cmnds, struct Command_Entry** last_move_cmnd)
{
    for (int i = cmnds->command_cnt - 1; i >= 0; --i)
    {
        struct Command_Entry* cmnd = &cmnds->command_queue[i];
        if (cmnd->pressed == true && (cmnd->command == MOVE_LEFT || cmnd->command == MOVE_RIGHT || cmnd->command == MOVE_UP || cmnd->command == MOVE_DOWN))
        {
            *last_move_cmnd = cmnd;
            return true;
        }
    }

    return false;
}

static void Idle_Execute(struct ECDB const *const ecdb, unsigned int entity_id, struct C_Player_State* state, int inputs_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    struct C_Input* input;
    if (ECDB_Try_Get_Entity_Component(ecdb, entity_id, inputs_handle, &input))
    {
        struct Vector2 input_direction = Calculate_Input_Direction(input);

        // if we attacked, swap to attack state
        if (Attack_Triggered(&input->commands))
        {
            state->timer_elapsed = 0;
            state->state = ATTACKING;
            return;
        }

        // If we've started moving, go to running
        if (input_direction.x != 0 || input_direction.y != 0)
        {            
            state->state = RUNNING;
            return;
        }

        if(ECDB_EntityHasComponent(ecdb, entity_id, player_physics_2d_handle))
        {
            struct C_Physics_2d* physics = ECDB_GetEntityComponent(ecdb, entity_id, player_physics_2d_handle);

            // slow down
            physics->velocity.x = CalculateMovementLeg(0, physics->velocity.x, delta_time_s, input->speed, physics->friction);
            physics->velocity.y = CalculateMovementLeg(0, physics->velocity.y, delta_time_s, input->speed, physics->friction);
        }
    }
}

static void Running_Execute(struct ECDB const *const ecdb, unsigned int entity_id, struct C_Player_State* state, int inputs_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    if (ECDB_EntityHasComponent(ecdb, entity_id, inputs_handle))
    {
        struct C_Input* input = ECDB_GetEntityComponent(ecdb, entity_id, inputs_handle);
        // build direction based on keyboard state
        struct Vector2 input_direction = Calculate_Input_Direction(input);

        // if we attacked, swap to attack state
        if (Attack_Triggered(&input->commands))
        {
            state->timer_elapsed = 0;
            state->state = ATTACKING;
            return;
        }

        // If we aren't moving, go back to idle
        if (input_direction.x == 0.0f && input_direction.y == 0.0f)
        {
            state->state = IDLE;
            return;
        }

        // If there has been input, face direction will be based on the last input received. if no movement input, stays the same
        struct Command_Entry* last_move_cmnd; // TODO: this doesnt really work
        if (Try_Get_Last_Movement_Input(&input->commands, &last_move_cmnd))
        {
            switch(last_move_cmnd->command)
            {
                case MOVE_LEFT:
                    state->direction = PLAYER_LEFT;
                    break;
                case MOVE_RIGHT:
                    state->direction = PLAYER_RIGHT;
                    break;
                case MOVE_UP:
                    state->direction = PLAYER_UP;
                    break;
                case MOVE_DOWN:
                    state->direction = PLAYER_DOWN;
                    break;
                default:
                    break;
            }
        }
        else
        {
            // If only going in one direction, update to that direction
            if (input_direction.y > 0 && input_direction.x == 0)
            {
                state->direction = PLAYER_DOWN;
            }
            else if (input_direction.y < 0 && input_direction.x == 0)
            {
                state->direction = PLAYER_UP;
            }
            else if (input_direction.x > 0 && input_direction.y == 0)
            {
                state->direction = PLAYER_RIGHT;
            }
            else if (input_direction.x < 0 && input_direction.y == 0)
            {
                state->direction = PLAYER_LEFT;
            }
        }

        // if input is being done, apply it to physics
        if(ECDB_EntityHasComponent(ecdb, entity_id, player_physics_2d_handle))
        {
            struct C_Physics_2d* physics = ECDB_GetEntityComponent(ecdb, entity_id, player_physics_2d_handle);

            // Apply input if there is any
            physics->velocity.x = CalculateMovementLeg(input_direction.x, physics->velocity.x, delta_time_s, input->speed, physics->friction);
            physics->velocity.y = CalculateMovementLeg(input_direction.y, physics->velocity.y, delta_time_s, input->speed, physics->friction);

            // clamp to max speed. todo: move to physics sim?
            float velocity_magnitude = sqrt(physics->velocity.x * physics->velocity.x + physics->velocity.y * physics->velocity.y);
            if (velocity_magnitude > physics->max_speed)
            {
                physics->velocity.x = (physics->velocity.x / velocity_magnitude) * physics->max_speed;
                physics->velocity.y = (physics->velocity.y / velocity_magnitude) * physics->max_speed;
            }
        }
    }
}

static void Run_Animate(struct ECDB const *const ecdb, unsigned int entity_id, struct C_Player_State* state, int inputs_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    struct C_Animation_Instance* animation_instance;
    if (ECDB_Try_Get_Entity_Component(ecdb, entity_id, animation_instance_handle, &animation_instance))
    {
        // todo: move to some resource manager
        unsigned int move_up_anim_index = 0;
        unsigned int move_down_anim_index = 1;
        unsigned int move_left_anim_index = 2;
        unsigned int move_right_anim_index = 3;

        switch(state->direction)
        {
            case PLAYER_UP:
                if (animation_instance->animation_index != move_up_anim_index)
                {
                    animation_instance->animation_index = move_up_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = true;
                }
                break;
            case PLAYER_DOWN:
                if (animation_instance->animation_index != move_down_anim_index)
                {
                    animation_instance->animation_index = move_down_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = true;
                }
                break;
            case PLAYER_LEFT:
                if (animation_instance->animation_index != move_left_anim_index)
                {
                    animation_instance->animation_index = move_left_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = true;
                }
                break;
            case PLAYER_RIGHT:
                if (animation_instance->animation_index != move_right_anim_index)
                {
                    animation_instance->animation_index = move_right_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = true;
                }
                break;
            default:
                break;
        }
    }
}

#define ATTACK_LEN_S 0.3
static void Attacking_Execute(struct ECDB const *const ecdb, unsigned int entity_id, struct C_Player_State* state, int inputs_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    state->timer_elapsed += delta_time_s;
    if (state->timer_elapsed >= ATTACK_LEN_S)
    {
        state->timer_elapsed = 0;
        state->state = IDLE;
        return;
    }

    if(ECDB_EntityHasComponent(ecdb, entity_id, player_physics_2d_handle) && ECDB_EntityHasComponent(ecdb, entity_id, inputs_handle))
    {
        struct C_Physics_2d* physics = ECDB_GetEntityComponent(ecdb, entity_id, player_physics_2d_handle);
        struct C_Input* input = ECDB_GetEntityComponent(ecdb, entity_id, inputs_handle);

        // slow down
        physics->velocity.x = CalculateMovementLeg(0, physics->velocity.x, delta_time_s, input->speed, physics->friction);
        physics->velocity.y = CalculateMovementLeg(0, physics->velocity.y, delta_time_s, input->speed, physics->friction);
    }
}

static void Attacking_Animate(struct ECDB const *const ecdb, unsigned int entity_id, struct C_Player_State* state, int inputs_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    struct C_Animation_Instance* animation_instance;
    if (ECDB_Try_Get_Entity_Component(ecdb, entity_id, animation_instance_handle, &animation_instance))
    {
        // todo: move to some resource manager
        unsigned int attack_up_anim_index = 5;
        unsigned int attack_down_anim_index = 6;
        unsigned int attack_left_anim_index = 7;
        unsigned int attack_right_anim_index = 8;

        switch(state->direction)
        {
            case PLAYER_UP:
                if (animation_instance->animation_index != attack_up_anim_index)
                {
                    animation_instance->animation_index = attack_up_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = false;
                }
                break;
            case PLAYER_DOWN:
                if (animation_instance->animation_index != attack_down_anim_index)
                {
                    animation_instance->animation_index = attack_down_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = false;
                }
                break;
            case PLAYER_LEFT:
                if (animation_instance->animation_index != attack_left_anim_index)
                {
                    animation_instance->animation_index = attack_left_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = false;
                }
                break;
            case PLAYER_RIGHT:
                if (animation_instance->animation_index != attack_right_anim_index)
                {
                    animation_instance->animation_index = attack_right_anim_index;
                    animation_instance->current_frame = 0;
                    animation_instance->frame_time_accumulator_ms = 0;
                    animation_instance->loop = false;
                }
                break;
            default:
                break;
        }
    }
}

void s_player_state_machine(struct ECDB const *const ecdb, int inputs_handle, int player_states_handle, int player_physics_2d_handle, int animation_instance_handle, float delta_time_s)
{
    struct C_Player_State* states = (struct C_Player_State*) ecdb->componentArrays[player_states_handle];
    struct C_Input* inputs = (struct C_Input*) ecdb->componentArrays[inputs_handle];
    struct C_Physics_2d* physics = (struct C_Physics_2d*) ecdb->componentArrays[player_physics_2d_handle];
    struct C_Animation_Instance* animation_instances = (struct C_Animation_Instance*) ecdb->componentArrays[animation_instance_handle];

    for(unsigned int i = 0; i < ecdb->_maxEntities; ++i)
    {
        if(ECDB_EntityHasComponent(ecdb, i, player_states_handle))
        {
            // Run execution logic
            switch(states[i].state)
            {
                case IDLE:
                    Idle_Execute(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                case RUNNING:
                    Running_Execute(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                case ATTACKING:
                    Attacking_Execute(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                default:
                    break;
            }

            // Run animation logic 
            switch(states[i].state)
            {
                case IDLE:
                    // Idle uses the same animations as running
                    Run_Animate(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                case RUNNING:
                    Run_Animate(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                case ATTACKING:
                    Attacking_Animate(ecdb, i, &states[i], inputs_handle, player_physics_2d_handle, animation_instance_handle, delta_time_s);
                    break;
                default:
                    break;
            }

            // Special logic for idle that doesn't make sense to put elsewhere atm
            if (ECDB_EntityHasComponent(ecdb, i, animation_instance_handle))
            {
                // Pause animation if we aren't moving, and restart if we are
                if (states[i].state == IDLE)
                {
                    animation_instances[i].paused = true;
                }
                else
                {
                    animation_instances[i].paused = false;
                }
            }
        }
    }
}