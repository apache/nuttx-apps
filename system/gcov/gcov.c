/****************************************************************************
 * apps/system/gcov/gcov.c
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

#include <gcov.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <nuttx/crc16.h>
#include <nuttx/streams.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gcov_arg
{
  FAR const char *name;
  struct lib_stdoutstream_s stdoutstream;
  struct lib_hexdumpstream_s hexstream;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  printf("\nUsage: %s [-d path] [-t strip] [-r] [-h]\n", progname);
  printf("\nWhere:\n");
  printf("  -d dump the coverage, path is the path to the coverage file, "
         "the default output is to stdout\n");
  printf("  -t strip the path prefix number\n");
  printf("  -r reset the coverage\n");
  printf("  -h show this text and exits.\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: gcov_dump
 ****************************************************************************/

static void gcov_dump(FAR const char * path, FAR const char *strip)
{
  if (path == NULL || access(path, F_OK) != 0 || atoi(strip) <= 0)
    {
      fprintf(stderr, "ERROR: Invalid parameter\n");
      return;
    }

  setenv("GCOV_PREFIX_STRIP", strip, 1);
  setenv("GCOV_PREFIX", path, 1);
  __gcov_dump();
}

/****************************************************************************
 * Name: stdout_dump
 ****************************************************************************/

#ifndef CONFIG_COVERAGE_TOOLCHAIN
static void stdout_dump(FAR const void *buffer, size_t size,
                        FAR void *arg)
{
  FAR struct gcov_arg *args = (FAR struct gcov_arg *)arg;
  uint16_t checksum = 0;
  int i;

  if (size == 0)
    {
      return;
    }

  for (i = 0; i < size; i++)
    {
      checksum += ((FAR const uint8_t *)buffer)[i];
    }

  lib_sprintf(&args->stdoutstream.common,
              "gcov start filename:%s size: %zuByte\n",
              args->name, size);
  lib_stream_puts(&args->hexstream, buffer, size);
  lib_stream_flush(&args->hexstream);
  lib_sprintf(&args->stdoutstream.common,
              "gcov end filename:%s checksum: %#0x\n",
              args->name, checksum);
  lib_stream_flush(&args->stdoutstream);
}

/****************************************************************************
 * Name: stdout_filename
 ****************************************************************************/

static void stdout_filename(const char *name, FAR void *arg)
{
  FAR struct gcov_arg *args = (FAR struct gcov_arg *)arg;
  args->name = name;
  __gcov_filename_to_gcfn(name, NULL, NULL);
}

/****************************************************************************
 * Name: gcov_stdout_dump
 *
 * Description:
 *   Dump the gcov information of all translation units to stdout.
 *
 ****************************************************************************/

static void gcov_stdout_dump(void)
{
  FAR struct gcov_info *info = __gcov_info_start;
  FAR struct gcov_info *end = __gcov_info_end;
  struct gcov_arg arg;

  lib_stdoutstream(&arg.stdoutstream, stdout);
  lib_hexdumpstream(&arg.hexstream, &arg.stdoutstream.common);

  while (info != end)
    {
      __gcov_info_to_gcda(info, stdout_filename, stdout_dump, NULL, &arg);
      info = info->next;
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *strip = "99";
  FAR const char *path = NULL;
  int option;

  if (argc < 2)
    {
      show_usage(argv[0]);
    }

  while ((option = getopt(argc, argv, "d::t:rh")) != ERROR)
    {
      switch (option)
        {
        case 'd':
          path = optarg;
          break;

        case 't':
          strip = optarg;
          break;

        case 'r':
          __gcov_reset();
          break;

        case '?':
        default:
          fprintf(stderr, "ERROR: Unrecognized option\n");

        case 'h':
          show_usage(argv[0]);
        }
    }

#ifndef CONFIG_COVERAGE_TOOLCHAIN
  if (path == NULL)
    {
      gcov_stdout_dump();
    }
  else
#endif
    {
      gcov_dump(path, strip);
    }

  return 0;
}
