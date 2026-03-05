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

void dashboard_init(void);
int  dashboard_update(const fcw_state_t *state);
void dashboard_shutdown(void);

int main(void)
{
    fcw_state_t state;
    int step;
    int quit = 0;

    /* â”€â”€ Initialise all modules â”€â”€ */
    sensor_init(&state);
    twin_init(&state);
    dashboard_init();        /* Opens ncurses screen (or plain header)  */

    /* â”€â”€ Main simulation loop â”€â”€ */
    for (step = 0; step < SIM_STEPS && !quit; step++) {

        /* 1. Sensor process â€” read/simulate physical sensor data */
        sensor_update(&state);

        /* 2. Digital twin process â€” mirror and smooth sensor data */
        twin_update(&state);

        /* 3. Prediction engine â€” compute TTC, classify alert level */
        prediction_compute_ttc(&state);

        /* 4. Warning system â€” emit appropriate alert output */
        warning_evaluate(&state);

        /* 5. Dashboard â€” refresh display (lowest priority, 500 ms) */
        quit = dashboard_update(&state);

        /* Stop simulation if obstacle reached */
        if (state.distance <= 0.0f) break;
    }

    dashboard_shutdown();

    printf("=================================================================\n");
    printf("  Simulation complete.\n");
    printf("  Final state: distance=%.2f m  TTC=%.2f s  AlertLevel=%d\n",
           state.distance, state.ttc, state.warning_flag);
    printf("=================================================================\n");

    return 0;
}