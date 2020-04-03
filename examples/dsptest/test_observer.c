/****************************************************************************
 * examples/dsptest/test_observer.c
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

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Dummy angle observer */

struct motor_observer_dummy_s
{
  float x;
};

/* Dummy speed observer */

struct motor_sobserver_dummy_s
{
  float x;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Initialize observer data */

static void test_observer_init(void)
{
  struct motor_observer_dummy_s  ao;
  struct motor_sobserver_dummy_s so;
  struct motor_observer_s        o;
  float per = 1e-11;

  motor_observer_init(&o, (void *)&ao, (void *)&so, per);

  TEST_ASSERT_EQUAL_FLOAT(0.0, o.angle);
  TEST_ASSERT_EQUAL_FLOAT(0.0, o.speed);
  TEST_ASSERT_EQUAL_FLOAT(per, o.per);
  TEST_ASSERT_NOT_NULL(o.ao);
  TEST_ASSERT_NOT_NULL(o.so);
}

/* Initialize SMO observer data */

static void test_observer_smo_init(void)
{
  struct motor_observer_smo_s ao;
  float kslide  = 0.99;
  float err_max = 0.99;

  /* Initialize SMO observer */

  motor_observer_smo_init(&ao, kslide, err_max);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(kslide, ao.k_slide);
  TEST_ASSERT_EQUAL_FLOAT(err_max, ao.err_max);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.F);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.G);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.emf_lp_filter1);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.emf_lp_filter2);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.emf.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.emf.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.z.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.z.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.i_est.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.i_est.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.v_err.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.v_err.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.i_err.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.i_err.b);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.sign.a);
  TEST_ASSERT_EQUAL_FLOAT(0.0, ao.sign.b);
}

/* Feed SMO observer with zeros */

static void test_observer_smo_zeros(void)
{
  struct motor_sobserver_dummy_s so;
  struct motor_observer_smo_s    ao;
  struct motor_observer_s        o;
  struct motor_phy_params_s      phy;
  ab_frame_t i_ab;
  ab_frame_t v_ab;
  float      k_slide = 1.0;
  float      err_max = 1.0;
  float      per     = 1e-6;
  float      angle   = 0.0;
  int        i       = 0;

  /* Initialize ab frames */

  i_ab.a = 0.0f;
  i_ab.b = 0.0f;
  v_ab.a = 0.0f;
  v_ab.b = 0.0f;

  /* Initialize motor physical parameters */

  phy.p   = 8;
  phy.res = 0.001;
  phy.ind = 10e-6;

  /* Initialize SMO angle observer */

  motor_observer_smo_init(&ao, k_slide, err_max);

  /* Initialize observer */

  motor_observer_init(&o, &ao, &so, per);

  /* Feed SMO observer with zeros. This should return
   * angle equal to observer phase shift (direction * PI)
   */

  i_ab.a = 0.0;
  i_ab.b = 0.0;
  v_ab.a = 0.0;
  v_ab.b = 0.0;

  for (i = 0; i < 1000000; i += 1)
    {
      motor_observer_smo(&o, &i_ab, &v_ab, &phy, DIR_CW);
    }

  angle = motor_observer_angle_get(&o);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(M_PI_F, angle);

  for (i = 0; i < 1000000; i += 1)
    {
      motor_observer_smo(&o, &i_ab, &v_ab, &phy, DIR_CCW);
    }

  angle = motor_observer_angle_get(&o);

  /* Test. NOTE: -M_PI_F normalised = 0.0 */

  TEST_ASSERT_FLOAT_WITHIN(5e-6, 0.0, angle);
}

/* Feed SMO in CW direction */

static void test_observer_smo_cw(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/* Feed SMO in CCW direction */

static void test_observer_smo_ccw(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/* SMO observer in linear region */

static void test_observer_smo_linear(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/* SMO observer gain saturation */

static void test_observer_smo_gain_saturate(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/* DIV speed observer initialization */

static void test_sobserver_div_init(void)
{
  struct motor_sobserver_div_s so;
  uint8_t samples = 10;
  float filter    = 0.1;
  float per       = 10e-6;

  /* Initialize DIV speed observer */

  motor_sobserver_div_init(&so, samples, filter, per);

  TEST_ASSERT_EQUAL_FLOAT(filter, so.filter);
  TEST_ASSERT_EQUAL_FLOAT(1.0 / (samples * per), so.one_by_dt);
  TEST_ASSERT_EQUAL_UINT8(samples, so.samples);
}

/* Feed DIV speed observer with zeros */

static void test_sobserver_div_zeros(void)
{
  struct motor_sobserver_div_s  so;
  struct motor_observer_s       o;
  struct motor_observer_dummy_s ao;
  uint8_t samples    = 10;
  float   filter     = 0.1;
  int     i          = 0;
  float   expected_s = 0.0;
  float   speed      = 0.0;
  float   per        = 1e-6;

  /* Initialize DIV speed observer */

  motor_sobserver_div_init(&so, samples, filter, per);

  /* Initialize observer */

  motor_observer_init(&o, &ao, &so, per);

  /* Feed observer with zeros in CW direction */

  expected_s = 0.0;

  for (i = 0; i < 1000; i += 1)
    {
      motor_sobserver_div(&o, 0.0, DIR_CW);
    }

  speed = motor_observer_speed_get(&o);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_s, speed);

  /* Feed observer with zeros in CCW direction */

  expected_s = 0.0;

  for (i = 0; i < 1000; i += 1)
    {
      motor_sobserver_div(&o, 0.0, DIR_CCW);
    }

  speed = motor_observer_speed_get(&o);

  /* Test */

  TEST_ASSERT_EQUAL_FLOAT(expected_s, speed);
}

/* Feed DIV speed observer in CW direction */

static void test_sobserver_div_cw(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/* Feed DIV speed observer in CCW direction */

static void test_sobserver_div_ccw(void)
{
  /* TODO */

  TEST_FAIL_MESSAGE("not implemented");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_observer
 ****************************************************************************/

void test_observer(void)
{
  UNITY_BEGIN();

  TEST_SEPARATOR();

  /* Test observer functions */

  RUN_TEST(test_observer_init);

  /* SMO observer */

  RUN_TEST(test_observer_smo_init);
  RUN_TEST(test_observer_smo_zeros);
  RUN_TEST(test_observer_smo_linear);
  RUN_TEST(test_observer_smo_gain_saturate);
  RUN_TEST(test_observer_smo_cw);
  RUN_TEST(test_observer_smo_ccw);

  /* DIV observer */

  RUN_TEST(test_sobserver_div_init);
  RUN_TEST(test_sobserver_div_zeros);
  RUN_TEST(test_sobserver_div_cw);
  RUN_TEST(test_sobserver_div_ccw);

  UNITY_END();
}
