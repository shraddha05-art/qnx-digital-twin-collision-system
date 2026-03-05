/*
 * dash_proc.c
 * -----------
 * MODULE 5 — Dashboard Process  (plain terminal, ANSI colors, no ncurses)
 *
 * Completed features:
 *   1. Real-time digital twin output — distance, speed, predicted_distance,
 *      collision_risk, TTC, anomaly flag
 *   2. ANSI color-coded alert banner  (Green=SAFE, Yellow=WARNING, Red=CRITICAL)
 *   3. Auto-refresh every 500 ms
 *   4. System log file  (system_log.txt)
 *   5. ASCII distance trend graph
 *   6. System Health Monitor panel
 *   7. Demo mode  — controlled scenario for judge presentation
 *   8. Screenshot capture  — saves plain-text snapshots to /docs/dashboard_output/
 *
 * QNX Parallel:
 *   dash_proc, Priority 10, SCHED_RR — lowest priority, never blocks RT tasks.
 *
 * Compile:
 *   gcc -D_POSIX_C_SOURCE=200809L ... -lm
 */

/*
 * dash_proc.c
 * -----------
 * MODULE 5 — Dashboard Process  (plain terminal, ANSI colors, no ncurses)
 *
 * Features:
 *   1. Real-time digital twin output — distance, speed, predicted_distance,
 *      collision_risk, TTC, anomaly flag
 *   2. ANSI color-coded alert banner  (Green=SAFE, Yellow=WARNING, Red=CRITICAL)
 *   3. Auto-refresh every 500 ms
 *   4. System log file  (system_log.txt)
 *   5. ASCII distance trend graph
 *   6. System Health Monitor panel
 *   7. Demo mode — controlled scenario for judge presentation
 *   8. Screenshot capture — saves plain-text snapshots to docs/dashboard_output/
 *
 * QNX Parallel:
 *   dash_proc, Priority 10, SCHED_RR — lowest priority, never blocks RT tasks.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fcw_types.h"

/* ─── ANSI Color Codes ────────────────────────────────────────────────────── */
#define R         "\033[0m"
#define BOLD      "\033[1m"
#define GREEN     "\033[32m"
#define YELLOW    "\033[33m"
#define RED       "\033[31m"
#define CYAN      "\033[36m"
#define WHITE     "\033[37m"
#define BG_GREEN  "\033[42m\033[30m"
#define BG_YELLOW "\033[43m\033[30m"
#define BG_RED    "\033[41m\033[37m"
#define CLR       "\033[2J\033[H"

/* ─── Layout ──────────────────────────────────────────────────────────────── */
#define BAR_W    30
#define GRAPH_W  44
#define GRAPH_H   8
#define GRAPH_CAP 44
#define LOG_N     6
#define BORDER  "================================================================"
#define DIV     "----------------------------------------------------------------"

/* ─── Internal State ─────────────────────────────────────────────────────── */
static float dist_hist[GRAPH_CAP];
static int   hist_count    = 0;
static char  log_buf[LOG_N][96];
static int   log_head      = 0;
static int   total_steps   = 0;
static int   warn_count    = 0;
static int   crit_count    = 0;
static int   anomaly_count = 0;
static FILE *log_fp        = NULL;
static int   screenshot_num = 0;

/* ── Utilities ───────────────────────────────────────────────────────────── */

static void get_ts(char *buf, int sz)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, sz, "%H:%M:%S", tm);
}

static void log_push(const char *msg)
{
    strncpy(log_buf[log_head], msg, 95);
    log_buf[log_head][95] = '\0';
    log_head = (log_head + 1) % LOG_N;
}

static const char *alert_str(int f)
{
    switch(f) {
        case ALERT_SAFE:     return "SAFE";
        case ALERT_WARNING:  return "WARNING";
        case ALERT_CRITICAL: return "CRITICAL";
        default:             return "UNKNOWN";
    }
}

