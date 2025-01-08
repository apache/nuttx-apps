/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_regulator.c
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
#include <cmocka.h>
#include <nuttx/power/consumer.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define REGULATOR_ID        "fake_regulator"
#define REGULATOR_SUPPLY_ID "fake_regulator_supply"

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int test_regulator_enable(FAR struct regulator_dev_s *rdev);
static int test_regulator_is_enabled(FAR struct regulator_dev_s *rdev);
static int test_regulator_disable(FAR struct regulator_dev_s *rdev);
static int test_regulator_set_mode(FAR struct regulator_dev_s *rdev,
                                   enum regulator_mode_e mode);
static enum regulator_mode_e
test_regulator_get_mode(FAR struct regulator_dev_s *rdev);
static int test_regulator_suspend_mode(FAR struct regulator_dev_s *rdev,
                                       enum regulator_mode_e mode);
static int test_regulator_suspend_voltage(FAR struct regulator_dev_s *,
                                          int uv);
static int test_regulator_resume(FAR struct regulator_dev_s *rdev);

struct test_regulator_s
{
  FAR struct regulator_dev_s *rdev;
  int state;
  enum regulator_mode_e lpmode;
  int lpuv;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct regulator_ops_s g_fake_regulator_ops =
{
  .enable              = test_regulator_enable,        /* enable */
  .is_enabled          = test_regulator_is_enabled,    /* is_enabled */
  .disable             = test_regulator_disable,       /* disable */
  .set_mode            = test_regulator_set_mode,
  .get_mode            = test_regulator_get_mode,
  .set_suspend_mode    = test_regulator_suspend_mode,
  .set_suspend_voltage = test_regulator_suspend_voltage,
  .resume              = test_regulator_resume,
};

static struct regulator_desc_s g_fake_regulator_desc =
{
  .name = REGULATOR_ID,
  .boot_on = 0,
  .always_on = 1,
};

static struct test_regulator_s g_fake_regulator =
{
  .rdev = NULL,
  .state = 0,
};

static struct regulator_desc_s g_fake_regulator_supply_desc =
{
  .name = REGULATOR_SUPPLY_ID,
  .boot_on = 0,
  .always_on = 0,
};

static struct test_regulator_s g_fake_regulator_supply =
{
  .rdev = NULL,
  .state = 0,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_regulator_enable(FAR struct regulator_dev_s *rdev)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  tr->state = 1;
  return OK;
}

static int test_regulator_is_enabled(FAR struct regulator_dev_s *rdev)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  return tr->state;
}

static int test_regulator_disable(FAR struct regulator_dev_s *rdev)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  tr->state = 0;
  return OK;
}

static int test_regulator_set_mode(FAR struct regulator_dev_s *rdev,
                                   enum regulator_mode_e mode)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  int ret = 0;

  switch (mode)
    {
      case REGULATOR_MODE_FAST:
      case REGULATOR_MODE_NORMAL:
      case REGULATOR_MODE_IDLE:
      case REGULATOR_MODE_STANDBY:
        tr->lpmode = mode;
        break;
      default:
        ret = -EPERM;
    }

  return ret;
}

static enum regulator_mode_e
test_regulator_get_mode(FAR struct regulator_dev_s *rdev)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  return tr->lpmode;
}

static int test_regulator_suspend_mode(FAR struct regulator_dev_s *rdev,
                                       enum regulator_mode_e mode)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  int ret = 0;

  switch (mode)
    {
      case REGULATOR_MODE_FAST:
      case REGULATOR_MODE_NORMAL:
      case REGULATOR_MODE_IDLE:
      case REGULATOR_MODE_STANDBY:
        tr->lpmode = mode;
          break;
      default:
        ret = -EPERM;
    }

  return ret;
}

static int test_regulator_suspend_voltage(FAR struct regulator_dev_s *rdev,
                                          int uv)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  tr->lpuv = uv;
  return 0;
}

static int test_regulator_resume(FAR struct regulator_dev_s *rdev)
{
  FAR struct test_regulator_s *tr = rdev->priv;
  tr->lpmode = REGULATOR_MODE_NORMAL;
  return 0;
}

static void drivertest_reg_register(FAR void **state)
{
  FAR struct regulator_dev_s *test = NULL;

  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  test = regulator_register(&g_fake_regulator_desc,
                            &g_fake_regulator_ops,
                            &g_fake_regulator);
  assert_true(NULL == test);
  regulator_unregister(g_fake_regulator.rdev);
}

