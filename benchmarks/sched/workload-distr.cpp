#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bsort.c>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>

class semaphore {
public:
    semaphore(int value) { sem_init(&sem, 0, value); };
    ~semaphore() { sem_destroy(&sem); }
    void wait() { sem_wait(&sem); }
    void post() { sem_post(&sem); }
private:
    sem_t sem;
};

using namespace std;

int loops_per_print = 1;

bool sequential = true;
semaphore seq_sem(1);

uint64_t cpu_mask = 0xffffffffffffffff;

struct synchronizer {
    uint64_t cpu_turn_mask = 1;
    unsigned work_done = 0;
    mutex m;
    condition_variable cv;
} s;


int bench_func()
{
    bsort_init();
    bsort_main();
    return 1;
}


void *benchmark_loop(void *ptr)
{
    int thread_id = (intptr_t)ptr;

    while (1) {
        if (sequential) {
            unique_lock<mutex> lock(s.m);
            while (((1 << thread_id) & s.cpu_turn_mask) == 0)
                s.cv.wait(lock);
        }

        printf("running at CPU%u\n", thread_id);
        fflush(stdout);
        int wd;
        for (int j = 0; j < loops_per_print || loops_per_print < 1; ++j)
            wd = bench_func();

        if (sequential) {
            int msb = 31 - __builtin_clz(cpu_mask);
            int lsb = __builtin_ctz(cpu_mask);
            //int bits = __builtin_popcount(cpu_mask);
            unique_lock<mutex> lock(s.m);
            s.work_done += wd;
            printf("work_done=%u\n", s.work_done);
            fflush(stdout);
            do {
                s.cpu_turn_mask <<= 1;
                if (31 - __builtin_clz(s.cpu_turn_mask) > msb)
                    s.cpu_turn_mask = (1 << lsb);
            } while ((s.cpu_turn_mask & cpu_mask) == 0);
            s.cv.notify_all();
        }
    }
    return NULL;
}

long int xstrtol(const char *str, const char *err_msg)
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
