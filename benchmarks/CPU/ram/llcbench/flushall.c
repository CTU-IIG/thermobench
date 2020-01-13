#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define MAX_MEM_TO_FLUSH 1<<24

void flushall(int yes)
{
  static char *buffer = NULL;
  static char val = 1;

  if (yes)
    {
      if (buffer == NULL)
	{
	  buffer = (char *)malloc(MAX_MEM_TO_FLUSH*sizeof(char));
	  if (buffer == NULL)
	    {
	      fprintf(stderr,"flushall.c: malloc of %d bytes failed.\n",MAX_MEM_TO_FLUSH);
	      exit(1);
	    }
	}
      memset(buffer,val++,MAX_MEM_TO_FLUSH);
    }
  else
    {
      if (buffer)
	{
	  free(buffer);
	  buffer = NULL;
	}
    }
}

