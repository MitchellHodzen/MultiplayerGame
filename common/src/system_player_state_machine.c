#include "system_player_state_machine.h"
#include "ecdb.h"
#include "vector2.h"
#include "component_input.h"
#include "component_player_state.h"
#include "component_physics_2d.h"
#include "component_animation.h"
#include <math.h>

bool AreSameSign(float a, float b)
{
    return (a >= 0 && b >= 0) || (a < 0 && b < 0);
}

float CalculateMovementLeg(float input_leg, float current_velocity_leg, float delta_time_s, float acceleration, float friction)
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
    enum Player_State* states = (enum Player_State*) ecdb->componentArrays[player_states_handle];
    struct C_Input* inputs = (struct C_Input*) ecdb->componentArrays[inputs_handle];
    struct C_Physics_2d* physics = (struct C_Physics_2d*) ecdb->componentArrays[player_physics_2d_handle];
    struct C_Animation_Instance* animation_instances = (struct C_Animation_Instance*) ecdb->componentArrays[animation_instance_handle];

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

        // if input is being done, apply it to physics
        if(ECDB_EntityHasComponent(ecdb, i, player_physics_2d_handle) && ECDB_EntityHasComponent(ecdb, i, inputs_handle))
        {
            struct C_Input input = inputs[i];
            physics[i].velocity.x = CalculateMovementLeg(input.direction.x, physics[i].velocity.x, delta_time_s, input.speed, physics[i].friction);
            physics[i].velocity.y = CalculateMovementLeg(input.direction.y, physics[i].velocity.y, delta_time_s, input.speed, physics[i].friction);

            // clamp to max speed
            float velocity_magnitude = sqrt(physics[i].velocity.x * physics[i].velocity.x + physics[i].velocity.y * physics[i].velocity.y);
            if (velocity_magnitude > physics[i].max_speed)
            {
                physics[i].velocity.x = (physics[i].velocity.x / velocity_magnitude) * physics[i].max_speed;
                physics[i].velocity.y = (physics[i].velocity.y / velocity_magnitude) * physics[i].max_speed;
            }

            // Todo: Don't change direction if moving already
            if (ECDB_EntityHasComponent(ecdb, i, animation_instance_handle))
            {
                if (abs(physics->velocity.x) > abs(physics->velocity.y))
                {
                    // Moving more sidways than up and down
                    if (physics->velocity.x > 0)
                    {
                        if (animation_instances[i].animation_index != 3)
                        {
                            animation_instances[i].animation_index = 3; // Move right
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                    }
                    else if (physics->velocity.x < 0)
                    {
                        if (animation_instances[i].animation_index != 2)
                        {
                            animation_instances[i].animation_index = 2; // Move left
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                    }
                }
                else 
                {
                    // Moving more up and down than sideways, with bias towards up and down if theyre equal
                    if (physics->velocity.y > 0)
                    {
                        if (animation_instances[i].animation_index != 1)
                        {
                            animation_instances[i].animation_index = 1; // Move down
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                    }
                    else if (physics->velocity.y < 0)
                    {
                        if (animation_instances[i].animation_index != 0)
                        {
                            animation_instances[i].animation_index = 0; // Move up
                            animation_instances[i].current_frame = 0;
                            animation_instances[i].frame_time_accumulator_ms = 0;
                        }
                    }
                }

                // Pause animation if we aren't moving, and restart if we are
                if (physics->velocity.x == 0 && physics->velocity.y == 0)
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