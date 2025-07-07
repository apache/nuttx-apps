/****************************************************************************
 * apps/system/coredump/coredump.c
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
#include <nuttx/sched.h>
#include <nuttx/coredump.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
#  define COREDUMP_FILE_SUFFIX ".lzf"
#else
#  define COREDUMP_FILE_SUFFIX ".core"
#endif

#define COREDUMP_FILE_SUFFIX_LEN (sizeof(COREDUMP_FILE_SUFFIX) - 1)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef CODE void (*dumpfile_cb_t)(FAR char *path, FAR const char *filename,
                                   FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_BOARD_MEMORY_RANGE
static struct memory_region_s g_memory_region[] =
  {
    CONFIG_BOARD_MEMORY_RANGE
  };
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * dumpfile_iterate
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE

static bool dumpfile_is_valid(FAR const char *name)
{
  FAR const char *suffix;
  size_t name_len;

  name_len = strlen(name);
  if (name_len < COREDUMP_FILE_SUFFIX_LEN)
    {
      return false;
    }

  suffix = name + name_len - COREDUMP_FILE_SUFFIX_LEN;
  return !!memcmp(suffix, COREDUMP_FILE_SUFFIX, COREDUMP_FILE_SUFFIX_LEN);
}

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
          printf("Coredump mkdir %s fail\n", path);
          return -errno;
        }
    }

  while ((entry = readdir(dir)) != NULL)
    {
      if (entry->d_type == DT_REG && dumpfile_is_valid(entry->d_name))
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
  int ret;

  sprintf(dumppath, "%s/%s", path, filename);
  printf("Remove %s\n", dumppath);
  ret = remove(dumppath);
  if (ret < 0)
    {
      printf("Remove %s fail\n", dumppath);
    }
}

/****************************************************************************
 * dumpfile_get_info
 ****************************************************************************/

static int dumpfile_get_info(int fd, FAR struct coredump_info_s *info)
{
  Elf_Ehdr ehdr;
  Elf_Phdr phdr;
  Elf_Nhdr nhdr;
  char name[COREDUMP_INFONAME_SIZE];

  if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr) ||
      memcmp(ehdr.e_ident, ELFMAG, EI_MAGIC_SIZE) != 0)
    {
      return -EINVAL;
    }

  /* The last program header is for NuttX core info note. */

  if (lseek(fd, ehdr.e_phoff + ehdr.e_phentsize * (ehdr.e_phnum - 1),
            SEEK_SET) < 0)
    {
      return -errno;
    }

  if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr) ||
      lseek(fd, phdr.p_offset, SEEK_SET) < 0)
    {
      return -errno;
    }

  /* The note header must match exactly. */

  if (read(fd, &nhdr, sizeof(nhdr)) != sizeof(nhdr) ||
      nhdr.n_type != COREDUMP_MAGIC ||
      nhdr.n_namesz != COREDUMP_INFONAME_SIZE ||
      nhdr.n_descsz != sizeof(struct coredump_info_s))
    {
      return -EINVAL;
    }

  if (read(fd, name, nhdr.n_namesz) != nhdr.n_namesz ||
      strcmp(name, "NuttX") != 0)
    {
      return -EINVAL;
    }

  if (read(fd, info, sizeof(*info)) != sizeof(*info))
    {
      return -errno;
    }

  return 0;
}

/****************************************************************************
 * coredump_restore
 ****************************************************************************/

static void coredump_restore(FAR char *savepath, size_t maxfile)
{
  struct coredump_info_s info;
  char dumppath[PATH_MAX];
  unsigned char *swap;
  ssize_t writesize;
  ssize_t readsize;
  size_t offset = 0;
  size_t max = 0;
  int dumpfd;
  int blkfd;
  off_t off;
  int ret;
  Elf_Nhdr nhdr =
    {
      0
    };

  blkfd = open(CONFIG_SYSTEM_COREDUMP_DEVPATH, O_RDWR);
  if (blkfd < 0)
    {
      return;
    }

  ret = dumpfile_get_info(blkfd, &info);
  if (ret < 0)
    {
      printf("No core data in %s\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
      goto blkfd_err;
    }

  ret = dumpfile_iterate(savepath, dumpfile_count, &max);
  if (ret < 0)
    {
      goto blkfd_err;
    }

  if (max >= maxfile)
    {
      ret = dumpfile_iterate(savepath, dumpfile_delete, dumppath);
      if (ret < 0)
        {
          goto blkfd_err;
        }
    }

  /* 'date -d @$(printf "%d" 0x6720C67E)' restore utc to date */

  ret = snprintf(dumppath, sizeof(dumppath),
                 "%s/%.16s-%llx"COREDUMP_FILE_SUFFIX,
                 savepath, info.name.version,
                 (unsigned long long)info.time.tv_sec);

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
      goto blkfd_err;
    }

  swap = malloc(CONFIG_SYSTEM_COREDUMP_SWAPBUFFER_SIZE);
  if (swap == NULL)
    {
      printf("Malloc fail\n");
      goto fd_err;
    }

  lseek(blkfd, 0, SEEK_SET);
  while (offset < info.size)
    {
      readsize = read(blkfd, swap, CONFIG_SYSTEM_COREDUMP_SWAPBUFFER_SIZE);
      if (readsize < 0)
        {
          printf("Read %s fail\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
          break;
        }
      else if (readsize == 0)
        {
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

  if (offset < info.size)
    {
      printf("Coredump error [%s] need [%zu], but just get %zu\n",
             dumppath, info.size, offset);
    }
  else
    {
      printf("Coredump finish [%s][%zu]\n", dumppath, info.size);
    }

  off  = info.size - sizeof(info);
  off -= COREDUMP_INFONAME_SIZE;
  off -= sizeof(Elf_Nhdr);
  off  = lseek(blkfd, off, SEEK_SET);
  if (off < 0)
    {
      printf("Seek %s fail\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
      goto swap_err;
    }

  writesize = write(blkfd, &nhdr, sizeof(nhdr));
  if (writesize != sizeof(nhdr))
    {
      printf("Write %s fail\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
    }

  /* Erase the core file header too */

  off = lseek(blkfd, 0, SEEK_SET);
  if (off < 0)
    {
      printf("Seek %s fail\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
      goto swap_err;
    }

  writesize = write(blkfd, "\0\0\0\0", EI_MAGIC_SIZE);
  if (writesize != EI_MAGIC_SIZE)
    {
      printf("Write %s fail\n", CONFIG_SYSTEM_COREDUMP_DEVPATH);
    }

swap_err:
  free(swap);
fd_err:
  close(dumpfd);
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
  logmask = setlogmask(LOG_UPTO(LOG_ALERT));

  /* Initialize hex output stream */

  lib_stdoutstream(outstream, file);
  if (file == stdout)
    {
      lib_hexdumpstream(hstream, (FAR void *)outstream);
      stream = hstream;
    }
  else
    {
      stream = outstream;
    }

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION

  /* Initialize LZF compression stream */

  lib_lzfoutstream(lstream, stream);
  stream = lstream;

#endif

  /* Do core dump */

#ifdef CONFIG_BOARD_MEMORY_RANGE
  coredump(g_memory_region, stream, pid);
#else
  coredump(NULL, stream, pid);
#endif

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

#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE
  fprintf(stderr, "Second usage, will restore coredump"
                  "from %s to savepath\n",
                   CONFIG_SYSTEM_COREDUMP_DEVPATH);
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
#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE
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
#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE
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
#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE
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

#ifdef CONFIG_SYSTEM_COREDUMP_RESTORE
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
