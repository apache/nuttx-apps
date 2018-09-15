/****************************************************************************
 * examples/dsptest/test_transform.c
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

/* Feed Clarke transform with some data */

static void test_transform_clarke(void)
{
  abc_frame_t abc;
  ab_frame_t  ab;

  /* a = 0.0, b = 0.0, c = 0.0 -> alpha = 0.0, beta = 0.0 */

  abc.a = 0.0;
  abc.b = 0.0;
  abc.c = 0.0;
  clarke_transform(&abc, &ab);

  TEST_ASSERT_EQUAL_FLOAT(0.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ab.b);

  /* a = 1.0, b = -2.0, c = 1.0 -> alpha = 1.0, beta =  -1.732 */

  abc.a = 1.0;
  abc.b = -2.0;
  abc.c = 1.0;
  clarke_transform(&abc, &ab);

  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.7320, ab.b);

  /* a = 1.0, b = 1.0, c = -2.0 -> alpha = 1.0, beta = 1.7320 */

  abc.a = 1.0;
  abc.b = 1.0;
  abc.c = -2.0;
  clarke_transform(&abc, &ab);

  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320, ab.b);

  /* a = -2.0, b = 1.0, c = 1.0 -> alpha = -2.0, beta = 0.0 */

  abc.a = -2.0;
  abc.b = 1.0;
  abc.c = 1.0;
  clarke_transform(&abc, &ab);

  TEST_ASSERT_EQUAL_FLOAT(-2.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ab.b);
}

/* Feed inverse Clarke transform with some data */

static void test_transform_invclarke(void)
{
  ab_frame_t  ab;
  abc_frame_t abc;

  /* alpha = 0.0, beta = 0.0 -> a = 0.0, b = 0.0, c = 0.0 */

  ab.a = 0.0;
  ab.b = 0.0;
  inv_clarke_transform(&ab, &abc);

  TEST_ASSERT_EQUAL_FLOAT(0.0, abc.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, abc.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, abc.c);

  /* alpha = 1.0, beta = 1.0 -> a = 1.0, b = 0.3660, c = -1.3660 */

  ab.a = 1.0;
  ab.b = 1.0;
  inv_clarke_transform(&ab, &abc);

  TEST_ASSERT_EQUAL_FLOAT(1.0, abc.a);
  TEST_ASSERT_EQUAL_FLOAT(0.3660, abc.b);
  TEST_ASSERT_EQUAL_FLOAT(-1.3660, abc.c);

  /* alpha = -1.0, beta = -1.0 -> a = -1.0, b = -0.3660, c = 1.3660 */

  ab.a = -1.0;
  ab.b = -1.0;
  inv_clarke_transform(&ab, &abc);

  TEST_ASSERT_EQUAL_FLOAT(-1.0, abc.a);
  TEST_ASSERT_EQUAL_FLOAT(-0.3660, abc.b);
  TEST_ASSERT_EQUAL_FLOAT(1.3660, abc.c);

  /* alpha = 1.0, beta = -1.0 -> a = 1.0, b = -1.3660, c = 0.3660 */

  ab.a = 1.0;
  ab.b = -1.0;
  inv_clarke_transform(&ab, &abc);

  TEST_ASSERT_EQUAL_FLOAT(1.0, abc.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.3660, abc.b);
  TEST_ASSERT_EQUAL_FLOAT(0.3660, abc.c);
}

/* Feed Park transform with some data */

static void test_transform_park(void)
{
  phase_angle_t angle;
  ab_frame_t    ab;
  dq_frame_t    dq;

  /* angle = 0.0, alpha = 0.0, beta = 0.0 -> d = 0.0, q = 0.0 */

  phase_angle_update(&angle, 0.0);
  ab.a = 0.0;
  ab.b = 0.0;
  park_transform(&angle, &ab, &dq);

  TEST_ASSERT_EQUAL_FLOAT(0.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(0.0, dq.q);

  /* angle = 0.0, alpha = 1.0, beta = 1.0 -> d = 1.0, q = 1.0 */

  phase_angle_update(&angle, 0.0);
  ab.a = 1.0;
  ab.b = 1.0;
  park_transform(&angle, &ab, &dq);

  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.q);

  /* angle = PI, alpha = 1.0, beta = 1.0 -> d = -1.0, q = -1.0 */

  phase_angle_update(&angle, M_PI_F);
  ab.a = 1.0;
  ab.b = 1.0;
  park_transform(&angle, &ab, &dq);

  TEST_ASSERT_EQUAL_FLOAT(-1.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, dq.q);

  /* angle = PI, alpha = -1.0, beta = 1.0 -> d = 1.0, q = -1.0 */

  phase_angle_update(&angle, M_PI_F);
  ab.a = -1.0;
  ab.b = 1.0;
  park_transform(&angle, &ab, &dq);

  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, dq.q);

  /* angle = -PI/2, alpha = 1.0, beta = 1.0 -> d = -1.0, q = 1.0 */

  phase_angle_update(&angle, -M_PI_F/2);
  ab.a = 1.0;
  ab.b = 1.0;
  park_transform(&angle, &ab, &dq);

  TEST_ASSERT_EQUAL_FLOAT(-1.0, dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.0, dq.q);
}

/* Feed inverse Park transform with some data */

static void test_transform_invpark(void)
{
  phase_angle_t angle;
  dq_frame_t    dq;
  ab_frame_t    ab;

  /* angle = 0.0, d = 0.0, q = 0.0 -> alpha = 0.0, beta = 0.0 */

  phase_angle_update(&angle, 0.0);
  dq.d = 0.0;
  dq.q = 0.0;
  inv_park_transform(&angle, &dq, &ab);

  TEST_ASSERT_EQUAL_FLOAT(0.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ab.b);

  /* angle = 0.0, d = 1.0, q = 1.0 -> alpha = 1.0, beta = 1.0 */

  phase_angle_update(&angle, 0.0);
  dq.d = 1.0;
  dq.q = 1.0;
  inv_park_transform(&angle, &dq, &ab);

  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.b);

  /* angle = PI, d = 1.0, q = 1.0 -> alpha = 0.0, beta = 0.0 */

  phase_angle_update(&angle, M_PI_F);
  dq.d = 1.0;
  dq.q = 1.0;
  inv_park_transform(&angle, &dq, &ab);

  TEST_ASSERT_EQUAL_FLOAT(-1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, ab.b);

  /* angle = PI, d = -1.0, q = 1.0 -> alpha = 0.0, beta = 0.0 */

  phase_angle_update(&angle, M_PI_F);
  dq.d = -1.0;
  dq.q = 1.0;
  inv_park_transform(&angle, &dq, &ab);

  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, ab.b);

  /* angle = -PI/2, d = 1.0, q = 1.0 -> alpha = 0.0, beta = 0.0 */

  phase_angle_update(&angle, -M_PI_F/2);
  dq.d = 1.0;
  dq.q = 1.0;
  inv_park_transform(&angle, &dq, &ab);

  TEST_ASSERT_EQUAL_FLOAT(1.0, ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.0, ab.b);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_transform
 ****************************************************************************/

void test_transform(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test 3 phase motor transformations */

  RUN_TEST(test_transform_clarke);
  RUN_TEST(test_transform_invclarke);
  RUN_TEST(test_transform_park);
  RUN_TEST(test_transform_invpark);

  UNITY_END();
}
