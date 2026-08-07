#include "system_player_state_machine.h"
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_player_state.h"
#include "component_physics_2d.h"
#include "component_animation.h"
#include <math.h>

static bool AreSameSign(float a, float b)
{
    return (a >= 0 && b >= 0) || (a < 0 && b < 0);
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
            if (ECDB_EntityHasComponent(ecdb, i, inputs_handle))
            {
                switch(states[i].state)
                {
                    case IDLE:
                        if (inputs[i].direction.x != 0 || inputs[i].direction.y != 0)
                        {
                            states[i].state = RUNNING;
                        }
                        break;
                    case RUNNING:
                        if (inputs[i].direction.x == 0.0f && inputs[i].direction.y == 0.0f)
                        {
                            states[i].state = IDLE;
                        }
                        break;
                    default:
                        break;
                }

                // TODO: replace by selecting the newest clicked direction
                if (inputs[i].direction.y > 0)
                {
                    // moving down
                    states[i].direction = PLAYER_DOWN;
                }
                else if (inputs[i].direction.y < 0)
                {
                    // moving up
                    states[i].direction = PLAYER_UP;
                }
                else if (inputs[i].direction.x > 0)
                {
                    // moving right
                    states[i].direction = PLAYER_RIGHT;
                }
                else if (inputs[i].direction.x < 0)
                {
                    // moving left
                    states[i].direction = PLAYER_LEFT;
                }

                // if input is being done, apply it to physics
                if(ECDB_EntityHasComponent(ecdb, i, player_physics_2d_handle))
                {
                    // Apply input if there is any
                    struct C_Input input = inputs[i];
                    physics[i].velocity.x = CalculateMovementLeg(input.direction.x, physics[i].velocity.x, delta_time_s, input.speed, physics[i].friction);
                    physics[i].velocity.y = CalculateMovementLeg(input.direction.y, physics[i].velocity.y, delta_time_s, input.speed, physics[i].friction);

                    // clamp to max speed. todo: move to physics sim?
                    float velocity_magnitude = sqrt(physics[i].velocity.x * physics[i].velocity.x + physics[i].velocity.y * physics[i].velocity.y);
                    if (velocity_magnitude > physics[i].max_speed)
                    {
                        physics[i].velocity.x = (physics[i].velocity.x / velocity_magnitude) * physics[i].max_speed;
                        physics[i].velocity.y = (physics[i].velocity.y / velocity_magnitude) * physics[i].max_speed;
                    }
                }
            }

            // Todo: Don't change direction if moving already
            if (ECDB_EntityHasComponent(ecdb, i, animation_instance_handle))
            {
                // todo: move to some resource manager
                unsigned int move_up_anim_index = 0;
                unsigned int move_down_anim_index = 1;
                unsigned int move_left_anim_index = 2;
                unsigned int move_right_anim_index = 3;

                switch(states[i].direction)
                {
                    case PLAYER_UP:
                        if (animation_instances[i].animation_index != move_up_anim_index)
                        {
                            animation_instances[i].animation_index = move_up_anim_index;
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                        break;
                    case PLAYER_DOWN:
                        if (animation_instances[i].animation_index != move_down_anim_index)
                        {
                            animation_instances[i].animation_index = move_down_anim_index;
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                        break;
                    case PLAYER_LEFT:
                        if (animation_instances[i].animation_index != move_left_anim_index)
                        {
                            animation_instances[i].animation_index = move_left_anim_index;
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                        break;
                    case PLAYER_RIGHT:
                        if (animation_instances[i].animation_index != move_right_anim_index)
                        {
                            animation_instances[i].animation_index = move_right_anim_index;
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                        break;
                    default:
                        break;
                }

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