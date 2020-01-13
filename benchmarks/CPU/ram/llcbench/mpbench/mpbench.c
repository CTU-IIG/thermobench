/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/* 
* File:    mpbench.c
* CVS:     $Id: mpbench.c,v 1.7 2005/01/19 17:10:23 mucci Exp $
* Author:  Philip Mucci
*          mucci@cs.utk.edu
* Mods:    Joe Thomas
*          jthomas@cs.utk.edu
* Mods:    <your name here>
*          <your email address>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/signal.h>
#include "mpi.h"
#include "llcbench.h"

/* DEFAULT SETTINGS */

#define LOG_MAX_MESSAGE_SIZE 26 
#define LOG_MIN_MESSAGE_SIZE 2 
#define MAX_MESSAGE_SIZE (1 << LOG_MAX_MESSAGE_SIZE)
#define NUM_SIZES (RESOLUTION * (LOG_MAX_MESSAGE_SIZE - LOG_MIN_MESSAGE_SIZE) +1)
#define RESOLUTION 1      
#define REPEAT_COUNT 1
#define ITERATIONS 1

/* Definitions */ 
#define FLUSH_BETWEEN_SIZES 1<<0
#define FLUSH_BETWEEN_REPEATS 1<<1
#define FLUSH_BETWEEN_ITERATIONS 1<<2
#define TEST_EMPTY      0
#define TEST_LATENCY	1<<0
#define TEST_ROUNDTRIP	1<<1
#define TEST_BANDWIDTH	1<<2
#define TEST_ALLTOALL	1<<3
#define TEST_BROADCAST	1<<4
#define TEST_REDUCE	1<<5
#define TEST_ALLREDUCE  1<<6
#define TEST_COLLECTIVE 1<<7
#define TEST_BIBANDWIDTH 1<<8

#define BROADCASTRESPONSE_TAG    3

/* Globals */

extern char *optarg;

int logmemsize = LOG_MAX_MESSAGE_SIZE, memsize = MAX_MESSAGE_SIZE, repeat_count = REPEAT_COUNT;
int resolution = RESOLUTION, num_sizes = NUM_SIZES;
int *sizes;
char *sendbuf = NULL;
char *destbuf = NULL;
int type = TEST_EMPTY;
int flush = 0;
int max_rank = -1;
int my_rank = -1;
int source_rank = -1;
int dest_rank = -1;

/*************************/
/*  Macros               */
/*************************/

INLINE int mp_broadcast(void *data,int bytes)
{
  return(MPI_Bcast(data,bytes,MPI_BYTE,0,MPI_COMM_WORLD));
}
INLINE int mp_reduce(void *data,int bytes)
{
  return(MPI_Reduce(data, destbuf, (bytes/(int)sizeof(int)), MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD));
}
INLINE int mp_allreduce(void *data,int bytes)
{
  return(MPI_Allreduce(data, destbuf, (bytes/(int)sizeof(int)), MPI_INT, MPI_SUM, MPI_COMM_WORLD));
}
INLINE int mp_alltoall(void *sendbuffer, void *receivebuffer, int bytes)
{
  return(MPI_Alltoall(sendbuffer, (bytes/max_rank), MPI_BYTE, receivebuffer, (bytes/max_rank), MPI_BYTE, MPI_COMM_WORLD));
}
INLINE int mp_send(int dest, int tag, void *data, int bytes)
{
  return(MPI_Send(data, bytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD));
}
INLINE int mp_recv(int dest,int tag, void *data,int bytes)
{
  MPI_Status stat; 
  return(MPI_Recv(data, bytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD, &stat));
}
INLINE int mp_any_recv(int tag,void *data, int bytes)
{
  return(mp_recv(MPI_ANY_SOURCE, tag, data, bytes));
}
INLINE int mp_isend(int dest, int tag, void *data, int bytes, MPI_Request *request)
{
  return(MPI_Isend(data, bytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD, request));
}
INLINE int mp_irecv(int dest, int tag, void *data, int bytes, MPI_Request *request)
{
  return(MPI_Irecv(data, bytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD, request));
}
INLINE int mp_exit(void)
{
  return(MPI_Finalize());
}
INLINE int mp_barrier(void)
{
  return(MPI_Barrier(MPI_COMM_WORLD));
}
#if 0
INLINE int mp_wait(void)
{
}
#endif

/*************************/
/*  Utility functions    */
/*************************/

int am_i_the_master(void)
{
  return(my_rank == 0);
}

int am_i_the_slave(void)
{
  return(my_rank == max_rank-1);
}

