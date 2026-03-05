/*
 * fcw_types.h
 * -----------
 * Shared data structures for the Digital Twin Forward Collision Warning System.
 *
 * VERSION 2 — QNX IPC Multi-Process Edition
 *
 * This header is included by ALL modules and ALL process entry files.
 * The fcw_state_t struct lives inside POSIX shared memory (shm_open/mmap)
 * and is accessed by all 4 processes concurrently under pthread_mutex.
 */

#ifndef FCW_TYPES_H
#define FCW_TYPES_H

#include <pthread.h>

/* ─── Shared Memory ──────────────────────────────────────────────────────── */
#define FCW_SHM_NAME  "/fcw_shared_memory"
#define FCW_SHM_SIZE  sizeof(fcw_state_t)

/* ─── Alert Level Codes ──────────────────────────────────────────────────── */
#define ALERT_SAFE      0
#define ALERT_WARNING   1
#define ALERT_CRITICAL  2

/* ─── QNX Pulse Code ─────────────────────────────────────────────────────── */
#if defined(__QNX__)
#include <sys/neutrino.h>
#define PULSE_ALERT_CHANGE  _PULSE_CODE_MINAVAIL
#endif

/* ─── TTC Thresholds (seconds) ───────────────────────────────────────────── */
#define TTC_WARNING_THRESHOLD   3.0f
#define TTC_CRITICAL_THRESHOLD  1.5f

/* ─── Timing Parameters ──────────────────────────────────────────────────── */
#define SIM_STEPS           20
#define SIM_STEP_TIME_S     0.5f
#define SENSOR_PERIOD_MS    100
#define DASH_PERIOD_MS      500

/* ─── Physical Constants ─────────────────────────────────────────────────── */
#define INITIAL_DISTANCE_M  100.0f
#define INITIAL_SPEED_MPS   20.0f
#define OBSTACLE_SPEED_MPS  5.0f

/*
 * fcw_state_t
 * -----------
 * Central shared state — lives in POSIX shared memory.
 * Lock mutex before every read or write.
 *
 * mutex              — POSIX process-shared mutex (MUST be first field)
 * distance           — Measured distance to obstacle (m)
 * vehicle_speed      — Ego-vehicle speed (m/s)
 * relative_speed     — Closing speed (m/s)
 * predicted_distance — Digital twin 1-step-ahead prediction (m)
 * ttc                — Time-To-Collision (seconds)
 * warning_flag       — ALERT_SAFE / ALERT_WARNING / ALERT_CRITICAL
 * anomaly_flag       — 1 if sensor divergence detected
 * step               — Current simulation step counter
 * alert_chid         — QNX channel ID published by alert_proc
 * running            — 1 = simulation active, 0 = shutdown signal
 */
typedef struct {
    pthread_mutex_t mutex;

    float distance;
    float vehicle_speed;
    float relative_speed;
    float predicted_distance;
    float ttc;

    int   warning_flag;
    int   anomaly_flag;
    int   step;

    int   alert_chid;
    int   running;
} fcw_state_t;

#endif /* FCW_TYPES_H */