/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_pm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/nuttx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <cmocka.h>
#include <nuttx/power/pm.h>
#include <sys/param.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ASSERT_EQUAL_IF_CHECK(actual, expected, check) \
  { \
    if (check) \
      { \
        assert_int_equal(actual, expected); \
      } \
  }

#define TEST_PM_LOOP_COUNT 1

/* please set num of domain by menuconfig, ensure TEST_DOMAIN is legal
 * (TEST_DOMAIN < CONFIG_PM_NDOMAINS) and no driver or module use it
 */

#define TEST_DOMAIN 0
#define TEST_STAYTIMEOUT 10 /* in ms */
#define TEST_WAITTIME 1000  /* in us */

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int test_pm_callback_prepare(FAR struct pm_callback_s *cb,
                                    int domain,
                                    enum pm_state_e pmstate);
static void test_pm_callback_notify(FAR struct pm_callback_s *cb,
                                    int domain,
                                    enum pm_state_e pmstate);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct test_pm_s
{
  enum pm_state_e state;
  bool prepare_fail;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct pm_callback_s g_test_pm_callback =
{
  .prepare = test_pm_callback_prepare,
  .notify  = test_pm_callback_notify
};

static struct pm_wakelock_s g_test_wakelock[PM_COUNT];
static char test_wakelock_name[PM_COUNT][32] =
{
  "test_pm_wakelock_normal",
  "test_pm_wakelock_idle",
  "test_pm_wakelock_standby",
  "test_pm_wakelock_sleep"
};

static struct test_pm_s g_test_pm_dev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_pm_fake_driver_init(void)
{
  g_test_pm_dev.state = PM_NORMAL;
  g_test_pm_dev.prepare_fail = false;
  return 0;
}

static int test_pm_callback_prepare(FAR struct pm_callback_s *cb,
                                    int domain,
                                    enum pm_state_e pmstate)
{
  if (g_test_pm_dev.prepare_fail)
    {
      return -EBUSY;
    }

  return 0;
}

static void test_pm_callback_notify(FAR struct pm_callback_s *cb,
                                   int domain,
                                   enum pm_state_e pmstate)
{
  switch (pmstate)
    {
      case PM_NORMAL:
      case PM_IDLE:
      case PM_STANDBY:
      case PM_SLEEP:
        {
          g_test_pm_dev.state = pmstate;
        }

        break;

      default:

        break;
    }

  return;
}

static void drivertest_pm(FAR void **argv)
{
  int persist_stay_cnt[PM_COUNT];
  int init_delay;
  int staycount;
  int target;
  bool check;
  int domain = TEST_DOMAIN;
  int ret    = 0;
  int cnt    = TEST_PM_LOOP_COUNT;

  if (CONFIG_PM_GOVERNOR_EXPLICIT_RELAX < 0)
    {
      check = false;
      init_delay = 0;
    }
  else
    {
      check = true;
      init_delay = MAX(CONFIG_PM_GOVERNOR_EXPLICIT_RELAX,
                       CONFIG_SERIAL_PM_ACTIVITY_PRIORITY);
    }

  usleep(init_delay * 1000000);
  usleep(TEST_WAITTIME);

  for (int i = 0; i < PM_COUNT; i++)
    {
      persist_stay_cnt[i] = pm_staycount(domain, i);
    }

  while (cnt--)
    {
      ret = pm_domain_register(domain, &g_test_pm_callback);
      assert_int_equal(ret, 0);

      /* test when pm prepare failed */

      g_test_pm_dev.prepare_fail = true;
      for (int state = 0; state < PM_COUNT; state++)
        {
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_stay(domain, state);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 2;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_relax(domain, state);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
        }

      for (int state = 0; state < PM_COUNT; state++)
        {
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);

          pm_wakelock_stay(&g_test_wakelock[state]);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);

          pm_wakelock_staytimeout(&g_test_wakelock[state], TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 2, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);

          pm_wakelock_relax(&g_test_wakelock[state]);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);

          pm_wakelock_staytimeout(&g_test_wakelock[state], TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);
        }

      /* test when pm prepare succeeded */

      g_test_pm_dev.prepare_fail = false;

      for (int state = 0; state < PM_COUNT; state++)
        {
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_stay(domain, state);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          target = persist_stay_cnt[state] + 2;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_relax(domain, state);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);

          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_IF_CHECK(pm_staycount(domain, state), target, check);
        }

      for (int state = 0; state < PM_COUNT; state++)
        {
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);

          pm_wakelock_stay(&g_test_wakelock[state]);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);

          pm_wakelock_staytimeout(&g_test_wakelock[state], TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 2, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);

          pm_wakelock_relax(&g_test_wakelock[state]);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);

          pm_wakelock_staytimeout(&g_test_wakelock[state], TEST_STAYTIMEOUT);
          usleep(TEST_WAITTIME);
          assert_int_equal(g_test_pm_dev.state, state);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), state, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 1, check);
          usleep(TEST_STAYTIMEOUT * 1000);
          assert_int_equal(g_test_pm_dev.state, PM_SLEEP);
          ASSERT_EQUAL_IF_CHECK(pm_querystate(domain), PM_SLEEP, check);
          staycount = pm_wakelock_staycount(&g_test_wakelock[state]);
          ASSERT_EQUAL_IF_CHECK(staycount, 0, check);
        }

      ret = pm_domain_unregister(domain, &g_test_pm_callback);
      assert_int_equal(ret, 0);
    }
}

static int setup(FAR void **argv)
{
  int domain = TEST_DOMAIN;
  int ret = 0;

  ret = test_pm_fake_driver_init();
  if (ret < 0)
    {
      return ret;
    }

  for (int state = 0; state < PM_COUNT; state++)
    {
      pm_wakelock_init(&g_test_wakelock[state],
                       test_wakelock_name[state],
                       domain,
                       state);
    }

  return ret;
}

static int teardown(FAR void **argv)
{
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(drivertest_pm, setup,
                                               teardown, NULL),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
