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
 *
 * Other cases write with O_APPEND: each write must go to the end of the
 * file, also after a seek, after fcntl(F_SETFL), with pwrite() and from two
 * handles.  One case fills the volume: a write that is cut short must
 * return the number of bytes that it wrote, and the next write must fail
 * with ENOSPC.
 *
 * Other cases check that a second handle reads the zeros after an
 * ftruncate() makes the file longer, that statfs() gives the number of
 * clusters that a file can get, and that stat() and fstat() give the size
 * of an open file before a sync.  On FAT32 a case checks the free cluster
 * count that mkfatfs writes in the FSINFO sector.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/boardctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
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
#define FATGAP_FILE2     FATGAP_MNT "/B.BIN"
#define FATGAP_SECTSIZE  512
#define FATGAP_FILL      0xa5
#define FATGAP_SMALLIO   37
#define FATGAP_FULLIO    3000          /* Not a multiple of a sector */
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

struct fatgap_wcase_s
{
  FAR const char *name;
  CODE void (*run)(void);
};

enum fatgap_mode_e
{
  MODE_REOPEN,                 /* A new handle writes past EOF */
  MODE_SAMEFD,                 /* The last sector is dirty in the handle */
  MODE_TWOFD,                  /* A second handle has the last sector */
  MODE_NUM
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void fatgap_append_seek(void);
static void fatgap_append_setfl(void);
static void fatgap_append_pwrite(void);
static void fatgap_append_twofd(void);
static void fatgap_full_volume(void);
static void fatgap_trunc_twofd(void);
static void fatgap_free_count(void);
static void fatgap_stat_open(void);

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

static const struct fatgap_wcase_s g_wcases[] =
{
  { "append_seek",   fatgap_append_seek   },
  { "append_setfl",  fatgap_append_setfl  },
  { "append_pwrite", fatgap_append_pwrite },
  { "append_twofd",  fatgap_append_twofd  },
  { "full_volume",   fatgap_full_volume   },
  { "trunc_twofd",   fatgap_trunc_twofd   },
  { "free_count",    fatgap_free_count    },
  { "stat_open",     fatgap_stat_open     },
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

static void fatgap_count_case(void)
{
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

/* Remove the file, check that all its clusters are free again and count
 * the case.
 */

static void fatgap_end_case(long free0)
{
  long nfree;

  unlink(FATGAP_FILE);
  fatgap_remount();

  nfree = fatgap_free_clusters(NULL);
  if (nfree != free0)
    {
      fatgap_fail("%ld free clusters after unlink, want %ld", nfree, free0);
    }

  fatgap_count_case();
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

  fatgap_end_case(free0);
}

/* Write a string; a short write is a failure */

static int fatgap_puts(int fd, FAR const char *str)
{
  size_t len = strlen(str);
  ssize_t ret = write(fd, str, len);

  if (ret != len)
    {
      fatgap_fail("write \"%s\": ret %zd errno %d", str, ret, errno);
      return -1;
    }

  return 0;
}

/* The file must hold each byte of want, unit times */

static void fatgap_check_units(FAR const char *want, size_t unit,
                               FAR const char *where)
{
  size_t len = strlen(want) * unit;
  ssize_t ret;
  size_t i;
  int fd;

  fd = open(FATGAP_FILE, O_RDONLY);
  if (fd < 0)
    {
      fatgap_fail("%s: open: errno %d", where, errno);
      return;
    }

  ret = read(fd, g_buf, sizeof(g_buf));
  close(fd);
  if (ret != len)
    {
      fatgap_fail("%s: size %zd, want %zu", where, ret, len);
      return;
    }

  for (i = 0; i < len; i++)
    {
      if (g_buf[i] != want[i / unit])
        {
          fatgap_fail("%s: byte %zu is 0x%02x, want 0x%02x", where, i,
                      g_buf[i], want[i / unit]);
          return;
        }
    }
}

static void fatgap_check_append(FAR const char *want, size_t unit)
{
  fatgap_check_units(want, unit, "new handle");
  fatgap_remount();
  fatgap_check_units(want, unit, "after remount");
}

/* O_APPEND: a write after a seek to the start goes to the end */

static void fatgap_append_seek(void)
{
  off_t off;
  int fd;

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  if (fatgap_puts(fd, "0123456789") == 0 && lseek(fd, 0, SEEK_SET) == 0 &&
      fatgap_puts(fd, "AB") == 0)
    {
      off = lseek(fd, 0, SEEK_CUR);
      if (off != 12)
        {
          fatgap_fail("offset after the write %" PRIdOFF ", want 12", off);
        }
    }

  close(fd);
  fatgap_check_append("0123456789AB", 1);
}

/* O_APPEND set with fcntl(F_SETFL) on an open handle */

static void fatgap_append_setfl(void)
{
  int flags;
  int fd;

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  if (fatgap_puts(fd, "0123456789") == 0)
    {
      flags = fcntl(fd, F_GETFL);
      if (flags < 0 || fcntl(fd, F_SETFL, flags | O_APPEND) < 0)
        {
          fatgap_fail("fcntl: errno %d", errno);
        }
      else if (lseek(fd, 0, SEEK_SET) == 0)
        {
          fatgap_puts(fd, "Z");
        }
    }

  close(fd);
  fatgap_check_append("0123456789Z", 1);
}

/* pwrite() on a handle with O_APPEND appends, as on Linux */

static void fatgap_append_pwrite(void)
{
  ssize_t ret;
  int fd;

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  if (fatgap_puts(fd, "0123456789") == 0)
    {
      ret = pwrite(fd, "P", 1, 2);
      if (ret != 1)
        {
          fatgap_fail("pwrite: ret %zd errno %d", ret, errno);
        }
    }

  close(fd);
  fatgap_check_append("0123456789P", 1);
}

/* Two handles with O_APPEND write whole sectors in turn */

static void fatgap_append_twofd(void)
{
  ssize_t ret;
  int fd[2];
  int i;

  fd[0] = open(FATGAP_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);
  fd[1] = open(FATGAP_FILE, O_WRONLY | O_APPEND);
  if (fd[0] < 0 || fd[1] < 0)
    {
      fatgap_fail("open: errno %d", errno);
    }
  else
    {
      for (i = 0; i < 3; i++)
        {
          memset(g_buf, "abc"[i], FATGAP_SECTSIZE);
          ret = write(fd[i & 1], g_buf, FATGAP_SECTSIZE);
          if (ret != FATGAP_SECTSIZE)
            {
              fatgap_fail("write %d: ret %zd errno %d", i, ret, errno);
              break;
            }
        }
    }

  for (i = 0; i < 2; i++)
    {
      if (fd[i] >= 0)
        {
          close(fd[i]);
        }
    }

  fatgap_check_append("abc", FATGAP_SECTSIZE);
}

/* Fill the volume.  The counts that write() returns must add up to the
 * offset and to the size of the file.
 */

static void fatgap_full_volume(void)
{
  off_t maxbytes = g_vol->nsectors * FATGAP_SECTSIZE;
  struct stat st;
  off_t total = 0;
  off_t off;
  ssize_t ret = 0;
  int err = 0;
  int fd;

  fd = open(FATGAP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  memset(g_buf, FATGAP_FILL, FATGAP_FULLIO);
  while (total <= maxbytes)
    {
      ret = write(fd, g_buf, FATGAP_FULLIO);
      if (ret <= 0)
        {
          err = errno;
          break;
        }

      total += ret;
    }

  if (ret >= 0 || err != ENOSPC)
    {
      fatgap_fail("last write: ret %zd errno %d, want -1 errno %d", ret,
                  err, ENOSPC);
    }

  printf("fatgap: FAT%d %s: %" PRIdOFF " bytes written\n", g_vol->fattype,
         g_case, total);

  off = lseek(fd, 0, SEEK_CUR);
  if (off != total)
    {
      fatgap_fail("offset %" PRIdOFF ", the counts add up to %" PRIdOFF, off,
                  total);
    }

  close(fd);

  if (stat(FATGAP_FILE, &st) < 0)
    {
      fatgap_fail("stat: errno %d", errno);
    }
  else if (st.st_size != total)
    {
      fatgap_fail("size %" PRIdOFF ", the counts add up to %" PRIdOFF,
                  st.st_size, total);
    }
}

/* A second handle must read zeros after ftruncate() on the first handle
 * makes the file longer: in the last sector of the file and on a new
 * cluster.
 */

static void fatgap_trunc_twofd(void)
{
  off_t clustsize;
  int fd;
  int fd2 = -1;

  if (fatgap_free_clusters(&clustsize) < 0)
    {
      return;
    }

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  if (fatgap_write(fd, 0, 2 * clustsize, 1, sizeof(g_buf)) < 0)
    {
      goto out;
    }

  fd2 = open(FATGAP_FILE, O_RDONLY);
  if (fd2 < 0)
    {
      fatgap_fail("open second handle: errno %d", errno);
      goto out;
    }

  if (ftruncate(fd, 100) < 0 || ftruncate(fd, 400) < 0)
    {
      fatgap_fail("ftruncate to 100, then 400: errno %d", errno);
      goto out;
    }

  fatgap_check(fd2, 0, 100, 1, "second handle", "old data");
  fatgap_check(fd2, 100, 300, -1, "second handle", "zeros in the sector");

  if (ftruncate(fd, clustsize + 100) < 0)
    {
      fatgap_fail("ftruncate to %" PRIdOFF ": errno %d", clustsize + 100,
                  errno);
      goto out;
    }

  fatgap_check(fd2, 100, clustsize, -1, "second handle",
               "zeros on a new cluster");

  close(fd);
  fd = -1;

  fatgap_check_fd(fd2, 100, clustsize + 100, 0, "second handle");
  close(fd2);
  fd2 = -1;

  fatgap_check_new_fd(100, clustsize + 100, 0, "new handle");
  fatgap_remount();
  fatgap_check_new_fd(100, clustsize + 100, 0, "after remount");

out:
  if (fd >= 0)
    {
      close(fd);
    }

  if (fd2 >= 0)
    {
      close(fd2);
    }
}

static void fatgap_run_wcase(FAR const struct fatgap_wcase_s *c,
                             long free0)
{
  g_case = c->name;
  g_mode = "write";
  g_case_fail = 0;

  c->run();
  fatgap_end_case(free0);
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

static uint32_t fatgap_get32(FAR const uint8_t *p)
{
  uint32_t v;

  memcpy(&v, p, sizeof(v));
  return le32toh(v);
}

/* FAT32: the free count in the FSINFO sector of a new volume is the number
 * of data clusters less the root directory cluster.
 */

static void fatgap_check_fsinfo(void)
{
  FAR const uint8_t *fsi = &g_buf[FATGAP_SECTSIZE];
  uint32_t nclusters;
  uint32_t nfree;
  off_t sector;
  int fd;

  g_case = "fsinfo";
  g_mode = "-";
  g_case_fail = 0;

  fd = open(FATGAP_DEV, O_RDONLY);
  if (fd < 0 || pread(fd, g_buf, FATGAP_SECTSIZE, 0) != FATGAP_SECTSIZE)
    {
      fatgap_fail("read the boot sector: errno %d", errno);
      goto out;
    }

  /* FSInfo at 48, TotSec32 at 32, RsvdSecCnt at 14, NumFATs at 16,
   * FATSz32 at 36, SecPerClus at 13.
   */

  sector = g_buf[48] | (g_buf[49] << 8);
  nclusters = (fatgap_get32(&g_buf[32]) - (g_buf[14] | (g_buf[15] << 8)) -
               g_buf[16] * fatgap_get32(&g_buf[36])) / g_buf[13];

  if (pread(fd, &g_buf[FATGAP_SECTSIZE], FATGAP_SECTSIZE,
            sector * FATGAP_SECTSIZE) != FATGAP_SECTSIZE)
    {
      fatgap_fail("read the FSINFO sector %" PRIdOFF ": errno %d", sector,
                  errno);
      goto out;
    }

  if (fatgap_get32(fsi) != 0x41615252 ||
      fatgap_get32(&fsi[484]) != 0x61417272)
    {
      fatgap_fail("no FSINFO signature in sector %" PRIdOFF, sector);
      goto out;
    }

  nfree = fatgap_get32(&fsi[488]);
  if (nfree != nclusters - 1)
    {
      fatgap_fail("free count %" PRIu32 ", want %" PRIu32 " (%" PRIu32
                  " clusters, one for the root directory)", nfree,
                  nclusters - 1, nclusters);
    }

out:
  if (fd >= 0)
    {
      close(fd);
    }

  fatgap_count_case();
}

/* FAT32: set the free count in the FSINFO sector to unknown (0xffffffff),
 * so that the file system counts the free clusters in the FAT.
 */

static int fatgap_fsinfo_unknown(void)
{
  off_t sector;
  int ret = -1;
  int fd;

  if (umount(FATGAP_MNT) < 0)
    {
      fatgap_fail("umount: errno %d", errno);
      return -1;
    }

  fd = open(FATGAP_DEV, O_RDWR);
  if (fd < 0 || pread(fd, g_buf, FATGAP_SECTSIZE, 0) != FATGAP_SECTSIZE)
    {
      fatgap_fail("read the boot sector: errno %d", errno);
      goto out;
    }

  /* BPB_FSInfo: the sector number of FSINFO */

  sector = g_buf[48] | (g_buf[49] << 8);
  if (pread(fd, g_buf, FATGAP_SECTSIZE, sector * FATGAP_SECTSIZE) !=
      FATGAP_SECTSIZE)
    {
      fatgap_fail("read the FSINFO sector: errno %d", errno);
      goto out;
    }

  /* FSI_Free_Count at 488 */

  memset(&g_buf[488], 0xff, 4);
  if (pwrite(fd, g_buf, FATGAP_SECTSIZE, sector * FATGAP_SECTSIZE) !=
      FATGAP_SECTSIZE)
    {
      fatgap_fail("write the FSINFO sector: errno %d", errno);
      goto out;
    }

  ret = 0;

out:
  if (fd >= 0)
    {
      close(fd);
    }

  if (mount(FATGAP_DEV, FATGAP_MNT, "vfat", 0, NULL) < 0)
    {
      fatgap_fail("mount: errno %d", errno);
      ret = -1;
    }

  return ret;
}

/* statfs() must give the number of clusters that a file can get.  On
 * FAT32 the file system must count them in the FAT, not take the count of
 * the FSINFO sector.
 */

static void fatgap_free_count(void)
{
  off_t clustsize;
  long before;
  long nclust = 0;
  long nfree;
  ssize_t ret;
  int fd;

  if (g_vol->fattype == 32 && fatgap_fsinfo_unknown() < 0)
    {
      return;
    }

  before = fatgap_free_clusters(&clustsize);
  if (before < 0)
    {
      return;
    }

  /* Fill the volume with whole clusters */

  fd = open(FATGAP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  memset(g_buf, FATGAP_FILL, clustsize);
  while ((ret = write(fd, g_buf, clustsize)) == clustsize)
    {
      nclust++;
    }

  if (ret != -1 || errno != ENOSPC)
    {
      fatgap_fail("write of cluster %ld: ret %zd errno %d", nclust, ret,
                  errno);
    }

  if (before != nclust)
    {
      fatgap_fail("%ld free clusters, a file gets %ld", before, nclust);
    }

  nfree = fatgap_free_clusters(NULL);
  if (nfree != 0)
    {
      fatgap_fail("%ld free clusters on a full volume", nfree);
    }

  close(fd);
}

/* stat() of path and fstat() of fd must give the size want */

static void fatgap_check_size(FAR const char *where, FAR const char *path,
                              int fd, off_t want)
{
  struct stat buf;

  if (stat(path, &buf) < 0)
    {
      fatgap_fail("%s: stat: errno %d", where, errno);
    }
  else if (buf.st_size != want)
    {
      fatgap_fail("%s: stat: size %" PRIdOFF ", want %" PRIdOFF, where,
                  buf.st_size, want);
    }

  if (fd < 0)
    {
      return;
    }

  if (fstat(fd, &buf) < 0)
    {
      fatgap_fail("%s: fstat: errno %d", where, errno);
    }
  else if (buf.st_size != want)
    {
      fatgap_fail("%s: fstat: size %" PRIdOFF ", want %" PRIdOFF, where,
                  buf.st_size, want);
    }
}

/* stat() and fstat() must give the size of an open file at once after a
 * write, before a sync, and after a rename of the open file.
 */

static void fatgap_stat_open(void)
{
  struct stat buf;
  int fd;
  int fd2 = -1;

  fd = open(FATGAP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      fatgap_fail("create: errno %d", errno);
      return;
    }

  if (fatgap_write(fd, 0, 1000, 1, sizeof(g_buf)) < 0)
    {
      goto out;
    }

  fatgap_check_size("new file", FATGAP_FILE, fd, 1000);

  if (fatgap_write(fd, 1000, 500, 1, sizeof(g_buf)) < 0)
    {
      goto out;
    }

  fatgap_check_size("longer file", FATGAP_FILE, fd, 1500);

  fd2 = open(FATGAP_FILE, O_RDONLY);
  if (fd2 < 0)
    {
      fatgap_fail("open second handle: errno %d", errno);
      goto out;
    }

  fatgap_check_size("second handle", FATGAP_FILE, fd2, 1500);

  if (fsync(fd) < 0)
    {
      fatgap_fail("fsync: errno %d", errno);
    }

  fatgap_check_size("after fsync", FATGAP_FILE, fd, 1500);

  if (rename(FATGAP_FILE, FATGAP_FILE2) < 0)
    {
      fatgap_fail("rename: errno %d", errno);
      goto out;
    }

  if (fatgap_write(fd, 1500, 100, 1, sizeof(g_buf)) < 0)
    {
      goto out;
    }

  fatgap_check_size("after rename", FATGAP_FILE2, fd, 1600);
  if (stat(FATGAP_FILE, &buf) == 0 || errno != ENOENT)
    {
      fatgap_fail("after rename: stat of the old name: errno %d", errno);
    }

  close(fd);
  fd = -1;
  close(fd2);
  fd2 = -1;

  fatgap_check_size("after close", FATGAP_FILE2, -1, 1600);
  fatgap_remount();
  fatgap_check_size("after remount", FATGAP_FILE2, -1, 1600);

out:
  if (fd >= 0)
    {
      close(fd);
    }

  if (fd2 >= 0)
    {
      close(fd2);
    }

  unlink(FATGAP_FILE2);
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

  if (g_vol->fattype == 32)
    {
      fatgap_check_fsinfo();
    }

  for (m = 0; m < MODE_NUM; m++)
    {
      for (c = 0; c < FATGAP_NITEMS(g_cases); c++)
        {
          fatgap_run_case(&g_cases[c], m, clustsize, free0);
        }
    }

  for (c = 0; c < FATGAP_NITEMS(g_wcases); c++)
    {
      fatgap_run_wcase(&g_wcases[c], free0);
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