static const char *alert_fg(int f)
{
    switch(f) {
        case ALERT_SAFE:     return GREEN;
        case ALERT_WARNING:  return YELLOW;
        case ALERT_CRITICAL: return RED;
        default:             return WHITE;
    }
}

static const char *alert_bg(int f)
{
    switch(f) {
        case ALERT_SAFE:     return BG_GREEN;
        case ALERT_WARNING:  return BG_YELLOW;
        case ALERT_CRITICAL: return BG_RED;
        default:             return "";
    }
}

static void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* ── Panel Renderers ─────────────────────────────────────────────────────── */

static void draw_distance_bar(float dist, float max)
{
    int filled = (int)((dist < 0 ? 0 : dist > max ? max : dist) / max * BAR_W);
    const char *col = dist > 50.0f ? GREEN : dist > 20.0f ? YELLOW : RED;
    int i;
    printf("  Distance Bar    : |%s", col);
    for (i = 0; i < BAR_W; i++) printf("%c", i < filled ? '#' : ' ');
    printf("%s|  %.2f m\n", R, dist < 0 ? 0.0f : dist);
}

static void draw_banner(int flag)
{
    printf("  %s%s  %-60s%s\n", BOLD, alert_bg(flag), alert_str(flag), R);
    switch(flag) {
        case ALERT_SAFE:
            printf("  %sStatus: SAFE%s      All systems nominal. No collision risk.\n",
                   GREEN, R);
            break;
        case ALERT_WARNING:
            printf("  %sStatus: WARNING%s   Collision risk detected! Reduce speed NOW.\n",
                   YELLOW, R);
            break;
        case ALERT_CRITICAL:
            printf("  %sStatus: COLLISION%s IMMINENT — APPLY EMERGENCY BRAKING!\n",
                   RED, R);
            break;
    }
}

static void draw_twin_panel(const fcw_state_t *s)
{
    float spd_kmh = s->vehicle_speed * 3.6f;
    float rel_kmh = s->relative_speed * 3.6f;

    printf("  %sSYSTEM STATE%s\n",  CYAN, R);
    printf("  %s------------%s\n\n", CYAN, R);
    printf("  Sensor Distance    : %s%7.2f m%s\n",      BOLD, s->distance, R);
    printf("  Vehicle Speed      : %s%5.1f km/h%s  (%.2f m/s)\n",
           BOLD, spd_kmh, R, s->vehicle_speed);
    printf("  Relative Speed     :  %5.1f km/h  (closing)\n", rel_kmh);
    printf("  Predicted Distance : %s%7.2f m%s  (1 step ahead)\n",
           BOLD, s->predicted_distance, R);
    printf("  Time-To-Collision  : ");
    if (s->ttc > 999.0f)
        printf("%s  -- (not closing)%s\n", GREEN, R);
    else if (s->ttc > TTC_WARNING_THRESHOLD)
        printf("%s%6.2f s%s\n", GREEN,  s->ttc, R);
    else if (s->ttc > TTC_CRITICAL_THRESHOLD)
        printf("%s%6.2f s%s\n", YELLOW, s->ttc, R);
    else
        printf("%s%6.2f s%s\n", RED,    s->ttc, R);
    printf("  Collision Risk     : %s%s%s\n",
           alert_fg(s->warning_flag), alert_str(s->warning_flag), R);
    printf("  Anomaly Flag       : %s%d  (%s)%s\n",
           s->anomaly_flag ? RED : GREEN,
           s->anomaly_flag,
           s->anomaly_flag ? "sensor spike detected" : "clean",
           R);
    printf("  Sim Step           :  %d\n\n", s->step);
    draw_distance_bar(s->distance, INITIAL_DISTANCE_M);
}

