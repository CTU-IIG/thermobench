#include <err.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bsort.c"
#include <semaphore.h>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <exception>
#include <cstring>
#include <chrono>

using namespace std;

int loops_per_print = 1;
int delay_ms = 0;

// C++ wrapper over C's cpu_set_t
class cpu_set {
public:
    static constexpr int max_cpus = 64;

    cpu_set() : s(CPU_ALLOC(max_cpus)) { zero(); }
    cpu_set(const cpu_set& o) : s(CPU_ALLOC(max_cpus)) { memcpy(s, o.s, size()); }
    cpu_set(cpu_set&& o) : s(o.s) { o.s = nullptr; }
    cpu_set(long int mask) : cpu_set() {
        for (size_t i = 0; i < sizeof(mask) * 8; i++)
            if (mask & (1UL<<i))
                set(i); }
    ~cpu_set() { CPU_FREE(s); }

    void zero() { CPU_ZERO_S(size(), s); }
    bool is_set(unsigned i) const { return !!CPU_ISSET_S(i, size(), s); }
    void set(unsigned i) { CPU_SET_S(i, size(), s); }
    void clr(unsigned i) { CPU_CLR_S(i, size(), s); }
    cpu_set& operator &=(const cpu_set& o) { CPU_AND_S(size(), s, s, o.s); return *this; }
    cpu_set& operator ^=(const cpu_set& o) { CPU_XOR_S(size(), s, s, o.s); return *this; }
    cpu_set& operator |=(const cpu_set& o) { CPU_OR_S(size(), s, s, o.s); return *this; }
    bool operator ==(cpu_set& o) { return CPU_EQUAL_S(size(), s, o.s); }
    bool operator !=(cpu_set& o) { return !(o == *this); }
    unsigned count() const { return CPU_COUNT_S(size(), s); }
    size_t size() const { return CPU_ALLOC_SIZE(max_cpus); }
    cpu_set_t* ptr() const { return s; }
    int as_int() const { int mask = 0; for (int i = 0; i < max_cpus; i++) if (is_set(i)) mask |= 1<<i; return mask; }
    explicit operator bool() const { return count() > 0; }

    cpu_set &operator =(const cpu_set& o) { memcpy(s, o.s, size()); return *this; }
    cpu_set operator &(const cpu_set& o) const { return cpu_set(*this) &= o; }
    cpu_set operator |(const cpu_set& o) const { return cpu_set(*this) |= o; }
    cpu_set operator ^(const cpu_set& o) const { return cpu_set(*this) ^= o; }

    unsigned highest() const {
        for (int i = max_cpus - 1; i >= 0; i--)
            if (is_set(i))
                return i;
        throw runtime_error("empty cpu_set");
    }
    unsigned lowest() const {
        for (int i = 0; i < max_cpus; i++)
            if (is_set(i))
                return i;
        throw runtime_error("empty cpu_set");
    }
    unsigned next_set(int j) {
        int i = j + 1;
        do {
            if (is_set(i))
                return i;
            i = (i + 1) % max_cpus;
        } while (i != j + 1);
        throw runtime_error("empty cpu_set");
    }
private:
    cpu_set_t *s;
};

cpu_set cpu_mask;

struct synchronizer {
    cpu_set run, done;
    unsigned num_cpus = 0, num_done = 0;
    unsigned first_cpu;

    unsigned work_done = 0;
    mutex m;
    condition_variable cv;

    void wait(int cpu) {
        unique_lock<mutex> lock(m);
        while (!run.is_set(cpu))
            cv.wait(lock);
//            printf("running at CPU%u\n", cpu);
//            fflush(stdout);
    }

    void completed(int cpu, unsigned wd, unsigned duration) {
        unique_lock<mutex> lock(m);
        run.clr(cpu);
        done.set(cpu);
        work_done += wd;
        cout << "work_done=" << work_done << endl;
        cout << "duration=" << duration << endl;
        cout << flush;

        if (++num_done == num_cpus) {
            usleep(delay_ms * 1000);
            cpu_set next, last = done;
            cout << "-- Next CPUs:";
            for (unsigned i = 0, cpu_it = first_cpu;
                 i < num_cpus;
                 i++, cpu_it = last.next_set(cpu_it)) {
                done.clr(cpu_it);
                unsigned next_cpu = cpu_it;
                do {
                    next_cpu = cpu_mask.next_set(next_cpu);
                } while (done.is_set(next_cpu) || next.is_set(next_cpu));
                cout << " " << cpu_it;
                next.set(next_cpu);
                if (i == 0)
                    first_cpu = next_cpu;
            }
            cout << endl << flush;

            num_done = 0;
            run = next;
            cv.notify_all();
        }
    }
} s;


int bench_func()
{
    bsort_init();
    bsort_main();
    return 1;
}


void *benchmark_loop(void *ptr)
{
    int cpu = (intptr_t)ptr;

    while (1) {
        s.wait(cpu);

        using namespace chrono;
        auto start = steady_clock::now();
        int wd = 0;
        for (int j = 0; j < loops_per_print || loops_per_print < 1; ++j)
            wd += bench_func();
        auto stop = steady_clock::now();

        s.completed(cpu, wd, duration_cast<milliseconds>(stop - start).count());
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
    int opt;

    sched_getaffinity(0, cpu_mask.size(), cpu_mask.ptr());

    while ((opt = getopt(argc, argv, "d:l:m:i:")) != -1) {
        switch (opt) {
        case 'd':
            delay_ms = xstrtol(optarg, "-d");
            break;
        case 'l':
            loops_per_print = xstrtol(optarg, "-l");
            break;
        case 'm': {
            cpu_set m(xstrtol(optarg, "-m"));
            if (!m)
                errx(1, "Mask cannot be 0");
            if (m ^ (m & cpu_mask))
                errx(1, "Invalid CPU(s) in mask; given: %#x, allowed: %#x", m.as_int(), cpu_mask.as_int());
            cpu_mask &= m;
            break;
        }
        case 'i': {
            cpu_set m(xstrtol(optarg, "-i"));
            if (!m)
                errx(1, "Initial mask cannot be 0");
            s.run = m;
            break;
        }
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-l loops ] [-m cpu_mask]\n",
                    argv[0]);
            exit(1);
        }
    }

    if (!s.run)
        s.run.set(cpu_mask.lowest());

    if ((cpu_mask & s.run) != s.run)
        errx(1, "Invalid initial mask");
    s.num_cpus = s.run.count();
    s.first_cpu = s.run.lowest();

    for (int i = 0; i < cpu_mask.max_cpus; i++) {
        if (!cpu_mask.is_set(i))
            continue;

        pthread_attr_t attr;
        cpu_set cpuset(1 << i);
        pthread_t tid;

        pthread_attr_init(&attr);
        pthread_attr_setaffinity_np(&attr, cpuset.size(), cpuset.ptr());
        pthread_create(&tid, &attr, benchmark_loop, (void *)(intptr_t)i);
    }

    pthread_exit(NULL);
}
