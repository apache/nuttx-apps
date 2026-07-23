/****************************************************************************
 * apps/system/xipfs/xipfs_main.c
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
#include <sys/statfs.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/fs/ioctl.h>
#include <nuttx/fs/xipfs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAP_COLS 50

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* One row of the file table, gathered before anything is printed so that a
 * failure part way through does not leave half a report behind.
 */

struct xipfs_entry_s
{
  /* A path relative to the mountpoint, not one component, so that a file in
   * a subdirectory is reported the way the volume names it.
   */

  char     name[XIPFS_PATH_MAX + 1];
  uint32_t start;                    /* Relative to the data region */
  uint32_t nblocks;
  uint32_t size;
  uint32_t pincount;
};

struct xipfs_survey_s
{
  FAR char *map;                     /* One char per erase block    */
  FAR struct xipfs_entry_s *files;
  int      nfiles;
  uint32_t nblocks;                  /* Blocks in the data region   */
  uint32_t blocksize;
  uint32_t used;
  uint32_t freeblocks;
  uint32_t largestrun;
  uint32_t nruns;                    /* Distinct free runs          */
  uint32_t pinned;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "USAGE: %s [-n] [-t <ms>] [<mountpoint>]\n", progname);
  fprintf(stderr, "\nCompact a xipfs volume, or report what one looks "
                  "like.\n");
  fprintf(stderr, "\nWhere:\n");
  fprintf(stderr, "\t-n: Dry run.  Report block usage and fragmentation "
                  "and exit\n");
  fprintf(stderr, "\t    without moving anything.\n");
  fprintf(stderr, "\t-t <ms>: Time budget for the compaction pass.  "
                  "0, the default,\n");
  fprintf(stderr, "\t    means run to completion.\n");
  fprintf(stderr, "\t<mountpoint>: Default: %s\n",
          CONFIG_SYSTEM_XIPFS_MOUNTPOINT);
  exit(exitcode);
}

/****************************************************************************
 * Name: survey_free
 ****************************************************************************/

static void survey_free(FAR struct xipfs_survey_s *s)
{
  free(s->map);
  free(s->files);
  s->map   = NULL;
  s->files = NULL;
}

/****************************************************************************
 * Name: parse_max_ms
 ****************************************************************************/

static int parse_max_ms(FAR const char *arg, FAR uint32_t *max_ms)
{
  FAR char *endptr;
  unsigned long value;

  if (arg[0] == '-')
    {
      return -EINVAL;
    }

  errno = 0;
  value = strtoul(arg, &endptr, 10);
  if (arg == endptr || *endptr != '\0' || errno == ERANGE ||
      value > UINT32_MAX)
    {
      return -EINVAL;
    }

  *max_ms = value;
  return OK;
}

/****************************************************************************
 * Name: survey_walk
 *
 * Description:
 *   Record where every file below 'dir' physically sits, descending into
 *   the directories xipfs synthesises from the names.  'rel' is the path of
 *   'dir' relative to the mount, which is what a file is recorded under so
 *   that the table names it the way the volume does.
 *
 ****************************************************************************/

static int survey_walk(FAR const char *dir, FAR const char *rel,
                       FAR struct xipfs_survey_s *s, FAR int *capacity)
{
  struct xipfs_extent_info_s info;
  FAR struct dirent *de;
  FAR DIR *dirp;
  uint32_t i;
  int ret;

  dirp = opendir(dir);
  if (dirp == NULL)
    {
      fprintf(stderr, "ERROR: opendir %s failed: %d\n", dir, errno);
      return -errno;
    }

  while ((de = readdir(dirp)) != NULL)
    {
      FAR struct xipfs_entry_s *e;
      char path[PATH_MAX];
      char sub[XIPFS_PATH_MAX + 1];
      int fd;

      snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);

      if (rel[0] == '\0')
        {
          strlcpy(sub, de->d_name, sizeof(sub));
        }
      else
        {
          snprintf(sub, sizeof(sub), "%s/%s", rel, de->d_name);
        }

      if (de->d_type == DTYPE_DIRECTORY)
        {
          ret = survey_walk(path, sub, s, capacity);
          if (ret < 0)
            {
              closedir(dirp);
              return ret;
            }

          continue;
        }

      fd = open(path, O_RDONLY | O_CLOEXEC);
      if (fd < 0)
        {
          continue;
        }

      ret = ioctl(fd, XIPFSIOC_EXTENTINFO, (unsigned long)(uintptr_t)&info);
      close(fd);

      if (ret < 0)
        {
          continue;
        }

      if (s->nfiles == *capacity)
        {
          FAR void *tmp;

          *capacity *= 2;
          tmp = realloc(s->files, *capacity * sizeof(struct xipfs_entry_s));
          if (tmp == NULL)
            {
              closedir(dirp);
              return -ENOMEM;
            }

          s->files = tmp;
        }

      e = &s->files[s->nfiles++];
      strlcpy(e->name, sub, sizeof(e->name));
      e->start    = info.start_block - info.data_start;
      e->nblocks  = info.nblocks;
      e->size     = info.size;
      e->pincount = info.pincount;

      if (e->pincount > 0)
        {
          s->pinned += e->nblocks;
        }

      for (i = 0; i < e->nblocks; i++)
        {
          if (e->start + i < s->nblocks)
            {
              s->map[e->start + i] = e->pincount > 0 ? 'P' : '#';
            }
        }
    }

  closedir(dirp);
  return OK;
}

