/****************************************************************************
 * apps/testing/drivertest/drivertest_block.c
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

#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

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
#include <nuttx/crc32.h>

/****************************************************************************
 * Private Type
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pre_build_s
{
  FAR const char *source;
  struct geometry cfg;
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

  pre->source = NULL;

  while ((option = getopt(argc, argv, "m:")) != ERROR)
    {
      switch (option)
        {
          case 'm':
            pre->source = optarg;
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  if (pre->source == NULL)
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

static void blktest_randcontext(FAR struct pre_build_s *pre, char *input)
{
  int i;
  for (i = 0; i < pre->cfg.geo_sectorsize - 1; i++)
    {
      input[i] = blktest_randchar();
    }

  input[i] = '\0';
}

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  FAR struct pre_build_s *pre = (FAR struct pre_build_s *)*state;
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
 * Name: blktest_stress
 ****************************************************************************/

static void blktest_stress(FAR void **state)
{
  FAR struct pre_build_s *pre;
  FAR char *input;
  FAR char *output;
  uint32_t input_crc;
  uint32_t output_crc;
  int ret;

  pre = (FAR struct pre_build_s *)*state;

  input = malloc(pre->cfg.geo_sectorsize);
  assert_true(input != NULL);
  output = malloc(pre->cfg.geo_sectorsize);
  assert_true(output != NULL);

  for (int i = 0; i < pre->cfg.geo_nsectors; i++)
    {
      input_crc = 0;
      output_crc = 0;

      blktest_randcontext(pre, input);
      input_crc = crc32((FAR uint8_t *)input, pre->cfg.geo_sectorsize);
      ret = write(pre->fd, input, pre->cfg.geo_sectorsize);
      assert_true(ret == pre->cfg.geo_sectorsize);
      fsync(pre->fd);

      lseek(pre->fd, i * pre->cfg.geo_sectorsize, SEEK_SET);

      ret = read(pre->fd, output, pre->cfg.geo_sectorsize);
      assert_int_equal(ret, pre->cfg.geo_sectorsize);

      output_crc = crc32((FAR uint8_t *)output, pre->cfg.geo_sectorsize);

      assert_false(output_crc != input_crc);
    }

  free(input);
  free(output);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(FAR void **state)
{
  FAR struct pre_build_s *pre;

  pre = (FAR struct pre_build_s *)*state;

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
  struct pre_build_s pre;

  parse_commandline(argc, argv, &pre);
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(blktest_stress, setup,
                                               teardown, &pre),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
