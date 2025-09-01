/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_mtd_log.c
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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <cmocka.h>
#include <sys/ioctl.h>

#include <nuttx/mtd/mtd_log.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void drivertest_mtdlog_case1(FAR void **state)
{
  char wrbuf[32];
  char rdbuf[32];
  int testcnt;
  int logfd;
  int ret;

  UNUSED(state);

  ret = open("/dev/mtdlog", O_RDWR);
  if (ret < 0)
    {
      printf("ERROR: Failed to open /dev/mtdlog: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  logfd = ret;
  testcnt = 1000;

  while (testcnt--)
    {
      /* Write a piece of data to the mtdlog device. */

      memset(wrbuf, 's', sizeof(wrbuf));
      wrbuf[sizeof(wrbuf) - 1] = 0;
      ret = write(logfd, wrbuf, sizeof(wrbuf));
      if (ret != sizeof(wrbuf))
        {
          printf("ERROR: Failed to write /dev/mtdlog: %d\n", errno);
          assert_int_equal((ret > 0), 1);
          return;
        }

      /* Read out the data written into the mtdlog device. */

      memset(rdbuf, 0, sizeof(rdbuf));
      ret = read(logfd, rdbuf, sizeof(rdbuf));
      if (ret != sizeof(rdbuf))
        {
          printf("ERROR: Failed to read /dev/mtdlog: %d\n", errno);
          assert_int_equal((ret > 0), 1);
          return;
        }

      /* Compare the written data with the read data. */

      ret = memcmp(wrbuf, rdbuf, sizeof(wrbuf));
      assert_int_equal(ret, 0);
    }

  close(logfd);
}

static void drivertest_mtdlog_case2(FAR void **state)
{
  struct mtdlog_blkinfo_s blkinfo;
  struct mtdlog_loginfo_s loginfo;
  struct mtdlog_status_s status;
  uint32_t log_count;
  uint32_t grp_count;
  uint32_t old_count;
  char wrbuf[32];
  int logfd;
  int ret;

  UNUSED(state);

  ret = open("/dev/mtdlog", O_RDWR);
  if (ret < 0)
    {
      printf("ERROR: Failed to open /dev/mtdlog: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  logfd = ret;
  assert_int_equal((ret > 0), 1);

  /* Obtain the block information and log information pointed to by
   * the current read pointer.
   */

  ret = ioctl(logfd, MTDLOGIOC_GET_BLOCKINFO, (unsigned long)&blkinfo);
  if (ret < 0)
    {
      printf("ERROR: Failed to get current block info: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  ret = ioctl(logfd, MTDLOGIOC_GET_ENTRYINFO, (unsigned long)&loginfo);
  if (ret < 0)
    {
      printf("ERROR: Failed to get current log info: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  assert_int_equal(blkinfo.valid, 1);
  assert_int_equal(loginfo.valid, 1);

  /* Obtain the total number of log entries and the total number of
   * block groups in the mtdlog device.
   */

  ret = ioctl(logfd, MTDLOGIOC_LOG_COUNT, (unsigned long)&log_count);
  if (ret < 0)
    {
      printf("ERROR: Failed to get log entry count: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  ret = ioctl(logfd, MTDLOGIOC_GRP_COUNT, (unsigned long)&grp_count);
  if (ret < 0)
    {
      printf("ERROR: Failed to get block group count: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  ret = ioctl(logfd, MTDLOGIOC_STATUS, (unsigned long)&status);
  if (ret < 0)
    {
      printf("ERROR: Failed to get current status: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  assert_int_equal((log_count > 0), 1);
  assert_int_equal((grp_count > 0), 1);
  assert_int_equal(log_count, status.logcnt);

  /* Skip all log entries and obtain the log information. */

  ret = ioctl(logfd, MTDLOGIOC_LOG_SEEK_SET, log_count);
  if (ret < 0)
    {
      printf("ERROR: Failed to seek log entry: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  ret = ioctl(logfd, MTDLOGIOC_GET_ENTRYINFO, (unsigned long)&loginfo);
  assert_int_equal(errno, ENOENT);

  /* Write a log and obtain the log information. */

  memset(wrbuf, 's', sizeof(wrbuf));
  wrbuf[sizeof(wrbuf) - 1] = 0;
  ret = write(logfd, wrbuf, sizeof(wrbuf));
  if (ret != sizeof(wrbuf))
    {
      printf("ERROR: Failed to write /dev/mtdlog: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  old_count = log_count;
  ret = ioctl(logfd, MTDLOGIOC_LOG_COUNT, (unsigned long)&log_count);
  if (ret < 0)
    {
      printf("ERROR: Failed to get log entry count: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  assert_int_equal((log_count - old_count), 1);

  ret = ioctl(logfd, MTDLOGIOC_GET_ENTRYINFO, (unsigned long)&loginfo);
  if (ret < 0)
    {
      printf("ERROR: Failed to get current log info: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  assert_int_equal(loginfo.valid, 1);

  /* Obtain the total number of block groups. If it is greater
   * than 1, skip the first block group.
   */

  ret = ioctl(logfd, MTDLOGIOC_GRP_COUNT, (unsigned long)&grp_count);
  if (ret < 0)
    {
      printf("ERROR: Failed to get block group count: %d\n", errno);
      assert_int_equal((ret > 0), 1);
      return;
    }

  if (grp_count > 1)
    {
      ret = ioctl(logfd, MTDLOGIOC_GRP_SEEK_SET, 1);
      if (ret < 0)
        {
          printf("ERROR: Failed to seek block group: %d\n", errno);
          assert_int_equal((ret > 0), 1);
          return;
        }
    }
  else
    {
      ret = ioctl(logfd, MTDLOGIOC_GRP_SEEK_SET, 1);
      assert_int_equal(errno, ENOENT);
    }

  close(logfd);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * drivertest_mtd_log_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest tests[] =
  {
    cmocka_unit_test(drivertest_mtdlog_case1),
    cmocka_unit_test(drivertest_mtdlog_case2),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
