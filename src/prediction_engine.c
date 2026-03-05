/*
 * prediction_engine.c
 * -------------------
 * MODULE 3 â€” Prediction Engine
 *
 * Responsibility:
 *   Implements the Time-To-Collision (TTC) algorithm â€” the analytical core of
 *   the Digital Twin. Using the current distance and relative (closing) speed
 *   from the shared state, it computes how many seconds remain until the
 *   vehicle reaches the obstacle and writes both the TTC value and the
 *   resulting alert level back into fcw_state_t.
 *
 * TTC Formula:
 *   TTC = distance / relative_speed
 *
 *   If relative_speed <= 0 the vehicles are not closing; TTC is effectively
 *   infinite and the state is SAFE.
 *
 * Alert Logic:
 *   TTC > TTC_WARNING_THRESHOLD (3.0 s)  â†’  ALERT_SAFE
 *   TTC_CRITICAL (1.5 s) < TTC â‰¤ 3.0 s  â†’  ALERT_WARNING
 *   TTC â‰¤ TTC_CRITICAL_THRESHOLD (1.5 s) â†’  ALERT_CRITICAL
 *
 * QNX Parallel:
 *   Runs inside twin_proc as analysis_thread at priority 23 (SCHED_FIFO).
 *   After computing alert level, notify_thread sends a QNX Pulse to
 *   alert_proc whenever warning_flag changes value.
 */

/*
 * prediction_engine.c
 * -------------------
 * MODULE 3 — Prediction Engine
 *
 * Implements the Time-To-Collision (TTC) algorithm.
 * Computes TTC from current distance and relative speed,
 * then classifies the alert level.
 *
 * TTC Formula:   TTC = distance / relative_speed
 *
 * Alert Logic:
 *   TTC > 3.0 s              → ALERT_SAFE
 *   1.5 s < TTC <= 3.0 s    → ALERT_WARNING
 *   TTC <= 1.5 s             → ALERT_CRITICAL
 *
 * QNX Parallel:
 *   Runs inside twin_proc as analysis_thread at priority 23 (SCHED_FIFO).
 */

#include <stdio.h>
#include "fcw_types.h"

/*
 * prediction_compute_ttc
 * ----------------------
 * Computes TTC and writes warning_flag into shared state.
 */
void prediction_compute_ttc(fcw_state_t *state)
{
    /* Guard: vehicles not closing */
    if (state->relative_speed <= 0.0f) {
        state->ttc          = 9999.0f;
        state->warning_flag = ALERT_SAFE;
        printf("[PREDICT] Step %2d | Vehicles not closing — TTC=inf  [SAFE]\n",
               state->step);
        return;
    }

    /* Guard: obstacle already reached */
    if (state->distance <= 0.0f) {
        state->ttc          = 0.0f;
        state->warning_flag = ALERT_CRITICAL;
        printf("[PREDICT] Step %2d | Distance=0 — COLLISION  [CRITICAL]\n",
               state->step);
        return;
    }

    /* Core TTC */
    state->ttc = state->distance / state->relative_speed;

    if (state->ttc > TTC_WARNING_THRESHOLD) {
        state->warning_flag = ALERT_SAFE;
        printf("[PREDICT] Step %2d | TTC=%5.2f s  →  [SAFE]\n",
               state->step, state->ttc);
    }
    else if (state->ttc > TTC_CRITICAL_THRESHOLD) {
        state->warning_flag = ALERT_WARNING;
        printf("[PREDICT] Step %2d | TTC=%5.2f s  →  [WARNING]\n",
               state->step, state->ttc);
    }
    else {
        state->warning_flag = ALERT_CRITICAL;
        printf("[PREDICT] Step %2d | TTC=%5.2f s  →  [CRITICAL]\n",
               state->step, state->ttc);
    }
}