/****************************************************************************
 * Name: survey_collect
 *
 * Description:
 *   Walk the mount and record where every file physically sits.  The block
 *   counts come from statfs so that an empty volume still reports its
 *   geometry -- XIPFSIOC_EXTENTINFO can only be issued against a regular
 *   file, so with no files there is nothing to ask.
 *
 ****************************************************************************/

static int survey_collect(FAR const char *mount,
                          FAR struct xipfs_survey_s *s)
{
  struct statfs sbuf;
  uint32_t run = 0;
  uint32_t i;
  int capacity = 8;
  int ret;

  memset(s, 0, sizeof(*s));

  if (statfs(mount, &sbuf) < 0)
    {
      fprintf(stderr, "ERROR: statfs %s failed: %d\n", mount, errno);
      return -errno;
    }

  s->nblocks   = sbuf.f_blocks;
  s->blocksize = sbuf.f_bsize;

  if (s->nblocks == 0)
    {
      fprintf(stderr, "ERROR: %s reports no blocks; not a xipfs mount?\n",
              mount);
      return -EINVAL;
    }

  s->map = malloc(s->nblocks);
  if (s->map == NULL)
    {
      return -ENOMEM;
    }

  memset(s->map, '.', s->nblocks);

  s->files = malloc(capacity * sizeof(struct xipfs_entry_s));
  if (s->files == NULL)
    {
      survey_free(s);
      return -ENOMEM;
    }

  ret = survey_walk(mount, "", s, &capacity);
  if (ret < 0)
    {
      survey_free(s);
      return ret;
    }

  /* Reduce the map to the numbers a caller actually decides on: how much is
   * free, and how much of that is reachable by a single allocation.
   */

  for (i = 0; i < s->nblocks; i++)
    {
      if (s->map[i] == '.')
        {
          s->freeblocks++;
          if (run == 0)
            {
              s->nruns++;
            }

          if (++run > s->largestrun)
            {
              s->largestrun = run;
            }
        }
      else
        {
          s->used++;
          run = 0;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: survey_report
 ****************************************************************************/

static void survey_report(FAR const char *mount,
                          FAR struct xipfs_survey_s *s)
{
  uint32_t frag;
  uint32_t i;
  int width;
  int f;

  printf("%s: %" PRIu32 " blocks of %" PRIu32 " bytes (%" PRIu64
         " KB)\n", mount, s->nblocks, s->blocksize,
         (uint64_t)s->nblocks * s->blocksize / 1024);

  if (s->nfiles > 0)
    {
      /* A name here is a path relative to the mountpoint, so it can be
       * longer than one component.  Widen the column to the longest one
       * rather than let it push the rest of the row out of alignment.
       */

      width = XIPFS_NAME_MAX;
      for (f = 0; f < s->nfiles; f++)
        {
          int len = (int)strlen(s->files[f].name);

          if (len > width)
            {
              width = len;
            }
        }

      printf("\n  %-*s %6s %7s %8s %4s\n", width, "file",
             "start", "blocks", "bytes", "pin");

      for (f = 0; f < s->nfiles; f++)
        {
          FAR struct xipfs_entry_s *e = &s->files[f];

          printf("  %-*s %6" PRIu32 " %7" PRIu32 " %8" PRIu32
                 " %4" PRIu32 "\n", width, e->name, e->start, e->nblocks,
                 e->size, e->pincount);
        }
    }

  printf("\n");

  for (i = 0; i < s->nblocks; i += MAP_COLS)
    {
      uint32_t n = s->nblocks - i;
      char row[MAP_COLS + 1];

      if (n > MAP_COLS)
        {
          n = MAP_COLS;
        }

      memcpy(row, &s->map[i], n);
      row[n] = '\0';
      printf("  %4" PRIu32 " |%s|\n", i, row);
    }

  printf("\n  '.' free   '#' in use   'P' pinned by a live mapping\n");

  /* The share of free space that a single allocation cannot reach.  This is
   * the number that says whether compacting is worth doing: at 0% the
   * largest possible file already fits, whatever the map looks like.
   */

  frag = 0;
  if (s->freeblocks > 0)
    {
      frag = (uint32_t)(((uint64_t)(s->freeblocks - s->largestrun) * 100) /
                        s->freeblocks);
    }

  printf("\n  used %" PRIu32 ", free %" PRIu32
         ", largest free run %" PRIu32 " blocks (%" PRIu64 " KB)\n",
         s->used, s->freeblocks, s->largestrun,
         (uint64_t)s->largestrun * s->blocksize / 1024);
  printf("  %" PRIu32 " free run%s, fragmentation %" PRIu32 "%%\n",
         s->nruns, s->nruns == 1 ? "" : "s", frag);

  if (s->pinned > 0)
    {
      printf("  %" PRIu32 " block%s pinned and cannot be relocated\n",
             s->pinned, s->pinned == 1 ? "" : "s");
    }

  if (frag == 0)
    {
      printf("  nothing to gain: all free space is already contiguous\n");
    }
  else
    {
      printf("  compacting could raise the largest run to %" PRIu32
             " blocks (%" PRIu64 " KB)\n", s->freeblocks,
             (uint64_t)s->freeblocks * s->blocksize / 1024);
    }
}

/****************************************************************************
 * Name: reason_str
 ****************************************************************************/

static FAR const char *reason_str(int reason)
{
  switch (reason)
    {
      case XIPFS_DEFRAG_DONE:
        return "done, nothing left to compact";
      case XIPFS_DEFRAG_BLOCKED_PINS:
        return "blocked by a live XIP mapping";
      case XIPFS_DEFRAG_BLOCKED_RAM:
        return "blocked by a transient resource shortage";
      case XIPFS_DEFRAG_TIME_BUDGET:
        return "ran out of the time budget";
      case XIPFS_DEFRAG_ERROR:
        return "stopped on a media error";
      case XIPFS_DEFRAG_BLOCKED_OPEN:
        return "blocked by an open file";
      default:
        return "unknown";
    }
}

/****************************************************************************
 * Name: run_defrag
 *
 * Description:
 *   Compact the volume.  The ioctl goes through a descriptor for the
 *   mountpoint directory rather than for a file inside the volume: no file
 *   is open, so nothing is held immovable by the act of asking and one pass
 *   can reach every extent.
 *
 ****************************************************************************/

static int run_defrag(FAR const char *mount, FAR struct xipfs_survey_s *s,
                      uint32_t max_ms)
{
  struct xipfs_defrag_arg_s arg;
  int fd;
  int ret;

  if (s->nfiles == 0)
    {
      printf("\n%s is empty; nothing to compact\n", mount);
      return OK;
    }

  memset(&arg, 0, sizeof(arg));
  arg.max_ms = max_ms;

  fd = open(mount, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open %s failed: %d\n", mount, errno);
      return -errno;
    }

  printf("\ncompacting %s ...\n", mount);

  ret = ioctl(fd, XIPFSIOC_DEFRAG, (unsigned long)(uintptr_t)&arg);
  close(fd);

  if (ret < 0)
    {
      fprintf(stderr, "ERROR: defrag failed: %d\n", errno);
      return -errno;
    }

  printf("  %s: moved %" PRIu32 " extent%s, reclaimed %" PRIu32
         " block%s, %" PRIu32 " pinned\n", reason_str(arg.result.reason),
         arg.result.extents_moved,
         arg.result.extents_moved == 1 ? "" : "s",
         arg.result.blocks_reclaimed,
         arg.result.blocks_reclaimed == 1 ? "" : "s",
         arg.result.blocks_pinned);

  printf("  largest free run now %zu bytes\n",
         arg.result.largest_free_run);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR const char *mount = CONFIG_SYSTEM_XIPFS_MOUNTPOINT;
  struct xipfs_survey_s survey;
  bool dryrun = false;
  uint32_t max_ms = 0;
  int option;
  int ret;

  while ((option = getopt(argc, argv, "nt:h")) != ERROR)
    {
      switch (option)
        {
          case 'n':
            dryrun = true;
            break;

          case 't':
            ret = parse_max_ms(optarg, &max_ms);
            if (ret < 0)
              {
                fprintf(stderr, "ERROR: invalid time budget: %s\n", optarg);
                show_usage(argv[0], EXIT_FAILURE);
              }
            break;

          case 'h':
            show_usage(argv[0], EXIT_SUCCESS);
            break;

          default:
            show_usage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  if (optind < argc)
    {
      mount = argv[optind];
    }

  ret = survey_collect(mount, &survey);
  if (ret < 0)
    {
      return EXIT_FAILURE;
    }

  survey_report(mount, &survey);

  if (!dryrun)
    {
      ret = run_defrag(mount, &survey, max_ms);
      survey_free(&survey);

      if (ret < 0)
        {
          return EXIT_FAILURE;
        }

      /* Re-survey so the map shown last is the one on the media now. */

      ret = survey_collect(mount, &survey);
      if (ret < 0)
        {
          return EXIT_FAILURE;
        }

      printf("\nafter compaction:\n");
      survey_report(mount, &survey);
    }

  survey_free(&survey);
  return EXIT_SUCCESS;
}
