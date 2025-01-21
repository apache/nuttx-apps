/****************************************************************************
 * apps/testing/drivers/sd_stress/sd_stress_main.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2016-2021 PX4 Development Team. All rights
 * reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/* Originally ported from PX4 https://github.com/PX4/PX4-Autopilot,
 * with the following additions:
 *
 * - The number of files can be specified from the command line.
 * - Bytes are written and read back from created files to verify integrity.
 * - The bytes written are obtained from a static set,
 *   rather than a constant 0xAA
 * - The results are reported as a floating point, millisecond value.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <nuttx/clock.h>

static const size_t MAX_PATH_LEN = 52;

static const char *TEMPDIR = CONFIG_TESTING_SD_STRESS_DEVICE"/stress";
static const char *TEMPDIR2 = CONFIG_TESTING_SD_STRESS_DEVICE"/moved";
static const char *TEMPFILE = "tmp";

static const size_t max_runs = 10000;
static const size_t min_runs = 1;
static const size_t default_runs = 32;

static const size_t max_bytes = 10000;
static const size_t min_bytes = 1;
static const size_t default_bytes = 4096;

static const size_t max_files = 999;
static const size_t min_files = 1;
static const size_t default_files = 64;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  printf("Stress test on a mount point\n");
  printf(CONFIG_TESTING_SD_STRESS_PROGNAME ": [-r] [-b] [-f]\n");
  printf("  -r   Number of runs (%zu-%zu), default %zu\n",
         min_runs, max_runs, default_runs);
  printf("  -b   Number of bytes (%zu-%zu), default %zu\n",
         min_bytes, max_bytes, default_bytes);
  printf("  -f   Number of files (%zu-%zu), default %zu\n",
         min_files, max_files, default_files);
}

static bool create_dir(const char *path)
{
  int ret = mkdir(TEMPDIR, S_IRWXU | S_IRWXG | S_IRWXO);

  if (ret < 0)
    {
      printf("mkdir %s failed, ret: %d, errno: %d -> %s\n",
             path, ret, errno, strerror(errno));
      return false;
    }

  return true;
}

static bool remove_dir(const char *path)
{
  int ret = rmdir(TEMPDIR2);

  if (ret < 0)
    {
      printf("rmdir %s failed, ret: %d, errno: %d -> %s\n",
             path, ret, errno, strerror(errno));
      return false;
    }

  return true;
}

static bool create_files(const char *dir, const char *name,
                         size_t num_files, char *bytes, size_t num_bytes)
{
  bool passed = true;
  if (num_files > 999)
    {
      printf("too many files\n");
      return false;
    }

  char *read_bytes = (char *)malloc(num_bytes);

  if (!read_bytes)
    {
      printf("malloc failed for read bytes bufffer\n");
      return false;
    }

  size_t path_len = strlen(dir) + strlen(name);

  if (path_len + 5 >= MAX_PATH_LEN)
    {
      printf("path name too long\n");
      return false;
    }

  for (size_t i = 0; i < num_files; ++i)
    {
      char path[MAX_PATH_LEN];
      snprintf(path, MAX_PATH_LEN, "%s/%s%03zu", dir, name, i);

      memset(read_bytes, 0x0, num_bytes);

      /* Fill the file with a set of incrementing bytes */

      for (size_t j = 0; j < num_bytes; j++)
        {
          bytes[j] = (bytes[j] + i) & 0xff;
        }

      int fd = open(path, O_CREAT | O_RDWR);

      if (fd < 0)
        {
          printf("open %s failed, errno: %d -> %s\n",
                 path, errno, strerror(errno));
          passed = false;
          break;
        }

      int ret = write(fd, bytes, num_bytes);

      if (ret != (int)num_bytes)
        {
          printf("write %s failed, ret: %d, errno %d -> %s\n",
                 path, ret, errno, strerror(errno));
          passed = false;
          break;
        }

      ret = lseek(fd, 0, SEEK_SET);

      if (ret < 0)
        {
          printf("lseek %s failed, ret: %d, errno %d -> %s\n",
                 path, ret, errno, strerror(errno));
          passed = false;
          break;
        }

      ret = read(fd, read_bytes, num_bytes);

      if (ret != (int)num_bytes)
        {
          printf("read %s failed, ret: %d, errno %d -> %s\n",
                 path, ret, errno, strerror(errno));
          passed = false;
          break;
        }

      if (memcmp(read_bytes, bytes, num_bytes) != 0)
        {
          printf("read and write buffers are not the same\n");
          passed = false;
          break;
        }

      ret = close(fd);

      if (ret < 0)
        {
          printf("close %s failed, ret: %d, errno %d -> %s\n",
                 path, ret, errno, strerror(errno));
          passed = false;
          break;
        }
    }

  free(read_bytes);
  return passed;
}

