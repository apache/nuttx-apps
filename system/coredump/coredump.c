/****************************************************************************
 * apps/system/coredump/coredump.c
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <syslog.h>
#include <dirent.h>
#include <elf.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>

#include <nuttx/binfmt/binfmt.h>
#include <nuttx/streams.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE void (*dumpfile_cb_t)(FAR char *path, FAR const char *filename,
                                   FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * dumpfile_iterate
 ****************************************************************************/

#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
static int dumpfile_iterate(FAR char *path, dumpfile_cb_t cb, FAR void *arg)
{
  FAR struct dirent *entry;
  FAR DIR *dir;
  int ret;

  dir = opendir(path);
  if (dir == NULL)
    {
      ret = mkdir(path, 0777);
      if (ret < 0)
        {
          return ret;
        }
    }

  while ((entry = readdir(dir)) != NULL)
    {
      if (entry->d_type == DT_REG && !strncmp(entry->d_name, "core-", 5))
        {
          cb(path, entry->d_name, arg);
        }
    }

  closedir(dir);
  return 0;
}

/****************************************************************************
 * dumpfile_count
 ****************************************************************************/

static void dumpfile_count(FAR char *path, FAR const char *filename,
                           FAR void *arg)
{
  FAR size_t *max = (FAR size_t *)arg;

  *max += 1;
}

/****************************************************************************
 * dumpfile_delete
 ****************************************************************************/

static void dumpfile_delete(FAR char *path, FAR const char *filename,
                            FAR void *arg)
{
  FAR char *dumppath = arg;

  sprintf(dumppath, "%s/%s", path, filename);
  printf("Remove %s\n", dumppath);
  remove(dumppath);
}

/****************************************************************************
 * coredump_restore
 ****************************************************************************/

static void coredump_restore(FAR char *savepath, size_t maxfile)
{
  FAR struct coredump_info_s *info;
  unsigned char *swap;
  char dumppath[PATH_MAX];
  struct geometry geo;
  ssize_t writesize;
  ssize_t readsize;
  struct tm *dtime;
  size_t offset = 0;
  size_t max = 0;
  int dumpfd;
  int blkfd;
  int ret;

  blkfd = open(CONFIG_BOARD_COREDUMP_BLKDEV_PATH, O_RDWR);
  if (blkfd < 0)
    {
      return;
    }

  ret = ioctl(blkfd, BIOC_GEOMETRY, (unsigned long)((uintptr_t)&geo));
  if (ret < 0)
    {
      goto blkfd_err;
    }

  info = malloc(geo.geo_sectorsize);
  if (info == NULL)
    {
      goto blkfd_err;
    }

  lseek(blkfd, (geo.geo_nsectors - 1) * geo.geo_sectorsize, SEEK_SET);
  read(blkfd, info, geo.geo_sectorsize);
  if (info->magic != COREDUMP_MAGIC)
    {
      printf("%s coredump not found!\n", CONFIG_BOARD_COREDUMP_BLKDEV_PATH);
      goto info_err;
    }

  ret = dumpfile_iterate(savepath, dumpfile_count, &max);
  if (ret < 0)
    {
      goto info_err;
    }

  if (max >= maxfile)
    {
      ret = dumpfile_iterate(savepath, dumpfile_delete, dumppath);
      if (ret < 0)
        {
          goto info_err;
        }
    }

  ret = snprintf(dumppath, sizeof(dumppath),
                 "%s/core-%s", savepath,
                 info->name.version);
  dtime = localtime(&info->time);
  if (dtime)
    {
      ret += snprintf(dumppath + ret, sizeof(dumppath) - ret,
                      "-%d-%d-%d-%d-%d", dtime->tm_mon + 1,
                      dtime->tm_mday, dtime->tm_hour,
                      dtime->tm_min, dtime->tm_sec);
    }

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  ret += snprintf(dumppath + ret, sizeof(dumppath) - ret, ".lzf");
#else
  ret += snprintf(dumppath + ret, sizeof(dumppath) - ret, ".core");
#endif

  while (ret--)
    {
      if (dumppath[ret] == ' ' || dumppath[ret] == ':')
        {
          dumppath[ret] = '-';
        }
    }

  dumpfd = open(dumppath, O_CREAT | O_WRONLY | O_TRUNC, 0777);
  if (dumpfd < 0)
    {
      printf("Open %s fail\n", dumppath);
      goto info_err;
    }

  swap = malloc(geo.geo_sectorsize);
  if (swap == NULL)
    {
      printf("Malloc fail\n");
      goto fd_err;
    }

  lseek(blkfd, 0, SEEK_SET);
  while (offset < info->size)
    {
      readsize = read(blkfd, swap, geo.geo_sectorsize);
      if (readsize < 0)
        {
          printf("Read %s fail\n", CONFIG_BOARD_COREDUMP_BLKDEV_PATH);
          break;
        }

      writesize = write(dumpfd, swap, readsize);
      if (writesize != readsize)
        {
          printf("Write %s fail\n", dumppath);
          break;
        }

      offset += writesize;
    }

  printf("Coredump finish [%s][%zu]\n", dumppath, info->size);
  info->magic = 0;
  lseek(blkfd, (geo.geo_nsectors - 1) * geo.geo_sectorsize, SEEK_SET);
  write(blkfd, info, geo.geo_sectorsize);
  free(swap);
fd_err:
  close(dumpfd);
info_err:
  free(info);
blkfd_err:
  close(blkfd);
}