static void draw_hardware_panel(int flag)
{
    printf("\n  %sHARDWARE OUTPUT%s\n", CYAN, R);
    printf("  %s---------------%s\n",   CYAN, R);
    switch(flag) {
        case ALERT_SAFE:
            printf("  LED    : %sOFF%s\n",                GREEN,  R);
            printf("  BUZZER : %sOFF%s\n",                GREEN,  R);
            break;
        case ALERT_WARNING:
            printf("  LED    : %sSlow blink  (1 Hz)%s\n", YELLOW, R);
            printf("  BUZZER : %s1 Hz pulse%s\n",          YELLOW, R);
            break;
        case ALERT_CRITICAL:
            printf("  LED    : %sSOLID ON%s\n",            RED,    R);
            printf("  BUZZER : %sRapid beep  (5 Hz)%s\n",  RED,    R);
            break;
    }
}

static void draw_graph(void)
{
    int cols  = hist_count < GRAPH_W ? hist_count : GRAPH_W;
    int start = hist_count > GRAPH_W ? hist_count - GRAPH_W : 0;
    int r, i, idx, bar_h;
    float val, threshold;

    if (cols == 0) return;

    printf("\n  %sDISTANCE TREND GRAPH%s\n",  CYAN, R);
    printf("  %s---------------------%s\n\n", CYAN, R);

    for (r = GRAPH_H; r >= 1; r--) {
        threshold = (float)r / GRAPH_H * INITIAL_DISTANCE_M;
        printf("  %3.0f m |", threshold);
        for (i = 0; i < cols; i++) {
            idx   = (start + i) % GRAPH_CAP;
            val   = dist_hist[idx];
            bar_h = (int)(val / INITIAL_DISTANCE_M * GRAPH_H);
            if (bar_h >= r)
                printf("%s*%s", val > 50.0f ? GREEN : val > 20.0f ? YELLOW : RED, R);
            else
                printf(" ");
        }
        printf("\n");
    }
    printf("        +");
    for (i = 0; i < cols; i++) printf("-");
    printf("> Steps\n");
}

static void draw_health_panel(void)
{
    char ts[16];
    float w_pct = total_steps > 0 ? (float)warn_count / total_steps * 100.0f : 0.0f;
    float c_pct = total_steps > 0 ? (float)crit_count / total_steps * 100.0f : 0.0f;
    get_ts(ts, sizeof(ts));

    printf("\n  %sSYSTEM HEALTH MONITOR%s\n",  CYAN, R);
    printf("  %s----------------------%s\n",   CYAN, R);
    printf("  Uptime (steps)    : %d\n",    total_steps);
    printf("  Current Time      : %s\n",    ts);
    printf("  WARNING events    : %s%d%s  (%.0f%%)\n", YELLOW, warn_count,  R, w_pct);
    printf("  CRITICAL events   : %s%d%s  (%.0f%%)\n", RED,    crit_count,  R, c_pct);
    printf("  Anomalies flagged : %d\n",    anomaly_count);
    printf("  Log file          : %ssystem_log.txt%s\n", CYAN, R);
}

static void draw_event_log(void)
{
    int i, idx;
    printf("\n  %sEVENT LOG (last %d steps)%s\n", CYAN, LOG_N, R);
    printf("  %s---------------------------%s\n",  CYAN, R);
    for (i = 0; i < LOG_N; i++) {
        idx = (log_head + i) % LOG_N;
        if (log_buf[idx][0] == '\0') continue;
        if      (strstr(log_buf[idx], "CRITICAL")) printf("  %s%s%s\n", RED,    log_buf[idx], R);
        else if (strstr(log_buf[idx], "WARNING"))  printf("  %s%s%s\n", YELLOW, log_buf[idx], R);
        else                                        printf("  %s%s%s\n", GREEN,  log_buf[idx], R);
    }
}

/* ── File Logging ─────────────────────────────────────────────────────────── */

static void log_to_file(const fcw_state_t *s)
{
    char ts[16];
    if (!log_fp) return;
    get_ts(ts, sizeof(ts));
    fprintf(log_fp, "%s | %7.2f m | %5.1f km/h | %7.2f m | %s\n",
            ts, s->distance, s->vehicle_speed * 3.6f,
            s->predicted_distance, alert_str(s->warning_flag));
    fflush(log_fp);
}

