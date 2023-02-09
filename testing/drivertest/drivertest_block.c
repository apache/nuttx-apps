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
 * Pre-processor Definitions
 ****************************************************************************/

#define BLKTEST_MAXLEN  255
#define BLKTEST_LOOPS   100

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pre_build
{
  FAR const char *mountpt;
};

struct test_state_s
{
  FAR char *context[BLKTEST_MAXLEN];
  size_t len[BLKTEST_LOOPS];
  uint32_t crc[BLKTEST_LOOPS];
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
  printf("Usage: %s -m <mountpt>\n", progname);
  printf("Where:\n");
  printf("  -m <mountpt> Block device or mtd device"
         " mount location[default location: dev/ram10].\n");
  exit(exitcode);
}

/****************************************************************************
 * Name: parse_commandline
 ****************************************************************************/

static void parse_commandline(int argc, FAR char **argv,
                              FAR struct pre_build *pre_build)
{
  int option;

  while ((option = getopt(argc, argv, "m:")) != ERROR)
    {
      switch (option)
        {
          case 'm':
            pre_build->mountpt = optarg;
            break;
          case '?':
            printf("Unknown option: %c\n", optopt);
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
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

static void blktest_randcontext(FAR struct test_state_s *test_state)
{
  int i;
  int j;
  int rnd;

  for (i = 0; i < BLKTEST_LOOPS; i++)
    {
      rnd = (rand() % BLKTEST_MAXLEN) + 1;
      test_state->context[i] = malloc(rnd + 1);
      assert_true(test_state->context[i] != NULL);
      for (j = 0; j < rnd; j++)
        {
          test_state->context[i][j] = blktest_randchar();
        }

      test_state->context[i][rnd] = '\0';
      size_t len = strlen(test_state->context[i]);
      test_state->len[i] = len;
      test_state->crc[i] = crc32((FAR uint8_t *)test_state->context[i], len);
    }
}

/****************************************************************************
 * Name: setup
 ****************************************************************************/

static int setup(FAR void **state)
{
  FAR struct test_state_s *test_state;
  FAR struct pre_build    *pre_build;
  time_t t;

  pre_build = (struct pre_build *)*state;

  /* Allocate memory space and initialize */

  test_state = zalloc(sizeof(struct test_state_s));
  assert_false(test_state == NULL);

  /* Seed the random number generated */

  srand((unsigned)time(&t));

  /* Open */

  test_state->fd = open(pre_build->mountpt, O_RDWR | O_DIRECT);
  assert_false(test_state->fd < 0);
  *state = test_state;

  return 0;
}

/****************************************************************************
 * Name: blktest_stress
 ****************************************************************************/

static void blktest_stress(FAR void **state)
{
  FAR struct test_state_s *test_state;
  int i;
  int ret;
  char *output;
  uint32_t output_crc;

  test_state = (struct test_state_s *)*state;

  /* Create some random context */

  blktest_randcontext(test_state);

  /* Writes all text to the block device bypassing the buffer cache,
   * ensuring that reads are read from the block device.
   * Get the value of crc32 after reading and compare it with the crc32
   * of the previously written text.
   */

  for (i = 0; i < BLKTEST_LOOPS; i++)
    {
      ret = write(test_state->fd, test_state->context[i],
                  test_state->len[i]);
      assert_true(ret == test_state->len[i]);
      fsync(test_state->fd);
    }

  /* Reset read and write position */

  lseek(test_state->fd, 0, SEEK_SET);

  output = malloc (BLKTEST_MAXLEN);
  assert_true(output != NULL);

  for (i = 0; i < BLKTEST_LOOPS; i++)
    {
      memset(output, 0, BLKTEST_MAXLEN);
      ret = read(test_state->fd, output, test_state->len[i]);
      assert_int_equal(ret, test_state->len[i]);

      output_crc = crc32((FAR uint8_t *)output, test_state->len[i]);
      assert_false(output_crc != test_state->crc[i]);
    }

  free(output);
}

/****************************************************************************
 * Name: teardown
 ****************************************************************************/

static int teardown(FAR void **state)
{
  FAR struct test_state_s *test_state;

  test_state = (struct test_state_s *)*state;

  close(test_state->fd);
  free(test_state);

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
  FAR struct pre_build pre_build = {
    .mountpt = "dev/ram10"
  };

  parse_commandline(argc, argv, &pre_build);
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test_prestate_setup_teardown(blktest_stress, setup,
                                               teardown, &pre_build),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
