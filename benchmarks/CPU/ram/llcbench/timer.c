#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include "llcbench.h"

#if defined(__i386__)
#define RDTSC_X86
#define USE_RDTSC
#elif defined(__x86_64__)
#define RDTSC_AMD64
#define USE_RDTSC
#endif

#if defined(USE_RDTSC)
#  if defined(RDTSC_AMD64)
#    define RDTSC(llptr) ({\
       __asm__ __volatile__ (\
       "rdtsc\n\t"\
       "shlq\t$32, %%rdx\n\t"\
       "orq\t%%rdx, %%rax\n\t"\
       : "=A" (llptr)\
       : /* no input vars */\
       : "%rdx"\
       );\
       }\
       )
#  elif defined(RDTSC_X86)
#    define RDTSC(llptr) ({\
       __asm__ __volatile__ (\
       "rdtsc\n\t"\
       : "=A" (llptr)\
       );\
       }\
       )
#  else
#    error "Need to define RDTSC_AMD64 or RDTSC_X86 with USE_RDTSC."
#  endif

static unsigned long long cycles_per_second()
{
  double seconds;
  unsigned long long starttick, endtick;
  unsigned long long starttime, endtime;
  struct timeval tv;

  gettimeofday(&tv, 0);
  starttime = tv.tv_sec*1000 + tv.tv_usec/1000;
  while ( 1 )
    {
      gettimeofday(&tv, 0);
      endtime = tv.tv_sec*1000 + tv.tv_usec/1000;
      if ( endtime != starttime )
        {
          break;
        }
    }

  while ( 1 )
    {
      gettimeofday(&tv, 0);
      endtime = tv.tv_sec*1000 + tv.tv_usec/1000;
      if ( (endtime - starttime) > 1 )
        {
          RDTSC(starttick);
          break;
        }
    }

  starttime = endtime;
  while ( 1 )
    {
      gettimeofday(&tv, 0);
      endtime = tv.tv_sec*1000 + tv.tv_usec/1000;
      if ( (endtime - starttime) > 1000 )
        {
          RDTSC(endtick);
          break;
        }
    }

  return (endtick - starttick);
}

static unsigned long long cps = 0;
#endif

static double t1, t2;

void timer_start(void)
{
#if defined(USE_GETTIMEOFDAY)
  {
    struct timeval ts;
    gettimeofday(&ts, (struct timezone*)0);
    t1 = (double)ts.tv_sec*1000000000.0 + (double)ts.tv_usec*1000.0; 
  }
#elif defined(USE_RDTSC)
  {
    unsigned long long ticks;
    if ( cps == 0 )
      cps = cycles_per_second();
    RDTSC(ticks);
    t1 = (ticks / (double)cps) * 1.0e9;
  }
#else
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    t1 = (double)ts.tv_sec*1000000000.0 + (double)ts.tv_nsec;
  }
#endif
  DBG(printf("START %f\n",t1))
}

void timer_stop(void)
{
#if defined(USE_GETTIMEOFDAY)
  {
    struct timeval ts;
    gettimeofday(&ts, (struct timezone*)0);
    t2 = (double)ts.tv_sec*1000000000.0 + (double)ts.tv_usec*1000.0; 
  }
#elif defined(USE_RDTSC)
  {
    unsigned long long ticks;
    if ( cps == 0 )
      cps = cycles_per_second();  /* probably not necessary, but safe */
    RDTSC(ticks);
    t2 = (ticks / (double)cps) * 1.0e9;
  }
#else
  {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    t2 = (double)ts.tv_sec*1000000000.0 + (double)ts.tv_nsec;
  }
#endif
  DBG(printf("STOP %f\n",t2))
}

double timer_elapsed(void)
{
  if (t2-t1 <= 0.0)
    {
      fprintf(stderr,"Warning! The timer is not precise enough. Consider increasing\nthe iteration count or changing the timer in timer.c\n");
      return(0.0);
    }
  return((t2-t1)/1000.0);
}
