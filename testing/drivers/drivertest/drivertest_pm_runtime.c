/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_pm_runtime.c
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>
#include <nuttx/power/pm_runtime.h>

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int test_pm_runtime_suspend(FAR struct pm_runtime_s *rpm);
static int test_pm_runtime_resume(FAR struct pm_runtime_s *rpm);
struct test_pm_runtime_s
{
  struct pm_runtime_s rpm;
  int state;
};

enum
{
  TEST_PM_RUTIME_FAKE_SUSPEND = 0,
  TEST_PM_RUTIME_FAKE_RESUME,
  TEST_PM_RUTIME_FAKE_UNKOWN,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct pm_runtime_ops_s g_test_pm_runtime_ops =
{
  test_pm_runtime_suspend,
  test_pm_runtime_resume,
};

static struct test_pm_runtime_s g_test_pm_runtime_dev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_pm_runtime_fake_driver_init(void)
{
  g_test_pm_runtime_dev.state = TEST_PM_RUTIME_FAKE_SUSPEND;
  pm_runtime_init(&g_test_pm_runtime_dev.rpm, RPM_SUSPENDED,
                  &g_test_pm_runtime_ops);
  return 0;
}

static int test_pm_runtime_suspend(FAR struct pm_runtime_s *dev)
{
  struct test_pm_runtime_s *tdev =
                         container_of(dev, struct test_pm_runtime_s, rpm);
  tdev->state = TEST_PM_RUTIME_FAKE_SUSPEND;
  return 0;
}

static int test_pm_runtime_resume(FAR struct pm_runtime_s *dev)
{
  struct test_pm_runtime_s *tdev =
                         container_of(dev, struct test_pm_runtime_s, rpm);
  tdev->state = TEST_PM_RUTIME_FAKE_RESUME;
  return 0;
}

static void drivertest_pm_runtime(FAR void **state)
{
  int ret = 0;
  int cnt = 10;

  while (cnt--)
    {
      ret = pm_runtime_get(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_RESUME);
      ret = pm_runtime_put(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_SUSPEND);
      ret = pm_runtime_put(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, -EPERM);
      pm_runtime_set_autosuspend_delay(&g_test_pm_runtime_dev.rpm, 200);
      ret = pm_runtime_get(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_RESUME);
      ret = pm_runtime_put_autosuspend(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_RESUME);
      usleep(210 * 1000);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_SUSPEND);
      ret = pm_runtime_get(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_RESUME);
      ret = pm_runtime_put_autosuspend(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      ret = pm_runtime_get(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_RESUME);
       pm_runtime_set_autosuspend_delay(&g_test_pm_runtime_dev.rpm, 0);
      ret = pm_runtime_put_autosuspend(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, 0);
      usleep(10 * 1000);
      assert_int_equal(g_test_pm_runtime_dev.state,
                       TEST_PM_RUTIME_FAKE_SUSPEND);
      ret = pm_runtime_put(&g_test_pm_runtime_dev.rpm);
      assert_int_equal(ret, -EPERM);
    }

  return;
}

static int setup(FAR void **state)
{
  return test_pm_runtime_fake_driver_init();
}

static int teardown(FAR void **state)
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
      cmocka_unit_test_prestate_setup_teardown(drivertest_pm_runtime, setup,
                                               teardown, NULL),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
