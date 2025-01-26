/****************************************************************************
 * apps/benchmarks/sd_bench/sd_bench_main.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2016-2021 PX4 Development Team.
 * All rights reserved.
 *
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
 * - Refactoring for NuttX code style.
 * - Test result output has been modified to display total MB written.
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

#include <nuttx/clock.h>

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

#define BUFFER_ALIGN CONFIG_TESTING_SD_MEM_ALIGN_BYTES
/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct sdb_config
{
  int num_runs;
  int run_duration;
  bool synchronized;
  bool aligned;
  size_t total_blocks_written;
} sdb_config_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *BENCHMARK_FILE =
    CONFIG_BENCHMARK_SD_BENCH_DEVICE "/sd_bench";

static const size_t max_block = 65536;
static const size_t min_block = 1;
static const size_t default_block = 512;

static const size_t max_runs = 10000;
static const size_t min_runs = 1;
static const size_t default_runs = 5;

static const size_t max_duration = 60000;
static const size_t min_duration = 1;
static const size_t default_duration = 2000;

static const bool default_keep_test = false;
static const bool default_fsync = false;
static const bool default_verify = true;
static const bool default_aligned = false;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void write_test(int fd, sdb_config_t *cfg, uint8_t *block,
                       int block_size);
static int read_test(int fd, sdb_config_t *cfg, uint8_t *block,
                     int block_size);

static uint64_t time_fsync_us(int fd);
static struct timespec get_abs_time(void);
static uint64_t get_elapsed_time_us(const struct timespec *start);
static uint64_t time_fsync_us(int fd);
static float ts_to_kb(uint64_t bytes, uint64_t elapsed);
static float block_count_to_mb(size_t blocks, size_t block_size);
static const char *print_bool(const bool value);
static void usage(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static struct timespec get_abs_time(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

static uint64_t get_time_delta_us(const struct timespec *start,
                                  const struct timespec *end)
{
  uint64_t elapsed;
  elapsed = (((uint64_t)end->tv_sec * NSEC_PER_SEC) + end->tv_nsec);
  elapsed -= (((uint64_t)start->tv_sec * NSEC_PER_SEC) + start->tv_nsec);
  return elapsed / 1000.;
}

static uint64_t get_elapsed_time_us(const struct timespec *start)
{
  struct timespec now = get_abs_time();
  return get_time_delta_us(start, &now);
}

static uint64_t time_fsync_us(int fd)
{
  struct timespec start = get_abs_time();
  fsync(fd);
  return get_elapsed_time_us(&start);
}

static float ts_to_kb(uint64_t bytes, uint64_t elapsed)
{
  return (bytes / 1024.) / (elapsed / 1e6);
}

static float block_count_to_mb(size_t blocks, size_t block_size)
{
  return blocks * block_size / (float)(1024 * 1024);
}

static const char *print_bool(const bool value)
{
  return value ? "true" : "false";
}

static void write_test(int fd, sdb_config_t *cfg, uint8_t *block,
                       int block_size)
{
  struct timespec start;
  struct timespec write_start;
  size_t written;
  size_t num_blocks;
  uint64_t max_write_time;
  uint64_t fsync_time;
  uint64_t write_time;
  uint64_t elapsed;
  uint64_t total_elapsed = 0.;
  size_t total_blocks = 0;
  size_t *blocknumber = (size_t *)(void *)&block[0];

  printf("\n");
  printf("Testing Sequential Write Speed...\n");

  cfg->total_blocks_written = 0;

  for (int run = 0; run < cfg->num_runs; ++run)
    {
      start = get_abs_time();
      num_blocks = 0;
      max_write_time = 0;
      fsync_time = 0;

      while (get_elapsed_time_us(&start) < cfg->run_duration)
        {
          *blocknumber = total_blocks + num_blocks;
          write_start = get_abs_time();
          written = write(fd, block, block_size);
          write_time = get_elapsed_time_us(&write_start);

          if (write_time > max_write_time)
            {
              max_write_time = write_time;
            }

          if ((int)written != block_size)
            {
              printf("Write error: %d\n", errno);
              return;
            }

          if (cfg->synchronized)
            {
              fsync_time += time_fsync_us(fd);
            }

          ++num_blocks;
        }

      /* Note: if testing a slow device (SD Card) and the OS buffers a lot,
       * fsync can take really long, and it looks like the process hangs.
       * But it does not and the reported result will still be correct.
       */

      if (!cfg->synchronized)
        {
          fsync_time += time_fsync_us(fd);
        }

      elapsed = get_elapsed_time_us(&start);
      printf("  Run %2i: %8.1f KB/s, max write time: %4.3f ms (%.1f KB/s), "
             "fsync: %4.3f ms\n", run + 1,
             ts_to_kb(block_size * num_blocks, elapsed),
             max_write_time / 1.e3,
             ts_to_kb(block_size, max_write_time), fsync_time / 1e3);

      total_elapsed += elapsed;
      total_blocks += num_blocks;
    }

  cfg->total_blocks_written = total_blocks;
  printf("  Avg   : %8.1f KB/s, %3.3f MB written.\n",
         ts_to_kb(block_size * total_blocks, total_elapsed),
         block_count_to_mb(total_blocks, block_size));
}

