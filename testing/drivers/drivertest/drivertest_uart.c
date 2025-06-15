/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_uart.c
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

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cmocka.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_TESTING_DRIVER_TEST_UART_DEVICE
#  define CONFIG_TESTING_DRIVER_TEST_UART_DEVICE "/dev/console"
#endif

#ifndef CONFIG_TESTING_DRIVER_TEST_UART_DEFAULT_CONTENT
#  define CONFIG_TESTING_DRIVER_TEST_UART_DEFAULT_CONTENT "0123456789abcdefg"\
      "hijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ,./<>?;':\"[]{}\\|!@#$%^"\
      "&*()-+_="
#  define DEFAULT_CONTENT CONFIG_TESTING_DRIVER_TEST_UART_DEFAULT_CONTENT
#endif

#ifndef CONFIG_TESTING_DRIVER_TEST_UART_BUFFER_SIZE
#  define CONFIG_TESTING_DRIVER_TEST_UART_BUFFER_SIZE 1024
#  define BUFFER_SIZE CONFIG_TESTING_DRIVER_TEST_UART_BUFFER_SIZE
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct test_confs_s
{
  FAR const char *dev_path;
  int test_case_id;
};

struct test_state_s
{
  FAR const char *dev_path;
  FAR char *buffer;
  int fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: read_until
 ****************************************************************************/

static uint32_t read_until(int fd, FAR char *buffer,
                           uint32_t buffer_len, char terminator)
{
  uint32_t cnt = 0;
  ssize_t len = 0;
  char tmp_char = 0;

  while (cnt <= buffer_len)
    {
      assert_true(cnt < buffer_len);

      len = read(fd, &tmp_char, 1);
      assert_true(len >= 0);

      if (len == 0 || tmp_char == terminator)
        {
          buffer[cnt] = '\0';
          return cnt;
        }

      buffer[cnt] = tmp_char;
      cnt++;
    }

  buffer[buffer_len - 1] = '\0';
  return UINT32_MAX;
}

/****************************************************************************
 * Name: read_length
 ****************************************************************************/

static uint32_t read_length(int fd, FAR char *buffer,
                            uint32_t max_size)
{
  uint32_t cnt = 0;
  ssize_t len = 0;

  while (cnt < max_size)
    {
      len = read(fd, buffer + cnt, max_size - cnt);
      assert_true(len >= 0);

      if (len == 0)
        {
          return cnt;
        }

      cnt += len;
    }

  return cnt;
}

/****************************************************************************
 * Name: write_length
 ****************************************************************************/

static uint32_t write_length(int fd, FAR const char *buffer,
                             uint32_t max_size)
{
  uint32_t cnt = 0;
  ssize_t len = 0;

  while (cnt < max_size)
    {
      len = write(fd, buffer + cnt, max_size - cnt);
      assert_true(len >= 0);

      if (len == 0)
        {
          return cnt;
        }

      cnt += len;
    }

  return cnt;
}

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  FAR struct test_confs_s *confs = (FAR struct test_confs_s *)*state;
  FAR struct test_state_s *test_state = malloc(sizeof(struct test_state_s));
  assert_true(test_state != NULL);

  test_state->dev_path = confs->dev_path;
  assert_true(sizeof(DEFAULT_CONTENT) <= BUFFER_SIZE);
  test_state->buffer = malloc(BUFFER_SIZE);
  assert_true(test_state->buffer != NULL);

  test_state->fd = open(test_state->dev_path, O_RDWR);
  assert_true(test_state->fd > 0);

