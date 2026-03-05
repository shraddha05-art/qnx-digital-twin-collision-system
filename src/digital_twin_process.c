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

/*
 * digital_twin_process.c
 * ----------------------
 * MODULE 2 — Digital Twin Process
 *
 * Maintains the software Digital Twin of the vehicle system.
 * Computes smoothed distance (5-sample moving average), detects
 * sensor anomalies via divergence metric, and writes
 * predicted_distance into shared state every cycle.
 *
 * BUG FIX v2:
 *   state->predicted_distance is now written at the end of twin_update().
 *   In v1 it was computed locally but never stored — dashboard showed 0.00 m.
 *
 * QNX Parallel:
 *   Runs inside twin_proc at priority 25 (SCHED_FIFO).
 */

#include <stdio.h>
#include <math.h>
#include "fcw_types.h"

/* ─── Internal Twin State ────────────────────────────────────────────────── */
#define HISTORY_SIZE      5
#define ANOMALY_THRESHOLD 10.0f

static float distance_history[HISTORY_SIZE] = {0};
static int   history_index  = 0;
static int   history_filled = 0;
static float twin_predicted_distance = INITIAL_DISTANCE_M;

/*
 * twin_init
 * ---------
 * Resets all internal twin state to match the initial sensor reading.
 */
void twin_init(fcw_state_t *state)
{
    int i;
    twin_predicted_distance = state->distance;
    for (i = 0; i < HISTORY_SIZE; i++)
        distance_history[i] = state->distance;
    history_index  = 0;
    history_filled = 0;

    printf("[TWIN]    Digital twin initialised — mirroring distance=%.1f m\n",
           state->distance);
}

/*
 * twin_update
 * -----------
 * Synchronises twin model with latest sensor reading.
 * Computes smoothed distance and writes predicted_distance into shared state.
 */
void twin_update(fcw_state_t *state)
{
    float sum = 0.0f;
    float smoothed_distance;
    float divergence;
    int   count, i;

    /* Step 1: Store raw sensor reading into rolling history */
    distance_history[history_index] = state->distance;
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_index == 0) history_filled = 1;

    /* Step 2: Moving average */
    count = history_filled ? HISTORY_SIZE : history_index;
    if (count == 0) count = 1;
    for (i = 0; i < count; i++)
        sum += distance_history[i];
    smoothed_distance = sum / (float)count;

    /* Step 3: Divergence = |sensor − twin prediction| */
    divergence = fabsf(state->distance - twin_predicted_distance);

    /* Step 4: Forward prediction for next cycle */
    twin_predicted_distance = smoothed_distance
                              - (state->relative_speed * SIM_STEP_TIME_S);
    if (twin_predicted_distance < 0.0f)
        twin_predicted_distance = 0.0f;

    /* Step 5: Write prediction into shared state (BUG FIX) */
    state->predicted_distance = twin_predicted_distance;

    /* Step 6: Anomaly flag */
    state->anomaly_flag = (divergence > ANOMALY_THRESHOLD) ? 1 : 0;

    printf("[TWIN]    Step %2d | smoothed=%.2f m | twin_pred=%.2f m"
           " | divergence=%.2f m%s\n",
           state->step, smoothed_distance,
           twin_predicted_distance, divergence,
           state->anomaly_flag ? " | *** ANOMALY ***" : "");
}