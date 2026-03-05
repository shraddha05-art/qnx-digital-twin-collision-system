/*
 * fcw_types.h
 * -----------
 * Shared data structures for the Digital Twin Forward Collision Warning System.
 * All modules include this header to access the common fcw_state_t structure,
 * constants, and alert level definitions.
 *
 * This mirrors the shared memory region used in a real QNX RTOS
 * implementation — here it is passed by pointer between modular functions.
 */

#ifndef FCW_TYPES_H
#define FCW_TYPES_H

/* ─── Alert Level Codes ──────────────────────────────────────────────────── */
#define ALERT_SAFE      0   /* No collision risk                              */
#define ALERT_WARNING   1   /* Moderate risk — reduce speed                   */
#define ALERT_CRITICAL  2   /* High risk — emergency braking required         */

/* ─── TTC Thresholds (seconds) ───────────────────────────────────────────── */
#define TTC_WARNING_THRESHOLD   3.0f
#define TTC_CRITICAL_THRESHOLD  1.5f

/* ─── Simulation Parameters ──────────────────────────────────────────────── */
#define SIM_STEPS           20
#define SIM_STEP_TIME_S     0.5f
#define INITIAL_DISTANCE_M  100.0f
#define INITIAL_SPEED_MPS   20.0f
#define OBSTACLE_SPEED_MPS  5.0f

/*
 * fcw_state_t
 * -----------
 * Central shared state. In QNX this lives in a shared memory region
 * (shm_open / mmap). Here it is passed by pointer — same data flow.
 */
typedef struct {
    float distance;             /* Measured distance to obstacle (m)          */
    float vehicle_speed;        /* Ego-vehicle speed (m/s)                    */
    float relative_speed;       /* Closing speed (m/s)                        */
    float predicted_distance;   /* Digital twin 1-second-ahead prediction (m) */
    float ttc;                  /* Time-To-Collision (seconds)                */
    int   warning_flag;         /* ALERT_SAFE / WARNING / CRITICAL            */
    int   anomaly_flag;         /* 1 if sensor spike detected                 */
    int   step;                 /* Current simulation step                    */
} fcw_state_t;

#endif /* FCW_TYPES_H */