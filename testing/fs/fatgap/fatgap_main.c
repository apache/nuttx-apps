/****************************************************************************
 * apps/testing/fs/fatgap/fatgap_main.c
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

/* A write past EOF on a FAT file must keep the old data and the gap must
 * read as zero, for every position of the old size in its cluster.
 *
 * Each volume is a RAM disk that is filled with 0xa5 before the format.  The
 * format does not write the data clusters, so a cluster that the file system
 * does not zero does not read as zero by accident.
 *
 * Each case reads the file back on a new handle and after a remount, and
 * compares the number of free clusters.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <sys/mount.h>
#include <sys/statfs.h>

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/drivers/ramdisk.h>

#include "fsutils/mkfatfs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FATGAP_MINOR     99
#define FATGAP_DEV       "/dev/ram99"
#define FATGAP_MNT       "/fatgap"
#define FATGAP_FILE      FATGAP_MNT "/A.BIN"
#define FATGAP_SECTSIZE  512
#define FATGAP_FILL      0xa5
#define FATGAP_SMALLIO   37
#define FATGAP_NITEMS(a) (sizeof(a) / sizeof((a)[0]))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fatgap_vol_s
{
  int fattype;
  uint32_t nsectors;
  int clustshift;              /* Log2 of the sectors per cluster */
};

/* The sizes are half clusters plus bytes */

struct fatgap_case_s
{
  FAR const char *name;
  int size_half;               /* Old size of the file */
  int size_byte;
  int pos_half;                /* Position of the write */
  int pos_byte;
  int len_half;                /* Length of the write */
  int len_byte;
};

