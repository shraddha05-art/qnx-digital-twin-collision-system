/*
 * twin_proc_main.c
 * ----------------
 * PROCESS 2 — twin_proc   (Priority 25, SCHED_FIFO)
 *
 * Responsibilities:
 *   1. Open shared memory created by sensor_proc
 *   2. Initialise digital twin
 *   3. Every SENSOR_PERIOD_MS: run twin_update() + prediction_compute_ttc()
 *   4. On alert level change: send QNX Pulse to alert_proc
 *      (Linux fallback: directly call warning_evaluate)
 *
 * Build (QNX):
 *   qcc -Vgcc_ntoaarch64le -o twin_proc twin_proc_main.c
 *       digital_twin_process.c prediction_engine.c
 *       -I../include -lm
 *
 * Build (Linux):
 *   gcc -o twin_proc twin_proc_main.c digital_twin_process.c
 *       prediction_engine.c warning_system.c
 *       -I../include -lm -lpthread -lrt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#if defined(__QNX__)
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#endif

#include "fcw_types.h"

void twin_init(fcw_state_t *state);
void twin_update(fcw_state_t *state);
void prediction_compute_ttc(fcw_state_t *state);

#if !defined(__QNX__)
void warning_evaluate(const fcw_state_t *state);
#endif

static void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static fcw_state_t *open_shared_memory(void)
{
    int shm_fd, retries = 30;
    fcw_state_t *state;

    while (retries-- > 0) {
        shm_fd = shm_open(FCW_SHM_NAME, O_RDWR, 0);
        if (shm_fd != -1) break;
        printf("[TWIN]    Waiting for shared memory...\n");
        sleep_ms(100);
    }
    if (shm_fd == -1) { perror("[TWIN]    shm_open failed"); return NULL; }

    state = (fcw_state_t *)mmap(NULL, sizeof(fcw_state_t),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED) { perror("[TWIN]    mmap failed"); return NULL; }
    return state;
}

int main(void)
{
    fcw_state_t *state;
    int          prev_flag = ALERT_SAFE;
    int          current_flag;

#if defined(__QNX__)
    int alert_coid = -1;
    struct sched_param param;
    param.sched_priority = 25;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif

    printf("[TWIN]    Starting twin_proc (priority 25)...\n");

    state = open_shared_memory();
    if (!state) return EXIT_FAILURE;
    printf("[TWIN]    Attached to: %s\n", FCW_SHM_NAME);

    pthread_mutex_lock(&state->mutex);
    twin_init(state);
    pthread_mutex_unlock(&state->mutex);

#if defined(__QNX__)
    {
        int retries = 50;
        while (retries-- > 0 && state->alert_chid == -1) sleep_ms(100);
        if (state->alert_chid != -1) {
            alert_coid = ConnectAttach(ND_LOCAL_NODE, 0,
                                       state->alert_chid,
                                       _NTO_SIDE_CHANNEL, 0);
            if (alert_coid == -1)
                perror("[TWIN]    ConnectAttach failed");
            else
                printf("[TWIN]    Connected to alert_proc chid=%d\n",
                       state->alert_chid);
        }
    }
#endif

    while (1) {
        sleep_ms(SENSOR_PERIOD_MS);

        pthread_mutex_lock(&state->mutex);
        if (!state->running) { pthread_mutex_unlock(&state->mutex); break; }

        twin_update(state);
        prediction_compute_ttc(state);
        current_flag = state->warning_flag;

        pthread_mutex_unlock(&state->mutex);

        if (current_flag != prev_flag) {
            printf("[TWIN]    Alert change: %d → %d\n", prev_flag, current_flag);

#if defined(__QNX__)
            if (alert_coid != -1)
                MsgSendPulse(alert_coid, -1, PULSE_ALERT_CHANGE, current_flag);
#else
            warning_evaluate(state);
#endif
            prev_flag = current_flag;
        }

        if (state->distance <= 0.0f) break;
    }

#if defined(__QNX__)
    if (alert_coid != -1) ConnectDetach(alert_coid);
#endif
    munmap(state, sizeof(fcw_state_t));
    printf("[TWIN]    twin_proc exiting.\n");
    return EXIT_SUCCESS;
}