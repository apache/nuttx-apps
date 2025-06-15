/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_pm_smp.c
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
#include <pthread.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_PM_LOOP_COUNT   5

#define TEST_STAYTIMEOUT     2   /* in ms */

#define TEST_SNAP_TICK_MAX   3

#define TEST_TIMEOUT_ENABLED 1

#define ASSERT_EQUAL_SUBTHREAD(actual, expected) \
  { \
    if ((actual) != (expected)) \
      { \
        ret = __LINE__; \
        goto error_out; \
      } \
  }

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

struct test_pm_entry_s
{
  pid_t tid;
  int cpu;
  int domain;
  cpu_set_t cpuset;
  bool fail_request;
  enum pm_state_e state;
  struct pm_callback_s cb;
  struct pm_wakelock_s wakelock[PM_COUNT];
};

struct test_pm_s
{
  struct test_pm_entry_s entry[CONFIG_SMP_NCPUS];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *test_pm_name[PM_COUNT] =
{
  "normal",
  "idle",
  "standby",
  "sleep"
};

static struct test_pm_s g_test_pm_smp;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_pm_fake_driver_init(void)
{
  memset(&g_test_pm_smp, 0, sizeof(g_test_pm_smp));
  for (int i = 0; i < CONFIG_SMP_NCPUS; i++)
    {
      struct test_pm_entry_s *entry = &g_test_pm_smp.entry[i];
      entry->cpu                    = i;
      CPU_ZERO(&entry->cpuset);
      CPU_SET(i, &entry->cpuset);
      entry->domain     = CONFIG_PM_NDOMAINS - CONFIG_SMP_NCPUS + i;
      entry->cb.prepare = test_pm_callback_prepare;
      entry->cb.notify  = test_pm_callback_notify;

      for (int state = 0; state < PM_COUNT; state++)
        {
          char name[32];
          snprintf(name, sizeof(name), "cpu%d-%s", i, test_pm_name[state]);
          pm_wakelock_init(
              &entry->wakelock[state], name, entry->domain, state);
        }
    }

  return 0;
}

static int test_pm_callback_prepare(FAR struct pm_callback_s *cb,
                                    int domain,
                                    enum pm_state_e pmstate)
{
  struct test_pm_entry_s *entry;
  entry = container_of(cb, struct test_pm_entry_s, cb);
  if (entry->fail_request)
    {
      return -EBUSY;
    }

  return 0;
}

static void test_pm_callback_notify(FAR struct pm_callback_s *cb,
                                    int domain,
                                    enum pm_state_e pmstate)
{
  struct test_pm_entry_s *entry;
  entry = container_of(cb, struct test_pm_entry_s, cb);
  switch (pmstate)
    {
      case PM_NORMAL:
      case PM_IDLE:
      case PM_STANDBY:
      case PM_SLEEP:
        entry->state = pmstate;
        break;

      default:
        break;
    }

  return;
}

#if TEST_TIMEOUT_ENABLED

/* Make sure the Idle can execute */

static void test_pm_smp_yield(void)
{
  /* two ticks to avoid very quick jump out and idle cannot run one cycle */

  usleep(USEC_PER_TICK * 2);
}
#endif

static void test_pm_smp_naps(void)
{
  int r = random() % (TEST_SNAP_TICK_MAX * USEC_PER_TICK);
  usleep(r + USEC_PER_TICK * 2);
}

static void *test_pm_smp_thread_entry(void *arg)
{
  struct test_pm_entry_s *entry = (struct test_pm_entry_s *)arg;

  int persist_stay_cnt[PM_COUNT];
  int init_delay;
  int staycount;
  int target;
  int cnt    = TEST_PM_LOOP_COUNT;
  int domain = entry->domain;
  int ret    = 0;

  init_delay = MAX(CONFIG_PM_GOVERNOR_EXPLICIT_RELAX,
                   CONFIG_SERIAL_PM_ACTIVITY_PRIORITY);

  usleep(init_delay * 1000000);
  test_pm_smp_naps();

  for (int i = 0; i < PM_COUNT; i++)
    {
      persist_stay_cnt[i] = pm_staycount(domain, i);
    }

  while (cnt--)
    {
      ret = pm_domain_register(domain, &entry->cb);
      ASSERT_EQUAL_SUBTHREAD(ret, 0);

      /* test when pm prepare failed */

      entry->fail_request = true;
      for (int state = 0; state < PM_COUNT; state++)
        {
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

          pm_stay(domain, state);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

#if TEST_TIMEOUT_ENABLED
          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 2;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
#endif

          pm_relax(domain, state);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

#if TEST_TIMEOUT_ENABLED
          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
#endif
        }

      for (int state = 0; state < PM_COUNT; state++)
        {
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);

          pm_wakelock_stay(&entry->wakelock[state]);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);

#if TEST_TIMEOUT_ENABLED
          pm_wakelock_staytimeout(&entry->wakelock[state], TEST_STAYTIMEOUT);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 2);
          usleep(TEST_STAYTIMEOUT * 1000);
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);
#endif

          pm_wakelock_relax(&entry->wakelock[state]);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);