  *state = test_state;
  return 0;
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(FAR void **state)
{
  FAR struct test_state_s *test_state = (FAR struct test_state_s *)*state;

  free(test_state->buffer);
  assert_int_equal(close(test_state->fd), 0);
  free(test_state);

  return 0;
}

/****************************************************************************
 * Name: drivertest_uart_write
 ****************************************************************************/

static void drivertest_uart_write(FAR void **state)
{
  FAR struct test_state_s *test_state = (FAR struct test_state_s *)*state;
  int res = write(test_state->fd,
                  DEFAULT_CONTENT,
                  sizeof(DEFAULT_CONTENT) - 1);

  assert_int_equal(res, sizeof(DEFAULT_CONTENT) - 1);
}

/****************************************************************************
 * Name: drivertest_uart_read
 ****************************************************************************/

static void drivertest_uart_read(FAR void **state)
{
  FAR struct test_state_s *test_state = (FAR struct test_state_s *)*state;
  size_t buffer_size = sizeof(DEFAULT_CONTENT);
  int cnt = 0;
  FAR char *buffer = test_state->buffer;
  assert_true(buffer != NULL);

  buffer[buffer_size - 1] = '\0';

  while (cnt < sizeof(DEFAULT_CONTENT) - 1)
    {
      ssize_t n = read(test_state->fd, buffer + cnt, buffer_size - cnt - 1);

      assert_true(n >= 0);
      if (n == 0)
        {
          break;
        }
      else
        {
          cnt += n;
        }
    }

  assert_string_equal(buffer, DEFAULT_CONTENT);
}

/****************************************************************************
 * Name: drivertest_uart_burst
 ****************************************************************************/

static void drivertest_uart_burst(FAR void **state)
{
  FAR struct test_state_s *test_state = (FAR struct test_state_s *)*state;
  int res = 0;
  char num_buffer[16];
  char ret_msg_buffer[8];

  num_buffer[0] = '\0';
  ret_msg_buffer[0] = '\0';

  while (true)
    {
      int num = 0;
      FAR char *read_buffer = test_state->buffer;

      res = read_until(test_state->fd, num_buffer, sizeof(num_buffer), '#');
      assert_true(res > 0);

      num = strtol(num_buffer, NULL, 10);
      assert_true(num >= 0);
      assert_true(num < BUFFER_SIZE);

      if (num == 0)
        {
          break;
        }

      res = read_length(test_state->fd, read_buffer, num);
      assert_int_equal(res, num);
      read_buffer[num] = '\0';

      res = write_length(test_state->fd, read_buffer, num);
      assert_int_equal(res, num);

      res = read_until(test_state->fd, ret_msg_buffer,
                       sizeof(ret_msg_buffer), '#');

      /* length of 'pass' or 'fail' */

      assert_int_equal(res, 4);

      assert_string_equal(ret_msg_buffer, "pass");
    }
}

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s -d <dev path>\n", progname);
  printf("Where:\n");
  printf("  -d <dev path> uart device path "
         "[default device: /dev/console].\n");
  printf("  -n <test_case_id> selects the testcase to uart.\n"
         "[default test: drivertest_uart_write].\n"
         "  Case 0: drivertest_uart_write test\n"
         "  Case 1: drivertest_uart_read test\n"
         "  Case 2: drivertest_uart_burst test\n"
        );
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(int argc, FAR char **argv,
                       FAR struct test_confs_s *conf)
{
  int option;

  while ((option = getopt(argc, argv, "d:n:")) != ERROR)
    {
      switch (option)
        {
          case 'd':
            conf->dev_path = optarg;
            break;
          case 'n':
            conf->test_case_id = atoi(optarg);
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cmocka_driver_uart_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  void (*drivertest_uart)(FAR void **state) = NULL;
  struct test_confs_s confs =
  {
    .dev_path = CONFIG_TESTING_DRIVER_TEST_UART_DEVICE,
    .test_case_id = 0
  };

  parse_args(argc, argv, &confs);
  switch (confs.test_case_id)
  {
    case 1:
      drivertest_uart = drivertest_uart_read;
      break;
    case 2:
      drivertest_uart = drivertest_uart_burst;
      break;
    default:
      drivertest_uart = drivertest_uart_write;
      break;
  }

  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(drivertest_uart, setup,
                                               teardown, &confs),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}

