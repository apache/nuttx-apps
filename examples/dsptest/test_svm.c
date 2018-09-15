/****************************************************************************
 * examples/dsptest/test_svm.c
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

#define SVM3_DUTY_MAX 0.95
#define SVM3_DUTY_MIN 0.00

#undef UNITY_FLOAT_PRECISION
#define UNITY_FLOAT_PRECISION   (0.0001f)

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

static void SVM3(FAR struct svm3_state_s *s, float a, float b)
{
  ab_frame_t ab;

  svm3_init(s, SVM3_DUTY_MIN, SVM3_DUTY_MAX);

  ab.a = a;
  ab.b = b;

  svm3(s, &ab);
}

/* Feed SVM3 with zeros */

static void test_svm3_zero(void)
{
  struct svm3_state_s s;

  SVM3(&s, 0.0, 0.0);

  TEST_ASSERT_EQUAL_FLOAT(0.5, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.5, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.5, s.d_w);
  TEST_ASSERT_EQUAL_INT8(2, s.sector);
}

/* Test duty cycle saturation */

static void test_svm3_saturation(void)
{
  struct svm3_state_s s;

  /* v_a = 1.0
   * v_b = 1.0
   * mag(v_ab) > 1.0 which is not valid for SVM
   * d_u > 1.0, d_w < 0.0
   */

  SVM3(&s, 1.0, 1.0);

  TEST_ASSERT_EQUAL_FLOAT(SVM3_DUTY_MAX, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.81698, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(SVM3_DUTY_MIN, s.d_w);
  TEST_ASSERT_EQUAL_INT8(1, s.sector);


  /* v_a = -1.0
   * v_b = -1.0
   * mag(v_ab) > 1.0 which is not valid for SVM
   * d_u < 0.0, dw > 1
   */

  SVM3(&s, -1.0, -1.0);

  TEST_ASSERT_EQUAL_FLOAT(SVM3_DUTY_MIN, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.183012, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(SVM3_DUTY_MAX, s.d_w);
  TEST_ASSERT_EQUAL_INT8(4, s.sector);
}

/* Test vectors from sector 1 to sector 6 */

static void test_svm3_s1s6(void)
{
  struct svm3_state_s s;

  /* Vector in sector 1 */

  SVM3(&s, 0.5, 0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.84150, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.65849, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.15849, s.d_w);
  TEST_ASSERT_EQUAL_INT8(1, s.sector);

  /* Vector in sector 2 */

  SVM3(&s, 0.0, 0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.5, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.75, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.25, s.d_w);
  TEST_ASSERT_EQUAL_INT8(2, s.sector);

  /* Vector in sector 3 */

  SVM3(&s, -0.5, 0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.15849, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.84150, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.34150, s.d_w);
  TEST_ASSERT_EQUAL_INT8(3, s.sector);

  /* Vector in sector 4 */

  SVM3(&s, -0.5, -0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.15849, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.34150, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.84150, s.d_w);
  TEST_ASSERT_EQUAL_INT8(4, s.sector);

  /* Vector in sector 5 */

  SVM3(&s, 0.0, -0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.5, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.25, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.75, s.d_w);
  TEST_ASSERT_EQUAL_INT8(5, s.sector);

  /* Vector in sector 6 */

  SVM3(&s, 0.5, -0.5);

  TEST_ASSERT_EQUAL_FLOAT(0.84150, s.d_u);
  TEST_ASSERT_EQUAL_FLOAT(0.15849, s.d_v);
  TEST_ASSERT_EQUAL_FLOAT(0.65849, s.d_w);
  TEST_ASSERT_EQUAL_INT8(6, s.sector);
}

/* Get SVM3 base voltage */

static void test_svm3_vbase(void)
{
  float vbus  = 0.0;
  float vbase = 0.0;

  vbus = 10.0;
  vbase = SVM3_BASE_VOLTAGE_GET(vbus);
  TEST_ASSERT_EQUAL_FLOAT(5.77350, vbase);

  vbus = 78.0;
  vbase = SVM3_BASE_VOLTAGE_GET(vbus);
  TEST_ASSERT_EQUAL_FLOAT(45.0333, vbase);
}

/* Correct ADC samples according to SVM3 state */

static void test_svm3_adc_correct(void)
{
  struct svm3_state_s s;
  int32_t c1 = 0;
  int32_t c2 = 0;
  int32_t c3 = 0;

  /* Sector 1 - ignore phase 1 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, 0.5, 0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(20, c1);
  TEST_ASSERT_EQUAL_INT(10, c2);
  TEST_ASSERT_EQUAL_INT(-30, c3);

  /* Sector 2 - ignore phase 2 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, 0.0, 0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(100, c1);
  TEST_ASSERT_EQUAL_INT(-70, c2);
  TEST_ASSERT_EQUAL_INT(-30, c3);

  /* Sector 3 - ignore phase 2 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, -0.5, 0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(100, c1);
  TEST_ASSERT_EQUAL_INT(-70, c2);
  TEST_ASSERT_EQUAL_INT(-30, c3);

  /* Sector 4 - ignore phase 3 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, -0.5, -0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(100, c1);
  TEST_ASSERT_EQUAL_INT(10, c2);
  TEST_ASSERT_EQUAL_INT(-110, c3);

  /* Sector 5 - ignore phase 3 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, 0.0, -0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(100, c1);
  TEST_ASSERT_EQUAL_INT(10, c2);
  TEST_ASSERT_EQUAL_INT(-110, c3);

  /* Sector 6 - ignore phase 1 */

  c1 = 100;
  c2 = 10;
  c3 = -30;
  SVM3(&s, 0.5, -0.5);
  svm3_current_correct(&s, &c1, &c2, &c3);

  TEST_ASSERT_EQUAL_INT(20, c1);
  TEST_ASSERT_EQUAL_INT(10, c2);
  TEST_ASSERT_EQUAL_INT(-30, c3);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_svm
 ****************************************************************************/

void test_svm(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test 3 phase space vector modulation */

  RUN_TEST(test_svm3_zero);
  RUN_TEST(test_svm3_saturation);
  RUN_TEST(test_svm3_s1s6);
  RUN_TEST(test_svm3_vbase);
  RUN_TEST(test_svm3_adc_correct);

  UNITY_END();
}
