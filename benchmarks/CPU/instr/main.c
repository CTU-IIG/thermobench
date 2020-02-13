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

#define TIMER_STOP_SIG SIGRTMIN
#define TIMER_START_SIG SIGRTMIN+1
#define MS_TO_NANO 1000000
#define SEC_TO_NANO 1000000000

sem_t mutex;
sem_t *notify;
unsigned input_mask = 0;
static int idle_thread = -1;

int loops_per_print = 1000000;
static void timer_stop_handler(int sig, siginfo_t *si, void *uc)
{
    if (sem_wait(&mutex) == -1)
        err(1, "%s", "timer_stop_handler() - sem_wait()");

    idle_thread = 1; /* stop thread */
 
    if (sem_post(&mutex) == -1)
        err(1, "%s", "timer_stop_handler() - sem_wait()");
}

static void timer_start_handler(int sig, siginfo_t *si, void *uc)
{
    if (sem_wait(&mutex) == -1)
        err(1, "%s", "timer_start_handler() - sem_wait()");

    idle_thread = 0; /* start thread */

    /* wake up the cores given in the program input */
    for(int i = 0; i < sysconf(_SC_NPROCESSORS_ONLN); i++){
        if ((input_mask & (1U << i)) == 0)
            continue;

        sem_post(notify+i); /* blocked thread can start now */
    }
    
    if (sem_post(&mutex) == -1)
        err(1, "%s", "timer_start_handler() - sem_wait()");
}

void *benchmark_loop(void *ptr)
{
    int thread_id = (intptr_t)ptr;
    uint64_t cpu_work_done = 0;

    while (1) {
        for (int j = 0; j < loops_per_print || loops_per_print < 1; ++j) {

            if(idle_thread != -1) { /* if period was defined */
                /* Do I need a lock here? Or can I handle one more iteration with idle_thread = 0 after the signal? */ 
                if(idle_thread == 1){
                    sem_wait(notify+thread_id);
                }
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

int main(int argc, char *argv[])
{
    const int num_proc = sysconf(_SC_NPROCESSORS_ONLN);
    unsigned cpu_mask = 0xffffffff;
    int opt;

    /* timer stuff*/ 
    struct itimerspec ts_stop, ts_start;
    struct sigaction sa_stop, sa_start;
    struct sigevent sev_stop, sev_start;
    timer_t *tidlist;
    long period = 0;
    long utilization_ratio = 100;
    float input_ratio = 0.0;

    while ((opt = getopt(argc, argv, "l:m:p:u:")) != -1) {
        switch (opt) {
        case 'l':
            loops_per_print = xstrtol(optarg, "-l");
            break;
        case 'm':
            cpu_mask = xstrtol(optarg, "-m");
            break;
        case 'p':
            period = xstrtol(optarg, "-p");
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

    if(period > 0){ /* period was passed in the input args */
        input_ratio = utilization_ratio/100.0;

        if(input_ratio < 1) /* thread starts idle */
            idle_thread = 1;
        if(input_ratio == 1) /* do not idle */
            idle_thread = 0;

        if (sem_init(&mutex, 0, 1) == -1)
            err(1, "%s", "sem_init");

        notify = (sem_t *) malloc(sysconf(_SC_NPROCESSORS_ONLN)*sizeof(sem_t));

        for(int i = 0; i < num_proc; i++){
            if ((cpu_mask & (1U << i)) == 0)
                continue;

            if (sem_init(notify+i, 0, 0) == -1)
                err(1, "%s", "sem_init");
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

    if (period > 0){

        input_mask = cpu_mask;

        unsigned long long total_period_ns = (long long) period * MS_TO_NANO;

        if(input_ratio > 0  && input_ratio < 1) {

            tidlist = calloc(2, sizeof(timer_t)); /* free is missing */

            /* handlers for notification signals */
            /* STOP Signal */ 
            sa_stop.sa_flags = SA_SIGINFO;
            sa_stop.sa_sigaction = timer_stop_handler;
            sigemptyset(&sa_stop.sa_mask);
            
            if (sigaction(TIMER_STOP_SIG, &sa_stop, NULL) == -1)
                err(1, "%s", "sigaction for sa_stop!");

            /* START Signal */ 
            sa_start.sa_flags = SA_SIGINFO;
            sa_start.sa_sigaction = timer_start_handler;
            sigemptyset(&sa_start.sa_mask);
            
            if (sigaction(TIMER_START_SIG, &sa_start, NULL) == -1)
                err(1, "%s", "sigaction for sa_start!");

            sev_stop.sigev_notify = SIGEV_SIGNAL;
            sev_stop.sigev_signo = TIMER_STOP_SIG;

            sev_start.sigev_notify = SIGEV_SIGNAL;
            sev_start.sigev_signo = TIMER_START_SIG;

            unsigned long long busy_time_ns = (long long) total_period_ns * input_ratio;

            /* period values for struct */
            double period = (double) total_period_ns / SEC_TO_NANO;
            double period_secs;
            double period_frac_ns = modf(period, &period_secs); /* get the fractional part of the number */
            period_frac_ns = period_frac_ns * SEC_TO_NANO; 

            /* busy values for struct */
            double busy_time = (double) busy_time_ns / SEC_TO_NANO;
            double busy_secs;
            double busy_frac_ns = modf(busy_time, &busy_secs);
            busy_frac_ns = busy_frac_ns * SEC_TO_NANO; 

            /* by default we allow for 3 secs in order to give time to boot everything (can be an input parm if needed) */
            ts_start.it_value.tv_sec = 3; /* initial expiration of the timer */
            ts_start.it_value.tv_nsec = 0;

            ts_start.it_interval.tv_sec = period_secs; /* repeats every X seconds */
            ts_start.it_interval.tv_nsec = (long) period_frac_ns; 

            ts_stop.it_value.tv_sec = ts_start.it_value.tv_sec + busy_secs;   /* initial expiration of the timer */
            ts_stop.it_value.tv_nsec = (long) busy_frac_ns; 

            ts_stop.it_interval.tv_sec = period_secs; /* repeats every X seconds */
            ts_stop.it_interval.tv_nsec = (long) period_frac_ns;  

            sev_stop.sigev_value.sival_ptr = &tidlist[0];
            sev_start.sigev_value.sival_ptr = &tidlist[1];

            if (timer_create(CLOCK_REALTIME, &sev_start, &tidlist[1]) == -1)
                err(1, "%s", "timer_create - sev_start!");

            if (timer_create(CLOCK_REALTIME, &sev_stop, &tidlist[0]) == -1)
                err(1, "%s", "timer_create - sev_stop!");

            if (timer_settime(tidlist[1], 0, &ts_start, NULL) == -1)
                err(1, "%s", "timer_settime - ts_start");

            if (timer_settime(tidlist[0], 0, &ts_stop, NULL) == -1)
                err(1, "%s", "timer_settime - ts_stop");
        }
    }

    pthread_exit(NULL);
}
