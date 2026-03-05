/*
 * alert_proc_main.c
 * -----------------
 * PROCESS 3 — alert_proc   (Priority 28, SCHED_FIFO)
 *
 * Highest-priority process in the alert chain. Event-driven — consumes
 * zero CPU while idle, wakes instantly on QNX Pulse from twin_proc.
 *
 * Responsibilities:
 *   1. Open shared memory
 *   2. Create QNX channel → publish chid into state->alert_chid
 *   3. Block on MsgReceive — zero CPU until pulse arrives
 *   4. On PULSE_ALERT_CHANGE: call warning_evaluate()
 *
 * Linux fallback: polls shared memory every 100 ms.
 *
 * Build (QNX):
 *   qcc -Vgcc_ntoaarch64le -o alert_proc alert_proc_main.c warning_system.c
 *       -I../include -lm
 *
 * Build (Linux):
 *   gcc -o alert_proc alert_proc_main.c warning_system.c
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

void warning_evaluate(const fcw_state_t *state);

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
        printf("[ALERT]   Waiting for shared memory...\n");
        sleep_ms(100);
    }
    if (shm_fd == -1) { perror("[ALERT]   shm_open failed"); return NULL; }

    state = (fcw_state_t *)mmap(NULL, sizeof(fcw_state_t),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED) { perror("[ALERT]   mmap failed"); return NULL; }
    return state;
}

int main(void)
{
    fcw_state_t *state;

#if defined(__QNX__)
    int chid;
    struct _pulse pulse;
    struct sched_param param;
    param.sched_priority = 28;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#else
    int prev_flag = -1;
#endif

    printf("[ALERT]   Starting alert_proc (priority 28)...\n");

    state = open_shared_memory();
    if (!state) return EXIT_FAILURE;
    printf("[ALERT]   Attached to: %s\n", FCW_SHM_NAME);

/* ══ QNX: Real pulse-driven path ════════════════════════════════════════════ */
#if defined(__QNX__)

    chid = ChannelCreate(0);
    if (chid == -1) {
        perror("[ALERT]   ChannelCreate failed");
        munmap(state, sizeof(fcw_state_t));
        return EXIT_FAILURE;
    }

    pthread_mutex_lock(&state->mutex);
    state->alert_chid = chid;
    pthread_mutex_unlock(&state->mutex);

    printf("[ALERT]   QNX channel created (chid=%d) — blocking on MsgReceive\n",
           chid);

    while (state->running) {
        int rcvid = MsgReceive(chid, &pulse, sizeof(pulse), NULL);
        if (rcvid < 0) {
            if (!state->running) break;
            perror("[ALERT]   MsgReceive error");
            continue;
        }
        if (rcvid == 0) {
            if (pulse.code == PULSE_ALERT_CHANGE) {
                pthread_mutex_lock(&state->mutex);
                warning_evaluate(state);
                pthread_mutex_unlock(&state->mutex);
            }
        } else {
            MsgReply(rcvid, 0, NULL, 0);
        }
    }

    ChannelDestroy(chid);

/* ══ Linux: Polling fallback ════════════════════════════════════════════════ */
#else

    pthread_mutex_lock(&state->mutex);
    state->alert_chid = 0;   /* Sentinel: alert_proc is up */
    pthread_mutex_unlock(&state->mutex);

    printf("[ALERT]   Linux mode — polling shared memory every 100 ms\n");

    while (state->running) {
        sleep_ms(100);

        pthread_mutex_lock(&state->mutex);
        int current_flag = state->warning_flag;
        pthread_mutex_unlock(&state->mutex);

        if (current_flag != prev_flag) {
            pthread_mutex_lock(&state->mutex);
            warning_evaluate(state);
            pthread_mutex_unlock(&state->mutex);
            prev_flag = current_flag;
        }
    }

#endif

    munmap(state, sizeof(fcw_state_t));
    printf("[ALERT]   alert_proc exiting.\n");
    return EXIT_SUCCESS;
}