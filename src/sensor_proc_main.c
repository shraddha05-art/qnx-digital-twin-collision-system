/*
 * sensor_proc_main.c
 * ------------------
 * PROCESS 1 — sensor_proc   (Priority 30, SCHED_FIFO)
 *
 * Responsibilities:
 *   1. Create POSIX shared memory and initialise fcw_state_t
 *   2. Set up process-shared pthread_mutex
 *   3. Call sensor_init()
 *   4. Fire sensor_update() every SENSOR_PERIOD_MS milliseconds
 *   5. Signal shutdown via state->running = 0
 *
 * Build (QNX):
 *   qcc -Vgcc_ntoaarch64le -o sensor_proc sensor_proc_main.c sensor_process.c
 *       -I../include -lm
 *
 * Build (Linux):
 *   gcc -o sensor_proc sensor_proc_main.c sensor_process.c
 *       -I../include -lm -lpthread -lrt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "fcw_types.h"

void sensor_init(fcw_state_t *state);
void sensor_update(fcw_state_t *state);

static void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int main(void)
{
    int              shm_fd;
    fcw_state_t     *state;
    pthread_mutexattr_t mattr;
    int              step;

#if defined(__QNX__)
    struct sched_param param;
    param.sched_priority = 30;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif

    printf("[SENSOR]  Starting sensor_proc (priority 30)...\n");

    /* ── Create shared memory ─────────────────────────────────────────────── */
    shm_unlink(FCW_SHM_NAME);

    shm_fd = shm_open(FCW_SHM_NAME, O_CREAT | O_RDWR,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (shm_fd == -1) { perror("[SENSOR]  shm_open failed"); return EXIT_FAILURE; }

    if (ftruncate(shm_fd, sizeof(fcw_state_t)) == -1) {
        perror("[SENSOR]  ftruncate failed");
        shm_unlink(FCW_SHM_NAME);
        return EXIT_FAILURE;
    }

    state = (fcw_state_t *)mmap(NULL, sizeof(fcw_state_t),
                                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED) {
        perror("[SENSOR]  mmap failed");
        shm_unlink(FCW_SHM_NAME);
        return EXIT_FAILURE;
    }

    memset(state, 0, sizeof(fcw_state_t));

    /* ── Process-shared mutex ─────────────────────────────────────────────── */
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&state->mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    /* ── Initialise sensor state ──────────────────────────────────────────── */
    pthread_mutex_lock(&state->mutex);
    sensor_init(state);
    state->running    = 1;
    state->alert_chid = -1;
    pthread_mutex_unlock(&state->mutex);

    printf("[SENSOR]  Shared memory created: %s (%zu bytes)\n",
           FCW_SHM_NAME, sizeof(fcw_state_t));
    printf("[SENSOR]  Waiting 300 ms for other processes to attach...\n");
    sleep_ms(300);

    /* ── Main sensor loop ─────────────────────────────────────────────────── */
    for (step = 0; step < SIM_STEPS; step++) {
        sleep_ms(SENSOR_PERIOD_MS);

        pthread_mutex_lock(&state->mutex);
        if (!state->running) { pthread_mutex_unlock(&state->mutex); break; }
        sensor_update(state);
        pthread_mutex_unlock(&state->mutex);

        if (state->distance <= 0.0f) {
            printf("[SENSOR]  Distance = 0 — stopping.\n");
            break;
        }
    }

    /* ── Signal shutdown ──────────────────────────────────────────────────── */
    pthread_mutex_lock(&state->mutex);
    state->running = 0;
    pthread_mutex_unlock(&state->mutex);

    printf("[SENSOR]  Complete after %d steps. Holding shm open for 3 s...\n",
           step + 1);
    sleep_ms(3000);

    pthread_mutex_destroy(&state->mutex);
    munmap(state, sizeof(fcw_state_t));
    shm_unlink(FCW_SHM_NAME);

    printf("[SENSOR]  sensor_proc exiting.\n");
    return EXIT_SUCCESS;
}