enum fatgap_mode_e
{
  MODE_REOPEN,                 /* A new handle writes past EOF */
  MODE_SAMEFD,                 /* The last sector is dirty in the handle */
  MODE_TWOFD,                  /* A second handle has the last sector */
  MODE_NUM
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct fatgap_vol_s g_vols[] =
{
  { 12, 1024,  2 },            /* 2 KiB clusters */
  { 16, 4400,  0 },            /* 512 byte clusters */
  { 32, 70000, 0 },            /* 512 byte clusters */
};

/* The old size is a multiple of the cluster size (exact), one byte below or
 * above a multiple, in the middle of a cluster, or zero.
 */

static const struct fatgap_case_s g_cases[] =
{
  { "exact1_gap",     2,  0,  2, 100, 0, 10 },
  { "exact2_gap",     4,  0,  4, 100, 0, 10 },
  { "exact2_gap1",    4,  0,  4,   1, 0, 10 },
  { "exact2_next",    4,  0,  6,   0, 0, 10 },
  { "exact2_several", 4,  0, 10, 100, 0, 10 },
  { "exact2_long",    4,  0,  4, 100, 6,  0 },
  { "below_gap1",     4, -1,  4,   0, 0, 10 },
  { "below_next",     4, -1,  4, 100, 0, 10 },
  { "above_same",     4,  1,  4, 100, 0, 10 },
  { "above_next",     4,  1,  6, 100, 0, 10 },
  { "mid_same",       3,  0,  3,  50, 0, 10 },
  { "mid_next",       3,  0,  4, 100, 0, 10 },
  { "size0_same",     0,  0,  0, 100, 0, 10 },
  { "size0_several",  0,  0,  6, 100, 0, 10 },
};

static FAR const char *g_modes[MODE_NUM] =
{
  "reopen", "samefd", "twofd"
};

static FAR const struct fatgap_vol_s *g_vol;
static FAR const char *g_case;
static FAR const char *g_mode;
static int g_case_fail;
static int g_fail;
static int g_pass;
static int g_skip;
static uint8_t g_buf[4096];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void fatgap_fail(FAR const char *fmt, ...)
{
  va_list ap;

  printf("fatgap: FAT%d %s %s FAIL: ", g_vol->fattype, g_case, g_mode);
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf("\n");
  g_case_fail++;
}

/* The pattern is never zero */

static uint8_t fatgap_pattern(off_t off, int seed)
{
  uint32_t x = (uint32_t)off * 2654435761u + (uint32_t)seed * 40503u;

  return (uint8_t)((x >> 13) | 1);
}

/* Write len pattern bytes at offset off, in chunks of at most chunk bytes */

static int fatgap_write(int fd, off_t off, size_t len, int seed,
                        size_t chunk)
{
  while (len > 0)
    {
      size_t n = len > chunk ? chunk : len;
      size_t i;
      ssize_t ret;

      for (i = 0; i < n; i++)
        {
          g_buf[i] = fatgap_pattern(off + i, seed);
        }

      ret = write(fd, g_buf, n);
      if (ret != (ssize_t)n)
        {
          fatgap_fail("write at %ld: ret %ld errno %d", (long)off, (long)ret,
                      errno);
          return -1;
        }

      off += n;
      len -= n;
    }

  return 0;
}

/* Compare a range with the pattern.  A negative seed expects zeros. */

static void fatgap_check(int fd, off_t off, size_t len, int seed,
                         FAR const char *where, FAR const char *what)
{
  if (lseek(fd, off, SEEK_SET) != off)
    {
      fatgap_fail("%s: %s: lseek to %ld: errno %d", where, what, (long)off,
                  errno);
      return;
    }

  while (len > 0)
    {
      size_t n = len > sizeof(g_buf) ? sizeof(g_buf) : len;
      size_t i;
      ssize_t ret = read(fd, g_buf, n);

      if (ret != (ssize_t)n)
        {
          fatgap_fail("%s: %s: read at %ld: ret %ld errno %d", where, what,
                      (long)off, (long)ret, errno);
          return;
        }

      for (i = 0; i < n; i++)
        {
          uint8_t want = seed < 0 ? 0 : fatgap_pattern(off + i, seed);

          if (g_buf[i] != want)
            {
              fatgap_fail("%s: %s: byte %ld is 0x%02x, want 0x%02x", where,
                          what, (long)(off + i), g_buf[i], want);
              return;
            }
        }

      off += n;
      len -= n;
    }
}

/* The expected file: old data, zeros, new data, EOF */

static void fatgap_check_fd(int fd, off_t size, off_t pos, off_t len,
                            FAR const char *where)
{
  off_t end = lseek(fd, 0, SEEK_END);

  if (end != pos + len)
    {
      fatgap_fail("%s: size %ld, want %ld", where, (long)end,
                  (long)(pos + len));
      return;
    }

  fatgap_check(fd, 0, size, 1, where, "old data");
  fatgap_check(fd, size, pos - size, -1, where, "gap");
  fatgap_check(fd, pos, len, 2, where, "new data");
}

static void fatgap_check_new_fd(off_t size, off_t pos, off_t len,
                                FAR const char *where)
{
  int fd = open(FATGAP_FILE, O_RDONLY);

  if (fd < 0)
    {
      fatgap_fail("%s: open: errno %d", where, errno);
      return;
    }

  fatgap_check_fd(fd, size, pos, len, where);
  close(fd);
}

static void fatgap_remount(void)
{
  if (umount(FATGAP_MNT) < 0)
    {
      fatgap_fail("umount: errno %d", errno);
    }

  if (mount(FATGAP_DEV, FATGAP_MNT, "vfat", 0, NULL) < 0)
    {
      fatgap_fail("mount: errno %d", errno);
    }
}

static long fatgap_free_clusters(FAR off_t *clustsize)
{
  struct statfs buf;

  if (statfs(FATGAP_MNT, &buf) < 0)
    {
      fatgap_fail("statfs: errno %d", errno);
      return -1;
    }

  if (clustsize != NULL)
    {
      *clustsize = buf.f_bsize;
    }

  return buf.f_bfree;
}

static void fatgap_run_case(FAR const struct fatgap_case_s *c, int mode,
                            off_t clustsize, long free0)
{
  off_t size = c->size_half * clustsize / 2 + c->size_byte;
  off_t pos = c->pos_half * clustsize / 2 + c->pos_byte;
  off_t len = c->len_half * clustsize / 2 + c->len_byte;
  long need = (pos + len + clustsize - 1) / clustsize;
  size_t chunk = mode == MODE_SAMEFD ? FATGAP_SMALLIO : sizeof(g_buf);
  long nfree;
  int fd;
  int fd2 = -1;

  g_case = c->name;
  g_mode = g_modes[mode];
  g_case_fail = 0;

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      goto out;
    }

  if (fatgap_write(fd, 0, size, 1, chunk) < 0)
    {
      goto out;
    }

  if (mode != MODE_SAMEFD)
    {
      close(fd);
      fd = open(FATGAP_FILE, O_RDWR);
      if (fd < 0)
        {
          fatgap_fail("open: errno %d", errno);
          goto out;
        }
    }

  if (mode == MODE_TWOFD)
    {
      fd2 = open(FATGAP_FILE, O_RDWR);
      if (fd2 < 0)
        {
          fatgap_fail("open second handle: errno %d", errno);
          goto out;
        }

      if (size > 0)
        {
          fatgap_check(fd2, size - 1, 1, 1, "second handle", "last byte");
        }
    }

  if (lseek(fd, pos, SEEK_SET) != pos)
    {
      fatgap_fail("lseek to %ld: errno %d", (long)pos, errno);
      goto out;
    }

  if (fatgap_write(fd, pos, len, 2, sizeof(g_buf)) < 0)
    {
      goto out;
    }

  if (mode == MODE_SAMEFD)
    {
      fatgap_check_fd(fd, size, pos, len, "same handle");
    }

