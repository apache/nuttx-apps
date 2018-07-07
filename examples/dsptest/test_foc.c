/****************************************************************************
 * examples/dsptest/test_foc.c
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

#if CONFIG_LIBDSP_PRECISION == 1
#  define UNITY_FLOAT_PRECISION   (0.1f)
#elif CONFIG_LIBDSP_PRECISION == 2
#  define UNITY_FLOAT_PRECISION   (0.001f)
#else
#  define UNITY_FLOAT_PRECISION   (0.999f)
#endif

/* Value close enough to zero */

#define TEST_ASSERT_EQUAL_FLOAT_ZERO(a)         \
  TEST_ASSERT_FLOAT_WITHIN(1e-6, 0.0, a);

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

/* Initialize FOC data */

static void test_foc_init(void)
{
  struct foc_data_s foc;
  float id_kp = 1.0;
  float id_ki = 2.0;
  float iq_kp = 3.0;
  float iq_ki = 4.0;

  foc_init(&foc, id_kp, id_ki, iq_kp, iq_ki);

  TEST_ASSERT_EQUAL_FLOAT(1.0, foc.id_pid.KP);
  TEST_ASSERT_EQUAL_FLOAT(2.0, foc.id_pid.KI);
  TEST_ASSERT_EQUAL_FLOAT(0.0, foc.id_pid.KD);
  TEST_ASSERT_EQUAL_FLOAT(3.0, foc.iq_pid.KP);
  TEST_ASSERT_EQUAL_FLOAT(4.0, foc.iq_pid.KI);
  TEST_ASSERT_EQUAL_FLOAT(0.0, foc.iq_pid.KD);

  foc_idq_ref_set(&foc, 1.0, 2.0);

  TEST_ASSERT_EQUAL_FLOAT(1.0, foc.i_dq_ref.d);
  TEST_ASSERT_EQUAL_FLOAT(2.0, foc.i_dq_ref.q);

  foc_vbase_update(&foc, 0.1);

  TEST_ASSERT_EQUAL_FLOAT(0.1, 1.0/foc.vab_mod_scale);
  TEST_ASSERT_EQUAL_FLOAT(0.1, foc.vdq_mag_max);
}

/* Feed FOC with zeros */

