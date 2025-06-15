/****************************************************************************
 * apps/testing/drivers/drivertest/drivertest_block.c
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
#include <nuttx/cache.h>
#include <nuttx/crc32.h>
#include <nuttx/mtd/mtd.h>

#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <cmocka.h>

#include "inode/inode.h"
#include "driver/driver.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SECTORS_RANGE 0.95

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pre_build_s
{
  FAR struct inode *driver;
  struct mtd_geometry_s geo;
  struct geometry cfg;
  char source[PATH_MAX];
  int fd;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("Usage: %s -m <source>\n", progname);
  printf("Where:\n");
  printf("  -m <source> Block device or mtd device"
         " mount location.\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct pre_build_s *pre)
{
  int option;

  pre->source[0] = '\0';

  while ((option = getopt(argc, argv, "m:")) != ERROR)
    {
      switch (option)
        {
          case 'm':
            strlcpy(pre->source, optarg, sizeof(pre->source));
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  if (pre->source[0] == '\0')
    {
      printf("Missing <source>\n");
      show_usage(argv[0], EXIT_FAILURE);
    }
}

/****************************************************************************
 * Name: blktest_randchar
 ****************************************************************************/

static inline char blktest_randchar(void)
{
  int value = rand() % 63;
  if (value == 0)
    {
      return '0';
    }
  else if (value <= 10)
    {
      return value + '0' - 1;
    }
  else if (value <= 36)
    {
      return value + 'a' - 11;
    }
  else
    {
      return value + 'A' - 37;
    }
}

/****************************************************************************
 * Name: blktest_randcontext
 ****************************************************************************/

static void blktest_randcontext(uint32_t size, FAR void *input)
{
  /* Construct a buffer here and fill it with random characters */

  int i;
  FAR char *tmp;
  tmp = input;
  for (i = 0; i < size - 1; i++)
    {
      tmp[i] = blktest_randchar();
    }

  tmp[i] = '\0';
}

/****************************************************************************
 * Name: setup_bch
 ****************************************************************************/

static int setup_bch(FAR void **state)
{
  FAR struct pre_build_s *pre = *state;
  struct stat mode;
  time_t t;
  int ret;

  pre->fd = open(pre->source, O_RDWR | O_DIRECT);
  assert_false(pre->fd < 0);

  ret = fstat(pre->fd, &mode);
  assert_int_equal(ret, 0);
  ret = (mode.st_mode & S_IFBLK) | (mode.st_mode & S_IFMTD);
  assert_true(ret != 0);

  ret = ioctl(pre->fd, BIOC_GEOMETRY, (unsigned long)((uintptr_t)&pre->cfg));
  assert_false(ret < 0);

  srand((unsigned)time(&t));
  *state = pre;
  return 0;
}

/****************************************************************************
 * Name: setup_driver
 ****************************************************************************/

static int setup_driver(FAR void **state)
{
  FAR struct pre_build_s *pre = *state;
  int ret;

  ret = find_mtddriver(pre->source, &pre->driver);
  if (ret != 0)
    {
      ret = find_blockdriver(pre->source, 0, &pre->driver);
      assert_false(ret != 0);

      ret = pre->driver->u.i_bops->geometry(pre->driver, &pre->cfg);
      assert_false(ret < 0);

      pre->geo.blocksize = pre->cfg.geo_sectorsize;
      pre->geo.erasesize = pre->cfg.geo_sectorsize;
      pre->geo.neraseblocks = pre->cfg.geo_nsectors;
    }
  else
    {
      ret = MTD_IOCTL(pre->driver->u.i_mtd, MTDIOC_GEOMETRY,
                      (unsigned long)&pre->geo);
      assert_false(ret < 0);
    }

  srand((unsigned)time(NULL));
  *state = pre;
  return 0;
}

/****************************************************************************
 * Name: drivertest_block_stress
 ****************************************************************************/

static void drivertest_block_stress(FAR void **state)
{
  FAR struct pre_build_s *pre;
  FAR void *input;
  FAR void *output;
  blkcnt_t nsectors;
  uint32_t input_crc;
  uint32_t output_crc;
  blkcnt_t i;
  int ret;

  pre = *state;

  input = malloc(pre->cfg.geo_sectorsize * 2);
  assert_true(input != NULL);
  output = input + pre->cfg.geo_sectorsize;

  /* We expect the physical bad block rate on nand flash to be no more
   * than 5%, we give the redundancy space at the end.
   */

  nsectors =  pre->cfg.geo_nsectors * SECTORS_RANGE;

  /* Test process: convert the device information of the storage device into
   * 'sectors' by bch, and fill each 'sector' with random characters, then
   * read it out and compare whether the writing and The difference between
   * write and store is verified by crc32.
   * This behavior simulates the behavior of commands such as 'dd' in the
   * system. The general flow is user->bch->ftl->driver
   */

  for (i = 0; i < nsectors; i++)
    {
      lseek(pre->fd, i * pre->cfg.geo_sectorsize, SEEK_SET);

      blktest_randcontext(pre->cfg.geo_sectorsize, input);
      input_crc = crc32(input, pre->cfg.geo_sectorsize);
      ret = write(pre->fd, input, pre->cfg.geo_sectorsize);
      assert_true(ret == pre->cfg.geo_sectorsize);

      fsync(pre->fd);

      /* Let's write each time we need to move the pointer back to the
       * beginning
       */

      lseek(pre->fd, i * pre->cfg.geo_sectorsize, SEEK_SET);

      ret = read(pre->fd, output, pre->cfg.geo_sectorsize);
      assert_int_equal(ret, pre->cfg.geo_sectorsize);
      output_crc = crc32(output, pre->cfg.geo_sectorsize);
      assert_false(output_crc != input_crc);
    }

  free(input);
}