/* ── Screenshot Capture ───────────────────────────────────────────────────── */

static void save_screenshot(const fcw_state_t *s)
{
    char path[128], ts[16], line[64];
    FILE *fp;
    int   i, idx;

    snprintf(path, sizeof(path),
             "docs/dashboard_output/%s_step%02d.txt",
             alert_str(s->warning_flag), s->step);

    fp = fopen(path, "w");
    if (!fp) {
        system("mkdir -p docs/dashboard_output");
        fp = fopen(path, "w");
        if (!fp) return;
    }

    get_ts(ts, sizeof(ts));
    fprintf(fp, "================================================================\n");
    fprintf(fp, "   DIGITAL TWIN FCW SYSTEM  |  QNX RTOS  |  Raspberry Pi 4\n");
    fprintf(fp, "================================================================\n\n");
    fprintf(fp, "  Captured    : %s\n",   ts);
    fprintf(fp, "  Alert State : %s\n\n", alert_str(s->warning_flag));
    fprintf(fp, "  SYSTEM STATE\n");
    fprintf(fp, "  ------------\n");
    fprintf(fp, "  Sensor Distance    : %7.2f m\n",     s->distance);
    fprintf(fp, "  Vehicle Speed      :  %5.1f km/h\n", s->vehicle_speed * 3.6f);
    fprintf(fp, "  Predicted Distance : %7.2f m\n",     s->predicted_distance);
    fprintf(fp, "  Time-To-Collision  : ");
    if (s->ttc > 999.0f) fprintf(fp, " -- (not closing)\n");
    else                  fprintf(fp, "%6.2f s\n", s->ttc);
    fprintf(fp, "  Collision Risk     : %s\n",   alert_str(s->warning_flag));
    fprintf(fp, "  Anomaly Flag       : %d\n\n", s->anomaly_flag);
    fprintf(fp, "  EVENT LOG\n");
    for (i = 0; i < LOG_N; i++) {
        idx = (log_head + i) % LOG_N;
        if (log_buf[idx][0]) fprintf(fp, "  %s\n", log_buf[idx]);
    }
    int filled = (int)(s->distance / INITIAL_DISTANCE_M * BAR_W);
    if (filled < 0) filled = 0;
    if (filled > BAR_W) filled = BAR_W;
    fprintf(fp, "\n  Distance Bar : |");
    for (i = 0; i < BAR_W; i++) fprintf(fp, "%c", i < filled ? '#' : ' ');
    snprintf(line, sizeof(line), "|  %.2f m\n", s->distance);
    fprintf(fp, "%s", line);
    fprintf(fp, "\n================================================================\n");
    fclose(fp);

    screenshot_num++;
    printf("[DASH]    Screenshot saved: %s\n", path);
}

/* ── Full Render ──────────────────────────────────────────────────────────── */

static int last_flag = -1;

static void render(const fcw_state_t *s)
{
    char ts[16], entry[96];
    get_ts(ts, sizeof(ts));

    printf(CLR);

    printf("%s%s\n", CYAN, BORDER);
    printf("   DIGITAL TWIN FCW SYSTEM  |  QNX RTOS  |  Raspberry Pi 4\n");
    printf("%s%s\n\n", BORDER, R);

    draw_banner(s->warning_flag);
    printf("\n%s\n", DIV);
    draw_twin_panel(s);
    printf("%s\n", DIV);
    draw_hardware_panel(s->warning_flag);
    printf("\n%s\n", DIV);
    draw_graph();
    printf("\n%s\n", DIV);
    draw_health_panel();
    printf("\n%s\n", DIV);
    draw_event_log();

    printf("\n%s%s%s\n", CYAN, BORDER, R);
    printf("  Refreshing every 500 ms  |  %s  |  Ctrl+C to exit\n", ts);
    printf("%s%s%s\n", CYAN, BORDER, R);

    snprintf(entry, sizeof(entry),
             "%s | dist=%6.2f m | pred=%6.2f m | TTC=%5.2f s | %s",
             ts, s->distance, s->predicted_distance,
             s->ttc > 999.0f ? 0.0f : s->ttc,
             alert_str(s->warning_flag));
    log_push(entry);

    if (s->warning_flag != last_flag) {
        save_screenshot(s);
        last_flag = s->warning_flag;
    }
}

