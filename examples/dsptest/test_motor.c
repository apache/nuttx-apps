/****************************************************************************
 * examples/dsptest/test_motor.c
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

/* Set float precision for this module */

#undef UNITY_FLOAT_PRECISION
#define UNITY_FLOAT_PRECISION (0.0001f)

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

/* Initialize openloop */

static void test_openloop_init(void)
{
  struct openloop_data_s op;
  float angle     = 0.0;
  float max_speed = 100;
  float per       = 10e-6;

  /* Initialize openlooop controller */

  motor_openloop_init(&op, max_speed, per);

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* Test values after initialization */

  TEST_ASSERT_EQUAL_FLOAT(0.0, angle);
  TEST_ASSERT_EQUAL_FLOAT(per, op.per);
  TEST_ASSERT_EQUAL_FLOAT(max_speed, op.max);
}

/* Single step openloop */

static void test_openloop_one_step(void)
{
  struct openloop_data_s op;
  float expected  = 0.0;
  float angle     = 0.0;
  float max_speed = 100;
  float speed     = 10;
  float per       = 10e-6;

  /* Initialize openlooop controller */

  motor_openloop_init(&op, max_speed, per);

  /* Do single iteration in CW direction */

  motor_openloop(&op, speed, DIR_CW);

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* Get expected value */

  expected = speed * per;

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected, angle);

  /* Do single iteration in CCW direction */

  motor_openloop(&op, speed, DIR_CCW);

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* Get expected value */

  expected = 0.0;

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected, angle);
}

/* Many steps in openloop */

static void test_openloop_many_steps(void)
{
  struct openloop_data_s op;
  float expected  = 0.0;
  float angle     = 0.0;
  float max_speed = 100;
  float speed     = 10;
  float per       = 50e-6;
  int   iter      = 10;
  int   i         = 0;

  /* Initialize openlooop controller */

  motor_openloop_init(&op, max_speed, per);

  /* Do some iterations in CW direction */

  for (i = 0; i < iter; i += 1)
    {
      motor_openloop(&op, speed, DIR_CW);
    }

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* Get expected value */

  expected = speed * per * iter;

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected, angle);

  /* Do some iterations in CCW direction */

  for (i = 0; i < iter; i += 1)
    {
      motor_openloop(&op, speed, DIR_CCW);
    }

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* We should return to 0 */

  expected = 0.0;

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected, angle);
}

/* Test maximum openloop speed */

static void test_openloop_max_speed(void)
{
  TEST_IGNORE_MESSAGE("not implemented");
}

/* Normalize angle in openloop */

static void test_openloop_normalize_angle(void)
{
  struct openloop_data_s op;
  float expected  = 0.0;
  float angle     = 0.0;
  float max_speed = 100;
  float speed     = 10;
  float per       = 10e-6;
  int   iter      = 1000;
  int   i         = 0;

  /* Initialize openlooop controller */

  motor_openloop_init(&op, max_speed, per);

  /* Do many iterations to exceed 2PI range */

  for (i = 0; i < iter; i += 1)
    {
      motor_openloop(&op, speed, DIR_CW);
    }

  /* Get openloop angle */

  angle = motor_openloop_angle_get(&op);

  /* Get expected value */

  expected = speed * per * iter;

  /* And normalize to <0.0, 2*PI> */

  while (expected > 2 * M_PI_F)
    {
      expected -= 2 * M_PI_F;
    }

  /* Test angle */

  TEST_ASSERT_EQUAL_FLOAT(expected, angle);
}

/* Initialize otor angle */

static void test_angle_init(void)
{
  struct motor_angle_s angle;
  float angle_m = 0.0;
  float angle_e = 0.0;
  uint8_t p     = 0;

  /* Initialize motor angle */

  p = 32;
  motor_angle_init(&angle, p);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test initial values */

  TEST_ASSERT_EQUAL_FLOAT(0.0, angle_e);
  TEST_ASSERT_EQUAL_FLOAT(0.0, angle_m);
  TEST_ASSERT_EQUAL_UINT8(p, angle.p);
  TEST_ASSERT_EQUAL_FLOAT((float)1.0 / p, angle.one_by_p);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
}

/* Update electrical angle in CW direction */

