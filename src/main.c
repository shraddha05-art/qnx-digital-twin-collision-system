/*
 * main.c
 * ------
 * Simulation Orchestrator â€” Digital Twin FCW System
 *
 * This file wires the four modules together in a loop that mimics how a
 * real QNX RTOS scheduler would sequence the processes:
 *
 *   sensor_update()          â€” Priority 30  (data acquisition)
 *   twin_update()            â€” Priority 25  (digital twin sync)
 *   prediction_compute_ttc() â€” Priority 23  (analysis)
 *   warning_evaluate()       â€” Priority 28  (alert output, preempts on pulse)
 *
 * In the real QNX implementation each of these is a separate process
 * communicating via shared memory and QNX IPC. Here they share a single
 * fcw_state_t struct passed by pointer â€” the logical data flow is identical.
 */

#include <stdio.h>
#include "fcw_types.h"

/* Function prototypes from each module */
void sensor_init(fcw_state_t *state);
void sensor_update(fcw_state_t *state);

void twin_init(fcw_state_t *state);
void twin_update(fcw_state_t *state);

void prediction_compute_ttc(fcw_state_t *state);

void warning_evaluate(const fcw_state_t *state);

int main(void)
{
    fcw_state_t state;   /* Shared state â€” the "shared memory region" */
    int step;

    printf("=================================================================\n");
    printf("  Digital Twin Forward Collision Warning System â€” Simulation\n");
    printf("  Vehicle: %.0f m/s  |  Obstacle: %.0f m/s approaching\n",
           INITIAL_SPEED_MPS, OBSTACLE_SPEED_MPS);
    printf("  Initial distance: %.0f m  |  Steps: %d  |  dt: %.1f s\n",
           INITIAL_DISTANCE_M, SIM_STEPS, SIM_STEP_TIME_S);
    printf("=================================================================\n\n");

    /* â”€â”€ Initialise all modules â”€â”€ */
    sensor_init(&state);
    twin_init(&state);
    printf("\n");

    /* â”€â”€ Main simulation loop â”€â”€ */
    for (step = 0; step < SIM_STEPS; step++) {

        printf("----- Simulation Step %2d ----------------------------------------\n",
               step + 1);

        /* 1. Sensor process â€” read/simulate physical sensor data */
        sensor_update(&state);

        /* 2. Digital twin process â€” mirror and smooth sensor data */
        twin_update(&state);

        /* 3. Prediction engine â€” compute TTC, classify alert level */
        prediction_compute_ttc(&state);

        /* 4. Warning system â€” emit appropriate alert output */
        warning_evaluate(&state);

        printf("\n");

        /* Stop simulation if obstacle reached */
        if (state.distance <= 0.0f) {
            printf(">>> SIMULATION ENDED: Vehicle has reached the obstacle. <<<\n");
            break;
        }
    }

    printf("=================================================================\n");
    printf("  Simulation complete.\n");
    printf("  Final state: distance=%.2f m  TTC=%.2f s  AlertLevel=%d\n",
           state.distance, state.ttc, state.warning_flag);
    printf("=================================================================\n");

    return 0;
}