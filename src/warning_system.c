/*
 * warning_system.c
 * ----------------
 * MODULE 4 â€” Warning System
 *
 * Responsibility:
 *   Consumes the alert level set by the prediction engine and generates the
 *   appropriate human-facing output. In a real embedded system this module
 *   would drive GPIO hardware: LED patterns, buzzer frequencies, and a CAN
 *   bus message to the instrument cluster. Here it prints clearly formatted
 *   console alerts that simulate those outputs.
 *
 * Alert Behaviour:
 *   ALERT_SAFE     â€” no output (system nominal)
 *   ALERT_WARNING  â€” single-line advisory with TTC and distance
 *   ALERT_CRITICAL â€” prominent multi-line alert box urging braking
 *
 * QNX Parallel:
 *   Runs as alert_proc at priority 28 (SCHED_FIFO) â€” higher than twin_proc
 *   so it preempts immediately on receipt of a QNX Pulse. In hardware mode
 *   gpio_thread within this process drives LED and buzzer.
 */

/*
 * warning_system.c
 * ----------------
 * MODULE 4 — Warning System
 *
 * Consumes the alert level set by the prediction engine and generates
 * the appropriate human-facing and hardware output.
 *
 * Alert Behaviour:
 *   ALERT_SAFE     — no alert (system nominal)
 *   ALERT_WARNING  — advisory + LED slow blink + buzzer 1 Hz
 *   ALERT_CRITICAL — emergency alert + LED solid on + buzzer 5 Hz
 *
 * QNX Parallel:
 *   Runs as alert_proc at priority 28 (SCHED_FIFO). Preempts twin_proc
 *   immediately on receipt of a QNX Pulse.
 */

#include <stdio.h>
#include "fcw_types.h"

/*
 * warning_evaluate
 * ----------------
 * Reads state->warning_flag and emits the corresponding alert message.
 */
void warning_evaluate(const fcw_state_t *state)
{
    switch (state->warning_flag) {

    case ALERT_SAFE:
        printf("[WARNING] Step %2d | Status: SAFE"
               "  (dist=%.2f m, TTC=%.2f s)\n",
               state->step, state->distance, state->ttc);
        break;

    case ALERT_WARNING:
        printf("[WARNING] Step %2d | !! COLLISION WARNING !!"
               "  dist=%.2f m  TTC=%.2f s"
               "  — Reduce speed NOW\n",
               state->step, state->distance, state->ttc);
        printf("          [LED: slow blink] [BUZZER: 1 Hz pulse]\n");
        break;

    case ALERT_CRITICAL:
        printf("[WARNING] Step %2d | !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",
               state->step);
        printf("          |  CRITICAL — IMMINENT COLLISION              |\n");
        printf("          |  distance = %6.2f m   TTC = %5.2f s         |\n",
               state->distance, state->ttc);
        printf("          |  APPLY EMERGENCY BRAKING IMMEDIATELY        |\n");
        printf("          !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("          [LED: SOLID ON] [BUZZER: 5 Hz rapid beep]\n");
        break;

    default:
        printf("[WARNING] Step %2d | Unknown alert level: %d\n",
               state->step, state->warning_flag);
        break;
    }
}