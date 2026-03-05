/*
 * main.c — Simulation Orchestrator
 *
 * Runs all 5 modules in priority order matching the QNX scheduler:
 *   sensor_update()          Priority 30 — data acquisition
 *   twin_update()            Priority 25 — digital twin sync
 *   prediction_compute_ttc() Priority 23 — TTC analysis
 *   warning_evaluate()       Priority 28 — alert output
 *   dashboard_update()       Priority 10 — display (lowest)
 *
 * Usage:
 *   ./fcw_sim           — run simulation
 *   ./fcw_sim --demo    — demo mode for judges
 *   ./fcw_sim --test    — integration test
 */

#include <stdio.h>
#include <string.h>
#include "fcw_types.h"

void sensor_init(fcw_state_t *s);
void sensor_update(fcw_state_t *s);
void twin_init(fcw_state_t *s);
void twin_update(fcw_state_t *s);
void prediction_compute_ttc(fcw_state_t *s);
void warning_evaluate(const fcw_state_t *s);
void dashboard_init(void);
int  dashboard_update(const fcw_state_t *s);
void dashboard_shutdown(void);
void dashboard_run_demo(void);
void dashboard_integration_test(void);

int main(int argc, char *argv[])
{
    fcw_state_t state;
    int step;

    /* ── Mode selection ── */
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        dashboard_init();
        dashboard_run_demo();
        dashboard_shutdown();
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        dashboard_init();
        dashboard_integration_test();
        dashboard_shutdown();
        return 0;
    }

    /* ── Normal simulation ── */
    sensor_init(&state);
    twin_init(&state);
    dashboard_init();

    for (step = 0; step < SIM_STEPS; step++) {
        sensor_update(&state);
        twin_update(&state);
        prediction_compute_ttc(&state);
        warning_evaluate(&state);
        dashboard_update(&state);
        if (state.distance <= 0.0f) break;
    }

    dashboard_shutdown();
    printf("Done. dist=%.2f m  TTC=%.2f s  Alert=%s\n",
           state.distance, state.ttc,
           state.warning_flag == 0 ? "SAFE" :
           state.warning_flag == 1 ? "WARNING" : "CRITICAL");
    return 0;
}