/****************************************************************************
 * apps/testing/drivertest/drivertest_i2c_spi.c
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
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <nuttx/sensors/bmi160.h>

#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ACC_DEVPATH      "/dev/accel0"
#define READ_TIMES       100

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

struct test_state_s
{
  FAR const char *dev_path;
  int fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  FAR struct test_state_s *test_state;
  test_state = malloc(sizeof(struct test_state_s));
  assert_true(test_state != NULL);

  test_state->dev_path = ACC_DEVPATH;
  test_state->fd = open(test_state->dev_path, O_RDONLY);
  assert_true(test_state->fd > 0);

  *state = test_state;
  return 0;
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(FAR void **state)
{
  FAR struct test_state_s *test_state;
  test_state = (FAR struct test_state_s *)*state;
  assert_int_equal(close(test_state->fd), 0);
  free(test_state);
  return 0;
}

/****************************************************************************
 * Name: read_from_device
 ****************************************************************************/

static void read_from_device(FAR void **state)
{
  FAR struct test_state_s *test_state;
  struct accel_gyro_st_s data;
  int times;
  int fd;

  test_state = (FAR struct test_state_s *)*state;
  fd = test_state->fd;

  for (times = 0; times < READ_TIMES; times++)
    {
      int ret;

      ret = read(fd, &data, sizeof(struct accel_gyro_st_s));
      assert_true(ret == sizeof(struct accel_gyro_st_s));

      /* If sensing time has been changed, show 6 axis data. */

      printf("[%" PRIu32 "] %d, %d, %d / %d, %d, %d\n",
             data.sensor_time,
             data.gyro.x, data.gyro.y, data.gyro.z,
             data.accel.x, data.accel.y, data.accel.z);
      fflush(stdout);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * drivertest_i2c_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_setup_teardown(read_from_device, setup, teardown),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