static bool remove_files(const char *dir, const char *name,
                          size_t num_files)
{
  if (num_files > 999)
    {
      printf("too many files\n");
      return false;
    }

  size_t path_len = strlen(dir) + strlen(name);

  if (path_len + 5 >= MAX_PATH_LEN)
    {
      printf("path name too long\n");
      return false;
    }

  for (size_t i = 0; i < num_files; ++i)
    {
      char path[MAX_PATH_LEN];
      snprintf(path, MAX_PATH_LEN, "%s/%s%03zu", dir, name, i);

      int ret = unlink(path);

      if (ret < 0)
        {
          printf("unlink %s failed, ret: %d, errno %d -> %s\n",
                 path, ret, errno, strerror(errno));
          return false;
        }
    }

  return true;
}

static bool rename_dir(const char *old_dir, const char *new_dir)
{
  int ret = rename(old_dir, new_dir);

  if (ret < 0)
    {
      printf("rename %s to %s failed, ret: %d, errno %d -> %s\n",
             old_dir, new_dir, ret, errno, strerror(errno));
      return false;
    }

  return true;
}

static struct timespec get_abs_time(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

static uint64_t get_time_delta(const struct timespec *start,
                               const struct timespec *end)
{
  uint64_t elapsed;
  elapsed = (((uint64_t)end->tv_sec * NSEC_PER_SEC) + end->tv_nsec);
  elapsed -= (((uint64_t)start->tv_sec * NSEC_PER_SEC) + start->tv_nsec);
  return elapsed / 1000;
}

static float get_elapsed_time_ms(const struct timespec *start)
{
  struct timespec now = get_abs_time();
  return get_time_delta(start, &now) / (float)USEC_PER_MSEC;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int ch;
  int ret;
  size_t num_runs = default_runs;
  size_t num_bytes = default_bytes;
  size_t num_files = default_files;
  float total_time;
  float elapsed_time;
  struct timespec start;
  char * bytes;

  while ((ch = getopt(argc, argv, "r:b:f:")) != EOF)
    {
      switch (ch)
        {
        case 'r':
          num_runs = strtol(optarg, NULL, 0);
          break;

        case 'b':
          num_bytes = strtol(optarg, NULL, 0);
          break;

        case 'f':
          num_files = strtol(optarg, NULL, 0);
          break;

        default:
          usage();
          return -1;
          break;
        }
    }

  if (num_bytes > max_bytes || num_bytes < min_bytes)
    {
      printf("Bytes outside allowable range\n");
      usage();
      exit(EXIT_FAILURE);
    }

  if (num_runs > max_runs || num_runs < min_runs)
    {
      printf("Runs outside allowable range\n");
      usage();
      exit(EXIT_FAILURE);
    }

  if (num_files > max_files || num_files < min_files)
    {
      printf("File count outside allowable range\n");
      usage();
      exit(EXIT_FAILURE);
    }

  printf("Start stress test with %zu files, %zu bytes and %zu iterations.\n",
         num_files, num_bytes, num_runs);

  bytes = (char *)malloc(num_bytes);
  ret = 0;

  if (!bytes)
    {
      printf("Failed to allocate byte buffer.\n");
      exit(EXIT_FAILURE);
    }

  for (size_t i = 0; i < num_bytes; i++)
    {
      bytes[i] = i & 0xff;
    }

  total_time = 0;

  for (size_t i = 0; i < num_runs; ++i)
    {
      start = get_abs_time();

      const bool result =
        create_dir(TEMPDIR)
        && create_files(TEMPDIR, TEMPFILE, num_files, bytes, num_bytes)
        && rename_dir(TEMPDIR, TEMPDIR2)
        && remove_files(TEMPDIR2, TEMPFILE, num_files)
        && remove_dir(TEMPDIR2);

      elapsed_time = get_elapsed_time_ms(&start);
      total_time += elapsed_time;
      printf("iteration %zu took %.3f ms: %s\n", i,
             elapsed_time, result ? "OK" : "FAIL");

      if (!result)
        {
          ret = -1;
          break;
        }
    }

  printf("Test %s: Average time: %.3f ms\n",
         ret ? "FAIL" : "OK", total_time / num_runs);

  free(bytes);
  return ret;
}
