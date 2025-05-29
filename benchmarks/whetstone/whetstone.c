/****************************************************************************
 * apps/benchmarks/whetstone/whetstone.c
 *
 * SPDX-License-Identifier: LicenseRef-Painter-Engineering-Whetstone
 *
 * Converted Whetstone Double Precision Benchmark
 *    Version 1.2  22 March 1998
 *
 *  (c) Copyright 1998 Painter Engineering, Inc.
 *    All Rights Reserved.
 *
 *    Permission is granted to use, duplicate, and
 *    publish this text and program as long as it
 *    includes this entire comment block and limited
 *    rights reference.
 *
 * Converted by Rich Painter, Painter Engineering, Inc. based on the
 * www.netlib.org benchmark/whetstoned version obtained 16 March 1998.
 *
 * A novel approach was used here to keep the look and feel of the
 * FORTRAN version.  Altering the FORTRAN-based array indices,
 * starting at element 1, to start at element 0 for C, would require
 * numerous changes, including decrementing the variable indices by 1.
 * Instead, the array e1[] was declared 1 element larger in C.  This
 * allows the FORTRAN index range to function without any literal or
 * variable indices changes.  The array element e1[0] is simply never
 * used and does not alter the benchmark results.
 *
 * The major FORTRAN comment blocks were retained to minimize
 * differences between versions.  Modules N5 and N12, like in the
 * FORTRAN version, have been eliminated here.
 *
 * An optional command-line argument has been provided [-c] to
 * offer continuous repetition of the entire benchmark.
 * An optional argument for setting an alternate loop count is also
 * provided.  Define PRINTOUT to cause the pout() function to print
 * outputs at various stages.  Final timing measurements should be
 * made with the PRINTOUT undefined.
 *
 * Questions and comments may be directed to the author at
 *      r.painter@ieee.org
 ****************************************************************************/

/****************************************************************************
 *     Benchmark #2 -- Double  Precision Whetstone (A001)
 *
 *     o  This is a REAL*8 version of
 *  the Whetstone benchmark program.
 *
 *     o  DO-loop semantics are ANSI-66 compatible.
 *
 *     o  Final measurements are to be made with all
 *  WRITE statements and FORMAT sttements removed.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* standard C library headers required */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* the following is optional depending on the timing function used */

#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* map the FORTRAN math functions, etc. to the C versions */

#define DSIN  sin
#define DCOS  cos
#define DATAN  atan
#define DLOG  log
#define DEXP  exp
#define DSQRT  sqrt
#define IF    if
#define USAGE  "usage: whetdc [-c] [loops]\n"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* function prototypes */

void pout(long n, long j, long k, double x1,
          double x2, double x3, double x4);
void pa(double e[]);
void p0(void);
void p3(double x, double y, FAR double *z);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* COMMON t,t1,t2,e1(4),j,k,l */

double t;
double t1;
double t2;
double e1[5];
int j;
int k;
int l;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* used in the FORTRAN version */

  long loop;
  long i;
  long n1;
  long n2;
  long n3;
  long n4;
  long n6;
  long n7;
  long n8;
  long n9;
  long n10;
  long n11;
  double x1;
  double x2;
  double x3;
  double x4;
  double x;
  double y;
  double z;
  int ii;
  int jj;

  /* added for this version */

  long loopstart;
  long startmsec;
  long finimsec;
  float KIPS;
  int continuous;
  struct timespec ts;

  loopstart = 1000;    /* see the note about loop below */
  continuous = 0;

  ii = 1;    /* start at the first arg (temp use of ii here) */
  while (ii < argc)
    {
      if (strncmp(argv[ii], "-c", 2) == 0 || argv[ii][0] == 'c')
        {
          continuous = 1;
        }
      else if (atol(argv[ii]) > 0)
        {
          loopstart = atol(argv[ii]);
        }
      else
        {
          fprintf(stderr, USAGE);
          return 1;
        }

      ii++;
    }

LCONT:

  /* Start benchmark timing at this point. */

  clock_gettime(CLOCK_REALTIME, &ts);
  startmsec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

  /* The actual benchmark starts here. */

  t  = .499975;
  t1 = 0.50025;
  t2 = 2.0;

  /* With loopcount loop=10, one million Whetstone instructions
   * will be executed in EACH MAJOR loop..A MAJOR loop IS EXECUTED
   * 'ii' TIMES TO INCREASE WALL-CLOCK TIMING ACCURACY.
   *
   * loop = 1000;
   */

  loop = loopstart;
  ii   = 1;

  jj = 1;

IILOOP:
  n1  = 0;
  n2  = 12 * loop;
  n3  = 14 * loop;
  n4  = 345 * loop;
  n6  = 210 * loop;
  n7  = 32 * loop;
  n8  = 899 * loop;
  n9  = 616 * loop;
  n10 = 0;
  n11 = 93 * loop;

  /* Module 1: Simple identifiers */

  x1  =  1.0;
  x2  = -1.0;
  x3  = -1.0;
  x4  = -1.0;

  for (i = 1; i <= n1; i++)
    {
      x1 = (x1 + x2 + x3 - x4) * t;
      x2 = (x1 + x2 - x3 + x4) * t;
      x3 = (x1 - x2 + x3 + x4) * t;
      x4 = (-x1 + x2 + x3 + x4) * t;
    }

#ifdef PRINTOUT
  IF (jj  ==  ii)
    {
      pout(n1, n1, n1, x1, x2, x3, x4);
    }