static int read_test(int fd, sdb_config_t *cfg, uint8_t *block,
                     int block_size)
{
  uint8_t *read_block;
  uint64_t total_elapsed;
  size_t total_blocks;
  struct timespec start;
  size_t num_blocks;
  uint64_t max_read_time;
  uint64_t read_time;
  uint64_t elapsed;
  struct timespec read_start;
  size_t nread;

  printf("\n");
  printf("Testing Sequential Read Speed...\n");

  if (cfg->aligned)
    {
      read_block = (uint8_t *)memalign(BUFFER_ALIGN, block_size);
    }
  else
    {
      read_block = (uint8_t *)malloc(block_size);
    }

  if (!read_block)
    {
      printf("Failed to allocate memory block\n");
      return -1;
    }

  total_elapsed = 0.;
  total_blocks = 0;
  size_t *blocknumber = (size_t *)(void *) &read_block[0];

  for (int run = 0; run < cfg->num_runs;  ++run)
    {
      start = get_abs_time();
      num_blocks = 0;
      max_read_time = 0;

      while (get_elapsed_time_us(&start) < cfg->run_duration
             && total_blocks + num_blocks < cfg->total_blocks_written)
        {
          read_start = get_abs_time();
          nread = read(fd, read_block, block_size);
          read_time = get_elapsed_time_us(&read_start);

          if (read_time > max_read_time)
            {
              max_read_time = read_time;
            }

          if ((int)nread != block_size)
            {
              printf("Read error\n");
              free(read_block);
              return -1;
            }

          if (*blocknumber !=  total_blocks + num_blocks)
            {
              printf("Read data error at block: %zu wrote:0x%04zx "
                     "read:0x%04zx", total_blocks + num_blocks,
                     total_blocks + num_blocks, *blocknumber);
            }

          for (unsigned int i = sizeof(*blocknumber);
              i < (block_size - sizeof(*blocknumber)); ++i)
            {
              if (block[i] != read_block[i])
                {
                  printf("Read data error at offset: %zu wrote:0x%02x "
                         "read:0x%02x", total_blocks + num_blocks + i,
                         block[i], read_block[i]);
                }
            }

          ++num_blocks;
        }

      elapsed = get_elapsed_time_us(&start);

      if (num_blocks)
        {
          printf("  Run %2i: %8.1f KB/s, max read/verify time: %3.4f ms "
                 "(%.1f KB/s)\n", run + 1,
                 ts_to_kb(block_size * num_blocks, elapsed),
                 max_read_time / 1e3,
                 ts_to_kb(block_size, max_read_time));

          total_elapsed += elapsed;
          total_blocks += num_blocks;
        }
    }

  printf("  Avg   : %8.1f KB/s, %3.3f MB and verified\n",
         ts_to_kb(block_size * total_blocks, total_elapsed),
         block_count_to_mb(total_blocks, block_size));

  free(read_block);
  return 0;
}

