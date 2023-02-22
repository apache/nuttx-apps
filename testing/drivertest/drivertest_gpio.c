/****************************************************************************
 * apps/testing/drivertest/drivertest_gpio.c
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

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <nuttx/ioexpander/gpio.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

#define GPIOTEST_MAXVALUE 5

struct pre_build_s
{
  FAR char *gpio_a;
  FAR char *gpio_b;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s -a <gpio_a>[dev/gpio0] -b"
         " <gpio_b>[dev/gpio1] \n", progname);
  printf("Where:\n");
  printf("  -a <gpio_a> gpio_a location [default location: dev/gpio0].\n");
  printf("  -b <gpio_b> gpio_b location [default location: dev/gpio1].\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct pre_build_s *pre_build)
{
  int option;

  while ((option = getopt(argc, argv, "a:b:")) != ERROR)
    {
      switch (option)
        {
          case 'a':
            pre_build->gpio_a = optarg;
            break;
          case 'b':
            pre_build->gpio_b = optarg;
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Name: gpiotest_randbin
 ****************************************************************************/

static inline bool gpiotest_randbin(void)
{
  int value = rand() % 18;
  if (value > 9)
    {
      return true;
    }
  else
    {
      return false;
    }

  return true;
}

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  FAR struct pre_build_s *pre_build;
  time_t t;
  pre_build = (FAR struct pre_build_s *)*state;

  /* Seed the random number generated */

  srand((unsigned)time(&t));
  *state = pre_build;
  return 0;
}

/****************************************************************************
 * Name: gpiotest01
 ****************************************************************************/

static void gpiotest01(FAR void **state)
{
  FAR struct pre_build_s *pre_build;
  int fd_a;
  int fd_b;
  bool outvalue;
  bool invalue;
  int ret;
  int i;

  pre_build = (FAR struct pre_build_s *)*state;

  fd_a = open(pre_build->gpio_a, O_RDWR);
  assert_false(fd_a < 0);
  fd_b = open(pre_build->gpio_b, O_RDWR);
  assert_false(fd_b < 0);

  /* Test the input function
   * Method: Output from gpio_a to gpio_b and
   * compare the read values.
   */

  /* gpio_a to gpio_b */

  ret = ioctl(fd_a, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);
  assert_false(ret < 0);
  ret = ioctl(fd_b, GPIOC_SETPINTYPE, (unsigned long) GPIO_INPUT_PIN);
  assert_false(ret < 0);

  for (i = 0; i < GPIOTEST_MAXVALUE; i++)
    {
      /* Set GPIO level */

      outvalue = gpiotest_randbin();
      assert_false(ret < 0);
      ret = ioctl(fd_a, GPIOC_WRITE, (unsigned long) outvalue);
      assert_false(ret < 0);

      /* Read Pin Test */

      ret = ioctl(fd_b, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
      assert_false(ret < 0);

      printf("[__Verify__]  outvalue is %d, invalue is %d\n",
            outvalue, invalue);
      assert_int_equal(invalue, outvalue);
    }

  ret = ioctl(fd_b, GPIOC_SETPINTYPE, (unsigned long) GPIO_OUTPUT_PIN);
  assert_false(ret < 0);
  ret = ioctl(fd_a, GPIOC_SETPINTYPE, (unsigned long) GPIO_INPUT_PIN);
  assert_false(ret < 0);

  /* gpio_b to gpio_a */

  for (i = 0; i < GPIOTEST_MAXVALUE; i++)
    {
      /* Set GPIO level */

      outvalue = gpiotest_randbin();
      assert_false(ret < 0);
      ret = ioctl(fd_b, GPIOC_WRITE, (unsigned long) outvalue);
      assert_false(ret < 0);

      /* Read Pin Test */

      ret = ioctl(fd_a, GPIOC_READ, (unsigned long)((uintptr_t)&invalue));
      assert_false(ret < 0);

      printf("[__Verify__]  outvalue is %d, invalue is %d\n",
             outvalue, invalue);
      assert_int_equal(invalue, outvalue);
    }

  close(fd_a);
  close(fd_b);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(FAR void **state)
{
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: drivertest_gpio_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct pre_build_s pre_build =
    {
      .gpio_a = "dev/gpio0",
      .gpio_b = "dev/gpio1"
    };

  parse_commandline(argc, argv, &pre_build);
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(gpiotest01, setup,
                                               teardown, &pre_build),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