#endif

  /* Module 2: Array elements */

  e1[1] =  1.0;
  e1[2] = -1.0;
  e1[3] = -1.0;
  e1[4] = -1.0;

  for (i = 1; i <= n2; i++)
    {
      e1[1] = (e1[1] + e1[2] + e1[3] - e1[4]) * t;
      e1[2] = (e1[1] + e1[2] - e1[3] + e1[4]) * t;
      e1[3] = (e1[1] - e1[2] + e1[3] + e1[4]) * t;
      e1[4] = (-e1[1] + e1[2] + e1[3] + e1[4]) * t;
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n2, n3, n2, e1[1], e1[2], e1[3], e1[4]);
    }
#endif

  /* Module 3: Array as parameter */

  for (i = 1; i <= n3; i++)
    {
      pa(e1);
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n3, n2, n2, e1[1], e1[2], e1[3], e1[4]);
    }
#endif

  /* Module 4: Conditional jumps */

  j = 1;
  for (i = 1; i <= n4; i++)
    {
      if (j == 1)
        {
          j = 2;
        }
      else
        {
          j = 3;
        }

      if (j > 2)
        {
          j = 0;
        }
      else
        {
          j = 1;
        }

      if (j < 1)
        {
          j = 1;
        }
      else
        {
          j = 0;
        }
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n4, j, j, x1, x2, x3, x4);
    }
#endif

/* Module 5: Omitted
 * Module 6: Integer arithmetic
 */

  j = 1;
  k = 2;
  l = 3;

  for (i = 1; i <= n6; i++)
    {
      j = j * (k - j) * (l - k);
      k = l * k - (l - j) * k;
      l = (l - k) * (k + j);
      e1[l - 1] = j + k + l;
      e1[k - 1] = j * k * l;
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n6, j, k, e1[1], e1[2], e1[3], e1[4]);
    }
#endif

  /* Module 7: Trigonometric functions */

  x = 0.5;
  y = 0.5;

  for (i = 1; i <= n7; i++)
    {
      x = t * DATAN(t2 * DSIN(x) * DCOS(x) /
             (DCOS(x + y) + DCOS(x - y) - 1.0));
      y = t * DATAN(t2 * DSIN(y) * DCOS(y) /
             (DCOS(x + y) + DCOS(x - y) - 1.0));
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n7, j, k, x, x, y, y);
    }
#endif

  /* Module 8: Procedure calls */

  x = 1.0;
  y = 1.0;
  z = 1.0;

  for (i = 1; i <= n8; i++)
    {
      p3(x, y, &z);
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n8, j, k, x, y, z, z);
    }
#endif

  /* Module 9: Array references */

  j = 1;
  k = 2;
  l = 3;
  e1[1] = 1.0;
  e1[2] = 2.0;
  e1[3] = 3.0;

  for (i = 1; i <= n9; i++)
    {
      p0();
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n9, j, k, e1[1], e1[2], e1[3], e1[4]);
    }
#endif

  /* Module 10: Integer arithmetic */

  j = 2;
  k = 3;

  for (i = 1; i <= n10; i++)
    {
      j = j + k;
      k = j + k;
      j = k - j;
      k = k - j - j;
    }

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n10, j, k, x1, x2, x3, x4);
    }
#endif

  /* Module 11: Standard functions */

  x = 0.75;

  for (i = 1; i <= n11; i++)
    x = DSQRT(DEXP(DLOG(x) / t1));

#ifdef PRINTOUT
  IF (jj == ii)
    {
      pout(n11, j, k, x, x, x, x);
    }
#endif

  /* THIS IS THE END OF THE MAJOR loop. */

  if (++jj <= ii)
    {
      goto IILOOP;
    }

  /* Stop benchmark timing at this point. */

  clock_gettime(CLOCK_REALTIME, &ts);
  finimsec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

  /* Performance in Whetstone KIP's per second is given by
   *
   * (100*loop*ii)/TIME
   *
   * where TIME is in seconds.
   */

  printf("\n");
  if (finimsec - startmsec <= 0)
    {
      printf("Insufficient duration- Increase the loop count\n");
      return 1;
    }

  printf("Loops: %ld, Iterations: %d, Duration: %ld millisecond.\n",
                                loop, ii, finimsec - startmsec);

  KIPS = (100.0 * loop * ii) / ((float)(finimsec - startmsec) * 1000);
  if (KIPS >= 1000.0)
    {
      printf("C Converted Double Precision Whetstones: %.1f MIPS\n",
                                                     KIPS / 1000.0);
    }
  else
    {
      printf("C Converted Double Precision Whetstones: %.1f KIPS\n", KIPS);
    }

  if (continuous)
    {
      goto LCONT;
    }

  return 0;
}

void
pa(double e[])
{
  j = 0;

L10:
  e[1] = (e[1] + e[2] + e[3] - e[4]) * t;
  e[2] = (e[1] + e[2] - e[3] + e[4]) * t;
  e[3] = (e[1] - e[2] + e[3] + e[4]) * t;
  e[4] = (-e[1] + e[2] + e[3] + e[4]) / t2;
  j += 1;

  if (j < 6)
    {
      goto L10;
    }
}

void
p0(void)
{
  e1[j] = e1[k];
  e1[k] = e1[l];
  e1[l] = e1[j];
}

void p3(double x, double y, FAR double *z)
{
  double x1;
  double y1;

  x1 = x;
  y1 = y;
  x1 = t * (x1 + y1);
  y1 = t * (x1 + y1);
  *z  = (x1 + y1) / t2;
}

#ifdef PRINTOUT
void
pout(long n, long j, long k, double x1, double x2, double x3, double x4)
{
  printf("%7ld %7ld %7ld %12.4e %12.4e %12.4e %12.4e\n",
            n, j, k, x1, x2, x3, x4);
}
#endif
