#include <linux/sched.h>
#include <sched.h>            /* Definition of SCHED_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "sched_deadline.h"

#ifdef __x86_64__
#define __NR_sched_setattr		314
#define __NR_sched_getattr		315
#endif

#ifdef __i386__
#define __NR_sched_setattr		351
#define __NR_sched_getattr		352
#endif

#ifdef __arm__
#define __NR_sched_setattr		380
#define __NR_sched_getattr		381
#endif

struct sched_attr {
	__u32 size;

	__u32 sched_policy;
	__u64 sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	__s32 sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	__u32 sched_priority;

	/* SCHED_DEADLINE (nsec) */
	__u64 sched_runtime;
	__u64 sched_deadline;
	__u64 sched_period;
};

int sched_setattr(pid_t pid,
		  const struct sched_attr *attr,
		  unsigned int flags)
{
	return syscall(__NR_sched_setattr, pid, attr, flags);
}

int sched_getattr(pid_t pid,
		  struct sched_attr *attr,
		  unsigned int size,
		  unsigned int flags)
{
	return syscall(__NR_sched_getattr, pid, attr, size, flags);
}

void setup_sched_deadline(uint64_t period_ns, uint64_t budget_ns)
{
    struct sched_attr attr;
    int ret;
    unsigned int flags = 0;

    attr.size = sizeof(attr);
    attr.sched_flags = 0;
    // attr.sched_flags |= SCHED_FLAG_DL_OVERRUN; /* Notify us about overruns */

    /* Use GRUB-PA algorithm. If we use less runtime than
     * specified in sched_runtime, the bandwidth is automatically
     * updated and CPU frequency scaled (if used with schedutil
     * governor). */
    attr.sched_flags |= SCHED_FLAG_RECLAIM;

    attr.sched_nice = 0;
    attr.sched_priority = 0;

    attr.sched_policy = SCHED_DEADLINE;
    attr.sched_runtime = budget_ns;
    attr.sched_period = attr.sched_deadline = period_ns;

    ret = sched_setattr(0, &attr, flags);
    if (ret < 0) {
        perror("sched_setattr(SCHED_DEADLINE)");
        exit(1);
    }
}