static void test_foc_process_zeros(void)
{
  struct foc_data_s foc;
  float id_kp = 1.00;
  float id_ki = 0.01;
  float iq_kp = 1.00;
  float iq_ki = 0.01;
  abc_frame_t i_abc;
  phase_angle_t angle;

  /* Initialize FOC */

  foc_init(&foc, id_kp, id_ki, iq_kp, iq_ki);

  /* Input:
   *   abc         = (0.0, 0.0, 0.0)
   *   i_dq_ref    = (1.0, 1.0)
   *   vab_scale   = 1.0
   *   vdq_mag_max = 1.0
   *   angle       = 0.0
   *
   * Expected:
   *   i_ab     = (0.0, 0.0)
   *   i_dq     = (0.0, 0.0)
   *   v_dq     = (0.0, 0.0)
   *   v_ab     = (0.0, 0.0)
   *   v_ab_mod = (0.0, 0.0)
   */

  i_abc.a = 0.0;
  i_abc.b = 0.0;
  i_abc.c = 0.0;
  phase_angle_update(&angle, 0.0);
  foc_idq_ref_set(&foc, 0.0, 0.0);
  foc_vbase_update(&foc, 1.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, -0.5, -0.5)
   *   i_dq_ref    = (1.0, 0.0)
   *   vab_scale   = 1.0
   *   vdq_mag_max = 1.0
   *   angle       = 0.0
   *
   * Expected:
   *   i_ab     = (1.0, 0.0)
   *   i_dq     = (1.0, 0.0)
   *   v_dq     = (0.0, 0.0)
   *   v_ab     = (0.0, 0.0)
   *   v_ab_mod = (0.0, 0.0)
   */

  i_abc.a = 1.0;
  i_abc.b = -0.5;
  i_abc.c = -0.5;
  foc_idq_ref_set(&foc, 1.0, 0.0);
  foc_vbase_update(&foc, 1.0);
  phase_angle_update(&angle, 0.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.b);

  /* Input:
   *   abc         = (-1.0, 0.5, 0.5)
   *   i_dq_ref    = (-1.0, 0.0)
   *   vab_scale   = 1.0
   *   vdq_mag_max = 1.0
   *   angle       = 0.0
   *
   * Expected:
   *   i_ab     = (-1.0, 0.0)
   *   i_dq     = (-1.0, 0.0)
   *   v_dq     = (0.0, 0.0)
   *   v_ab     = (0.0, 0.0)
   *   v_ab_mod = (0.0, 0.0)
   */

  i_abc.a = -1.0;
  i_abc.b = 0.5;
  i_abc.c = 0.5;
  foc_idq_ref_set(&foc, -1.0, 0.0);
  foc_vbase_update(&foc, 1.0);
  phase_angle_update(&angle, 0.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT_ZERO(foc.v_ab_mod.b);
}

/* Proces FOC with some test data */

static void test_foc_process(void)
{
  struct foc_data_s foc;
  float id_kp = 1.00;
  float id_ki = 0.00;
  float iq_kp = 1.00;
  float iq_ki = 0.00;
  abc_frame_t i_abc;
  phase_angle_t angle;
  float i_d_ref = 0.0;
  float i_q_ref = 0.0;

  /* Initialize FOC.
   * For simplicity KP = 1.0, KI = 0.0
   */

  foc_init(&foc, id_kp, id_ki, iq_kp, iq_ki);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (2.0, 2.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10.0
   *   angle       = PI
   *
   * Expected:
   *   i_ab     = (1.0, 1.7320)
   *   i_dq     = (-1.0, -1.7320)
   *   v_dq     = (3.0, 3.7320)
   *   v_ab     = (0.30, 0.37320)
   *   v_ab_mod = (0.30, 0.37320)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = 2.0;
  i_q_ref = 2.0;
  phase_angle_update(&angle, M_PI_F);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0    , foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320 , foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-1.0   , foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-1.7320, foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(3.0    , foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(3.7320 , foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-3.0   , foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-3.7320, foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-0.3   , foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(-0.3732, foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (2.0, 2.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10
   *   angle       = -PI
   *
   * Expected:
   *   i_ab     = (1.0, 1.7320)
   *   i_dq     = (-1.0, -1.7320)
   *   v_dq     = (3.0, 3.7320)
   *   v_ab     = (-0.30, -0.37320)
   *   v_ab_mod = (-0.30, -0.37320)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = 2.0;
  i_q_ref = 2.0;
  phase_angle_update(&angle, -M_PI_F);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0    , foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320 , foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-1.0   , foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-1.7320, foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(3.0    , foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(3.7320 , foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-3.0   , foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-3.7320, foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-0.30  , foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(-0.3732, foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (2.0, 2.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10
   *   angle       = 0.1
   *
   * Expected:
   *   i_ab     = (1.0, 1.7320)
   *   i_dq     = (1.1679, 1.6236)
   *   v_dq     = (0.8321, 0.3764)
   *   v_ab     = (0.7903, 0.4576)
   *   v_ab_mod = (0.07903, 0.04576)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = 2.0;
  i_q_ref = 2.0;
  phase_angle_update(&angle, 0.1);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0    , foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320 , foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(1.1679 , foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.6236 , foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(0.8321 , foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(0.3764 , foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(0.7903 , foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(0.4576 , foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(0.07903, foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(0.04576, foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (2.0, 2.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10.0
   *   angle       = -0.1
   *
   * Expected:
   *   i_ab     = (1.0   , 1.7320)
   *   i_dq     = (0.8221, 1.8232)
   *   v_dq     = (1.1779, 0.1768)
   *   v_ab     = (0.11897, 0.00583)
   *   v_ab_mod = (0.11897, 0.00583)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = 2.0;
  i_q_ref = 2.0;
  phase_angle_update(&angle, -0.1);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0, foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320, foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(0.8221, foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.8232, foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(1.1779, foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(0.1768, foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(1.1897, foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0583, foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(0.11897, foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(0.00583, foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (-2.0, 0.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10.0
   *   angle       = 0.1
   *
   * Expected:
   *   i_ab     = (1.0, 1.7320)
   *   i_dq     = (1.1679, 1.6236)
   *   v_dq     = (-3.1679, -1.6236)
   *   v_ab     = (-0.29900, -0.19317)
   *   v_ab_mod = (-0.29900, -0.19317)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = -2.0;
  i_q_ref = 0.0;
  phase_angle_update(&angle, 0.1);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0    , foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320 , foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(1.1679 , foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.6236 , foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-3.1679 , foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-1.6236 , foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-2.9900 , foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-1.9317 , foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-0.29900 , foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(-0.19317 , foc.v_ab_mod.b);

  /* Input:
   *   abc         = (1.0, 1.0, -2.0)
   *   i_dq_ref    = (-2.0, -2.0)
   *   vab_scale   = 0.1
   *   vdq_mag_max = 10.0
   *   angle       = 0.1
   *
   * Expected:
   *   i_ab     = (1.0, 1.7320)
   *   i_dq     = (1.1679, 1.6236)
   *   v_dq     = (-3.1679, -3.6236)
   *   v_ab     = (-0.27903, -0.39217)
   *   v_ab_mod = (-0.27903, -0.39217)
   */

  i_abc.a = 1.0;
  i_abc.b = 1.0;
  i_abc.c = -2.0;
  i_d_ref = -2.0;
  i_q_ref = -2.0;
  phase_angle_update(&angle, 0.1);
  foc_idq_ref_set(&foc, i_d_ref, i_q_ref);
  foc_vbase_update(&foc, 10.0);

  foc_process(&foc, &i_abc, &angle);

  TEST_ASSERT_EQUAL_FLOAT(1.0    , foc.i_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(1.7320 , foc.i_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(1.1679 , foc.i_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(1.6236 , foc.i_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-3.1679 , foc.v_dq.d);
  TEST_ASSERT_EQUAL_FLOAT(-3.6236 , foc.v_dq.q);
  TEST_ASSERT_EQUAL_FLOAT(-2.7903 , foc.v_ab.a);
  TEST_ASSERT_EQUAL_FLOAT(-3.9217 , foc.v_ab.b);
  TEST_ASSERT_EQUAL_FLOAT(-0.27903, foc.v_ab_mod.a);
  TEST_ASSERT_EQUAL_FLOAT(-0.39217, foc.v_ab_mod.b);
}

/* Test FOC saturation */

static void test_foc_saturation(void)
{
  TEST_FAIL_MESSAGE("not implemented");
}

/* Test FOC vdq modulation */

static void test_foc_vdq_modulation(void)
{
  TEST_FAIL_MESSAGE("not implemented");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_foc
 ****************************************************************************/

void test_foc(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test FOC */

  RUN_TEST(test_foc_init);
  RUN_TEST(test_foc_process_zeros);
  RUN_TEST(test_foc_process);
  RUN_TEST(test_foc_saturation);
  RUN_TEST(test_foc_vdq_modulation);

  UNITY_END();
}
