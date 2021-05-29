/**********************************************/
/* Memory latency benchmark		      */
/* 					      */
/* Author: Michal Sojka <sojkam1@fel.cvut.cz> */
/**********************************************/

#define _GNU_SOURCE
#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define STRINGIFY(val) #val
#define TOSTRING(val) STRINGIFY(val)
#define LOC __FILE__ ":" TOSTRING(__LINE__) ": "

#define CHECK(cmd) ({ int ret = (cmd); if (ret == -1) { perror(LOC #cmd); exit(1); }; ret; })
#define CHECKPTR(cmd) ({ void *ptr = (cmd); if (ptr == (void*)-1) { perror(LOC #cmd); exit(1); }; ptr; })
#define CHECKNULL(cmd) ({ typeof(cmd) ptr = (cmd); if (ptr == NULL) { perror(LOC #cmd); exit(1); }; ptr; })
#define CHECKFGETS(s, size, stream) ({ void *ptr = fgets(s, size, stream); if (ptr == NULL) { if (feof(stream)) fprintf(stderr, LOC "fgets(" #s "): Unexpected end of stream\n"); else perror(LOC "fgets(" #s ")"); exit(1); }; ptr; })
#define CHECKTRUE(bool, msg) ({ if (!(bool)) { fprintf(stderr, "Error: " msg "\n"); exit(1); }; })

#define CACHE_LINE_SIZE 64

struct cfg {
	bool sequential;
	unsigned size;
	unsigned num_threads;
	unsigned read_count;
	cpu_set_t cpu_set;
	bool write;
	unsigned ofs;
	bool use_cycles; /* instead of ns */
	unsigned report_bandwidth; /* 0 = disabled, 1 = B/s, 1024 = KiB/s, 1024^2 = MiB/s, ... */
	bool forever;
};

struct s {
        struct s *ptr;
        uint32_t dummy[(CACHE_LINE_SIZE - sizeof(struct s*))/sizeof(uint32_t)];
};

static_assert(sizeof(struct s) == CACHE_LINE_SIZE, "Struct size differs from cacheline size");

#define MAX_CPUS 8

#ifdef __aarch64__
#define MRS32(reg) ({ uint32_t v; asm volatile ("mrs %0," # reg : "=r" (v)); v; })
#define MRS64(reg) ({ uint64_t v; asm volatile ("mrs %0," # reg : "=r" (v)); v; })

#define MSR(reg, v) ({ asm volatile ("msr " # reg ",%0" :: "r" (v)); })

static void ccntr_init(void)
{
	MSR(PMCNTENSET_EL0, 0x80000000);
	MSR(PMCR_EL0, MRS32(PMCR_EL0) | 1);
}

static uint64_t ccntr_get(void)
{
	return MRS64(PMCCNTR_EL0);
}
#else
static void ccntr_init(void)
{
	errx(1, "Reporting in clock cycles is not supported on this architecture\n");
}
static uint64_t ccntr_get(void) { return 0; }
#endif