#if TEST_TIMEOUT_ENABLED
          pm_wakelock_staytimeout(&entry->wakelock[state], TEST_STAYTIMEOUT);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);
#endif
        }

      /* test when pm prepare succeeded */

      entry->fail_request = false;

      for (int state = 0; state < PM_COUNT; state++)
        {
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

          pm_stay(domain, state);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

#if TEST_TIMEOUT_ENABLED
          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          target = persist_stay_cnt[state] + 2;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
#endif

          pm_relax(domain, state);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);

#if TEST_TIMEOUT_ENABLED
          pm_staytimeout(domain, state, TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          target = persist_stay_cnt[state] + 1;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          target = persist_stay_cnt[state] + 0;
          ASSERT_EQUAL_SUBTHREAD(pm_staycount(domain, state), target);
#endif
        }

      for (int state = 0; state < PM_COUNT; state++)
        {
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);

          pm_wakelock_stay(&entry->wakelock[state]);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);

#if TEST_TIMEOUT_ENABLED
          pm_wakelock_staytimeout(&entry->wakelock[state], TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 2);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);
#endif

          pm_wakelock_relax(&entry->wakelock[state]);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);

#if TEST_TIMEOUT_ENABLED
          pm_wakelock_staytimeout(&entry->wakelock[state], TEST_STAYTIMEOUT);
          test_pm_smp_yield();
          ASSERT_EQUAL_SUBTHREAD(entry->state, state);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), state);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 1);
          usleep(TEST_STAYTIMEOUT * 1000);
          test_pm_smp_naps();
          ASSERT_EQUAL_SUBTHREAD(entry->state, PM_SLEEP);
          ASSERT_EQUAL_SUBTHREAD(pm_querystate(domain), PM_SLEEP);
          staycount = pm_wakelock_staycount(&entry->wakelock[state]);
          ASSERT_EQUAL_SUBTHREAD(staycount, 0);
#endif
        }

      ret = pm_domain_unregister(domain, &entry->cb);
      ASSERT_EQUAL_SUBTHREAD(ret, 0);
    }

error_out:
  return (void *)(uintptr_t)ret;
}

static void drivertest_pm_smp(FAR void **argv)
{
  for (int i = 0; i < CONFIG_SMP_NCPUS; i++)
    {
      struct test_pm_entry_s *entry = &g_test_pm_smp.entry[i];
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &entry->cpuset);
      attr.priority = CONFIG_SCHED_HPWORKPRIORITY + 1;
      pthread_create(&entry->tid, &attr, test_pm_smp_thread_entry, entry);
    }

  for (int i = 0; i < CONFIG_SMP_NCPUS; i++)
    {
      struct test_pm_entry_s *entry = &g_test_pm_smp.entry[i];
      pthread_addr_t pthread_ret;
      int ret;
      pthread_join(entry->tid, &pthread_ret);
      ret = (uintptr_t)pthread_ret;
      assert_int_equal(ret, 0);
    }
}

static int setup(FAR void **argv)
{
  int ret = 0;

  if (CONFIG_PM_GOVERNOR_EXPLICIT_RELAX < 0)
    {
      return -1;
    }

  ret = test_pm_fake_driver_init();
  if (ret < 0)
    {
      return ret;
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
      cmocka_unit_test_prestate_setup_teardown(drivertest_pm_smp, setup,
                                               teardown, NULL),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
