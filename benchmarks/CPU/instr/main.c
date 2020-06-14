#define _GNU_SOURCE
#include "bench.h"
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h> 
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include BENCH_H

#define MS_TO_NANO 1000000
#define SEC_TO_NANO 1000000000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int idle_thread = -1;

int loops_per_print = 1000000;

void *benchmark_loop(void *ptr)
{
    int thread_id = (intptr_t)ptr;
    uint64_t cpu_work_done = 0;

    while (1) {
        for (int j = 0; j < loops_per_print || loops_per_print < 1; ++j) {

            if(idle_thread != -1) { /* if period was defined */
                /* Do I need a lock here? Or can I handle one more iteration with idle_thread = 0 after the signal? */ 
                pthread_mutex_lock(&mutex);
                while (idle_thread == 1)
                    pthread_cond_wait(&cond, &mutex);
                pthread_mutex_unlock(&mutex);
            }
            cpu_work_done += bench_func();
        }
        printf("CPU%d_work_done=%lu\n", thread_id, cpu_work_done);
        fflush(stdout);
    }
    return NULL;
}

long int xstrtol(const char *str, char *err_msg)
{
    long int val;
    char *endptr;

    errno = 0; /* To distinguish success/failure after call */
    val = strtol(str, &endptr, 0);

    /* Check for various possible errors */

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
        || (errno != 0 && val == 0))
        err(1, "%s", err_msg);

    if (endptr == str)
        errx(1, "%s: No digits were found", err_msg);

    /* If we got here, strtol() successfully parsed a number */

    if (*endptr != '\0')
        errx(1, "%s: Further characters after number: %s", err_msg, endptr);

    return val;
}

void timespec_add_ms(struct timespec *ts, int ms)
{
    ts->tv_sec += ((ts->tv_nsec) + (long long)ms * MS_TO_NANO) / SEC_TO_NANO;
    ts->tv_nsec = ((ts->tv_nsec) + (long long)ms * MS_TO_NANO) % SEC_TO_NANO;
}

int main(int argc, char *argv[])
{
    const int num_proc = sysconf(_SC_NPROCESSORS_CONF);
    unsigned cpu_mask = 0xffffffff;
    int opt;

    /* timer stuff*/ 
    long period_ms = 0;
    long utilization_ratio = 100;

    while ((opt = getopt(argc, argv, "l:m:p:u:")) != -1) {
        switch (opt) {
        case 'l':
            loops_per_print = xstrtol(optarg, "-l");
            break;
        case 'm':
            cpu_mask = xstrtol(optarg, "-m");
            break;
        case 'p':
            period_ms = xstrtol(optarg, "-p");
            break;
        case 'u':
            utilization_ratio = xstrtol(optarg, "-u");
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-l loops ] [-m cpu_mask]\n",
                    argv[0]);
            exit(1);
        }
    }

    if(utilization_ratio < 0 || utilization_ratio > 100)
        errx(1, "%s", "utilization ratio is not in the interval [0,100]!");

    if (utilization_ratio == 0)
        idle_thread = 1;
    if (utilization_ratio == 100)
        idle_thread = 0;
    else
        idle_thread = 0;

    for (int i = 0; i < num_proc; i++) {
        pthread_attr_t attr;
        cpu_set_t cpuset;
        pthread_t tid;

        if ((cpu_mask & (1U << i)) == 0)
            continue;

        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_attr_init(&attr);
        pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
        int ret = pthread_create(&tid, &attr, benchmark_loop, (void *)(intptr_t)i);
        if (ret != 0)
            fprintf(stderr, "Warning: Thread %d creation error %d\n", i, ret);
    }

    if (period_ms > 0){
        if(utilization_ratio > 0  && utilization_ratio < 100) {
            struct timespec next;

            clock_gettime(CLOCK_MONOTONIC, &next);

            while (1) {
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

                pthread_mutex_lock(&mutex);
                idle_thread = 0; /* start thread */
                pthread_cond_broadcast(&cond); /* Wake-up all waiting threads */
                pthread_mutex_unlock(&mutex);

                timespec_add_ms(&next, period_ms * utilization_ratio / 100);
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

                pthread_mutex_lock(&mutex);
                idle_thread = 1; /* stop thread */
                pthread_mutex_unlock(&mutex);

                timespec_add_ms(&next, period_ms * (100 - utilization_ratio) / 100);
            }
        }
    }

    pthread_exit(NULL);
}
