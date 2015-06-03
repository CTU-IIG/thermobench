/*
 * benchmark program:   complex_update.c
 * 
 * benchmark suite:     DSP-kernel
 * 
 * description:         complex_update - filter benchmarking
 * 
 * This program performs a complex update of the form D = C + A*B,
 * where A, B, C and D are complex numbers .
 * 
 *          A = Ar + j Ai
 *          B = Br + j Bi
 *          C = Cr + j Ci
 *          D = C + A*B =   Dr  +  j Di
 *                      =>  Dr = Cr + Ar*Br - Ai*Bi
 *                      =>  Di = Ci + Ar*Bi + Ai*Br
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
 * history:             11-5-94 creation (Martinez Velarde)
 *
 *                      $Author: lokuciej $
 *                      $Revision: 1.1 $
 *                      $Date: 2009-07-20 21:31:16 $
 */

#define STORAGE_CLASS register
#define TYPE  int

void
pin_down(TYPE *p)
{
  *p++ = 0;
  *p   = 0;
}


int main()
{
  static TYPE A[2] = { 2,1 };
  static TYPE B[2] = { 2,5 };
  static TYPE C[2] = { 3,4 };
  static TYPE D[2] = { 0,0 };

  STORAGE_CLASS TYPE *p_a = &A[0];
  STORAGE_CLASS TYPE *p_b = &B[0];
  STORAGE_CLASS TYPE *p_c = &C[0];
  STORAGE_CLASS TYPE *p_d = &D[0];

  pin_down(&D[0]);

  *p_d    = *p_c++ + *p_a++ * *p_b++;
  *p_d++ -=          *p_a   * *p_b--;

  *p_d    = *p_c   + *p_a-- * *p_b++;
  *p_d   +=          *p_a   * *p_b;

  pin_down(&D[0]);

  return 0;
}



