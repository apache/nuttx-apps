/****************************************************************************
 * apps/examples/calib_udelay/calib_udelay_main.c
 *
 *   Copyright (C) 2017 Haltian Ltd All rights reserved.
 *   Author: Jussi Kivilinna <jussi.kivilinna@haltian.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_CALIB_UDELAY_NUM_MEASUREMENTS
# define CONFIG_EXAMPLES_CALIB_UDELAY_NUM_MEASUREMENTS 3
#endif

#ifndef CONFIG_EXAMPLES_CALIB_UDELAY_NUM_RESULTS
# define CONFIG_EXAMPLES_CALIB_UDELAY_NUM_RESULTS 20
#endif

#define DELAY_TEST_ITERS 100000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct measurement_s
{
  int count;
  uint64_t nsecs;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint64_t gettime_nsecs(void)
{
  struct timespec ts;
  uint64_t nsecs;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  nsecs  = ts.tv_sec;
  nsecs *= 1000 * 1000 * 1000;
  nsecs += ts.tv_nsec;

  return nsecs;
}

static int compare_measurements(const void *va, const void *vb)
{
  const struct measurement_s *a = va;
  const struct measurement_s *b = vb;

  if (a->nsecs == b->nsecs)
    {
      return 0;
    }
  else if (a->nsecs < b->nsecs)
    {
      return -1;
    }
  else
    {
      return 1;
    }
}

static void __attribute__((noinline)) calib_udelay_test(uint32_t count)
{
  volatile int i;

  while (count > 0)
    {
      for (i = 0; i < DELAY_TEST_ITERS; i++)
        {
        }

      count--;
    }
}

static void perform_measurements(int loop_count,
                                 FAR struct measurement_s *measurements,
                                 int num_measurements)
{
  int n;

  memset(measurements, 0, sizeof(*measurements) * num_measurements);

  for (n = 0; n < num_measurements; n++)
    {
      measurements[n].nsecs = gettime_nsecs();
      calib_udelay_test(loop_count);
      measurements[n].nsecs = gettime_nsecs() - measurements[n].nsecs;
      measurements[n].count = loop_count;
    }

  qsort(measurements, num_measurements, sizeof(measurements[0]),
        compare_measurements);
}

static double getx(FAR struct measurement_s *point)
{
  return point->count * (double)DELAY_TEST_ITERS;
}

static double gety(struct measurement_s *point)
{
  return point->nsecs;
}

static int linreg(FAR struct measurement_s *point, int num_points,
                  FAR double *m, FAR double *b, FAR double *r2)
{
  double sumx  = 0.0;
  double sumx2 = 0.0;
  double sumxy = 0.0;
  double sumy  = 0.0;
  double sumy2 = 0.0;
  double x;
  double y;
  double denom;
  double inv_denom;
  int i;

  for (i = 0; i < num_points; i++)
    {
      x      = getx(point + i);
      y      = gety(point + i);
      sumx  += x;
      sumx2 += x * x;
      sumxy += x * y;
      sumy  += y;
      sumy2 += y * y;
    }

  denom = num_points * sumx2 - sumx * sumx;
  if (denom == 0)
    {
      *m = 0;
      *b = 0;

      if (r2)
        {
          *r2 = 0;
        }

      return ERROR;
    }

  inv_denom = 1.0 / denom;

  *m = ((num_points * sumxy) - (sumx * sumy)) * inv_denom;
  *b = ((sumy * sumx2) - (sumx * sumxy)) * inv_denom;

  if (r2)
    {
      double term1  = ((num_points * sumxy) - (sumx * sumy));
      double term2  = ((num_points * sumx2) - (sumx * sumx));
      double term3  = ((num_points * sumy2) - (sumy * sumy));
      double term23 = (term2 * term3);

      *r2 = 1.0;
      if (fabs(term23) > 1e-10)
        {
          *r2 = (term1 * term1) / term23;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * calib_udelay_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const int num_measurements = CONFIG_EXAMPLES_CALIB_UDELAY_NUM_MEASUREMENTS;
  const int num_results = CONFIG_EXAMPLES_CALIB_UDELAY_NUM_RESULTS;
  const int min_timer_resolution_steps = 3;
  const int calibration_step_multiplier = 10;
  struct measurement_s measurements[num_measurements];
  struct measurement_s result;
  struct measurement_s results[num_results];
  double iters_per_nsec;
  double iters_per_msec;
  uint64_t timer_resolution;
  double slope_m, slope_b, slope_r2;
  int min_step;
  int loop_count;
  double duration;
  int i;

  printf("\n");

  sched_lock();

  printf("Calibrating timer for main calibration...\n");
  usleep(200 * 1000);

  /* Find out timer resolution. */

  loop_count = 0;
  do
    {
      loop_count++;
      perform_measurements(loop_count, measurements, num_measurements);
      result = measurements[0];
    }
  while (result.nsecs == 0);

  timer_resolution = result.nsecs;

  /* Find first loop count where timer steps five times. */

  do
    {
      loop_count++;
      perform_measurements(loop_count, measurements, num_measurements);
      result = measurements[0];
    }
  while (result.nsecs < timer_resolution * min_timer_resolution_steps);

  min_step = result.count;

  /* Get calibration slope for loop function. */

  for (duration = 0, loop_count = min_step, i = 0; i < num_results; i++,
       loop_count += min_step * calibration_step_multiplier)
    {
      duration += (double)num_measurements * loop_count *
                          timer_resolution * min_timer_resolution_steps / min_step;
    }

  printf("Performing main calibration for udelay. This will take approx. %.3f seconds.\n",
         duration * 1e-9);
  usleep(200 * 1000);

  for (loop_count = min_step, i = 0; i < num_results; i++,
       loop_count += min_step * calibration_step_multiplier)
    {
      perform_measurements(loop_count, measurements, num_measurements);
      results[i] = measurements[0];
    }

  if (linreg(results, num_results, &slope_m, &slope_b, &slope_r2) == OK)
    {
      printf("Calibration slope for udelay:\n"
             "  Y = m*X + b, where\n"
             "    X is loop iterations,\n"
             "    Y is time in nanoseconds,\n"
             "    b is base overhead,\n"
             "    m is nanoseconds per loop iteration.\n\n"

             "  m = %.08f nsec/iter\n"
             "  b = %.08f nsec\n\n"

             "  Correlation coefficient, R² = %.4f\n\n",
             slope_m, slope_b, slope_r2);

      iters_per_nsec = (1.0 / slope_m);
      iters_per_msec = iters_per_nsec * 1000 * 1000;

      printf("Without overhead, %.8f iterations per nanosecond and %.2f "
             "iterations per millisecond.\n\n",
             iters_per_nsec, iters_per_msec);

      printf("Recommended setting for CONFIG_BOARD_LOOPSPERMSEC:\n"
             "   CONFIG_BOARD_LOOPSPERMSEC=%.0f\n", ceil(iters_per_msec));
    }
  else
    {
      printf("cannot solve\n");
    }

  sched_unlock();
  return 0;
}
