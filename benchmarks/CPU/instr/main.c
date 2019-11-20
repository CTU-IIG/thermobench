#define _GNU_SOURCE
#include "bench.h"
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include BENCH_H

int loops_per_print = 1000000;

void *benchmark_loop(void *ptr)
{
    int thread_id = (intptr_t)ptr;
    uint64_t cpu_work_done = 0;

    while (1) {
        for (int j = 0; j < loops_per_print || loops_per_print < 1; ++j)
            cpu_work_done += bench_func();

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

int main(int argc, char *argv[])
{
    const int num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    unsigned cpu_mask = 0xffffffff;
    int opt;

    while ((opt = getopt(argc, argv, "l:m:")) != -1) {
        switch (opt) {
        case 'l':
            loops_per_print = xstrtol(optarg, "-l");
            break;
        case 'm':
            cpu_mask = xstrtol(optarg, "-m");
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-l loops ] [-m cpu_mask]\n",
                    argv[0]);
            exit(1);
        }
    }

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
        pthread_create(&tid, &attr, benchmark_loop, (void *)(intptr_t)i);
    }

    pthread_exit(NULL);
}
