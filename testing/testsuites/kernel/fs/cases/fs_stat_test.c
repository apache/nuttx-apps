/****************************************************************************
 * apps/testing/testsuites/kernel/fs/cases/fs_stat_test.c
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
#include <stdio.h>
#include <string.h>
#include <utime.h>
#include <time.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "fstest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_FILE "stat_test_file"
#define BUF_SIZE 1024

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_printtime
 ****************************************************************************/

__attribute__((unused)) static void test_nuttx_fs_printtime(struct tm *TM)
{
  syslog(LOG_INFO, "    tm_year: %d\n", TM->tm_year + 1900);
  syslog(LOG_INFO, "    tm_mon: %d\n", TM->tm_mon);
  syslog(LOG_INFO, "    tm_mday: %d\n", TM->tm_mday);
  syslog(LOG_INFO, "    tm_hour: %d\n", TM->tm_hour);
  syslog(LOG_INFO, "    tm_min: %d\n", TM->tm_min);
  syslog(LOG_INFO, "    tm_sec: %d\n", TM->tm_sec);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_fs_stat01
 ****************************************************************************/

void test_nuttx_fs_stat01(FAR void **state)
{
  int fd;
  int ret;
  int ret2;
  struct stat file_s;
  char buf[BUF_SIZE] =
  {
    0
  };

  struct tm *tm_1 = NULL;
  struct tm *tm_2 = NULL;
  int year1;
  int year2;
  int month1;
  int month2;
  int day1;
  int day2;
  int hour1;
  int hour2;
  int min1;
  int min2;
  time_t t_1;
  time_t t_2;
  struct fs_testsuites_state_s *test_state;

  test_state = (struct fs_testsuites_state_s *)*state;

  /* set memory */

  memset(buf, 65, BUF_SIZE);

  /* open file */

  fd = open(TEST_FILE, O_RDWR | O_CREAT, 0777);
  assert_true(fd > 0);
  test_state->fd1 = 0;

  /* do write */

  ret2 = write(fd, buf, BUF_SIZE);
  assert_int_in_range(ret2, 1, 1024);

  close(fd);

  /* get system time */

  time(&t_1);
  tm_1 = gmtime(&t_1);
  assert_non_null(tm_1);

  /* set time */

  year1 = tm_1->tm_year;
  month1 = tm_1->tm_mon;
  day1 = tm_1->tm_mday;
  hour1 = tm_1->tm_hour;
  min1 = tm_1->tm_min;

  /* get file info */

  ret = stat(TEST_FILE, &file_s);
  assert_int_equal(ret, 0);

  /* output stat struct information */

  t_2 = file_s.st_mtime;
  tm_2 = gmtime(&t_2);

  assert_non_null(tm_2);

  /* set time */

  year2 = tm_2->tm_year;
  month2 = tm_2->tm_mon;
  day2 = tm_2->tm_mday;
  hour2 = tm_2->tm_hour;
  min2 = tm_2->tm_min;

  /* compare time and size */

  assert_int_equal(year1, year2);
  assert_int_equal(month1, month2);
  assert_int_equal(day1, day2);
  assert_int_equal(hour1, hour2);
  assert_int_equal(min1, min2);
  assert_int_equal(file_s.st_size, BUF_SIZE);
}
