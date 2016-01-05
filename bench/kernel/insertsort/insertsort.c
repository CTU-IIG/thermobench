/*

  This program is part of the TACLeBench benchmark suite.
  Version V 1.x

  Name: insertsort

  Author: unknown

  Function: Insertion sort for 10 integer numbers.
     The integer array insertsort_a[] is initialized in main function.
     Input-data dependent nested loop with worst-case of
     (n^2)/2 iterations (triangular loop).

  Source: MRTC
          http://www.mrtc.mdh.se/projects/wcet/wcet_bench/insertsort/insertsort.c

  Changes: a brief summary of major functional changes (not formatting)

  License: general open-source

*/

#include <stdio.h>

/*
  Forward declaration of functions
*/
void insertsort_initialize(unsigned int* array);
void insertsort_init(void);
int insertsort_return(void);
void insertsort_main(void);
int main( void );

/*
  Declaration of global variables
*/
unsigned int insertsort_a[11];
int insertsort_iters_i = 0, insertsort_min_i = 100000, insertsort_max_i = 0;
int insertsort_iters_a = 0, insertsort_min_a = 100000, insertsort_max_a = 0;

/*
  Initialization- and return-value-related functions
*/

void insertsort_initialize(unsigned int* array)
{

    register int i;
    _Pragma( "loopbound min 10 max 10" )
    for ( i = 0; i < 10; i++ )
        insertsort_a[i] = array[i];

}


void insertsort_init()
{
    unsigned int a[11] = {0, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
    insertsort_initialize(a);
}

int insertsort_return()
{
    return 0;
}


/*
  Main functions
*/


void _Pragma( "entrypoint" ) insertsort_main()
{
    int  i,j, temp;
    i = 2;

    insertsort_iters_i = 0;

    _Pragma("loopbound min 9 max 9")
    while(i <= 10) {

        insertsort_iters_i++;

        j = i;

        insertsort_iters_a = 0;

        _Pragma("loopbound min 1 max 9")
        while (insertsort_a[j] < insertsort_a[j-1])
        {
            insertsort_iters_a++;

            temp = insertsort_a[j];
            insertsort_a[j] = insertsort_a[j-1];
            insertsort_a[j-1] = temp;
            j--;
        }

        if ( insertsort_iters_a < insertsort_min_a )
            insertsort_min_a = insertsort_iters_a;
        if ( insertsort_iters_a > insertsort_max_a )
            insertsort_max_a = insertsort_iters_a;

        i++;
    }

    if ( insertsort_iters_i < insertsort_min_i )
        insertsort_min_i = insertsort_iters_i;
    if ( insertsort_iters_i > insertsort_max_i )
        insertsort_max_i = insertsort_iters_i;


    printf( "i-loop: [%d, %d]\n", insertsort_min_i, insertsort_max_i );
    printf( "a-loop: [%d, %d]\n", insertsort_min_a, insertsort_max_a );

}

int main( void )
{
    insertsort_init();
    insertsort_main();
    return (insertsort_return());
}
