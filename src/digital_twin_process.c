/*
 * digital_twin_process.c
 * ----------------------
 * MODULE 2 â€” Digital Twin Process
 *
 * Responsibility:
 *   Maintains the software Digital Twin of the vehicle system. The twin
 *   mirrors the physical state reported by the sensor module and enriches it
 *   with derived quantities: a smoothed distance estimate, a 5-step history
 *   buffer, and a divergence metric showing how much the twin's own model
 *   prediction differs from the raw sensor reading.
 *
 *   This divergence is a key innovation: if the sensor spikes abnormally the
 *   twin's smoothed value stays stable, and the large divergence value flags
 *   a potential sensor fault rather than a real obstacle event.
 *
 * QNX Parallel:
 *   Runs as twin_proc at priority 25 (SCHED_FIFO). Receives sensor data via
 *   MsgSend / MsgReceive and updates the shared memory region every cycle.
 *   Sends a QNX Pulse to alert_proc when warning_flag changes state.
 */

#include <stdio.h>
#include <math.h>
#include "fcw_types.h"

/* â”€â”€â”€ Internal Twin State â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€ */

#define HISTORY_SIZE 5

/* Rolling history buffer for distance smoothing */
static float distance_history[HISTORY_SIZE] = {0};
static int   history_index = 0;
static int   history_filled = 0;   /* 1 once the buffer has SIM_STEPS entries */

/* Twin's own internally predicted distance (updated each step) */
static float twin_predicted_distance = INITIAL_DISTANCE_M;

/*
 * twin_init
 * ---------
 * Resets all internal twin state to match the initial sensor reading.
 * Call once before the simulation loop.
 *
 * Parameters:
 *   state â€” pointer to the shared fcw_state_t structure
 */
void twin_init(fcw_state_t *state)
{
    int i;
    twin_predicted_distance = state->distance;

    for (i = 0; i < HISTORY_SIZE; i++) {
        distance_history[i] = state->distance;
    }
    history_index  = 0;
    history_filled = 0;

    printf("[TWIN]    Digital twin initialised â€” mirroring distance=%.1f m\n",
           state->distance);
}

/*
 * twin_update
 * -----------
 * Synchronises the twin model with the latest sensor reading, computes a
 * smoothed distance estimate using a simple moving average over the last
 * HISTORY_SIZE readings, and reports the divergence between the twin's own
 * forward prediction and the actual sensor measurement.
 *
 * Parameters:
 *   state â€” pointer to the shared fcw_state_t structure
 */
void twin_update(fcw_state_t *state)
{
    float sum = 0.0f;
    float smoothed_distance;
    float divergence;
    int   count, i;

    /* â”€â”€ Step 1: Store raw sensor reading into rolling history â”€â”€ */
    distance_history[history_index] = state->distance;
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_index == 0) history_filled = 1;

    /* â”€â”€ Step 2: Compute moving average (smoothed distance) â”€â”€ */
    count = history_filled ? HISTORY_SIZE : history_index;
    if (count == 0) count = 1;
    for (i = 0; i < count; i++) {
        sum += distance_history[i];
    }
    smoothed_distance = sum / (float)count;

    /* â”€â”€ Step 3: Divergence = |sensor reading âˆ’ twin prediction| â”€â”€ */
    divergence = fabsf(state->distance - twin_predicted_distance);

    /* â”€â”€ Step 4: Update twin's forward prediction for next cycle â”€â”€ */
    /*    Twin predicts distance = smoothed âˆ’ relative_speed Ã— dt     */
    twin_predicted_distance = smoothed_distance
                              - (state->relative_speed * SIM_STEP_TIME_S);
    if (twin_predicted_distance < 0.0f) twin_predicted_distance = 0.0f;

    printf("[TWIN]    Step %2d | smoothed=%.2f m | twin_pred=%.2f m"
           " | divergence=%.2f m\n",
           state->step, smoothed_distance,
           twin_predicted_distance, divergence);

    /* Flag large divergence (sensor fault or sudden environment change) */
    if (divergence > 10.0f) {
        printf("[TWIN]    *** ANOMALY DETECTED â€” divergence=%.2f m"
               " (possible sensor fault) ***\n", divergence);
    }
}