static void usage(void)
{
  printf("Test the speed of an SD card or mount point\n");
  printf(CONFIG_BENCHMARK_SD_BENCH_PROGNAME
         ": [-b] [-r] [-d] [-k] [-s] [-a] [-v]\n");
  printf("  -b   Block size per write (%zu-%zu), default %zu\n",
         min_block, max_block, default_block);
  printf("  -r   Number of runs (%zu-%zu), default %zu\n",
         min_runs, max_runs, default_runs);
  printf("  -d   Max duration of a test (ms) (%zu-%zu), default %zu\n",
         min_duration, max_duration, default_duration);
  printf("  -k   Keep test file when finished, default %s\n",
         print_bool(default_keep_test));
  printf("  -s   Call fsync after each block, false calls fsync\n"
         "       only at the end of each run, default %s\n",
         print_bool(default_fsync));
  printf("  -a   Test performance on aligned data, default %s\n",
         print_bool(default_aligned));
  printf("  -v   Verify data and block number, default %s\n",
         print_bool(default_verify));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  size_t block_size = default_block;
  bool verify = default_verify;
  bool keep = default_keep_test;
  int ch;
  int bench_fd;
  sdb_config_t cfg;
  uint8_t *block = NULL;

  cfg.synchronized = default_fsync;
  cfg.num_runs = default_runs;
  cfg.run_duration = default_duration;
  cfg.aligned = default_aligned;

  while ((ch = getopt(argc, argv, "b:r:d:ksav")) != EOF)
    {
      switch (ch)
        {
        case 'b':
          block_size = strtol(optarg, NULL, 0);
          break;

        case 'r':
          cfg.num_runs = strtol(optarg, NULL, 0);
          break;

        case 'd':
          cfg.run_duration = strtol(optarg, NULL, 0);
          break;

        case 'k':
          keep = !default_keep_test;
          break;

        case 's':
          cfg.synchronized = !default_fsync;
          break;

        case 'a':
          cfg.aligned = !default_aligned;
          break;

        case 'v':
          verify = !default_verify;
          break;

        default:
          usage();
          return -1;
          break;
        }
    }

  if (cfg.run_duration > max_duration || cfg.run_duration < min_duration)
    {
      printf("Duration outside of allowable range.\n");
      usage();
      exit(EXIT_FAILURE);
    }

  if (block_size > max_block || block_size < min_block)
    {
      printf("Bytes outside allowable range.\n");
      usage();
      exit(EXIT_FAILURE);
    }

  if (cfg.num_runs > max_runs || cfg.num_runs < min_runs)
    {
      printf("Runs outside allowable range.\n");
      usage();
      exit(EXIT_FAILURE);
    }

  cfg.run_duration *= 1000;
  bench_fd = open(BENCHMARK_FILE,
                  O_CREAT | (verify ? O_RDWR : O_WRONLY) | O_TRUNC);

  if (bench_fd < 0)
    {
      printf("Can't open benchmark file %s (%d)\n",
             BENCHMARK_FILE, bench_fd);
      exit(EXIT_FAILURE);
    }

  if (cfg.aligned)
    {
      block = (uint8_t *)memalign(BUFFER_ALIGN, block_size);
    }
  else
    {
      block = (uint8_t *)malloc(block_size);
    }

  if (!block)
    {
      printf("Failed to allocate memory block\n");
      close(bench_fd);
      exit(EXIT_FAILURE);
    }

  for (int j = 0; j < block_size; ++j)
    {
      block[j] = (uint8_t)j;
    }

  printf("Using block size = %zu bytes, sync = %s\n", block_size,
         print_bool(cfg.synchronized));

  write_test(bench_fd, &cfg, block, block_size);

  if (verify)
    {
      fsync(bench_fd);
      lseek(bench_fd, 0, SEEK_SET);
      read_test(bench_fd, &cfg, block, block_size);
    }

  free(block);
  close(bench_fd);

  if (!keep)
  {
    unlink(BENCHMARK_FILE);
  }

  return 0;
}