void my_two_printf(char *str,unsigned int a, float b)
{
  if (am_i_the_master())
    printf(str,a,b);
}


void initialize_sizes(int *fixed_sizes, int num_fixed_sizes)
{
  int i,j;

  if (num_fixed_sizes > 0)
      {
	  num_sizes = num_fixed_sizes;
	  sizes = fixed_sizes;
	  return;
      }

  assert(sizes = (int *)malloc(num_sizes*sizeof(int)));
  memset(sizes,0x00,(num_sizes*sizeof(int)));
  
  for (j=0; j < num_sizes; j+=resolution)
    {
      sizes[j] = 1 << (LOG_MIN_MESSAGE_SIZE + j/resolution);
      DBG(printf("POW: %d %d\n",j,sizes[j]));
      for (i=1; i<resolution; i++)
	{
	  if (j+i < num_sizes)
	    {
	      sizes[j+i] = sizes[j] + i*(sizes[j]/resolution);
	      sizes[j+i] = sizes[j+i] - sizes[j+i]%(int)sizeof(int);
	      DBG(printf("SUB: %d %d\n",j+i,sizes[j+i]));
	    }
	}
    }
}

/*************************/
/*  Benchmarks           */
/*************************/

double calibrate_cache_flush(int cnt)
{
    double tmp = 1.0;
    int i;
    TIMER_START;
    for (i=0;i<cnt;i++)
	{
	    if (flush & FLUSH_BETWEEN_ITERATIONS)
		flushall(1);	
	}
    TIMER_STOP;
    tmp = TIMER_ELAPSED;
    return tmp;
}

/* ALso known as the gap time by Berkeley, 
   the time to launch a message in the network's buffers. */

double latency(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  if (am_i_the_master())
    {
      TIMER_START;
      for (i=0; i<cnt; i++)
      {
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_send(dest_rank, 1, sendbuf, bytes);
      }
      TIMER_STOP;
      mp_recv(dest_rank, 2, destbuf, 4);
      total = TIMER_ELAPSED;
      total -= calibrate_cache_flush(cnt);
      return(total/(double)cnt);   
    }
  else if (am_i_the_slave())
    {
      for (i=0; i<cnt; i++)
      {
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_recv(source_rank, 1, destbuf, bytes);
      }
      mp_send(source_rank, 2, sendbuf, 4); 
      return(0.0);
    }
  else
    return(0.0);
}

/* This might be your more familiar definition of latency...*/

double roundtrip(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  if (am_i_the_master())
    {
      TIMER_START;
      for (i=0; i<cnt; i++)
	{
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_send(dest_rank, 1, sendbuf, bytes);
	  mp_recv(dest_rank, 2, destbuf, bytes);
	}
      TIMER_STOP;
      total = TIMER_ELAPSED;
      total -= calibrate_cache_flush(cnt);
      return((double)cnt / (total*1.0E-6)); /* Transactions/sec */
    }
  else if (am_i_the_slave())
    {
      for (i=0; i<cnt; i++)
	{
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_recv(source_rank, 1, destbuf, bytes);
	  mp_send(source_rank, 2, sendbuf, bytes);
	}
      return(0.0);
    }
  else
    return(0.0);
}

