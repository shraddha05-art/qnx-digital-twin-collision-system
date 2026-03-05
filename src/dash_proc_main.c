/*
 * dash_proc_main.c
 * ----------------
 * PROCESS 4 — dash_proc   (Priority 10, SCHED_RR)
 *
 * Lowest priority — never blocks real-time tasks.
 *
 * Snapshot-copy design:
 *   Copies fcw_state_t under mutex into a local snapshot, then releases
 *   the lock BEFORE calling dashboard_update(). This means the mutex is
 *   held for microseconds, not the full 500 ms render cycle.
 *
 * Build (QNX):
 *   qcc -Vgcc_ntoaarch64le -o dash_proc dash_proc_main.c dash_proc.c
 *       -I../include -lm
 *
 * Build (Linux):
 *   gcc -o dash_proc_bin dash_proc_main.c dash_proc.c
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

#include "fcw_types.h"

void dashboard_init(void);
int  dashboard_update(const fcw_state_t *state);
void dashboard_shutdown(void);

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
        printf("[DASH]    Waiting for shared memory...\n");
        sleep_ms(100);
    }
    if (shm_fd == -1) { perror("[DASH]    shm_open failed"); return NULL; }

    state = (fcw_state_t *)mmap(NULL, sizeof(fcw_state_t),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED) { perror("[DASH]    mmap failed"); return NULL; }
    return state;
}

int main(void)
{
    fcw_state_t *state;
    fcw_state_t  snapshot;

#if defined(__QNX__)
    struct sched_param param;
    param.sched_priority = 10;
    pthread_setschedparam(pthread_self(), SCHED_RR, &param);
#endif

    printf("[DASH]    Starting dash_proc (priority 10)...\n");

    state = open_shared_memory();
    if (!state) return EXIT_FAILURE;
    printf("[DASH]    Attached to: %s\n", FCW_SHM_NAME);

    dashboard_init();

    while (1) {
        /* ── Snapshot under mutex — lock held for microseconds only ───────── */
        pthread_mutex_lock(&state->mutex);

        if (!state->running && state->step == 0) {
            pthread_mutex_unlock(&state->mutex);
            sleep_ms(50);
            continue;
        }

        memcpy(&snapshot, state, sizeof(fcw_state_t));
        int still_running = state->running;

        pthread_mutex_unlock(&state->mutex);

        /* ── Render outside mutex ─────────────────────────────────────────── */
        dashboard_update(&snapshot);

        if (!still_running) break;
        if (snapshot.distance <= 0.0f) break;
    }

    dashboard_shutdown();
    munmap(state, sizeof(fcw_state_t));
    printf("[DASH]    dash_proc exiting.\n");
    return EXIT_SUCCESS;
}