/*
 * benchmark program:   startup.c
 * 
 * benchmark suite:     control
 * 
 * description:         V32-call-mode-modem startup routine (begin)
 * 
 * reference code:      target assembly
 * 
 * f. verification:     simulator
 * 
 *  organization:        Aachen University of Technology - IS2 
 *                       DSP Tools Group
 *                       phone:  +49(241)807887 
 *                       fax:    +49(241)8888195
 *                       e-mail: zivojnov@ert.rwth-aachen.de 
 *
 * author:              Markus Willems
 * 
 * history:             12-04-95 creation (Markus Willems)
 *                      12-06-95 adaption to gnu-compiler-requirements 
 *                                (Jani Ollikainen) 
 *                      25-08-95 START and END PROFILING Macros included
 *                               pin_down inserted (M.Willems)
 *                      28-08-95 stdio removed (M.Willems)
 *                      30-08-95 pre-Increment replaced by post-increment 
 *                                (M.Willems)
 *
 *                      $Author: lokuciej $
 *                      $Revision: 1.1 $
 *                      $Date: 2009-07-20 21:31:52 $
 */

/* Input sequence for testing the V32-call mode modem startup sequence. 
   It is ordered as follows:
   8 AC, 64 CA, 8 AC, 16 free, 2 R1, 16 free, 2 R3, 2E */

#define A 0
#define B 1
#define C 3
#define D 2

  int In[240] =
    {
      A, C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, 
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A, 
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, A,
      A, C, A, C, A, C, A, C, A, C, A, C, A, C, A, C, 
      D, C, A, B, C, D, A, D, A, D, D, C, C, C, A, B,
      0, 0, 1, 1, 2, 1, 0, 1, 0, 0, 1, 1, 2, 1, 0, 1, 
      D, C, A, B, C, D, A, D, A, D, D, C, C, C, A, B,
      0, 0, 1, 1, 2, 1, 0, 1, 0, 0, 1, 1, 2, 1, 0, 1, 
      3, 3, 1, 1, 2, 1, 0, 1, 3, 3, 1, 1, 2, 1, 0, 1
      };

void pin_down(int i)
{
  i = 0;
}

int main(void)
{
  int i, x, y, Out[240];

  int N, tmp;
  int temp1, temp2;
  int *Input, *Output;

  Input = In;
  Output = Out;

  pin_down(i);

  /*START_PROFILING;*/
  
  /* here the ANS check has to be performed. Because of complexity reasons 
    we skip this an assume to be in the startup mode already */
  
  /* now that the ANS sequence has been detected start Outputmitting signal A
    as long as no signal combination CC is detected. CC is detected when 
    two successive received samples are equal for the first time */

  temp1 = *Input++;
  temp2 = *Input;

  _Pragma("loopbound min 8 max 8")
  while (temp1 != temp2) 
    {
      *Output++ = A;
      Input++;
      temp1 = *(Input);
      
      if (temp1 != temp2)
        { 
          *Output++ = A;
          Input++;
          temp2 = *(Input);
        }
    }
  
  /*now CC has been detected, so continue sending A 64 times */

  _Pragma("loopbound min 64 max 64")
  for (i=0; i<64; i++)
    *Output++ = A;

  /* while sending A 64 times ,new input samples have been detected which 
    do not influence the transmitted sequence */
  
  Input = Input + 64; 
  
  /* send C as long as no AA is detected (see above)
    it has to be stored by N how many samples C are transmitted. */
  N = 0;              
  temp1 = *Input++;
  temp2 = *Input;

  _Pragma("loopbound min 32 max 32")
  while (temp1 != temp2)
    {
      *Output++ = C;
      Input++;
      temp1 = *(Input);
      N++;

      if (temp1 != temp2)
        {
          *Output++ = C;
          Input++;
          temp2 = *(Input);
          N++;
        }
    }

  /* Detection of the received sequence R1 starts here.
    Sequence R1 is detected when two 16-bit sequences match exactly.
    We assume that the sequence is already descrambled and decoded so that 
    it is only necessary to compare the last 8 symbols */
  
  i = 0;

  _Pragma("loopbound min 4 max 4")
  do {
    /* wcc note: this assignment expression was added to avoid assignment of
     * multiple loop bound annotations to same loop. */
    y=0;

    /* loop has to be executed at least once */  
    _Pragma("loopbound min 1 max 30")
    while (i<8)
      {
        Input++;
        (*(Input-8) == *(Input)) ? i++ : (i=0);
      }

    /* now we have detected two sequences of length 8. If these do not 
      match the R1 requirements, it becomes only necessary to compare
      one additional symbol */
    
    i=7;
    
    tmp = ((*(Input-7) == 0) && 
     (*(Input-6) == 0) && 
     ((*(Input-4) & 1) == 1)  &&
     ((*(Input-2) & 1) == 1)  &&
     ((*(Input) & 1) == 1));

  } while (tmp == 0);           
  /* R1 includes the information concerning the possible data rates and 
    encoding schemes. These have to be identified */
  
  /* now that R1 has been detected, sequence S (AB) is transmitted N/2 times.
    N is the time that has been achieved above. */
  
  N = (N/2);  /*  We can save a few program steps if we don't have to 
    count N/2 on every cycle of the for loop.  */
  /* N was replaced by "31" due to ILP problems. */
  _Pragma("loopbound min 32 max 32")
  for (i=0; i <= 31; i++) /* < replaced with <= because N is an int (value 31) */
    {
      *Output++ = A;
      *Output++ = B;
    }

  /*END_PROFILING;*/

  pin_down(i);

  return(0);
}