static void drivertest_reg_always_on(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int ret = 0;
  int cnt = 10;

  g_fake_regulator_desc.boot_on = 0;
  g_fake_regulator_desc.always_on = 1;
  g_fake_regulator_desc.supply_name = NULL;
  g_fake_regulator.state = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);

  assert_int_equal(g_fake_regulator.state, 1);
  while (cnt--)
    {
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      ret = regulator_disable(test);
      assert_int_equal(ret, -EIO);
    }

  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  return;
}

static void drivertest_reg_supply_always_on_boot_off(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int cnt = 10;
  int ret = 0;

  g_fake_regulator_desc.supply_name = REGULATOR_SUPPLY_ID;
  g_fake_regulator_desc.boot_on = 0;
  g_fake_regulator_desc.always_on = 1;
  g_fake_regulator.state = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_true(NULL == g_fake_regulator.rdev);
  g_fake_regulator_supply.state = 0;
  g_fake_regulator_supply.rdev = regulator_register(
                                             &g_fake_regulator_supply_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator_supply);
  assert_false(NULL == g_fake_regulator_supply.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 0);
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 1);
  assert_int_equal(g_fake_regulator.state, 1);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);
  while (cnt--)
    {
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_int_equal(ret, -EIO);
    }

  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator.rdev = NULL;
  regulator_unregister(g_fake_regulator_supply.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

static void drivertest_reg_supply_boot_on_always_on(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int cnt = 10;
  int ret = 0;

  g_fake_regulator_desc.supply_name = REGULATOR_SUPPLY_ID;
  g_fake_regulator_desc.boot_on = 1;
  g_fake_regulator_desc.always_on = 1;
  g_fake_regulator.state = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_true(NULL == g_fake_regulator.rdev);
  g_fake_regulator_supply.state = 0;
  g_fake_regulator_supply.rdev = regulator_register(
                                             &g_fake_regulator_supply_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator_supply);
  assert_false(NULL == g_fake_regulator_supply.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 0);
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 1);
  assert_int_equal(g_fake_regulator.state, 1);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);
  while (cnt--)
    {
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_int_equal(ret, -EIO);
    }

  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator.rdev = NULL;
  regulator_unregister(g_fake_regulator_supply.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

static void drivertest_reg_supply_boot_on_always_off(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int cnt = 10;
  int ret = 0;

  g_fake_regulator_desc.supply_name = REGULATOR_SUPPLY_ID;
  g_fake_regulator_desc.boot_on = 1;
  g_fake_regulator_desc.always_on = 0;
  g_fake_regulator.state = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_true(NULL == g_fake_regulator.rdev);
  g_fake_regulator_supply.state = 0;
  g_fake_regulator_supply.rdev = regulator_register(
                                             &g_fake_regulator_supply_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator_supply);
  assert_false(NULL == g_fake_regulator_supply.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 0);
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 1);
  assert_int_equal(g_fake_regulator.state, 1);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);
  while (cnt--)
    {
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 0);
      ret = regulator_disable(test);
      assert_int_equal(ret, -EIO);
    }

  ret = regulator_enable(test);
  assert_false(ret < 0);
  assert_int_equal(g_fake_regulator.state, 1);
  assert_int_equal(g_fake_regulator_supply.state, 1);

  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator.rdev = NULL;
  regulator_unregister(g_fake_regulator_supply.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

static void drivertest_reg_supply_boot_off_always_off(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int cnt = 10;
  int ret = 0;

  g_fake_regulator_desc.supply_name = REGULATOR_SUPPLY_ID;
  g_fake_regulator_desc.boot_on = 0;
  g_fake_regulator_desc.always_on = 0;
  g_fake_regulator.state = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  test = regulator_get(REGULATOR_ID);
  assert_true(NULL == test);
  g_fake_regulator_supply.state = 0;
  g_fake_regulator_supply.rdev = regulator_register(
                                             &g_fake_regulator_supply_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator_supply);
  assert_false(NULL == g_fake_regulator_supply.rdev);
  assert_int_equal(g_fake_regulator_supply.state, 0);
  assert_int_equal(g_fake_regulator.state, 0);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);
  while (cnt--)
    {
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_enable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 1);
      assert_int_equal(g_fake_regulator_supply.state, 1);
      ret = regulator_disable(test);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.state, 0);
      assert_int_equal(g_fake_regulator_supply.state, 0);
      ret = regulator_disable(test);
      assert_int_equal(ret, -EIO);
    }

  ret = regulator_enable(test);
  assert_false(ret < 0);
  assert_int_equal(g_fake_regulator.state, 1);
  assert_int_equal(g_fake_regulator_supply.state, 1);
  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator.rdev = NULL;
  regulator_unregister(g_fake_regulator_supply.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

static void drivertest_reg_mode(FAR void **state)
{
  FAR struct regulator_s *test = NULL;
  int cnt = 10;
  int ret = 0;

  g_fake_regulator.lpmode = 0;
  g_fake_regulator_desc.supply_name = NULL;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  test = regulator_get(REGULATOR_ID);
  assert_false(NULL == test);

  while (cnt--)
    {
      ret = regulator_set_mode(test, REGULATOR_MODE_INVALID);
      assert_true(ret < 0);
      ret = regulator_set_mode(test, REGULATOR_MODE_FAST);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_FAST);
      ret = regulator_set_mode(test, REGULATOR_MODE_NORMAL);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_NORMAL);
      ret = regulator_set_mode(test, REGULATOR_MODE_IDLE);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_IDLE);
      ret = regulator_set_mode(test, REGULATOR_MODE_STANDBY);
      assert_false(ret < 0);
      assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_STANDBY);
    }

  regulator_put(test);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

