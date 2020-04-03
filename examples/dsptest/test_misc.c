/****************************************************************************
 * examples/dsptest/test_misc.c
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
#define UNITY_FLOAT_PRECISION   (0.0001f)

#define TEST_SINCOS_ANGLE_STEP  0.01
#define TEST_SINCOS2_ANGLE_STEP 0.01
#define TEST_ATAN2_XY_STEP      0.01

/* Angle sin/cos depends on libdsp precision */

#if CONFIG_LIBDSP_PRECISION == 1
#  define ANGLE_SIN(val) fast_sin2(val)
#  define ANGLE_COS(val) fast_cos2(val)
#elif CONFIG_LIBDSP_PRECISION == 2
#  define ANGLE_SIN(val) sin(val)
#  define ANGLE_COS(val) cos(val)
#else
#  define ANGLE_SIN(val) fast_sin(val)
#  define ANGLE_COS(val) fast_cos(val)
#endif

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

/* Test float value saturation */

static void test_f_saturate(void)
{
  float val = 0.0;

  /* Value in the range */

  val = 0.9;
  f_saturate(&val, 0.0, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(0.9, val);

  /* Upper limit */

  val = 1.2;
  f_saturate(&val, 0.0, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(1.0, val);

  /* Lower limit */

  val = 0.0;
  f_saturate(&val, 0.1, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(0.1, val);
}

/* Test 2D vector magnitude */

static void test_vector2d_mag(void)
{
  float mag = 0.0;

  /* mag([0.0, 0.0]) = 0.0 */

  mag = vector2d_mag(0.0, 0.0);

  TEST_ASSERT_EQUAL_FLOAT(0.0, mag);

  /* mag([1.0, 1.0]) = 1.4142 */

  mag = vector2d_mag(1.0, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(1.4142, mag);

  /* mag([-1.0, 1.0]) = 1.4142 */

  mag = vector2d_mag(-1.0, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(1.4142, mag);
}

/* Test 2D vector saturation */

static void test_vector2d_saturate(void)
{
  float x       = 0.0;
  float y       = 0.0;
  float mag     = 0.0;
  float mag_ref = 0.0;

  /* Vector magnitude 0.0 */

  x = 0.0;
  y = 0.0;
  mag_ref = 1.0;
  vector2d_saturate(&x, &y, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(0.0, x);
  TEST_ASSERT_EQUAL_FLOAT(0.0, y);

  /* Vector magnitude 0.0, saturation 0.0 */

  x = 0.0;
  y = 0.0;
  mag_ref = 0.0;
  vector2d_saturate(&x, &y, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(mag_ref, x);
  TEST_ASSERT_EQUAL_FLOAT(mag_ref, y);

  /* Vector magnitude 1.4142, saturation 0.0 */

  x = 1.0;
  y = 1.0;
  mag_ref = 0.0;
  vector2d_saturate(&x, &y, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(0.0, x);
  TEST_ASSERT_EQUAL_FLOAT(0.0, y);

  /* Vector magnitude 1.4142, saturation 3.0 */

  x = 1.0;
  y = 1.0;
  mag_ref = 3.0;
  vector2d_saturate(&x, &y, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(1.0, x);
  TEST_ASSERT_EQUAL_FLOAT(1.0, y);

  /* Vector magnitude 1.4142, saturation 1.0 - truncate */

  x = 1.0;
  y = 1.0;
  mag_ref = 1.0;
  vector2d_saturate(&x, &y, mag_ref);
  mag = vector2d_mag(x, y);

  TEST_ASSERT_EQUAL_FLOAT(mag_ref, mag);
}

/* Test dq vector magnitude */

static void test_dq_mag(void)
{
  TEST_IGNORE_MESSAGE("test_dq_mag not implemented");
}

/* Test dq vector saturation */

static void test_dq_saturate(void)
{
  dq_frame_t dq;
  float mag_ref = 0.0;
  float mag     = 0.0;

  /* Vector magnitude 0.0 */

  dq.d = 0.0;
  dq.q = 0.0;
  mag_ref = 1.0;
  dq_saturate(&dq, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(0.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(0.0, dq.q);

  /* Vector magnitude 0.0, saturation 0.0 */

  dq.d = 0.0;
  dq.q = 0.0;
  mag_ref = 0.0;
  dq_saturate(&dq, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(mag_ref, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(mag_ref, dq.q);

  /* Vector magnitude 1.4142, saturation 0.0 */

  dq.d = 1.0;
  dq.q = 1.0;
  mag_ref = 0.0;
  dq_saturate(&dq, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(mag_ref, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(mag_ref, dq.q);

  /* Vector magnitude 1.4142, saturation 3.0 */

  dq.d = 1.0;
  dq.q = 1.0;
  mag_ref = 3.0;
  dq_saturate(&dq, mag_ref);

  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.q);

  /* Vector magnitude 1.4142, saturation 1.0 - truncate */

  dq.d = 1.0;
  dq.q = 1.0;
  mag_ref = 1.0;
  dq_saturate(&dq, mag_ref);
  mag = dq_mag(&dq);

  TEST_ASSERT_EQUAL_FLOAT(mag_ref, mag);
}

/* Test fast sine */

static void test_fast_sin(void)
{
  float s_ref = 0.0;
  float angle = 0.0;
  float s     = 0.0;

  /* Compare with LIBC sine */

  for (angle = 0.0; angle < 2 * M_PI_F; angle += TEST_SINCOS_ANGLE_STEP)
    {
      s_ref = sinf(angle);
      s     = fast_sin(angle);

      TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, s_ref, s);
    }
}

/* Test fast cosine */

static void test_fast_cos(void)
{
  float c_ref = 0.0;
  float angle = 0.0;
  float c     = 0.0;

  /* Compare with LIBC cosine */

  for (angle = 0.0; angle < 2 * M_PI_F; angle += TEST_SINCOS_ANGLE_STEP)
    {
      c_ref = cosf(angle);
      c     = fast_cos(angle);

      TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS_DELTA, c_ref, c);
    }
}

/* Test fast sine (better accuracy) */

static void test_fast_sin2(void)
{
  float s_ref = 0.0;
  float angle = 0.0;
  float s     = 0.0;

  /* Compare with LIBC sine */

  for (angle = 0.0; angle < 2 * M_PI_F; angle += TEST_SINCOS2_ANGLE_STEP)
    {
      s_ref = sinf(angle);
      s     = fast_sin2(angle);

      TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS2_DELTA, s_ref, s);
    }
}

/* Test fast cosine (better accuracy) */

static void test_fast_cos2(void)
{
  float c_ref = 0.0;
  float angle = 0.0;
  float c     = 0.0;

  /* Compare with LIBC cosine */

  for (angle = 0.0; angle < 2 * M_PI_F; angle += TEST_SINCOS2_ANGLE_STEP)
    {
      c_ref = cosf(angle);
      c     = fast_cos2(angle);

      TEST_ASSERT_FLOAT_WITHIN(TEST_SINCOS2_DELTA, c_ref, c);
    }
}

/* Test fast atan2 */

static void test_fast_atan2(void)
{
  float angle_ref = 0.0;
  float angle     = 0.0;
  float x         = 0.0;
  float y         = 0.0;

  /* atan2(0, 0) - special case when atan2 is not defined */

  angle = fast_atan2(y, x);

  /* Expect non inf and non nan */

  TEST_ASSERT_FLOAT_IS_DETERMINATE(angle);

  /* Compare with LIBC atan2 */

  for (x = TEST_ATAN2_XY_STEP; x < M_PI_F; x += TEST_ATAN2_XY_STEP)
    {
      for (y = TEST_ATAN2_XY_STEP; y < M_PI_F; y += TEST_ATAN2_XY_STEP)
        {
          angle_ref = atan2f(y, x);
          angle     = fast_atan2(y, x);

          TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);
        }
    }

  for (x = -TEST_ATAN2_XY_STEP; x > -M_PI_F; x -= TEST_ATAN2_XY_STEP)
    {
      for (y = -TEST_ATAN2_XY_STEP; y > -M_PI_F; y -= TEST_ATAN2_XY_STEP)
        {
          angle_ref = atan2f(y, x);
          angle     = fast_atan2(y, x);

          TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);
        }
    }

  for (x = TEST_ATAN2_XY_STEP; x < M_PI_F; x += TEST_ATAN2_XY_STEP)
    {
      for (y = -TEST_ATAN2_XY_STEP; y > -M_PI_F; y -= TEST_ATAN2_XY_STEP)
        {
          angle_ref = atan2f(y, x);
          angle     = fast_atan2(y, x);

          TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);
        }
    }

  for (x = -TEST_ATAN2_XY_STEP; x > -M_PI_F; x -= TEST_ATAN2_XY_STEP)
    {
      for (y = TEST_ATAN2_XY_STEP; y < M_PI_F; y += TEST_ATAN2_XY_STEP)
        {
          angle_ref = atan2f(y, x);
          angle     = fast_atan2(y, x);

          TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);
        }
    }

  /* Test some big numbers */

  x = 1000000.0;
  y = 2.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);

  x = 2.0;
  y = 1000000.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);

  x = 1000000.0;
  y = 1000000.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);

  x = -1000000.0;
  y = 1000000.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);

  x = 1000000.0;
  y = -1000000.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);

  x = -1000000.0;
  y = -1000000.0;

  angle_ref = atan2f(y, x);
  angle     = fast_atan2(y, x);

  TEST_ASSERT_FLOAT_WITHIN(TEST_ATAN2_DELTA, angle_ref, angle);
}

/* Test angle normalization */

static void test_angle_norm(void)
{
  float angle  = 0.0;
  float per    = 0.0;
  float bottom = 0.0;
  float top    = 0.0;

  /* range = (0.0, 2PI)  */

  per    = 2 * M_PI_F;
  bottom = 0.0;
  top    = 2 * M_PI_F;

  /* in range */

  angle  = 0.0;
  angle_norm(&angle, per, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.0, angle);

  /* in range */

  angle  = 1.0;
  angle_norm(&angle, per, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(1.0, angle);

  /* wrap to 0.2 */

  angle  = 2 * M_PI_F + 0.2;
  angle_norm(&angle, per, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.2, angle);

  /* wrap to 0.2 */

  angle  = -2 * M_PI_F + 0.2;
  angle_norm(&angle, per, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.2, angle);
}

/* Test angle normalization with 2*PI period length */

static void test_angle_norm_2pi(void)
{
  float angle  = 0.0;
  float bottom = 0.0;
  float top    = 0.0;

  /* range = (0.0, 2PI)  */

  bottom = 0.0;
  top    = 2 * M_PI_F;

  /* in range */

  angle = 0.0;
  angle_norm_2pi(&angle, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.0, angle);

  /* in range */

  angle = 1.0;
  angle_norm_2pi(&angle, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(1.0, angle);

  /* wrap to 0.2 */

  angle = 2 * M_PI_F + 0.2;
  angle_norm_2pi(&angle, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.2, angle);

  /* wrap to 0.2 */

  angle = -2 * M_PI_F + 0.2;
  angle_norm_2pi(&angle, bottom, top);

  TEST_ASSERT_EQUAL_FLOAT(0.2, angle);
}

/* Test phase angle update */

static void test_phase_angle_update(void)
{
  struct phase_angle_s angle;
  float val = 0.0;
  float s   = 0.0;
  float c   = 0.0;

  /* angle = 0.0 */

  val = 0.0;
  s   = ANGLE_SIN(val);
  c   = ANGLE_COS(val);
  phase_angle_update(&angle, val);

  TEST_ASSERT_EQUAL_FLOAT(val, angle.angle);
  TEST_ASSERT_EQUAL_FLOAT(s, angle.sin);
  TEST_ASSERT_EQUAL_FLOAT(c, angle.cos);

  /* angle = 1.5 */

  val = 1.5;
  s   = ANGLE_SIN(val);
  c   = ANGLE_COS(val);
  phase_angle_update(&angle, val);

  TEST_ASSERT_EQUAL_FLOAT(val, angle.angle);
  TEST_ASSERT_EQUAL_FLOAT(s, angle.sin);
  TEST_ASSERT_EQUAL_FLOAT(c, angle.cos);

  /* angle = 8, should be normalize to (0.0, 2PI) range */

  val = 8;
  s   = ANGLE_SIN(val);
  c   = ANGLE_COS(val);
  phase_angle_update(&angle, val);

  TEST_ASSERT_EQUAL_FLOAT(val - 2 * M_PI_F, angle.angle);
  TEST_ASSERT_EQUAL_FLOAT(s, angle.sin);
  TEST_ASSERT_EQUAL_FLOAT(c, angle.cos);

  /* angle = -1.5, should be normalize to (0.0, 2PI) range */

  val = -1.5;
  s   = ANGLE_SIN(val);
  c   = ANGLE_COS(val);
  phase_angle_update(&angle, val);

  TEST_ASSERT_EQUAL_FLOAT(val + 2 * M_PI_F, angle.angle);
  TEST_ASSERT_EQUAL_FLOAT(s, angle.sin);
  TEST_ASSERT_EQUAL_FLOAT(c, angle.cos);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_misc
 ****************************************************************************/

void test_misc(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test helper functions */

  RUN_TEST(test_f_saturate);

  /* Test vector functions */

  RUN_TEST(test_vector2d_mag);
  RUN_TEST(test_vector2d_saturate);
  RUN_TEST(test_dq_mag);
  RUN_TEST(test_dq_saturate);

  /* Test fast trigonometric functions */

  RUN_TEST(test_fast_sin);
  RUN_TEST(test_fast_cos);
  RUN_TEST(test_fast_sin2);
  RUN_TEST(test_fast_cos2);
  RUN_TEST(test_fast_atan2);

  /* Test angle functions */

  RUN_TEST(test_angle_norm);
  RUN_TEST(test_angle_norm_2pi);
  RUN_TEST(test_phase_angle_update);

  UNITY_END();
}
