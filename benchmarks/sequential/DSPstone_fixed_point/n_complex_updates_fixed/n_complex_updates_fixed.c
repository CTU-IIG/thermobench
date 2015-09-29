/*
 * benchmark program:   n_complex_updates.c
 * 
 * benchmark suite:     DSP-kernel
 * 
 * description:         n complex updates - filter benchmarking
 * 
 * This program performs n complex updates of the form 
 *           D(i) = C(i) + A(i)*B(i),
 * where A(i), B(i), C(i) and D(i) are complex numbers,
 * and i = 1,...,N
 * 
 *          A(i) = Ar(i) + j Ai(i)
 *          B(i) = Br(i) + j Bi(i)
 *          C(i) = Cr(i) + j Ci(i)
 *          D(i) = C(i) + A(i)*B(i) =   Dr(i)  +  j Di(i)
 *                      =>  Dr(i) = Cr(i) + Ar(i)*Br(i) - Ai(i)*Bi(i)
 *                      =>  Di(i) = Ci(i) + Ar(i)*Bi(i) + Ai(i)*Br(i)
 * 
 * reference code:      target assembly
 * 
 * f. verification:     simulator based
 * 
 *  organization:        Aachen University of Technology - IS2 
 *                       DSP Tools Group
 *                       phone:  +49(241)807887 
 *                       fax:    +49(241)8888195
 *                       e-mail: zivojnov@ert.rwth-aachen.de 
 *
 * author:              Juan Martinez Velarde
 * 
 * history:             13-5-94 creation (Martinez Velarde)
 *
 *                      $Author: lokuciej $
 *                      $Revision: 1.1 $
 *                      $Date: 2009-07-20 21:31:45 $
 */

#define STORAGE_CLASS register
#define TYPE          int
#define N             16

void
pin_down(TYPE *pa, TYPE *pb, TYPE *pc, TYPE *pd)
{
  STORAGE_CLASS int i ; 

  _Pragma("loopbound min 16 max 16")
  for (i=0 ; i < N ; i++)
    {
      *pa++ = 2 ;
      *pa++ = 1 ;
      *pb++ = 2 ; 
      *pb++ = 5 ; 
      *pc++ = 3 ;
      *pc++ = 4 ;  
      *pd++ = 0 ; 
      *pd++ = 0 ; 
    }
}


int main()
{
  static TYPE A[2*N], B[2*N], C[2*N], D[2*N] ; 

  STORAGE_CLASS TYPE *p_a = &A[0], *p_b = &B[0] ;
  STORAGE_CLASS TYPE *p_c = &C[0], *p_d = &D[0] ;
  STORAGE_CLASS TYPE i ; 

  pin_down(&A[0], &B[0], &C[0], &D[0]) ;

  _Pragma("loopbound min 16 max 16")
  for (i = 0 ; i < N ; i++, p_a++) 
    {
      *p_d    = *p_c++ + *p_a++ * *p_b++ ;
      *p_d++ -=          *p_a   * *p_b-- ; 
      
      *p_d    = *p_c++ + *p_a-- * *p_b++ ; 
      *p_d++ +=          *p_a++ * *p_b++ ;
    }

  pin_down(&A[0], &B[0], &C[0], &D[0]) ; 

  return 0;
}