#ifdef CONFIG_PM
static void drivertest_reg_pm_register(FAR void **state)
{
  g_fake_regulator_desc.auto_lp = 0;
  g_fake_regulator.lpmode = REGULATOR_MODE_INVALID;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  assert_true(NULL == g_fake_regulator.rdev->pm_cb.notify);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator_desc.auto_lp = 1;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  assert_false(NULL == g_fake_regulator.rdev->pm_cb.notify);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}

static void drivertest_reg_pm_callback(FAR void **state)
{
  g_fake_regulator_desc.supply_name = NULL;
  g_fake_regulator_desc.auto_lp = 1;
  g_fake_regulator_desc.domain = 1;
  g_fake_regulator_desc.states[PM_NORMAL].mode = REGULATOR_MODE_NORMAL;
  g_fake_regulator_desc.states[PM_NORMAL].uv = 1;
  g_fake_regulator_desc.states[PM_IDLE].mode = REGULATOR_MODE_NORMAL;
  g_fake_regulator_desc.states[PM_IDLE].uv = 2;
  g_fake_regulator_desc.states[PM_STANDBY].mode = REGULATOR_MODE_IDLE;
  g_fake_regulator_desc.states[PM_STANDBY].uv = 3;
  g_fake_regulator_desc.states[PM_SLEEP].mode = REGULATOR_MODE_STANDBY;
  g_fake_regulator_desc.states[PM_SLEEP].uv = 4;
  g_fake_regulator.lpmode = REGULATOR_MODE_FAST;
  g_fake_regulator.lpuv = 0;
  g_fake_regulator.rdev = regulator_register(&g_fake_regulator_desc,
                                             &g_fake_regulator_ops,
                                             &g_fake_regulator);
  assert_false(NULL == g_fake_regulator.rdev);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      2, PM_SLEEP);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_FAST);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      1, PM_RESTORE);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_NORMAL);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      1, PM_NORMAL);
  assert_int_equal(g_fake_regulator.lpuv, 1);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_NORMAL);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      1, PM_IDLE);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_NORMAL);
  assert_int_equal(g_fake_regulator.lpuv, 2);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      1, PM_STANDBY);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_IDLE);
  assert_int_equal(g_fake_regulator.lpuv, 3);
  g_fake_regulator.rdev->pm_cb.notify(&g_fake_regulator.rdev->pm_cb,
                                      1, PM_SLEEP);
  assert_int_equal(g_fake_regulator.lpmode, REGULATOR_MODE_STANDBY);
  assert_int_equal(g_fake_regulator.lpuv, 4);
  regulator_unregister(g_fake_regulator.rdev);
  g_fake_regulator_supply.rdev = NULL;
  return;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate(drivertest_reg_register, NULL),
      cmocka_unit_test_prestate(drivertest_reg_always_on, NULL),
      cmocka_unit_test_prestate(drivertest_reg_supply_always_on_boot_off,
                                NULL),
      cmocka_unit_test_prestate(drivertest_reg_supply_boot_on_always_on,
                                NULL),
      cmocka_unit_test_prestate(drivertest_reg_supply_boot_on_always_off,
                                NULL),
      cmocka_unit_test_prestate(drivertest_reg_supply_boot_off_always_off,
                                NULL),
      cmocka_unit_test_prestate(drivertest_reg_mode, NULL),
#ifdef CONFIG_PM
      cmocka_unit_test_prestate(drivertest_reg_pm_register, NULL),
      cmocka_unit_test_prestate(drivertest_reg_pm_callback, NULL),
#endif
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
