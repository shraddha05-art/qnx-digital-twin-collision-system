/*
 * fcw_types.h
 * -----------
 * Shared data structures for the Digital Twin Forward Collision Warning System.
 * All modules include this header to access the common fcw_state_t structure,
 * constants, and alert level definitions.
 *
 * This mirrors the shared memory region that would be used in a real QNX RTOS
 * implementation — here it is passed by pointer between modular functions.
 */

#ifndef FCW_TYPES_H
#define FCW_TYPES_H

/* ─── Alert Level Codes ──────────────────────────────────────────────────── */
#define ALERT_SAFE      0   /* No collision risk detected                     */
#define ALERT_WARNING   1   /* Moderate risk — driver should be cautious      */
#define ALERT_CRITICAL  2   /* High risk — immediate braking recommended      */

/* ─── TTC Thresholds (seconds) ───────────────────────────────────────────── */
#define TTC_WARNING_THRESHOLD   3.0f  /* Warn when TTC drops below 3 seconds  */
#define TTC_CRITICAL_THRESHOLD  1.5f  /* Critical alert below 1.5 seconds     */

/* ─── Simulation Parameters ──────────────────────────────────────────────── */
#define SIM_STEPS           20      /* Number of simulation cycles to run     */
#define SIM_STEP_TIME_S     0.5f    /* Time elapsed per simulation step (s)   */
#define INITIAL_DISTANCE_M  100.0f  /* Starting distance to obstacle (metres) */
#define INITIAL_SPEED_MPS   20.0f   /* Vehicle speed in metres per second     */
#define OBSTACLE_SPEED_MPS  5.0f    /* Obstacle moving toward vehicle (m/s)   */

/*
 * fcw_state_t
 * -----------
 * The central shared state structure. In a QNX system this would live in a
 * shared memory region (shm_open / mmap). Here it is passed by pointer so
 * every module operates on the same data — simulating that pattern cleanly.
 *
 * Fields:
 *   distance        — current measured distance to obstacle (metres)
 *   vehicle_speed   — ego-vehicle speed (m/s)
 *   relative_speed  — closing speed between vehicle and obstacle (m/s)
 *   ttc             — predicted Time-To-Collision (seconds)
 *   warning_flag    — alert level: ALERT_SAFE / WARNING / CRITICAL
 *   step            — current simulation step counter
 */
typedef struct {
    float distance;         /* Distance to obstacle in metres                 */
    float vehicle_speed;    /* Ego-vehicle speed in m/s                       */
    float relative_speed;   /* Closing speed (vehicle + obstacle) in m/s      */
    float ttc;              /* Predicted Time-To-Collision in seconds          */
    int   warning_flag;     /* Alert level (ALERT_SAFE / WARNING / CRITICAL)  */
    int   step;             /* Current simulation step index                  */
} fcw_state_t;

#endif /* FCW_TYPES_H */