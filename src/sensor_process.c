/*
 * sensor_process.c
 * ----------------
 * MODULE 1 — Sensor Process
 *
 * Simulates the hardware sensor layer of the FCW system.
 * In a real vehicle this interfaces with a radar/ultrasonic sensor
 * via a QNX resource manager (/dev/fcw_sensor).
 *
 * QNX Parallel:
 *   Runs inside sensor_proc at priority 30 (SCHED_FIFO), firing a
 *   POSIX periodic timer every SENSOR_PERIOD_MS milliseconds.
 */

#include <stdio.h>
#include "fcw_types.h"

/*
 * sensor_init
 * -----------
 * Initialises shared state with starting conditions.
 * Call once before the simulation loop.
 */
void sensor_init(fcw_state_t *state)
{
    state->distance       = INITIAL_DISTANCE_M;
    state->vehicle_speed  = INITIAL_SPEED_MPS;
    state->relative_speed = INITIAL_SPEED_MPS + OBSTACLE_SPEED_MPS;
    state->ttc            = 0.0f;
    state->warning_flag   = ALERT_SAFE;
    state->anomaly_flag   = 0;
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
 *   Both ego-vehicle and obstacle are moving toward each other.
 *   Distance closes by (vehicle_speed + obstacle_speed) * dt each step.
 *   Vehicle speed decreases once distance < 60 m (driver reaction).
 */
void sensor_update(fcw_state_t *state)
{
    float closing_speed = state->vehicle_speed + OBSTACLE_SPEED_MPS;

    state->distance -= closing_speed * SIM_STEP_TIME_S;

    if (state->distance < 0.0f)
        state->distance = 0.0f;

    /* Simulate gradual driver reaction below 60 m */
    if (state->distance < 60.0f && state->vehicle_speed > 5.0f)
        state->vehicle_speed -= 0.5f;

    state->relative_speed = state->vehicle_speed + OBSTACLE_SPEED_MPS;
    state->step++;

    printf("[SENSOR]  Step %2d | distance=%6.2f m | vehicle_speed=%5.2f m/s"
           " | closing_speed=%5.2f m/s\n",
           state->step, state->distance,
           state->vehicle_speed, state->relative_speed);
}