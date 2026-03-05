/*
 * dash_proc.c
 * -----------
 * MODULE 5 — Dashboard Process (plain terminal, no external libraries)
 *
 * Responsibility:
 *   Reads the shared fcw_state_t every simulation step and prints a clean,
 *   formatted dashboard to the terminal showing all system state fields,
 *   a visual ASCII distance bar, alert level, hardware output status, and
 *   a scrolling event log — using only standard C (stdio, string, time).
 *
 * QNX Parallel:
 *   Runs as dash_proc at priority 10 (SCHED_RR) — lowest priority so it
 *   never interferes with the real-time sensor, twin, or alert processes.
 *   In the real QNX system this reads the shared memory region (shm_open /
 *   mmap) without a mutex since it only reads, never writes.
 *
 * Compile:  gcc ... -lm      (no -lncurses, no special flags needed)
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/fcw_types.h"

/* ─── Configuration ───────────────────────────────────────────────────────── */
#define BAR_WIDTH   30         /* Characters wide for the distance bar        */
#define LOG_LINES    6         /* Number of scrolling event log lines         */
#define BORDER  "============================================================"
#define DIVIDER "------------------------------------------------------------"

/* ─── Scrolling Event Log ─────────────────────────────────────────────────── */
static char log_buf[LOG_LINES][80];
static int  log_head = 0;

/*
 * dash_sleep_500ms
 * ----------------
 * Sleeps for 1 second between dashboard refreshes.
 * Uses sleep() from <unistd.h> — no extra flags needed.
 */
static void dash_sleep_500ms(void)
{
    sleep(1);
}

/*
 * log_push
 * --------
 * Inserts a new message into the circular event log buffer.
 */
static void log_push(const char *msg)
{
    strncpy(log_buf[log_head], msg, 79);
    log_buf[log_head][79] = '\0';
    log_head = (log_head + 1) % LOG_LINES;
}

/*
 * alert_label
 * -----------
 * Returns a short string label for the given warning_flag value.
 */
static const char *alert_label(int flag)
{
    switch (flag) {
        case ALERT_SAFE:     return "SAFE";
        case ALERT_WARNING:  return "WARNING";
        case ALERT_CRITICAL: return "CRITICAL";
        default:             return "UNKNOWN";
    }
}

/*
 * draw_distance_bar
 * -----------------
 * Prints an ASCII progress bar showing distance as a fraction of
 * INITIAL_DISTANCE_M.  The # fill shrinks as the obstacle closes in.
 *
 *   Full (100 m):    |##############################|  100.00 m
 *   Half  (50 m):    |###############               |   50.00 m
 *   Empty  (0 m):    |                              |    0.00 m
 */
static void draw_distance_bar(float distance, float max)
{
    int filled, i;

    if (distance < 0.0f) distance = 0.0f;
    if (distance > max)  distance = max;

    filled = (int)((distance / max) * BAR_WIDTH);

    printf("  Distance Bar : |");
    for (i = 0; i < BAR_WIDTH; i++) {
        printf("%c", i < filled ? '#' : ' ');
    }
    printf("|  %.2f m\n", distance);
}

/*
 * render_dashboard
 * ----------------
 * Clears the terminal then draws one complete dashboard frame.
 * The ANSI escape \033[2J\033[H clears the screen and moves the cursor to
 * the top-left — works on Linux, macOS, and QNX terminals.
 */