/****************************************************************************
 * Name: drivertest_block_cache_write
 ****************************************************************************/

static void drivertest_block_cache_write(FAR void **state)
{
  FAR struct pre_build_s *pre;
  FAR void *input;
  FAR void *output;
  uint32_t input_crc;
  uint32_t output_crc;
  uint32_t size;
  unsigned int block;
  unsigned int eblock;
  size_t i;
  int ret;

  pre = *state;

  /* There is a possibility that the cache size (default 0) is not
   * available when the case is not opened, so the case should be checked
   * to see if the corresponding cache config is open.
   */

  size = up_get_dcache_size();

  if (size > pre->geo.blocksize && (size % pre->geo.blocksize) == 0)
    {
      block = size / pre->geo.blocksize;
    }
  else
    {
      block = (pre->geo.blocksize + size) / pre->geo.blocksize;
    }

  /* When we can't get the cachesize, we execute it again as
   * blktest_single_write
   */

  size = block * pre->geo.blocksize;

  if (size > pre->geo.erasesize * pre->geo.neraseblocks)
    {
      printf("Warning: Total block size too small,"
             "need larger than %" PRId32 "\n",
             size);
      return;
    }

  if (size > pre->geo.erasesize && (size % pre->geo.erasesize) == 0)
    {
      eblock = size / pre->geo.erasesize;
    }
  else
    {
      eblock = (pre->geo.erasesize + size) / pre->geo.erasesize;
    }

  output = malloc(size);
  assert_false(output == NULL);

  for (i = 4; i <= 32; i *= 2)
    {
      /* This case is designed to simulate the behavior of a
       * filesystem write. Simulate the case where the buffer constructed
       * at one time may be larger than the cache size for the file system
       */

      input = memalign(i, size);
      assert_false(input == NULL);
      blktest_randcontext(pre->geo.blocksize, input);
      input_crc = crc32(input, size);
      if (INODE_IS_MTD(pre->driver))
        {
          /* Before writing we need to erase the mtd device */

          ret = MTD_ERASE(pre->driver->u.i_mtd, 0, eblock);
          assert_false(ret < 0);
          ret = MTD_BWRITE(pre->driver->u.i_mtd, 0, block, input);
          assert_false(ret != block);
          ret = MTD_BREAD(pre->driver->u.i_mtd, 0, block, output);
          assert_false(ret != block);
        }
      else
        {
          ret = pre->driver->u.i_bops->write(pre->driver, input, 0, block);
          assert_false(ret < 0);
          ret = pre->driver->u.i_bops->read(pre->driver, output, 0, block);
          assert_false(ret < 0);
        }

      output_crc = crc32(output, size);
      assert_false(input_crc != output_crc);

      free(input);
    }

  free(output);
}

/****************************************************************************
 * Name: drivertest_block_single_write
 ****************************************************************************/

static void drivertest_block_single_write(FAR void **state)
{
  FAR struct pre_build_s *pre;
  FAR void *input;
  FAR void *output;
  uint32_t input_crc;
  uint32_t output_crc;
  size_t i;
  int ret;

  pre = *state;

  output = malloc(pre->geo.blocksize);
  assert_false(output == NULL);

  /* Obviously, this is just to simulate the case where the file system
   * is written one eraseblock at a time
   */

  for (i = 4; i <= 32; i *= 2)
    {
      input = memalign(i, pre->geo.blocksize);
      assert_false(input == NULL);

      blktest_randcontext(pre->geo.blocksize, input);

      input_crc = crc32(input, pre->geo.blocksize);

      if (INODE_IS_MTD(pre->driver))
        {
          ret = MTD_ERASE(pre->driver->u.i_mtd, 0, 1);
          assert_false(ret < 0);
          ret = MTD_BWRITE(pre->driver->u.i_mtd, 0, 1, input);
          assert_false(ret != 1);
          ret = MTD_BREAD(pre->driver->u.i_mtd, 0, 1, output);
          assert_false(ret != 1);
        }
      else
        {
          ret = pre->driver->u.i_bops->write(pre->driver, input, 0, 1);
          assert_false(ret < 0);
          ret = pre->driver->u.i_bops->read(pre->driver, output, 0, 1);
          assert_false(ret < 0);
        }

      output_crc = crc32(output, pre->geo.blocksize);
      assert_false(input_crc != output_crc);

      free(input);
    }

  free(output);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown_driver(FAR void **state)
{
  return 0;
}

/****************************************************************************
 * Name: teardown_bch
 ****************************************************************************/

static int teardown_bch(FAR void **state)
{
  FAR struct pre_build_s *pre;

  pre = *state;

  close(pre->fd);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: blktest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct pre_build_s *pre;
  int ret;

  pre = kmm_zalloc(sizeof(*pre));
  if (pre == NULL)
    {
      return -ENOMEM;
    }

  parse_commandline(argc, argv, pre);
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(drivertest_block_stress,
                                               setup_bch, teardown_bch, pre),
      cmocka_unit_test_prestate_setup_teardown(drivertest_block_single_write,
                                               setup_driver, teardown_driver,
                                               pre),
      cmocka_unit_test_prestate_setup_teardown(drivertest_block_cache_write,
                                               setup_driver, teardown_driver,
                                               pre),
    };

  ret = cmocka_run_group_tests(tests, NULL, NULL);
  kmm_free(pre);
  return ret;
}
