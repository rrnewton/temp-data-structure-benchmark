/*-----------------------------------------------------------------------*/
/* Program: Stream                                                       */
/* Revision: $Id: stream.c,v 5.9 2009/04/11 16:35:00 mccalpin Exp $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2005: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/
# include <stdio.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>


#ifndef BENCH_N
#   define BENCH_N	2000000
#endif
#ifndef NTIMES
#   define NTIMES	10
#endif
#ifndef OFFSET
#   define OFFSET	0
#endif

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

static double avgtime = 0, maxtime = 0, mintime = FLT_MAX;

static double bytes = 2 * sizeof(double) * BENCH_N;

extern double mysecond();
extern void init(int nthreads);
extern void kernel();
extern void reset();
extern void finalize();

int main() {
  int quantum, checktick();
  int BytesPerWord;
  register int j, k;
  double scalar, t, times[NTIMES];

  /* Initialize benchmark. */
  init(2);

  /*	--- MAIN LOOP --- repeat kernel NTIMES times --- */
  for (k = 0; k < NTIMES; k++) {
    reset();

    times[k] = mysecond();
      kernel();
    times[k] = mysecond() - times[k];
  }

  /*	--- SUMMARY --- */
  for (k = 1; k < NTIMES; k++) /* note -- skip first iteration */
  {
    avgtime = avgtime + times[k];
    mintime = MIN(mintime, times[k]);
    maxtime = MAX(maxtime, times[k]);
  }
  avgtime = avgtime / (double) (NTIMES - 1);

  printf(HLINE);
  printf("Rate (MB/s)   Avg time     Min time     Max time\n");
  printf("%11.4f  %11.4f  %11.4f  %11.4f\n",
      1.0E-06 * bytes / mintime, avgtime, mintime, maxtime);
  printf(HLINE);

  finalize();

  return 0;
}

# define	M	20

int checktick() {
  int i, minDelta, Delta;
  double t1, t2, timesfound[M];

  /*  Collect a sequence of M unique time values from the system. */

  for (i = 0; i < M; i++) {
    t1 = mysecond();
    while (((t2 = mysecond()) - t1) < 1.0E-6)
      ;
    timesfound[i] = t1 = t2;
  }

  /*
   * Determine the minimum difference between these M values.
   * This result will be our estimate (in microseconds) for the
   * clock granularity.
   */

  minDelta = 1000000;
  for (i = 1; i < M; i++) {
    Delta = (int) (1.0E6 * (timesfound[i] - timesfound[i - 1]));
    minDelta = MIN(minDelta, MAX(Delta,0));
  }

  return (minDelta);
}

/* A gettimeofday routine to give access to the wall
 clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond() {
  struct timeval tp;
  struct timezone tzp;
  int i;

  i = gettimeofday(&tp, &tzp);
  return ((double) tp.tv_sec + (double) tp.tv_usec * 1.e-6);
}
