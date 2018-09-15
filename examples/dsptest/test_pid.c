/****************************************************************************
 * examples/dsptest/test_pid.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Mateusz Szafoni <raiden00@railab.me>
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

#include "dsptest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_NAME "PID"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Initialize PI controller */

static void test_pi_controller_init(void)
{
  pid_controller_t pi;
  float max = 0.990;
  float min = 0.001;
  float kp  = 2.0;
  float ki  = 0.001;

  /* Initialize PI controller */

  pi_controller_init(&pi, kp, ki);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(kp, pi.KP);
  TEST_ASSERT_EQUAL_FLOAT(ki, pi.KI);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.KD);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.out);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.err);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.err_prev);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.part[0]);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.part[1]);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.part[2]);

  /* Initialize saturation */

  pi_saturation_set(&pi, min, max);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(max, pi.sat.max);
  TEST_ASSERT_EQUAL_FLOAT(min, pi.sat.min);
}

/* Feed PI controller with zeros */

static void test_pi_controller_zeros(void)
{
  pid_controller_t pi;
  float max = 0.990;
  float min = 0.000;
  float kp  = 2.0;
  float ki  = 0.001;
  int   i   = 0;

  /* Initialize PI controller */

  pi_controller_init(&pi, kp, ki);

  /* Initialize saturation */

  pid_saturation_set(&pi, min, max);

  for (i=0; i< 10000; i+=1)
    {
      pi_controller(&pi, 0.0);
    }

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(0.0, pi.out);
}

/* Feed PI controller with some data */

static void test_pi_controller(void)
{
  pid_controller_t pi;
  float kp  = 0.0;
  float ki  = 0.0;
  float max = 0.0;
  float min = 0.0;
  float p1  = 0.0;
  float p2  = 0.0;
  float p3  = 0.0;
  float out = 0.0;
  float err = 0.0;
  int   i   = 0;

  kp = 1.0;
  ki = 0.1;
  pi_controller_init(&pi, kp, ki);

  /* Step 1 */

  err = 0.1;

  pi_controller(&pi, err);

  p1  = err * kp;
  p2  += err * ki;
  p3  = err * 0;
  out = p1 + p2 + p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pi.out);

  /* Step 2 */

  err = 0.2;

  pi_controller(&pi, err);
  p1 = err * kp;
  p2 += err * ki;
  p3 = err * 0;
  out = p1 + p2 + p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pi.out);

  /* Step 3 */

  err = -0.3;

  pi_controller(&pi, err);
  p1 = err * kp;
  p2 += err * ki;
  p3 = err * 0;
  out = p1 + p2 + p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pi.out);

  /* Now set saturation */

  max = 0.3;
  min = 0.0;
  pi_saturation_set(&pi, min, max);

  /* Test saturation max */

  err = 0.4;
  for (i = 0; i < 20; i += 1)
    {
      pi_controller(&pi, err);
    }

  out = max;
  TEST_ASSERT_EQUAL_FLOAT(out, pi.out);

  err = -0.8;
  for (i = 0; i < 20; i += 1)
    {
      pi_controller(&pi, err);
    }

  /* Test saturation min */

  out = min;
  TEST_ASSERT_EQUAL_FLOAT(out, pi.out);
}

/* Saturate PI controller */

static void test_pi_controller_saturation(void)
{
  pid_controller_t pi;
  float max = 0.990;
  float min = 0.000;
  float kp  = 2.0;
  float ki  = 0.001;
  int   i   = 0;

  /* Initialize PI controller */

  pi_controller_init(&pi, kp, ki);

  /* Initialize saturation */

  pi_saturation_set(&pi, min, max);

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pi_controller(&pi, 0.01);
      TEST_ASSERT_LESS_OR_EQUAL(max, pi.out);
      TEST_ASSERT_LESS_OR_EQUAL(max, pi.part[1]);
    }

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pi_controller(&pi, -0.01);
      TEST_ASSERT_GREATER_OR_EQUAL(min, pi.out);
      TEST_ASSERT_GREATER_OR_EQUAL(min, pi.part[1]);
    }
}

/* PI windup protection */

static void test_pi_windup_protection(void)
{
  pid_controller_t pi;
  float max = 0.990;
  float min = 0.000;
  float kp  = 2.0;
  float ki  = 0.1;
  int   i   = 0;

  /* Initialize PI controller */

  pi_controller_init(&pi, kp, ki);

  /* Initialize saturation */

  pi_saturation_set(&pi, min, max);

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pi_controller(&pi, 0.01);
      TEST_ASSERT_LESS_OR_EQUAL(max, pi.part[1]);
    }

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pi_controller(&pi, -0.01);
      TEST_ASSERT_GREATER_OR_EQUAL(min, pi.part[1]);
    }
}

/* Initialize PID controller */

