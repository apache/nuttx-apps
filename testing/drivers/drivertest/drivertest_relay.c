/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_relay.c
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

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <nuttx/power/relay.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct relay_args_s
{
  FAR char *devname;   /* Relay device name */
  int       autotest;  /* autotest or not */
  bool      set;       /* set the relay driver */
  bool      setvalue;  /* the setvalue, 0 : open, 1: close */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s -d <device name> -s <0/1> -g\n", progname);
  printf("Where:\n");
  printf("  -d relay device name [default name: /dev/relay0].\n");
  printf("  -a autotest num, if autotest enable -s and -g are ignored.\n");
  printf("  -s set relay status to [0 : open] or [1 : close].\n\n");
  printf("Example 0:\n");
  printf("  Description: auto test the relay driver 10 times\n");
  printf("  cmocka_driver_relay -d /dev/relay0 -a 10\n\n");
  printf("Example 1:\n");
  printf("  Description: set the relay to close and then get the status\n");
  printf("  cmocka_driver_relay -d /dev/relay0 -s 1\n\n");
  printf("Example 2:\n");
  printf("  Description: get the relay status\n");
  printf("  cmocka_driver_relay -d /dev/relay0\n\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct relay_args_s *relay_args)
{
  int option;

  while ((option = getopt(argc, argv, "d:a:s:")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            relay_args->devname = optarg;
            break;
          case 'a':
            relay_args->autotest = atoi(optarg);
            break;
          case 's':
            relay_args->set = true;
            relay_args->setvalue = atoi(optarg) != 0;
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  printf("Relay test args:\n");
  printf("  devname  = %s\n", relay_args->devname);
  printf("  autotest = %d\n", relay_args->autotest);
  printf("  set      = %d\n", relay_args->set);
  printf("  setvalue = %d\n", relay_args->setvalue);
}

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  return 0;
}

/****************************************************************************
 * Name: drivertest_relay
 ****************************************************************************/

static void drivertest_relay(FAR void **state)
{
  FAR struct relay_args_s *relay_args;
  int fd;
  bool getstatus;
  bool setstatus;
  int ret;
  int i;

  relay_args = (FAR struct relay_args_s *)*state;

  fd = open(relay_args->devname, O_RDWR);
  assert_true(fd > 0);

  /* Auto test, repeat set different status to the relay and check the
   * relay status.
   */

  ret = ioctl(fd, RELAYIOC_GET, &getstatus);
  assert_return_code(ret, OK);

  for (i = 0; i < relay_args->autotest; i++)
    {
      setstatus = !getstatus;
      ret = ioctl(fd, RELAYIOC_SET, &setstatus);
      assert_return_code(ret, OK);

      ret = ioctl(fd, RELAYIOC_GET, &getstatus);
      assert_return_code(ret, OK);

      assert_true(setstatus == getstatus);
      usleep(100000);
    }

  /* Auto test, repeate set same status to the relay and check the relay
   * status
   */

  for (i = 0; i < relay_args->autotest; i++)
    {
      setstatus = getstatus;
      ret = ioctl(fd, RELAYIOC_SET, &setstatus);
      assert_return_code(ret, OK);

      ret = ioctl(fd, RELAYIOC_GET, &getstatus);
      assert_return_code(ret, OK);

      assert_true(setstatus == getstatus);
      usleep(100000);
    }

  /* If autotest enable, ignore the set/get operation */

  if (relay_args->autotest != 0)
    {
      goto out;
    }

  /* Set/get the relay status */

  if (relay_args->set)
    {
      ret = ioctl(fd, RELAYIOC_GET, &getstatus);
      assert_return_code(ret, OK);
      printf("Relay status before set: %s\n", getstatus ? "open" : "close");

      ret = ioctl(fd, RELAYIOC_SET, &relay_args->setvalue);
      assert_return_code(ret, OK);
      printf("Set relay to: %s\n", relay_args->setvalue ? "open" : "close");

      ret = ioctl(fd, RELAYIOC_GET, &getstatus);
      assert_return_code(ret, OK);
      printf("Relay status after set: %s\n", getstatus ? "open" : "close");
    }
  else
    {
      ret = ioctl(fd, RELAYIOC_GET, &getstatus);
      assert_return_code(ret, OK);
      printf("Relay status: %s\n", getstatus ? "open" : "close");
    }

out:
  close(fd);
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
 * Name: drivertest_relay_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct relay_args_s relay_args =
    {
      .devname  = "/dev/relay0", /* Relay device name */
      .autotest = 0,             /* autotest or not */
      .set      = false,         /* set the relay driver */
      .setvalue = false,         /* the setvalue, 0 : open, 1: close */
    };

  parse_commandline(argc, argv, &relay_args);
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(drivertest_relay, setup,
                                               teardown, &relay_args),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
