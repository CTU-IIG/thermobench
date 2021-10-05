#ifndef SCHED_DEADLINE_H
#define SCHED_DEADLINE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void setup_sched_deadline(uint64_t period_ns, uint64_t budget_ns);

#ifdef __cplusplus
}
#endif

#endif