double bandwidth(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  if (am_i_the_master())
    {
      TIMER_START;
      for (i=0; i<cnt; i++)
	  {
	      if (flush & FLUSH_BETWEEN_ITERATIONS)
		  flushall(1);
	      mp_send(dest_rank, 1, sendbuf, bytes);
	  }
      mp_recv(dest_rank, 2, destbuf, 4);
      TIMER_STOP;
      total = TIMER_ELAPSED;
      total -= calibrate_cache_flush(cnt);
      return(((double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
    }
  else if (am_i_the_slave())
    {
      for (i=0; i<cnt; i++)
	  {
	      if (flush & FLUSH_BETWEEN_ITERATIONS)
		  flushall(1);
	      mp_recv(source_rank, 1, destbuf, bytes);
	  }
      mp_send(source_rank, 2, sendbuf, 4);
      return(0.0);
    }
  else
    return(0.0);
}

/*  New bidirectional bandwidth test */

double bibandwidth(int cnt, int bytes)
{
  int i;
  double total = 0.0;
  MPI_Request requestarray[2];
  MPI_Status  statusarray[2];

  if (am_i_the_master()) 
    {
      TIMER_START;
      for (i=0; i<cnt; i++)
	{
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_irecv(dest_rank, 2, destbuf, bytes, &requestarray[1]);
	  mp_isend(dest_rank, 1, sendbuf, bytes, &requestarray[0]);
	  MPI_Waitall(2, requestarray, statusarray);
	} 
      TIMER_STOP;
      total = TIMER_ELAPSED;
      total -= calibrate_cache_flush(cnt);
      return((2.0*(double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
    }
  else if (am_i_the_slave())
    {
      /* This is the last process */
      for (i=0; i<cnt; i++)
	{
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	  mp_irecv(source_rank, 1, destbuf, bytes, &requestarray[0]);
	  mp_isend(source_rank, 2, sendbuf, bytes, &requestarray[1]);
	  MPI_Waitall(2, requestarray, statusarray);
	}
      return(0.0);
    }
  else
    return(0.0);
}

double alltoall(int cnt, int bytes)
{
  int i;
  double total = 0.0;
  
  TIMER_START;
  for (i=0;i < cnt;i++)
    {
	if (flush & FLUSH_BETWEEN_ITERATIONS)
	    flushall(1);
      mp_alltoall(sendbuf, destbuf, bytes);
    }
  TIMER_STOP;
  total = TIMER_ELAPSED;
  total -= calibrate_cache_flush(cnt);
  return(((double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
}

double reduce(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  TIMER_START;
  for (i=0; i<cnt; i++)
    {
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
      mp_reduce(sendbuf, bytes);
    }
  TIMER_STOP;
  total = TIMER_ELAPSED;
  total -= calibrate_cache_flush(cnt);
  return(((double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
}

double allreduce(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  TIMER_START;
  for (i=0; i<cnt; i++)
    {
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
      mp_allreduce(sendbuf, bytes);
    }
  TIMER_STOP;
  total = TIMER_ELAPSED;
  total -= calibrate_cache_flush(cnt);
  return(((double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
}

double broadcast(int cnt, int bytes)
{
  int i;
  double total = 0.0;

  if (am_i_the_master())
    {
      TIMER_START;
      for (i=0; i<cnt; i++)
	  {
	      if (flush & FLUSH_BETWEEN_ITERATIONS)
		  flushall(1);
	      mp_broadcast(sendbuf, bytes);
	  }
      for (i=1; i<max_rank; i++)
	mp_any_recv(BROADCASTRESPONSE_TAG, destbuf, 4);
      TIMER_STOP;
      total = TIMER_ELAPSED;
      total -= calibrate_cache_flush(cnt);
      return(((double)cnt*(double)bytes)/(total*1.0E-6*1024.0)); /* KB/sec */
    }
  else
    {  
      for (i=0; i<cnt; i++)
	  {
	  if (flush & FLUSH_BETWEEN_ITERATIONS)
	      flushall(1);
	      mp_broadcast(destbuf, bytes);
	  }
      mp_send(source_rank, BROADCASTRESPONSE_TAG, destbuf, 4);
      return(0.0);
    }
}

#ifdef PAPI
#include "papi.h"
#define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }
int EventSet = PAPI_NULL;
long long valuea[3] = { 0, 0, 0 };
long long value[3] = { 0, 0, 0 };
void papi_init(void)
{
    int a, retval;
    
    if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT )
	ERROR_RETURN(retval);
    
    if ( (retval = PAPI_create_eventset(&EventSet)) != PAPI_OK)
	ERROR_RETURN(retval);
    
    if ( (retval = PAPI_add_event(EventSet, PAPI_TOT_INS)) != PAPI_OK)
	ERROR_RETURN(retval);
    
    if ( (retval = PAPI_add_event(EventSet, PAPI_L1_ICM)) != PAPI_OK)
       ERROR_RETURN(retval);

    if ( (retval = PAPI_add_event(EventSet, PAPI_L1_ICA)) != PAPI_OK)
	ERROR_RETURN(retval);

    if ( (retval = PAPI_start(EventSet)) != PAPI_OK)
	ERROR_RETURN(retval);
}

void papi_start()
{
    int retval;

    if ( (retval = PAPI_read(EventSet, valuea)) != PAPI_OK)
	ERROR_RETURN(retval);
}

void papi_stop(int p)
{
    int retval;

    if ( (retval = PAPI_read(EventSet, value)) != PAPI_OK)
	ERROR_RETURN(retval);

    value[0] = value[0] - valuea[0];  
    value[1] = value[1] - valuea[1];  
    value[2] = value[2] - valuea[2];  

    if (p)
	{
	    fprintf(stderr,"RANK %d PAPI_TOT_INS %lld PAPI_TOT_ICM %lld PAPI_L1_ICA %lld ICHR %f ICMpI %f\n",my_rank,value[0],value[1],value[2],
	    (double)1.0-(double)value[1]/(double)value[2],
	    (double)value[1]/(double)value[0]);
	}
}

void papi_shutdown()
{
    int retval;
    if ( (retval = PAPI_stop(EventSet, value)) != PAPI_OK)
	ERROR_RETURN(retval);
   PAPI_destroy_eventset(&EventSet);  
   PAPI_shutdown();
}
#endif

void loop(int iterations, double (*test)(int, int))
{
  int i, j;

#ifdef PAPI
  papi_init();
  papi_start();
  papi_stop(0);
#endif

  /* Allow routing/cache setup ahead of time */
  test(1,memsize);
  
  for( i=0; i < num_sizes; i++){
      if (flush & FLUSH_BETWEEN_SIZES)
	  flushall(1);
      for( j=1; j <= repeat_count; j++){
	  if (flush & FLUSH_BETWEEN_REPEATS)
	      flushall(1);
	  mp_barrier();
#ifdef PAPI
	  papi_start();
#endif
	  my_two_printf("%u %f\n",sizes[i],test(iterations, sizes[i]));
#ifdef PAPI
	  papi_stop(1);
#endif
	  mp_barrier();
      }
  }

#ifdef PAPI
  papi_shutdown();
#endif

  flushall(0);
}

int parse_fixed_msg_sizes(char *str, int **dest)
{
    int tmp;
    char tmp_str[PATH_MAX];
    int cnt = 0, *tmp_dest = NULL;
    char *a = str;
    int str_loc = 0, str_len = strlen(str);
    
 again:
    if (sscanf(a,"%d",&tmp) != 1)
	{
	    *dest = tmp_dest;
	    return(cnt);
	}
    if (tmp < 1)
	{
	    *dest = tmp_dest;
	    return(cnt);
	}
    sprintf(tmp_str,"%d",tmp);
    str_loc += strlen(tmp_str);
    a = a + strlen(tmp_str);
    if (str_loc < str_len)
	{
	    if  ((*a == 'b') || (*a == 'B'))
		{
		    a++; str_loc++;
		}		    
	    else if  ((*a == 'k') || (*a == 'K'))
	    {
		tmp *= 1024;
		a++; str_loc++;
	    }
	    else if ((*a == 'm') || (*a == 'M'))
	    {
		tmp *= 1024*1024;
		a++; str_loc++;
	    }
	}
    assert((tmp_dest = (int *)realloc(tmp_dest,(cnt+1)*sizeof(int))) != NULL);
    tmp_dest[cnt] = tmp;
    cnt++;
    if (str_loc < str_len)
	{
	    if (*a != ',')
	    {
		fprintf(stderr,"Bad format for message size.\n");
		return(0);
	    }
	    else
	    {
		a++; 
		str_loc++;
		if (str_loc >= str_len)
		    {
			*dest = tmp_dest;
			return(cnt);
		    }
	    }
	goto again;
	}
    else
    {
	*dest = tmp_dest;
	return(cnt);
    }
}

/*******************************/
/* new --- added by JT 7/98 */
/*******************************/

void usage(int argc, char **argv, int *iterations, int **fixed_sizes, int *num_fixed_sizes)
{
  int c;
  int errflg = 0;
  int i;  /* counter */

  if (MPI_Init(&argc, &argv) != 0) {
      fprintf(stderr,"%s error: MPI_Init failed.\n",argv);
      MPI_Finalize();
      exit(1);
  }

   MPI_Comm_size(MPI_COMM_WORLD, &max_rank);
   if (max_rank < 2){
      fprintf(stderr,"%s error: Must have at least 2 tasks.\n");
      MPI_Finalize();
      exit(1);
   }

   MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
   source_rank = 0;
   dest_rank = max_rank-1;

   DBG(
     for (i=0; i < argc; i++)
       printf("argc: %d   rank: %d    argv[%d]: %s\n", argc, rank, i, argv[i]);
   )
 
  while ((c = getopt(argc, argv, "i:m:x:e:bdlracyzhfFXM:")) != -1)
    switch (c) {   
	case 'M':
	    if ((*num_fixed_sizes = parse_fixed_msg_sizes(optarg,fixed_sizes)) <= 0)
		errflg++;
	    break;
    case 'i':
      if ((*iterations = atoi(optarg)) < 0)
        errflg++;
      break;
    case 'm':
      if ((logmemsize = atoi(optarg)) < 0)
	errflg++;
      memsize = 1<<logmemsize;
      break;
    case 'x':
      if ((resolution = atoi(optarg)) < 0)
	errflg++;
      resolution++; /* Correct for my usage, since zero measurements between powers of 2 is */
      break;        /* a resolution of 1 */
    case 'e':
      if ((repeat_count = atoi(optarg)) <= 0)
	errflg++;
      break;
    case 'b':
      type = TEST_BANDWIDTH;
      break;
    case 'd':
      type = TEST_BIBANDWIDTH;
      break;
    case 'l':
      type = TEST_LATENCY;
      break;
    case 'r':
      type = TEST_ROUNDTRIP;
      break;
    case 'a':
      type = TEST_ALLTOALL;
      break;
      /* there aren't easy to remember */
    case 'c':
      type = TEST_BROADCAST;
      break;
    case 'y':
      type = TEST_REDUCE;
      break;
    case 'z':
      type = TEST_ALLREDUCE;
      break;
    case 'f':
      flush |= FLUSH_BETWEEN_SIZES;
      break;
    case 'F':
      flush |= FLUSH_BETWEEN_REPEATS;
      break;
    case 'X':
      flush |= FLUSH_BETWEEN_ITERATIONS;
      break;
    case 'h': /* prints usage */
      errflg++;
      break;
    case '?': /* error condition */
      errflg++;
      break; }
  
  if (type == TEST_EMPTY)
    errflg++;

  if (*iterations < 1)
    errflg++;

  if (errflg) {
    if(am_i_the_master()){
      fprintf(stderr, "Usage: %s -blracyzfFX [-i #] [-x #] [-m #] [-d #] [-e #] [-M #bBkKmM,...]\n",argv[0]);
      fprintf(stderr, "\t -b Do bandwidth benchmark\n");
      fprintf(stderr, "\t -d Do bidirectional bandwidth benchmark\n");
      fprintf(stderr, "\t -l Do latency benchmark\n");
      fprintf(stderr, "\t -r Do roundtrip benchmark\n"); 
      fprintf(stderr, "\t -a Do all-to-all benchmark\n"); 
      fprintf(stderr, "\t -c Do broadcast benchmark\n"); 
      fprintf(stderr, "\t -y Do reduce benchmark\n"); 
      fprintf(stderr, "\t -z Do allreduce benchmark\n"); 
      fprintf(stderr, "\t -i Specify the iterations over which to average. \n");
      fprintf(stderr, "\t -x Specify the number of measurements between powers of 2.  \n");
      fprintf(stderr, "\t -m Specify the log base 2 of maximum message size.\n");
      fprintf(stderr, "\t -e Specify the repeat count per message size. \n");
      fprintf(stderr, "\t -f Flush the cache between message sizes.\n");
      fprintf(stderr, "\t -F Flush the cache between repeats.\n");
      fprintf(stderr, "\t -X Flush the cache between iterations.\n");
      fprintf(stderr, "\t -M num[bBkKmM],num[bBkKmM]...Fix message sizes in B (default), kB or MB (base 2) to use.\n");
    }  
    mp_exit();
    exit(1);
  }

  num_sizes = resolution * (logmemsize - LOG_MIN_MESSAGE_SIZE) + 1;

  /* Allocate space for send and receive buffers. */
  assert((sendbuf = (char *)malloc(memsize)));
  assert((destbuf = (char *)malloc(memsize)));
  for (i=0; i < memsize; i++){
    sendbuf[i] = destbuf[i] = (char)(i&0xff);
  }
 
  DBG(printf("logmemsize = %d, memsize =  %d, num_sizes =  %d\n",logmemsize,memsize,num_sizes)); 
}

/********************/
/*  main           **/
/********************/

int main(int argc, char **argv)
{
  int iterations = ITERATIONS;
  int *sizes = NULL, num_sizes = 0;

  /* set up the run */

  setbuf(stdout,NULL);

  usage(argc, argv, &iterations, &sizes, &num_sizes);
  initialize_sizes(sizes, num_sizes);

  /* run appropriate test */
  if (iterations) {
      switch (type)
	{
	case TEST_LATENCY:
	  loop(iterations,latency);
	  break;
	case TEST_ROUNDTRIP:
	  loop(iterations,roundtrip);
	  break;
	case TEST_BANDWIDTH:
	  loop(iterations,bandwidth);
	  break;
	case TEST_BROADCAST:
	  loop(iterations, broadcast);
	  break;
	case TEST_REDUCE:
	  loop(iterations, reduce);
	  break;
	case TEST_ALLREDUCE:
	  loop(iterations, allreduce);
	  break;
	case TEST_ALLTOALL:
	  loop(iterations, alltoall);
	  break;
	case TEST_BIBANDWIDTH:
	  loop(iterations, bibandwidth);
	  break;
	default:
	  abort();
	}
  }
  free(sendbuf);
  free(destbuf);
  mp_exit();
  exit(0);
}