#endif

/****************************************************************************
 * coredump_now
 ****************************************************************************/

static int coredump_now(int pid, FAR char *filename)
{
  FAR struct lib_stdoutstream_s *outstream;
  FAR struct lib_hexdumpstream_s *hstream;
#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  FAR struct lib_lzfoutstream_s *lstream;
#endif
  FAR void *stream;
  FAR FILE *file;
  int logmask;

  if (filename != NULL)
    {
      file = fopen(filename, "w");
      if (file == NULL)
        {
          return -errno;
        }
    }
  else
    {
      file = stdout;
    }

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  hstream = malloc(sizeof(*hstream) + sizeof(*lstream) +
                   sizeof(*outstream));
#else
  hstream = malloc(sizeof(*hstream) + sizeof(*outstream));
#endif

  if (hstream == NULL)
    {
      if (filename != NULL)
        {
          fclose(file);
        }

      return -ENOMEM;
    }

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  lstream = (FAR void *)(hstream + 1);
  outstream = (FAR void *)(lstream + 1);
#else
  outstream = (FAR void *)(hstream + 1);
#endif

  printf("Start coredump:\n");
  logmask = setlogmask(LOG_ALERT);

  /* Initialize hex output stream */

  lib_stdoutstream(outstream, file);
  lib_hexdumpstream(hstream, (FAR void *)outstream);
  stream = hstream;

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION

  /* Initialize LZF compression stream */

  lib_lzfoutstream(lstream, stream);
  stream = lstream;

#endif

  /* Do core dump */

  core_dump(NULL, stream, pid);
  setlogmask(logmask);
#  ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  printf("Finish coredump (Compression Enabled).\n");
#  else
  printf("Finish coredump.\n");
#  endif

  free(hstream);
  if (filename != NULL)
    {
      fclose(file);
    }

  return 0;
}

/****************************************************************************
 * usage
 ****************************************************************************/

static void usage(FAR const char *progname, int exitcode)
{
  fprintf(stderr, "%s [option]:\n", progname);
  fprintf(stderr, "Default usage, will coredump directly\n");
  fprintf(stderr, "\t -p, --pid <pid>, Default, all thread\n");
  fprintf(stderr, "\t -f, --filename <filename>, Default stdout\n");

#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
  fprintf(stderr, "Second usage, will restore coredump"
                  "from %s to savepath\n",
                   CONFIG_BOARD_COREDUMP_BLKDEV_PATH);
  fprintf(stderr, "\t -s, --savepath <savepath>\n");
  fprintf(stderr, "\t -m, --maxfile <maxfile>,"
                  "Maximum number of coredump files, Default 1\n");
#endif
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * coredump_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
  FAR char *savepath = NULL;
  size_t maxfile = 1;
#endif
  char *name = NULL;
  int pid = INVALID_PROCESS_ID;
  int ret;

  struct option options[] =
    {
      {"pid", 1, NULL, 'p'},
      {"filename", 1, NULL, 'f'},
#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
      {"savepath", 1, NULL, 's'},
      {"maxfile", 1, NULL, 'm'},
#endif
      {"help", 0, NULL, 'h'}
    };

  while ((ret = getopt_long(argc, argv, "p:f:s:m:h", options, NULL))
         != ERROR)
    {
      switch (ret)
        {
          case 'p':
            pid = atoi(optarg);
            break;
          case 'f':
            name = optarg;
            break;
#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
          case 's':
            savepath = optarg;
            break;
          case 'm':
            maxfile = atoi(optarg);
            break;
#endif
          case 'h':
          default:
            usage(argv[0], EXIT_SUCCESS);
            break;
        }
    }

#ifdef CONFIG_BOARD_COREDUMP_BLKDEV
  if (savepath != NULL)
    {
      coredump_restore(savepath, maxfile);
    }
  else
#endif
    {
      coredump_now(pid, name);
    }

  return 0;
}