static void render_dashboard(const fcw_state_t *state)
{
    int  i, idx;
    char log_entry[80];

    /* Clear terminal screen */
    printf("\033[2J\033[H");

    /* ── Header ── */
    printf("%s\n", BORDER);
    printf("   DIGITAL TWIN FCW SYSTEM  |  QNX RTOS  |  Raspberry Pi 4\n");
    printf("%s\n", BORDER);

    /* ── Alert status ── */
    printf("  Alert Status  : ");
    switch (state->warning_flag) {
        case ALERT_SAFE:
            printf("[ SAFE     ]  All clear.\n");
            break;
        case ALERT_WARNING:
            printf("[ WARNING  ]  Reduce speed - collision risk ahead.\n");
            break;
        case ALERT_CRITICAL:
            printf("[ CRITICAL ]  APPLY EMERGENCY BRAKING NOW!\n");
            break;
        default:
            printf("[ UNKNOWN  ]\n");
    }

    printf("%s\n", DIVIDER);

    /* ── Real system state ── */
    printf("  -- REAL SYSTEM STATE --\n");
    draw_distance_bar(state->distance, INITIAL_DISTANCE_M);
    printf("  Vehicle Speed  : %.2f m/s\n", state->vehicle_speed);
    printf("  Relative Speed : %.2f m/s\n", state->relative_speed);
    printf("  Sim Step       : %d\n",        state->step);

    printf("%s\n", DIVIDER);

    /* ── Digital twin / prediction ── */
    printf("  -- DIGITAL TWIN PREDICTION --\n");
    if (state->ttc > 999.0f)
        printf("  Time-To-Coll.  : --  (vehicles not closing)\n");
    else
        printf("  Time-To-Coll.  : %.2f s\n", state->ttc);

    printf("  TTC Thresholds : WARNING < %.1f s  |  CRITICAL < %.1f s\n",
           TTC_WARNING_THRESHOLD, TTC_CRITICAL_THRESHOLD);

    printf("%s\n", DIVIDER);

    /* ── Hardware output status ── */
    printf("  -- HARDWARE OUTPUT --\n");
    switch (state->warning_flag) {
        case ALERT_SAFE:
            printf("  LED    : OFF\n");
            printf("  BUZZER : OFF\n");
            break;
        case ALERT_WARNING:
            printf("  LED    : Slow blink (1 Hz)\n");
            printf("  BUZZER : 1 Hz pulse\n");
            break;
        case ALERT_CRITICAL:
            printf("  LED    : SOLID ON\n");
            printf("  BUZZER : Rapid 5 Hz beep\n");
            break;
    }

    printf("%s\n", DIVIDER);

    /* ── Scrolling event log ── */
    printf("  -- EVENT LOG (last %d steps) --\n", LOG_LINES);
    for (i = 0; i < LOG_LINES; i++) {
        idx = (log_head + i) % LOG_LINES;
        if (log_buf[idx][0] != '\0') {
            printf("  %s\n", log_buf[idx]);
        }
    }

    printf("%s\n", BORDER);

    /* Add this step to the event log for the next frame */
    snprintf(log_entry, sizeof(log_entry),
             "Step %2d | dist=%6.2f m | TTC=%5.2f s | %s",
             state->step, state->distance, state->ttc,
             alert_label(state->warning_flag));
    log_push(log_entry);
}

/* ─── Public API ──────────────────────────────────────────────────────────── */

/*
 * dashboard_init
 * --------------
 * Clears the log buffer. Call once before the simulation loop starts.
 */
void dashboard_init(void)
{
    int i;
    for (i = 0; i < LOG_LINES; i++) {
        log_buf[i][0] = '\0';
    }
    log_push("Dashboard started.");
    printf("[DASH]  Dashboard initialised.\n");
}

/*
 * dashboard_update
 * ----------------
 * Renders one dashboard frame then pauses 500 ms.
 * Call once per simulation step after all other modules have run.
 *
 * Parameters:
 *   state — pointer to the shared fcw_state_t (read-only)
 *
 * Returns:
 *   0 — always continue simulation
 */
int dashboard_update(const fcw_state_t *state)
{
    render_dashboard(state);
    dash_sleep_500ms();
    return 0;
}

/*
 * dashboard_shutdown
 * ------------------
 * Prints a closing message. Call after the simulation loop ends.
 */
void dashboard_shutdown(void)
{
    printf("[DASH]  Dashboard closed.\n");
}