static uint64_t get_time(struct cfg *cfg)
{
	if (cfg->use_cycles == false) {
		struct timespec t;

		clock_gettime(CLOCK_MONOTONIC, &t);

		return (uint64_t)t.tv_sec * 1000000000 + t.tv_nsec;
	} else {
		return ccntr_get();
	}
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static void prepare(struct s *array, unsigned size, bool sequential)
{
	int i, j;
	int count = size / sizeof(struct s);
        assert(count > 0);

	if (sequential) {
		for (i = 0; i < count - 1; i++)
			array[i].ptr = &array[i+1];
		array[count - 1].ptr = &array[0];
	} else {
		memset(array, 0, size);
		struct s *p = &array[0];
		for (i = 0; i < count - 1; i++) {
			p->ptr = (struct s*)1; /* Mark as occupied to avoid self-loop */
			for (j = rand() % count;
			     array[j].ptr != NULL;
			     j = (j >= count) ? 0 : j+1);
			p = p->ptr = &array[j];
		}
		p->ptr = &array[0];
	}
}

static void do_read(struct s *array, unsigned reads)
{
	unsigned i = reads / 32;
	volatile struct s *p = &array[0];
	while (--i) {
		p = p->ptr;	/* 0 */
		p = p->ptr;	/* 1 */
		p = p->ptr;	/* 2 */
		p = p->ptr;	/* 3 */
		p = p->ptr;	/* 4 */
		p = p->ptr;	/* 5 */
		p = p->ptr;	/* 6 */
		p = p->ptr;	/* 7 */
		p = p->ptr;	/* 8 */
		p = p->ptr;	/* 9 */
		p = p->ptr;	/* 10 */
		p = p->ptr;	/* 11 */
		p = p->ptr;	/* 12 */
		p = p->ptr;	/* 13 */
		p = p->ptr;	/* 14 */
		p = p->ptr;	/* 15 */
		p = p->ptr;	/* 16 */
		p = p->ptr;	/* 17 */
		p = p->ptr;	/* 18 */
		p = p->ptr;	/* 19 */
		p = p->ptr;	/* 20 */
		p = p->ptr;	/* 21 */
		p = p->ptr;	/* 22 */
		p = p->ptr;	/* 23 */
		p = p->ptr;	/* 24 */
		p = p->ptr;	/* 25 */
		p = p->ptr;	/* 26 */
		p = p->ptr;	/* 27 */
		p = p->ptr;	/* 28 */
		p = p->ptr;	/* 29 */
		p = p->ptr;	/* 30 */
		p = p->ptr;	/* 31 */
	}
}

static void do_write(struct s *array, unsigned accesses, unsigned ofs)
{
	unsigned i = accesses / 32;
	volatile struct s *p = &array[0];
	while (--i) {
		p->dummy[ofs]++; p = p->ptr; /* 0 */
		p->dummy[ofs]++; p = p->ptr; /* 1 */
		p->dummy[ofs]++; p = p->ptr; /* 2 */
		p->dummy[ofs]++; p = p->ptr; /* 3 */
		p->dummy[ofs]++; p = p->ptr; /* 4 */
		p->dummy[ofs]++; p = p->ptr; /* 5 */
		p->dummy[ofs]++; p = p->ptr; /* 6 */
		p->dummy[ofs]++; p = p->ptr; /* 7 */
		p->dummy[ofs]++; p = p->ptr; /* 8 */
		p->dummy[ofs]++; p = p->ptr; /* 9 */
		p->dummy[ofs]++; p = p->ptr; /* 10 */
		p->dummy[ofs]++; p = p->ptr; /* 11 */
		p->dummy[ofs]++; p = p->ptr; /* 12 */
		p->dummy[ofs]++; p = p->ptr; /* 13 */
		p->dummy[ofs]++; p = p->ptr; /* 14 */
		p->dummy[ofs]++; p = p->ptr; /* 15 */
		p->dummy[ofs]++; p = p->ptr; /* 16 */
		p->dummy[ofs]++; p = p->ptr; /* 17 */
		p->dummy[ofs]++; p = p->ptr; /* 18 */
		p->dummy[ofs]++; p = p->ptr; /* 19 */
		p->dummy[ofs]++; p = p->ptr; /* 20 */
		p->dummy[ofs]++; p = p->ptr; /* 21 */
		p->dummy[ofs]++; p = p->ptr; /* 22 */
		p->dummy[ofs]++; p = p->ptr; /* 23 */
		p->dummy[ofs]++; p = p->ptr; /* 24 */
		p->dummy[ofs]++; p = p->ptr; /* 25 */
		p->dummy[ofs]++; p = p->ptr; /* 26 */
		p->dummy[ofs]++; p = p->ptr; /* 27 */
		p->dummy[ofs]++; p = p->ptr; /* 28 */
		p->dummy[ofs]++; p = p->ptr; /* 29 */
		p->dummy[ofs]++; p = p->ptr; /* 30 */
		p->dummy[ofs]++; p = p->ptr; /* 31 */
	}
}

struct benchmark_thread {
	pthread_t id;
	unsigned cpu;
	double result;
	struct cfg *cfg;
};

pthread_barrier_t barrier;

struct s array[MAX_CPUS][64*0x100000/sizeof(struct s)] __attribute__ ((aligned (2*1024*1024)));

bool print = true;

static void *benchmark_thread(void *arg)
{
	struct benchmark_thread *me = arg;
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(me->cpu, &set);

	if (pthread_setaffinity_np(me->id, sizeof(set), &set) != 0)
		errx(1, "Failed setting pthread_setaffinity_np to CPU %d", me->cpu);

	prepare(array[me->cpu], me->cfg->size, me->cfg->sequential);

	pthread_barrier_wait(&barrier);

	if (print)
		fprintf(stderr, "CPU %d starts measurement\n", me->cpu);

        unsigned work_done = 0;
        do {
                uint64_t tic, tac;
                tic = get_time(me->cfg);
                if (me->cfg->write == false)
                        do_read(array[me->cpu], me->cfg->read_count);
                else
                        do_write(array[me->cpu], me->cfg->read_count, me->cfg->ofs);

                tac = get_time(me->cfg);
                me->result = (double)(tac - tic) / me->cfg->read_count;
                if (me->cfg->forever) {
                        printf("CPU%d_work_done=%u\n", me->cpu, work_done++);
                        printf("CPU%d size:%d time:%g\n", me->cpu, me->cfg->size, (tac - tic) / 1000000000.0);
			fflush(stdout);
                }
        } while (me->cfg->forever);
	return NULL;
}

static void run_benchmark(struct cfg *cfg)
{
	struct benchmark_thread thread[MAX_CPUS];
	unsigned i;
	cpu_set_t set = cfg->cpu_set;

	if (CPU_COUNT(&set) == 0) {
		/* If CPU affinity is not specified on the command
		 * line, use the default affinity, we inherited from
		 * our parent. */
		sched_getaffinity(0, sizeof(set), &set);
	}

	pthread_barrier_init(&barrier, NULL, cfg->num_threads);
	for (i = 0; i < cfg->num_threads; i++) {
		thread[i].cfg = cfg;
		for (int j = 0; j < MAX_CPUS; j++) {
			if (CPU_ISSET(j, &set)) {
				thread[i].cpu = j;
				CPU_CLR(j, &set);
				break;
			}
		}
		if (print)
			fprintf(stderr, "Running thread %d on CPU %d\n", i, thread[i].cpu);
		pthread_create(&thread[i].id, NULL, benchmark_thread, &thread[i]);
	}
	for (i = 0; i < cfg->num_threads; i++) {
		pthread_join(thread[i].id, NULL);
	}
	pthread_barrier_destroy(&barrier);

	double sum = 0;
	printf("%d", cfg->size);
	for (i = 0; i < cfg->num_threads; i++) {
		double number;
		if (cfg->report_bandwidth == 0)
			number = thread[i].result;
		else
			number = CACHE_LINE_SIZE / thread[i].result * 1e9 / cfg->report_bandwidth;
		sum += number;
		printf("\t%#.3g", number);
	}
	if (cfg->num_threads > 1 && cfg->report_bandwidth != 0)
		printf("\t∑%#.3g", sum);
	printf("\n");
	fflush(stdout);
	print = false;
}

static void print_help(char *argv[], struct cfg *dflt)
{
	printf("Usage: %s [ options ]\n"
	       "\n"
	       "Tool for measuring memory latency.\n"
	       "\n"
	       "Supported options are:\n"
	       "  -b[unit]    Report memory bandwidth instead of latency. Reported\n"
	       "              numbers are in bytes/s (default), or bigger units as\n"
	       "              specified by [unit] (one of K, M, G).\n"
	       "  -c <count>  Count of read (or read-write) operations per benchmark\n"
	       "              (default is %#x)\n"
	       "  -C <CPU#>   Run the benchmark on given CPU# (can be specified\n"
	       "              multiple times), also see -t; the default is to go from\n"
	       "	      CPU 0 upwards\n"
	       "  -f          Execute forever in a loop\n"
	       "  -h          Show this help\n"
	       "  -o <ofs>    Offset of write operation within the cache line (see -w)\n"
	       "  -r          Traverse memory in random order (default is sequential)\n"
	       "  -s <WSS>    Run benchmark for given working set size; the default is\n"
	       "              to benchmark a sequence of multiple WSSs\n"
	       "  -t <#thr>   The number of benchmark threads to run; use -C to\n"
	       "              specify their CPU affinity. If -C is specified, -t defaults\n"
	       "              to the number of -C occurrences\n"
	       "  -w          Perform both memory reads and writes (default is only\n"
	       "              reads)\n"
	       "  -y          Report the memory access duration in clock cycles rather\n"
	       "              then in nanoseconds\n"
	       "\n"
	       "Memory latency is reported as nanoseconds (with -y as clock cycles)\n"
	       "per memory operation.\n"
	       ,
	       argv[0],
	       dflt->read_count);
}

int main(int argc, char *argv[])
{
	srand(time(NULL));

	struct cfg cfg = {
		.sequential = true,
		.num_threads = 0,
		.size = 0,
		.read_count = 0x2000000,
		.write = false,
		.ofs = 0,
		.use_cycles = false, /* i.e. use nanoseconds */
	};
	CPU_ZERO(&cfg.cpu_set);
	unsigned C_cnt = 0;

	int opt;
	while ((opt = getopt(argc, argv, "b::c:C:fho:rs:t:wy")) != -1) {
		switch (opt) {
		case 'b':
			if (!optarg) {
				cfg.report_bandwidth = 1;
			} else {
				if (strlen(optarg) != 1)
					errx(1, "-b unit should be one letter. not '%s'", optarg);
				switch (*optarg) {
				case 'K': cfg.report_bandwidth = 1024; break;
				case 'M': cfg.report_bandwidth = 1024*1024; break;
				case 'G': cfg.report_bandwidth = 1024*1024*1024; break;
				default: errx(1, "Unsupported unit: %s", optarg);
				}
			}
			break;
		case 'c':
			cfg.read_count = atol(optarg);
			break;
		case 'f':
			cfg.forever = true;
			break;
		case 'o':
			cfg.ofs = atol(optarg);
			break;
		case 'r':	/* random */
			cfg.sequential = false;
			break;
		case 's':
			cfg.size = atol(optarg);
                        if (cfg.size > sizeof(array[0]))
                            errx(1, "Maximum size for -s is %zu", sizeof(array[0]));
			break;
		case 't':
			cfg.num_threads = atol(optarg);
			break;
		case 'C':
			CPU_SET(atol(optarg), &cfg.cpu_set);
			C_cnt++;
			break;
		case 'w':
			cfg.write = true;
			break;
		case 'y':
			cfg.use_cycles = true;
			break;
		case 'h':
			print_help(argv, &cfg);
			exit(0);
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s ... TODO\n", argv[0]);
			exit(1);
		}
	}

	if (cfg.num_threads == 0)
		cfg.num_threads = (C_cnt != 0) ? C_cnt : 1; /* default value */

	if (cfg.write) {
		struct s s;
		assert(cfg.ofs < ARRAY_SIZE(s.dummy));
	}

	if (cfg.use_cycles && cfg.report_bandwidth)
		errx(1, "Flags -y and -b are not compatible");
	if (cfg.use_cycles)
		ccntr_init();

	if (cfg.size != 0) {
		run_benchmark(&cfg);
	} else {
		unsigned order, size, step;
		for (order = 10; order <= 24; order++) {
			for (step = 0; step < 2; step++) {
				size = 1 << order;
				if (step == 1)
					size += size / 2;

				cfg.size = size;
				run_benchmark(&cfg);
			}
		}
	}
	return 0;
}

/* Local Variables: */
/* c-basic-offset: 8 */
/* indent-tabs-mode: t */
/* End: */