static void test_pid_controller_init(void)
{
  pid_controller_t pid;
  float max = 0.990;
  float min = 0.001;
  float kp  = 2.0;
  float ki  = 0.001;
  float kd  = 0.001;

  /* Initialize PID controller */

  pid_controller_init(&pid, kp, ki, kd);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(kp, pid.KP);
  TEST_ASSERT_EQUAL_FLOAT(ki, pid.KI);
  TEST_ASSERT_EQUAL_FLOAT(kd, pid.KD);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.out);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.err);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.err_prev);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.part[0]);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.part[1]);
  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.part[2]);

  /* Initialize saturation */

  pid_saturation_set(&pid, min, max);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(max, pid.sat.max);
  TEST_ASSERT_EQUAL_FLOAT(min, pid.sat.min);
}

/* Feed PID controller with zeros */

static void test_pid_controller_zeros(void)
{
  pid_controller_t pid;
  float max = 0.990;
  float min = 0.000;
  float kp  = 2.0;
  float ki  = 0.001;
  float kd  = 0.001;
  int   i   = 0;

  /* Initialize PI controller */

  pid_controller_init(&pid, kp, ki, kd);

  /* Initialize saturation */

  pid_saturation_set(&pid, min, max);

  for (i=0; i< 10000; i+=1)
    {
      pid_controller(&pid, 0.0);
    }

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(0.0, pid.out);
}

/* Feed PID controller with some data */

static void test_pid_controller(void)
{
  pid_controller_t pid;
  float kp   = 0.0;
  float ki   = 0.0;
  float kd   = 0.0;
  float max  = 0.0;
  float min  = 0.0;
  float p1   = 0.0;
  float p2   = 0.0;
  float p3   = 0.0;
  float out  = 0.0;
  float err  = 0.0;
  float prev = 0.0;
  int   i    = 0;

  kp = 1.0;
  ki = 0.1;
  kd = 0.01;
  pid_controller_init(&pid, kp, ki, kd);

  /* Step 1 */

  prev = 0.0;
  err  = 0.1;

  pid_controller(&pid, err);

  p1  = err * kp;
  p2  += err * ki;
  p3  = (err - prev) * kd;
  out = p1 + p2 + p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pid.out);

  /* Step 2 */

  prev = err;
  err  = 0.2;

  pid_controller(&pid, err);
  p1  = err * kp;
  p2  += err * ki;
  p3  = (err - prev) * kd;
  out = p1 + p2 + p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pid.out);

  /* Step 3 */

  prev = err;
  err = -0.3;

  pid_controller(&pid, err);
  p1  = err * kp;
  p2  += err * ki;
  p3  = (err - prev) * kd;
  out = p1 + p2 +p3;

  TEST_ASSERT_EQUAL_FLOAT(out, pid.out);

  /* Now set saturation */

  max = 0.3;
  min = 0.0;
  pid_saturation_set(&pid, min, max);

  /* Test saturation max */

  prev = err;
  err  = 0.4;
  for (i = 0; i < 20; i += 1)
    {
      pid_controller(&pid, err);
    }

  out = max;
  TEST_ASSERT_EQUAL_FLOAT(out, pid.out);

  prev = err;
  err = -0.8;
  for (i = 0; i < 20; i += 1)
    {
      pid_controller(&pid, err);
    }

  /* Test saturation min */

  out = min;
  TEST_ASSERT_EQUAL_FLOAT(out, pid.out);
}

/* Saturate PID controller */

static void test_pid_controller_saturation(void)
{
  pid_controller_t pid;
  float max = 0.990;
  float min = 0.000;
  float kp  = 2.0;
  float ki  = 0.001;
  float kd  = 0.001;
  int   i   = 0;

  /* Initialize PID controller */

  pid_controller_init(&pid, kp, ki, kd);

  /* Initialize saturation */

  pid_saturation_set(&pid, min, max);

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pid_controller(&pid, 0.01);
      TEST_ASSERT_LESS_OR_EQUAL(max, pid.out);
      TEST_ASSERT_LESS_OR_EQUAL(max, pid.part[1]);
    }

  /* Feed controller */

  for (i=0; i< 1000; i+=1)
    {
      pid_controller(&pid, -0.01);
      TEST_ASSERT_GREATER_OR_EQUAL(min, pid.out);
      TEST_ASSERT_GREATER_OR_EQUAL(min, pid.part[1]);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_pid
 ****************************************************************************/

void test_pid(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test PID functions */

  RUN_TEST(test_pi_controller_init);
  RUN_TEST(test_pi_controller_zeros);
  RUN_TEST(test_pi_controller);
  RUN_TEST(test_pi_controller_saturation);
  RUN_TEST(test_pi_windup_protection);
  RUN_TEST(test_pid_controller_init);
  RUN_TEST(test_pid_controller_zeros);
  RUN_TEST(test_pid_controller);
  RUN_TEST(test_pid_controller_saturation);

  UNITY_END();
}
