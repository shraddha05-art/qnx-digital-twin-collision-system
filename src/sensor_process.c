/*
 * sensor_process.c
 * ----------------
 * MODULE 1 — Sensor Process
 *
 * Responsibility:
 *   Simulates the hardware sensor layer of the FCW system. In a real vehicle
 *   this module would interface with a radar or ultrasonic sensor via a QNX
 *   resource manager (/dev/fcw_sensor). Here it computes physically realistic
 *   distance and speed values each simulation step and writes them into the
 *   shared fcw_state_t structure.
 *
 * QNX Parallel:
 *   This would run as sensor_proc at scheduler priority 30 (SCHED_FIFO),
 *   firing a POSIX periodic timer every 100 ms and publishing to shared memory.
 */

#include <stdio.h>
#include "fcw_types.h"

/*
 * sensor_init
 * -----------
 * Initialises the shared state with starting conditions.
 * Call this once before the simulation loop begins.
 *
 * Parameters:
 *   state — pointer to the shared fcw_state_t structure
 */
void sensor_init(fcw_state_t *state)
{
    state->distance       = INITIAL_DISTANCE_M;
    state->vehicle_speed  = INITIAL_SPEED_MPS;
    state->relative_speed = INITIAL_SPEED_MPS + OBSTACLE_SPEED_MPS;
    state->ttc            = 0.0f;
    state->warning_flag   = ALERT_SAFE;
    state->step           = 0;

    printf("[SENSOR]  Initialised — distance=%.1f m  vehicle_speed=%.1f m/s"
           "  obstacle_speed=%.1f m/s\n",
           state->distance, state->vehicle_speed, OBSTACLE_SPEED_MPS);
}

/*
 * sensor_update
 * -------------
 * Advances the physical simulation by one time step (SIM_STEP_TIME_S seconds).
 *
 * Physics model:
 *   Both the ego-vehicle and the obstacle are moving toward each other.
 *   The distance closes by (vehicle_speed + obstacle_speed) * dt each step.
 *   Vehicle speed decreases slightly each step to simulate the driver
 *   easing off the accelerator as an obstacle is perceived — this keeps
 *   the simulation realistic and prevents distance going negative abruptly.
 *
 * Parameters:
 *   state — pointer to the shared fcw_state_t structure
 */
void sensor_update(fcw_state_t *state)
{
    /* Closing speed = ego speed + obstacle approach speed */
    float closing_speed = state->vehicle_speed + OBSTACLE_SPEED_MPS;

    /* Reduce distance by how far both vehicles travel in one time step */
    state->distance -= closing_speed * SIM_STEP_TIME_S;

    /* Clamp distance at zero — obstacle has been reached */
    if (state->distance < 0.0f) {
        state->distance = 0.0f;
    }

    /*
     * Simulate gradual driver reaction: vehicle speed decreases by 0.5 m/s
     * per step once distance falls below 60 m (driver perceives the risk).
     */
    if (state->distance < 60.0f && state->vehicle_speed > 5.0f) {
        state->vehicle_speed -= 0.5f;
    }

    /* Relative (closing) speed used by prediction engine */
    state->relative_speed = state->vehicle_speed + OBSTACLE_SPEED_MPS;

    state->step++;

    printf("[SENSOR]  Step %2d | distance=%6.2f m | vehicle_speed=%5.2f m/s"
           " | closing_speed=%5.2f m/s\n",
           state->step, state->distance,
           state->vehicle_speed, state->relative_speed);
}