/* ── Demo Mode ────────────────────────────────────────────────────────────── */

void dashboard_run_demo(void)
{
    fcw_state_t s;
    int i;

    struct {
        float dist; float spd; float rel; float pred;
        float ttc; int flag; int hold_ms; const char *label;
    } stages[] = {
        { 100.0f, 20.0f, 25.0f,  75.0f, 4.00f, ALERT_SAFE,     2000, "Stage 1 — Safe Driving"         },
        {  75.0f, 20.0f, 25.0f,  50.0f, 3.00f, ALERT_WARNING,  2000, "Stage 2 — Approaching Obstacle" },
        {  30.0f, 18.0f, 23.0f,   7.0f, 1.30f, ALERT_WARNING,  2000, "Stage 3 — Warning Triggered"    },
        {  10.0f, 16.0f, 21.0f,   0.0f, 0.48f, ALERT_CRITICAL, 3000, "Stage 4 — COLLISION Alert"      },
    };
    int n = (int)(sizeof(stages) / sizeof(stages[0]));

    for (i = 0; i < LOG_N; i++) log_buf[i][0] = '\0';
    log_push("Demo mode started.");
    for (i = 0; i < GRAPH_CAP; i++) dist_hist[i] = 0.0f;
    hist_count = 0; total_steps = 0; warn_count = 0; crit_count = 0;
    last_flag = -1;

    printf(CLR);
    printf("%s%s%s\n", CYAN, BORDER, R);
    printf("   %sDEMO MODE — Digital Twin FCW System%s\n", BOLD, R);
    printf("   Follow along with the 4-stage collision scenario.\n");
    printf("%s%s%s\n\n", CYAN, BORDER, R);
    sleep_ms(1500);

    for (i = 0; i < n; i++) {
        memset(&s, 0, sizeof(s));
        s.distance           = stages[i].dist;
        s.vehicle_speed      = stages[i].spd;
        s.relative_speed     = stages[i].rel;
        s.predicted_distance = stages[i].pred;
        s.ttc                = stages[i].ttc;
        s.warning_flag       = stages[i].flag;
        s.anomaly_flag       = 0;
        s.step               = i + 1;

        total_steps++;
        if (s.warning_flag == ALERT_WARNING)  warn_count++;
        if (s.warning_flag == ALERT_CRITICAL) crit_count++;
        dist_hist[hist_count % GRAPH_CAP] = s.distance;
        hist_count++;

        log_to_file(&s);

        printf(CLR);
        printf("\n\n  %s%s%s — %s%s%s\n\n",
               BOLD, CYAN, stages[i].label,
               alert_fg(stages[i].flag), alert_str(stages[i].flag), R);
        sleep_ms(800);

        render(&s);
        sleep_ms(stages[i].hold_ms);
    }

    printf(CLR);
    printf("\n  %sDemo complete.%s  All 4 stages demonstrated.\n\n", BOLD, R);
    printf("  Screenshots saved in docs/dashboard_output/\n");
    printf("  Log written to system_log.txt\n\n");
}

/* ── Integration Test ─────────────────────────────────────────────────────── */