  if (mode == MODE_TWOFD)
    {
      fsync(fd);
      fatgap_check_fd(fd2, size, pos, len, "second handle");
    }

  close(fd);
  fd = -1;

  if (fd2 >= 0)
    {
      close(fd2);
      fd2 = -1;
    }

  fatgap_check_new_fd(size, pos, len, "new handle");
  fatgap_remount();
  fatgap_check_new_fd(size, pos, len, "after remount");

  nfree = fatgap_free_clusters(NULL);
  if (free0 - nfree != need)
    {
      fatgap_fail("the file uses %ld clusters, want %ld", free0 - nfree,
                  need);
    }

out:
  if (fd >= 0)
    {
      close(fd);
    }

  if (fd2 >= 0)
    {
      close(fd2);
    }

  unlink(FATGAP_FILE);
  fatgap_remount();

  nfree = fatgap_free_clusters(NULL);
  if (nfree != free0)
    {
      fatgap_fail("%ld free clusters after unlink, want %ld", nfree, free0);
    }

  if (g_case_fail > 0)
    {
      g_fail++;
    }
  else
    {
      printf("fatgap: FAT%d %s %s PASS\n", g_vol->fattype, g_case, g_mode);
      g_pass++;
    }
}

/* Write the fill byte to the whole RAM disk */

static int fatgap_fill(void)
{
  uint32_t left = g_vol->nsectors;
  size_t nsect = sizeof(g_buf) / FATGAP_SECTSIZE;
  int fd = open(FATGAP_DEV, O_WRONLY);

  if (fd < 0)
    {
      return -1;
    }

  memset(g_buf, FATGAP_FILL, sizeof(g_buf));
  while (left > 0)
    {
      size_t n = left < nsect ? left : nsect;

      if (write(fd, g_buf, n * FATGAP_SECTSIZE) !=
          (ssize_t)(n * FATGAP_SECTSIZE))
        {
          close(fd);
          return -1;
        }

      left -= n;
    }

  close(fd);
  return 0;
}

static void fatgap_run_volume(void)
{
  struct boardioc_mkrd_s desc;
  struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
  off_t clustsize = 0;
  long free0;
  size_t c;
  int m;

  /* Volumes for the larger FAT types do not fit in a small board */

  desc.minor    = FATGAP_MINOR;
  desc.nsectors = g_vol->nsectors;
  desc.sectsize = FATGAP_SECTSIZE;
  desc.rdflags  = RDFLAG_WRENABLED | RDFLAG_FUNLINK;
  if (boardctl(BOARDIOC_MKRD, (uintptr_t)&desc) < 0)
    {
      if (errno == ENOMEM)
        {
          printf("fatgap: FAT%d SKIP: no memory for %lu sectors\n",
                 g_vol->fattype, (unsigned long)g_vol->nsectors);
          g_skip++;
        }
      else
        {
          printf("fatgap: FAT%d FAIL: MKRD: errno %d\n", g_vol->fattype,
                 errno);
          g_fail++;
        }

      return;
    }

  fmt.ff_fattype = g_vol->fattype;
  fmt.ff_clustshift = g_vol->clustshift;
  if (fatgap_fill() < 0 || mkfatfs(FATGAP_DEV, &fmt) < 0 ||
      mount(FATGAP_DEV, FATGAP_MNT, "vfat", 0, NULL) < 0)
    {
      printf("fatgap: FAT%d FAIL: prepare the volume: errno %d\n",
             g_vol->fattype, errno);
      g_fail++;
      unlink(FATGAP_DEV);
      return;
    }

  g_case = "prepare";
  g_mode = "-";
  free0 = fatgap_free_clusters(&clustsize);
  if (free0 < 0)
    {
      g_fail++;
      goto out;
    }

  printf("fatgap: FAT%d %ld byte clusters, %ld free\n", g_vol->fattype,
         (long)clustsize, free0);

  for (m = 0; m < MODE_NUM; m++)
    {
      for (c = 0; c < FATGAP_NITEMS(g_cases); c++)
        {
          fatgap_run_case(&g_cases[c], m, clustsize, free0);
        }
    }

out:
  umount(FATGAP_MNT);
  unlink(FATGAP_DEV);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  size_t v;
  int i;

  g_fail = 0;
  g_pass = 0;
  g_skip = 0;

  for (v = 0; v < FATGAP_NITEMS(g_vols); v++)
    {
      bool selected = argc <= 1;

      for (i = 1; i < argc; i++)
        {
          selected = selected || atoi(argv[i]) == g_vols[v].fattype;
        }

      if (selected)
        {
          g_vol = &g_vols[v];
          fatgap_run_volume();
        }
    }

  printf("fatgap: DONE pass %d fail %d skipped volumes %d\n", g_pass, g_fail,
         g_skip);
  return g_fail == 0 && g_pass > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
