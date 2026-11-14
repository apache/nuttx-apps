/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_adc_open.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <cmocka.h>
#include <nuttx/analog/adc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOCK_ADC_PATH "/dev/adc_open_test"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned int g_setup_count;
static unsigned int g_rxint_count;
static unsigned int g_shutdown_count;
static bool g_rxint_enabled;
static struct adc_dev_s g_adcdev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int mock_adc_bind(FAR struct adc_dev_s *dev,
                         FAR const struct adc_callback_s *callback)
{
  (void)dev;
  (void)callback;
  return 0;
}

static void mock_adc_reset(FAR struct adc_dev_s *dev)
{
  (void)dev;
}

static int mock_adc_setup(FAR struct adc_dev_s *dev)
{
  (void)dev;
  g_setup_count++;

  if (g_setup_count == 1)
    {
      return -EIO;
    }

  return 0;
}

static void mock_adc_shutdown(FAR struct adc_dev_s *dev)
{
  (void)dev;
  g_shutdown_count++;
}

static void mock_adc_rxint(FAR struct adc_dev_s *dev, bool enable)
{
  (void)dev;
  g_rxint_count++;
  g_rxint_enabled = enable;
}

static int mock_adc_ioctl(FAR struct adc_dev_s *dev, int cmd,
                          unsigned long arg)
{
  (void)dev;
  (void)cmd;
  (void)arg;
  return -ENOTTY;
}

static const struct adc_ops_s g_adc_ops =
{
  .ao_bind     = mock_adc_bind,
  .ao_reset    = mock_adc_reset,
  .ao_setup    = mock_adc_setup,
  .ao_shutdown = mock_adc_shutdown,
  .ao_rxint    = mock_adc_rxint,
  .ao_ioctl    = mock_adc_ioctl,
};

static void test_adc_open_setup_failure(FAR void **state)
{
  int fd1;
  int fd2;
  int ret;

  (void)state;

  fd1 = open(MOCK_ADC_PATH, O_RDONLY);
  assert_int_equal(fd1, -1);
  assert_int_equal(errno, EIO);
  assert_int_equal(g_setup_count, 1);
  assert_int_equal(g_adcdev.ad_ocount, 0);
  assert_int_equal(g_rxint_count, 0);
  assert_false(g_rxint_enabled);
  assert_int_equal(g_shutdown_count, 0);

  fd1 = open(MOCK_ADC_PATH, O_RDONLY);
  assert_true(fd1 >= 0);
  assert_int_equal(g_setup_count, 2);
  assert_int_equal(g_adcdev.ad_ocount, 1);
  assert_int_equal(g_rxint_count, 1);
  assert_true(g_rxint_enabled);
  assert_int_equal(g_shutdown_count, 0);

  fd2 = open(MOCK_ADC_PATH, O_RDONLY);
  assert_true(fd2 >= 0);
  assert_int_equal(g_setup_count, 2);
  assert_int_equal(g_adcdev.ad_ocount, 2);
  assert_int_equal(g_rxint_count, 1);
  assert_int_equal(g_shutdown_count, 0);

  ret = close(fd1);
  assert_int_equal(ret, 0);
  assert_int_equal(g_adcdev.ad_ocount, 1);
  assert_int_equal(g_shutdown_count, 0);

  ret = close(fd2);
  assert_int_equal(ret, 0);
  assert_int_equal(g_adcdev.ad_ocount, 0);
  assert_int_equal(g_shutdown_count, 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(test_adc_open_setup_failure),
  };

  int ret;

  (void)argc;
  (void)argv;

  g_adcdev.ad_ops = &g_adc_ops;
  ret = adc_register(MOCK_ADC_PATH, &g_adcdev);
  if (ret < 0)
    {
      printf("adc_register failed: %d\n", ret);
      return 1;
    }

  return cmocka_run_group_tests(tests, NULL, NULL);
}