void dashboard_integration_test(void)
{
    struct {
        float dist; float rel_spd; int expected_flag; const char *label;
    } tests[] = {
        { 80.0f, 25.0f, ALERT_SAFE,     "T1: dist=80m  → SAFE"     },
        { 50.0f, 25.0f, ALERT_WARNING,  "T2: dist=50m  → WARNING"  },
        { 10.0f, 25.0f, ALERT_CRITICAL, "T3: dist=10m  → CRITICAL" },
    };
    int n = (int)(sizeof(tests)/sizeof(tests[0]));
    int i, passed = 0;
    fcw_state_t s;

    printf("\n%s%s%s\n", CYAN, BORDER, R);
    printf("   %sINTEGRATION TEST — Full Pipeline Verification%s\n", BOLD, R);
    printf("   Sensor → Digital Twin → Prediction → Dashboard\n");
    printf("%s%s%s\n\n", CYAN, BORDER, R);

    for (i = 0; i < n; i++) {
        memset(&s, 0, sizeof(s));
        s.distance       = tests[i].dist;
        s.relative_speed = tests[i].rel_spd;
        s.vehicle_speed  = 20.0f;
        s.step           = i + 1;

        if (s.relative_speed <= 0.0f) {
            s.ttc = 9999.0f; s.warning_flag = ALERT_SAFE;
        } else {
            s.ttc = s.distance / s.relative_speed;
            if      (s.ttc > TTC_WARNING_THRESHOLD)  s.warning_flag = ALERT_SAFE;
            else if (s.ttc > TTC_CRITICAL_THRESHOLD) s.warning_flag = ALERT_WARNING;
            else                                      s.warning_flag = ALERT_CRITICAL;
        }
        s.predicted_distance = s.distance - (s.relative_speed * 1.0f);
        if (s.predicted_distance < 0.0f) s.predicted_distance = 0.0f;

        int ok = (s.warning_flag == tests[i].expected_flag);
        if (ok) passed++;

        printf("  %s%-30s%s  TTC=%.2f s  flag=%s  %s%s%s\n",
               BOLD, tests[i].label, R,
               s.ttc, alert_str(s.warning_flag),
               ok ? GREEN : RED, ok ? "PASS" : "FAIL", R);
    }

    printf("\n  Result: %s%d / %d tests passed%s\n",
           passed == n ? GREEN : RED, passed, n, R);
    printf("\n%s%s%s\n\n", CYAN, BORDER, R);
    sleep_ms(2000);
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void dashboard_init(void)
{
    int i;
    char ts[16];

    for (i = 0; i < LOG_N;     i++) log_buf[i][0] = '\0';
    for (i = 0; i < GRAPH_CAP; i++) dist_hist[i]   = 0.0f;
    hist_count = 0; total_steps = 0; warn_count = 0;
    crit_count = 0; anomaly_count = 0; last_flag = -1;

    log_push("Dashboard started.");
    system("mkdir -p docs/dashboard_output");

    log_fp = fopen("system_log.txt", "a");
    if (log_fp) {
        get_ts(ts, sizeof(ts));
        fprintf(log_fp,
                "# ── Simulation started %s ──\n"
                "# Time     | Distance  |    Speed  | Predicted | Risk\n", ts);
    } else {
        printf("[DASH]    Warning: could not open system_log.txt\n");
    }

    printf("[DASH]    Dashboard ready.  Log: system_log.txt  "
           "Screenshots: docs/dashboard_output/\n");
}

int dashboard_update(const fcw_state_t *s)
{
    total_steps++;
    if (s->warning_flag == ALERT_WARNING)  warn_count++;
    if (s->warning_flag == ALERT_CRITICAL) crit_count++;
    if (s->anomaly_flag)                   anomaly_count++;

    dist_hist[hist_count % GRAPH_CAP] = s->distance;
    hist_count++;

    log_to_file(s);
    render(s);
    sleep_ms(500);
    return 0;
}

void dashboard_shutdown(void)
{
    if (log_fp) {
        char ts[16];
        get_ts(ts, sizeof(ts));
        fprintf(log_fp, "# ── Simulation ended %s ──\n\n", ts);
        fclose(log_fp);
        log_fp = NULL;
    }
    printf("[DASH]    Closed. %d screenshots in docs/dashboard_output/  "
           "Log: system_log.txt\n", screenshot_num);
}