static void test_angle_el_update_cw(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Update electrical angle with 0.0 */

  angle_step = 0.0;
  expected_e = 0.0;
  expected_m = 0.0;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_e_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);

  /* Update electrical angle with 0.1 */

  angle_step = 0.1;
  expected_e = 0.1;
  expected_m = 0.1 / p;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_e_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);

  /* Update electrical angle with 2*PI + 0.2 in three steps.
   * This should increase pole counter in angle structure by 1.
   */

  angle_step = 2 * M_PI_F + 0.2;
  expected_e = 0.2;
  expected_m = angle_step / p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_e_update(&angle, M_PI_F, DIR_CW);
  motor_angle_e_update(&angle, 2 * M_PI_F, DIR_CW);
  motor_angle_e_update(&angle, 0.2, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(1, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
}

/* Update electrical angle in CCW direction */

static void test_angle_el_update_ccw(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Move angle 0.1 in CCW direction from 0.0.
   * We start from 0.0 and move angle CCW by 0.1.
   */

  angle_step = MOTOR_ANGLE_E_MAX - 0.1;
  expected_e = angle_step;
  expected_m = (p - 1) * MOTOR_ANGLE_M_MAX / p + expected_e / p;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_e_update(&angle, angle_step, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(p - 1, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);

  /* Update electrical angle with 2PI+0.1 in CCW direction in three steps */

  angle_step = (MOTOR_ANGLE_E_MAX + 0.1);
  expected_e = MOTOR_ANGLE_E_MAX - 0.1;
  expected_m = (p - 2) * MOTOR_ANGLE_M_MAX / p + expected_e / p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_e_update(&angle, MOTOR_ANGLE_E_MAX - M_PI_F, DIR_CCW);
  motor_angle_e_update(&angle, MOTOR_ANGLE_E_MAX - 2 * M_PI_F, DIR_CCW);
  motor_angle_e_update(&angle, MOTOR_ANGLE_E_MAX - 0.1, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(p - 2, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
}

/* Update electrical angle and overflow electrical angle in CW direction */

static void test_angle_el_update_cw_overflow(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;
  float   a          = 0.0;
  int     i          = 0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Update electrical angle to achieve full mechanical rotation */

  angle_step = 0.1;
  expected_e = angle_step;
  expected_m = angle_step / p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < p; i += 1)
    {
      for (a = 0.0; a <= MOTOR_ANGLE_E_MAX; a += angle_step)
        {
          motor_angle_e_update(&angle, a, DIR_CW);
        }
    }

  /* Test poles counter before final step */

  TEST_ASSERT_EQUAL_INT8(p - 1, angle.i);

  /* One more step after overflow mechanical angle */

  a = angle_step;
  motor_angle_e_update(&angle, a, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
}

/* Update electrical angle and overflow electrical angle in CCW direction */

static void test_angle_el_update_ccw_overflow(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;
  float   a          = 0.0;
  int     i          = 0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Update electrical angle to achieve full mechanical rotation */

  angle_step = 0.1;
  expected_e = MOTOR_ANGLE_E_MAX - angle_step;
  expected_m = MOTOR_ANGLE_M_MAX - angle_step / p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < p; i += 1)
    {
      for (a = MOTOR_ANGLE_E_MAX; a >= 0.0; a -= angle_step)
        {
          motor_angle_e_update(&angle, a, DIR_CCW);
        }
    }

  /* Test poles counter before final step */

  TEST_ASSERT_EQUAL_INT8(0, angle.i);

  /* One more step after overflow mechanical angle */

  a = MOTOR_ANGLE_E_MAX - 0.1;
  motor_angle_e_update(&angle, a, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(7, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
}

/* Update electric angle and change direction */

static void test_angle_el_change_dir(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  int     i          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;
  float   a          = 0.0;

  /* Initialize motor angle */

  p = 7;
  motor_angle_init(&angle, p);

  /* Move electrical angle with 4*(2PI) + 0.1.
   * It give us pole counter = 4
   */

  angle_step = 0.1;
  expected_m = 4 * MOTOR_ANGLE_M_MAX / p + angle_step / p;
  expected_e = 0.1;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < 4; i += 1)
    {
      for (a = 0.0; a <= MOTOR_ANGLE_E_MAX; a += angle_step)
        {
          motor_angle_e_update(&angle, a, DIR_CW);
        }
    }

  /* Test poles counter before final step */

  TEST_ASSERT_EQUAL_INT8(3, angle.i);

  /* And rest 0.1 */

  motor_angle_e_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(4, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);

  /* Now move angle backward  2*(2PI) + 0.1 */

  angle_step = 0.1;
  expected_m = 2 * MOTOR_ANGLE_M_MAX / p - angle_step / p;
  expected_e = MOTOR_ANGLE_E_MAX - angle_step;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < 2; i += 1)
    {
      for (a = angle_m; a >= 0.0; a -= angle_step)
        {
          motor_angle_e_update(&angle, a, DIR_CCW);
        }
    }

  /* Test poles counter before final step */

  TEST_ASSERT_EQUAL_INT8(2, angle.i);

  /* And rest 0.1 */

  motor_angle_e_update(&angle, MOTOR_ANGLE_E_MAX - angle_step, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(1, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);

  /* and again in forward direction 4*(2PI) + 0.1 */

  angle_step = 0.1;
  expected_m = 5 * MOTOR_ANGLE_M_MAX / p + angle_step / p;
  expected_e = 0.1;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < 4; i += 1)
    {
      for (a = angle_e; a <= MOTOR_ANGLE_E_MAX; a += angle_step)
        {
          motor_angle_e_update(&angle, a, DIR_CW);
        }
    }

  /* Test poles counter before final step */

  TEST_ASSERT_EQUAL_INT8(4, angle.i);

  /* And rest 0.1 */

  motor_angle_e_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(5, angle.i);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
}

/* Update mechanical angle in CW direction */

static void test_angle_m_update_cw(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Update mechanical angle with 0.0 */

  angle_step = 0.0;
  expected_m = 0.0;
  expected_e = 0.0;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_m_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);

  /* Update mechanical angle with 0.1 */

  angle_step = 0.1;
  expected_m = angle_step;
  expected_e = angle_step * p - 0*MOTOR_ANGLE_E_MAX / p;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_m_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);

  /* Update mechanical angle to get one electrical angle rotation + 0.1 */

  angle_step = MOTOR_ANGLE_M_MAX / p + 0.1;
  expected_m = angle_step;
  expected_e = angle_step * p - 1 * MOTOR_ANGLE_E_MAX;

  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, angle_step / 3, DIR_CW);
  motor_angle_m_update(&angle, 2 * angle_step / 3, DIR_CW);
  motor_angle_m_update(&angle, 3 * angle_step / 3, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(1, angle.i);
}

/* Update mechanical angle in CCW direction */

static void test_angle_m_update_ccw(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 8;
  motor_angle_init(&angle, p);

  /* Update mechanical angle with 1.0
   * For 8 poles, one electrical angle rotationa takes ~0.785.
   * So with angle step = 1.0 we have 1 electical angle rotation plus
   * some rest.
   */

  angle_step = 1.0;
  expected_m = angle_step;
  expected_e = angle_step * p - 1 * MOTOR_ANGLE_E_MAX;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_m_update(&angle, angle_step, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(1, angle.i);

  /* Update mechanical angle to get one electrical angle rotation */

  angle_step = angle_step - MOTOR_ANGLE_E_MAX / p;
  expected_m = angle_step;
  expected_e = angle_step * p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, angle_step / 3, DIR_CCW);
  motor_angle_m_update(&angle, 2 * angle_step / 3, DIR_CCW);
  motor_angle_m_update(&angle, 3 * angle_step / 3, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
}

/* Update mechanical angle and overflow mechanical angle in CW direction */

static void test_angle_m_update_cw_overflow(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 3;
  motor_angle_init(&angle, p);

  /* Full mechanical angle rotation (2PI) + 0.1 in CW direction */

  angle_step = 0.1;
  expected_m = angle_step;
  expected_e = 0 * MOTOR_ANGLE_E_MAX / p + angle_step * p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, 0.0, DIR_CW);
  motor_angle_m_update(&angle, MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 1 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 2 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 3 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 4 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, angle_step, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(0, angle.i);
}

/* Update mechanical angle and overflow mechanical angle in CCW direction */

static void test_angle_m_update_ccw_overflow(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 3;
  motor_angle_init(&angle, p);

  /* Full mechanical angle rotation (2PI) + 0.1 in CCW direction */

  angle_step = MOTOR_ANGLE_M_MAX - 0.1;
  expected_m = angle_step;
  expected_e = MOTOR_ANGLE_E_MAX - 0.1 * p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, 0.0, DIR_CCW);
  motor_angle_m_update(&angle, 4 * MOTOR_ANGLE_M_MAX / 4, DIR_CCW);
  motor_angle_m_update(&angle, 3 * MOTOR_ANGLE_M_MAX / 4, DIR_CCW);
  motor_angle_m_update(&angle, 2 * MOTOR_ANGLE_M_MAX / 4, DIR_CCW);
  motor_angle_m_update(&angle, 1 * MOTOR_ANGLE_M_MAX / 4, DIR_CCW);
  motor_angle_m_update(&angle, angle_step, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(p - 1, angle.i);
}

/* Update mechanical angle and change direction */

static void test_angle_m_change_dir(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   expected_i = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;

  /* Initialize motor angle */

  p = 3;
  motor_angle_init(&angle, p);

  /* Move mechanical angle by 3*(2PI)/4 in CW direction */

  angle_step = 3 * MOTOR_ANGLE_M_MAX / 4;
  expected_m = angle_step;
  expected_i = ((int)(angle_step * p / MOTOR_ANGLE_M_MAX));
  expected_e = angle_step * p - expected_i * MOTOR_ANGLE_E_MAX;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, 0.0, DIR_CW);
  motor_angle_m_update(&angle, 1 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 2 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);
  motor_angle_m_update(&angle, 3 * MOTOR_ANGLE_M_MAX / 4, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(expected_i, angle.i);

  /* Move mechanical angle by 1.0 in CCW direction */

  angle_step = 3 * MOTOR_ANGLE_M_MAX / 4 - 2.0;
  expected_m = angle_step;
  expected_i = ((int)(angle_step * p / MOTOR_ANGLE_M_MAX));
  expected_e = angle_step * p - expected_i * MOTOR_ANGLE_E_MAX;
  s = sin(expected_e);
  c = cos(expected_e);

  motor_angle_m_update(&angle, 3 * MOTOR_ANGLE_M_MAX / 4 - 1.0, DIR_CCW);
  motor_angle_m_update(&angle, 3 * MOTOR_ANGLE_M_MAX / 4 - 2.0, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(expected_i, angle.i);
}

/* Mix update mechanical angle and update electrical angle */

static void test_angle_m_el_mixed(void)
{
  struct motor_angle_s angle;
  uint8_t p          = 0;
  int     i          = 0;
  float   angle_step = 0.0;
  float   angle_m    = 0.0;
  float   angle_e    = 0.0;
  float   expected_e = 0.0;
  float   expected_m = 0.0;
  float   expected_i = 0.0;
  float   s          = 0.0;
  float   c          = 0.0;
  float   a          = 0.0;

  /* Initialize motor angle */

  p = 27;
  motor_angle_init(&angle, p);

  /* Update mechanical angle to get 4 electrical angle
   * rotations + 0.1 in CW direction
   */

  angle_step = 4 * MOTOR_ANGLE_M_MAX / p + 0.1;
  expected_m = angle_step;
  expected_i = ((int)(angle_step * p / MOTOR_ANGLE_M_MAX));
  expected_e = angle_step * p - expected_i * MOTOR_ANGLE_E_MAX;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move in a few steps */

  motor_angle_m_update(&angle, 0.0, DIR_CW);
  motor_angle_m_update(&angle, MOTOR_ANGLE_M_MAX / p, DIR_CW);
  motor_angle_m_update(&angle, 4 * MOTOR_ANGLE_M_MAX / p, DIR_CW);
  motor_angle_m_update(&angle, 4 * MOTOR_ANGLE_M_MAX / p + 0.1, DIR_CW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(expected_i, angle.i);

  /* Now move electrical angle by 2 full rotation in CCW direction.
   * This should give us the same electrical angle and mechanical
   * angle reduced by 2 electrical rotations.
   */

  angle_step = 2 * MOTOR_ANGLE_E_MAX;
  expected_i = expected_i - 2;
  expected_m = expected_m - 2 * MOTOR_ANGLE_M_MAX / p;
  s = sin(expected_e);
  c = cos(expected_e);

  /* Move angle in loop */

  for (i = 0; i < 2; i += 1)
    {
      for (a = expected_e ; a >= 0.0; a -= 0.1)
        {
          motor_angle_e_update(&angle, a, DIR_CCW);
        }
    }

  /* Final step */

  motor_angle_e_update(&angle, expected_e, DIR_CCW);

  angle_m = motor_angle_m_get(&angle);
  angle_e = motor_angle_e_get(&angle);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_e, angle_e);
  TEST_ASSERT_EQUAL_FLOAT(expected_m, angle_m);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s, angle.angle_el.sin);
  TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c, angle.angle_el.cos);
  TEST_ASSERT_EQUAL_INT8(expected_i, angle.i);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_motor
 ****************************************************************************/

void test_motor(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test some definitions */

  TEST_ASSERT_EQUAL_FLOAT(1.0, DIR_CW);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, DIR_CCW);

  /* Openloop control functions */

  RUN_TEST(test_openloop_init);
  RUN_TEST(test_openloop_one_step);
  RUN_TEST(test_openloop_many_steps);
  RUN_TEST(test_openloop_max_speed);
  RUN_TEST(test_openloop_normalize_angle);

  /* Motor angle */

  RUN_TEST(test_angle_init);
  RUN_TEST(test_angle_el_update_cw);
  RUN_TEST(test_angle_el_update_ccw);
  RUN_TEST(test_angle_el_update_cw_overflow);
  RUN_TEST(test_angle_el_update_ccw_overflow);
  RUN_TEST(test_angle_el_change_dir);
  RUN_TEST(test_angle_m_update_cw);
  RUN_TEST(test_angle_m_update_ccw);
  RUN_TEST(test_angle_m_update_cw_overflow);
  RUN_TEST(test_angle_m_update_ccw_overflow);
  RUN_TEST(test_angle_m_change_dir);
  RUN_TEST(test_angle_m_el_mixed);

  UNITY_END();
}
