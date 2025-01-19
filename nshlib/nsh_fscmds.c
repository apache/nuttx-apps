/****************************************************************************
 * apps/nshlib/nsh_fscmds.c
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
#include <sys/ioctl.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <libgen.h>
#include <errno.h>
#include <debug.h>

#include "nsh.h"

#if !defined(CONFIG_DISABLE_MOUNTPOINT)
#  include <sys/mount.h>
#  include <sys/boardctl.h>
#  include <nuttx/drivers/ramdisk.h>
#  ifdef CONFIG_DEV_LOOP
#    include <nuttx/fs/loop.h>
#  endif
#  ifdef CONFIG_FS_SMARTFS
#    include "fsutils/mksmartfs.h"
#  endif
#  ifdef CONFIG_SMART_DEV_LOOP
#    include <nuttx/fs/smart.h>
#  endif
#  ifdef CONFIG_MTD_LOOP
#    include <nuttx/fs/loopmtd.h>
#  endif
#  ifdef CONFIG_NFS
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <nuttx/fs/nfs.h>
#  endif
#  ifdef CONFIG_RAMLOG_SYSLOG
#    include <nuttx/syslog/ramlog.h>
#  endif
#endif

#ifdef CONFIG_FSUTILS_MKFATFS
#  include "fsutils/mkfatfs.h"
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LSFLAGS_SIZE          1
#define LSFLAGS_LONG          2
#define LSFLAGS_RECURSIVE     4
#define LSFLAGS_UID_GID       8
#define LSFLAGS_HUMANREADBLE  16

#define KB                   (1UL << 10)
#define MB                   (1UL << 20)
#define GB                   (1UL << 30)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cp_handler
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_CP
static int cp_handler(FAR struct nsh_vtbl_s *vtbl, FAR const char *srcpath,
                      FAR const char *destpath)
{
  struct stat buf;
  FAR char *allocpath = NULL;
  int oflags = O_WRONLY | O_CREAT | O_TRUNC;
  int rdfd;
  int wrfd;
  int ret = ERROR;

  rdfd = open(srcpath, O_RDONLY);
  if (rdfd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "cp", "open_rdfd", NSH_ERRNO);
      return ret;
    }

  /* Check if the destination is a directory */

  if (stat(destpath, &buf) == 0)
    {
      /* Something exists here... is it a directory? */

      if (S_ISDIR(buf.st_mode))
        {
          /* Yes, it is a directory.
           * Remove any trailing '/' characters from the path
           */

          nsh_trimdir((FAR char *)destpath);

          /* Construct the full path to the new file */

          allocpath = nsh_getdirpath(vtbl, destpath,
                      basename((FAR char *)srcpath));
          if (!allocpath)
            {
              nsh_error(vtbl, g_fmtcmdoutofmemory, "cp");
              goto errout_with_rdfd;
            }

          /* Open then dest for writing */

          destpath = allocpath;
        }
      else if (!S_ISREG(buf.st_mode))
        {
          /* Maybe it is a driver? */

          oflags = O_WRONLY;
        }
    }

  wrfd = open(destpath, oflags, 0666);
  if (wrfd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "cp", "open_wrfd", NSH_ERRNO);
      goto errout_with_allocpath;
    }

  for (; ; )
    {
      ssize_t nbyteswritten;
      ssize_t nbytesread;
      FAR char *iobuffer = vtbl->iobuffer;

      nbytesread = read(rdfd, iobuffer, IOBUFFERSIZE);
      if (nbytesread == 0)
        {
          /* End of file */

          ret = OK;
          goto errout_with_wrfd;
        }
      else if (nbytesread < 0)
        {
          /* EINTR is not an error (but will still stop the copy) */

          if (errno == EINTR)
            {
              nsh_error(vtbl, g_fmtsignalrecvd, "cp");
            }
          else
            {
              /* Read error */

              nsh_error(vtbl, g_fmtcmdfailed, "cp", "read",
                        NSH_ERRNO);
            }

          goto errout_with_wrfd;
        }

      do
        {
          nbyteswritten = write(wrfd, iobuffer, nbytesread);
          if (nbyteswritten >= 0)
            {
              nbytesread -= nbyteswritten;
              iobuffer += nbyteswritten;
            }
          else
            {
              /* EINTR is not an error (but will still stop the copy) */

              if (errno == EINTR)
                {
                  nsh_error(vtbl, g_fmtsignalrecvd, "cp");
                }
              else
                {
                  /* Read error */

                  nsh_error(vtbl, g_fmtcmdfailed, "cp", "write",
                            NSH_ERRNO);
                }

              goto errout_with_wrfd;
            }
        }
      while (nbytesread > 0);
    }

errout_with_wrfd:
  close(wrfd);

errout_with_allocpath:
  free(allocpath);

errout_with_rdfd:
  close(rdfd);

  return ret;
}
#endif

/****************************************************************************
 * Name: cp_recursive
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_CP
static int cp_recursive(FAR struct nsh_vtbl_s *vtbl, FAR const char *srcpath,
                        FAR const char *destpath)
{
  FAR struct dirent *entry;
  FAR char *allocdestpath;
  FAR char *allocsrcpath;
  struct stat buf;
  int ret = OK;
  DIR *dp;

  dp = opendir(srcpath);
  if (dp == NULL)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "cp", "opendir", NSH_ERRNO);
      return ERROR;
    }

  while ((entry = readdir(dp)) != NULL && ret == OK)
    {
      if (strcmp(entry->d_name, ".") == 0 ||
          strcmp(entry->d_name, "..") == 0)
        {
          continue;
        }

      allocsrcpath = nsh_getdirpath(vtbl, srcpath, entry->d_name);
      if (allocsrcpath == NULL)
        {
          ret = ERROR;
          continue;
        }

      ret = stat(allocsrcpath, &buf);
      if (ret != OK)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "cp", "stat", NSH_ERRNO);
          goto errout_with_allocsrcpath;
        }

      allocdestpath = nsh_getdirpath(vtbl, destpath, entry->d_name);
      if (allocdestpath == NULL)
        {
          ret = ERROR;
          goto errout_with_allocsrcpath;
        }

      if (S_ISDIR(buf.st_mode))
        {
#if !defined(CONFIG_DISABLE_MOUNTPOINT) || !defined(CONFIG_DISABLE_PSEUDOFS_OPERATIONS)
          ret = mkdir(allocdestpath, S_IRWXU | S_IRWXG | S_IROTH |
                      S_IXOTH);
          if (ret != OK)
            {
              nsh_error(vtbl, g_fmtcmdfailed, "cp", "mkdir", NSH_ERRNO);
              goto errout_with_allocdestpath;
            }
#endif

          ret = cp_recursive(vtbl, allocsrcpath, allocdestpath);
          if (ret != OK)
            {
              goto errout_with_allocdestpath;
            }
        }
      else
        {
          ret = cp_handler(vtbl, allocsrcpath, allocdestpath);
          if (ret != OK)
            {
              goto errout_with_allocdestpath;
            }
        }

errout_with_allocdestpath:
      free(allocdestpath);

errout_with_allocsrcpath:
      free(allocsrcpath);
    }

  closedir(dp);
  return ret;
}
#endif

/****************************************************************************
 * Name: ls_specialdir
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_LS)
static inline int ls_specialdir(FAR const char *dir)
{
  /* '.' and '..' directories are not listed like normal directories */

  return (strcmp(dir, ".")  == 0 || strcmp(dir, "..") == 0);
}
#endif

/****************************************************************************
 * Name: ls_handler
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_LS)
static int ls_handler(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                      FAR struct dirent *entryp, FAR void *pvarg)
{
  unsigned int lsflags = (unsigned int)((uintptr_t)pvarg);
#ifdef CONFIG_PSEUDOFS_SOFTLINKS
  bool isdir = false;
#endif
  int ret;

  /* Check if any options will require that we stat the file */

  if ((lsflags & (LSFLAGS_SIZE | LSFLAGS_LONG | LSFLAGS_UID_GID)) != 0)
    {
      struct stat buf;

      memset(&buf, 0, sizeof(struct stat));

      /* stat the file */

      if (entryp != NULL)
        {
          FAR char *fullpath = nsh_getdirpath(vtbl, dirpath, entryp->d_name);
          ret = stat(fullpath, &buf);
          free(fullpath);
        }
      else
        {
          /* NULL entry signifies that we are running ls on a single file */

          ret = stat(dirpath, &buf);
        }

      if (ret != 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, "ls", "stat", NSH_ERRNO);
          return ERROR;
        }

      if ((lsflags & LSFLAGS_LONG) != 0)
        {
          char details[] = "----------";

#ifdef CONFIG_PSEUDOFS_SOFTLINKS
          if (S_ISLNK(buf.st_mode))
            {
              details[0] = 'l';  /* Takes precedence over type of the target */
              isdir = S_ISDIR(buf.st_mode);
            }
          else
#endif
          if (S_ISBLK(buf.st_mode))
            {
              details[0] = 'b';
            }
          else if (S_ISCHR(buf.st_mode))
            {
              details[0] = 'c';
            }
          else if (S_ISDIR(buf.st_mode))
            {
              details[0] = 'd';
            }
#ifdef CONFIG_MTD
          else if (S_ISMTD(buf.st_mode))
            {
              details[0] = 'f';
            }
#endif
#ifdef CONFIG_FS_SHMFS
          else if (S_ISSHM(buf.st_mode))
            {
              details[0] = 'h';
            }
#endif
#ifndef CONFIG_DISABLE_MQUEUE
          else if (S_ISMQ(buf.st_mode))
            {
              details[0] = 'm';
            }
#endif
#ifdef CONFIG_NET
          else if (S_ISSOCK(buf.st_mode))
            {
              details[0] = 'n';
            }
#endif
#ifdef CONFIG_FS_NAMED_SEMAPHORES
          else if (S_ISSEM(buf.st_mode))
            {
              details[0] = 's';
            }
#endif
          else if (!S_ISREG(buf.st_mode))
            {
              details[0] = '?';
            }

          if ((buf.st_mode & S_IRUSR) != 0)
            {
              details[1] = 'r';
            }

          if ((buf.st_mode & S_IWUSR) != 0)
            {
              details[2] = 'w';
            }

          if ((buf.st_mode & S_IXUSR) != 0 && (buf.st_mode & S_ISUID) != 0)
            {
              details[3] = 's';
            }
          else if ((buf.st_mode & S_ISUID) != 0)
            {
              details[3] = 'S';
            }
          else if ((buf.st_mode & S_IXUSR) != 0)
            {
              details[3] = 'x';
            }

          if ((buf.st_mode & S_IRGRP) != 0)
            {
              details[4] = 'r';
            }

          if ((buf.st_mode & S_IWGRP) != 0)
            {
              details[5] = 'w';
            }

          if ((buf.st_mode & S_IXGRP) != 0 && (buf.st_mode & S_ISGID) != 0)
            {
              details[6] = 's';
            }
          else if ((buf.st_mode & S_ISGID) != 0)
            {
              details[6] = 'S';
            }
          else if ((buf.st_mode & S_IXGRP) != 0)
            {
              details[6] = 'x';
            }

          if ((buf.st_mode & S_IROTH) != 0)
            {
              details[7] = 'r';
            }

          if ((buf.st_mode & S_IWOTH) != 0)
            {
              details[8] = 'w';
            }

          if ((buf.st_mode & S_IXOTH) != 0)
            {
              details[9] = 'x';
            }

          nsh_output(vtbl, " %s", details);
        }

#ifdef CONFIG_SCHED_USER_IDENTITY
      if ((lsflags & LSFLAGS_UID_GID) != 0)
        {
          nsh_output(vtbl, "%8d", buf.st_uid);
          nsh_output(vtbl, "%8d", buf.st_gid);
        }
#endif

      if ((lsflags & LSFLAGS_SIZE) != 0)
        {
#ifdef CONFIG_HAVE_FLOAT
          if (lsflags & LSFLAGS_HUMANREADBLE && buf.st_size >= KB)
            {
              if (buf.st_size >= GB)
                {
                  nsh_output(vtbl, "%11.1fG", (float)buf.st_size / GB);
                }
              else if (buf.st_size >= MB)
                {
                  nsh_output(vtbl, "%11.1fM", (float)buf.st_size / MB);
                }
              else
                {
                  nsh_output(vtbl, "%11.1fK", (float)buf.st_size / KB);
                }
            }
          else
#endif
            {
              nsh_output(vtbl, "%12" PRIdOFF, buf.st_size);
            }
        }
    }

  /* Then provide the filename that is common to normal and verbose output */

  if (entryp != NULL)
    {
      nsh_output(vtbl, " %s", entryp->d_name);

#ifdef CONFIG_PSEUDOFS_SOFTLINKS
      if (DIRENT_ISLINK(entryp->d_type))
        {
          FAR char *fullpath;
          ssize_t len;

          /* Get the target of the symbolic link */

          fullpath = nsh_getdirpath(vtbl, dirpath, entryp->d_name);
          len = readlink(fullpath, vtbl->iobuffer, IOBUFFERSIZE);
          free(fullpath);

          if (len < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, "ls", "readlink", NSH_ERRNO);
              return ERROR;
            }

          if (isdir)
            {
              nsh_output(vtbl, "/ ->%s\n", vtbl->iobuffer);
            }
          else
            {
              nsh_output(vtbl, " ->%s\n", vtbl->iobuffer);
            }
        }
      else
#endif
      if (DIRENT_ISDIRECTORY(entryp->d_type) &&
          !ls_specialdir(entryp->d_name))
        {
          nsh_output(vtbl, "/\n");
        }
      else
        {
          nsh_output(vtbl, "\n");
        }
    }
  else
    {
      /* A single file */

      nsh_output(vtbl, " %s\n", dirpath);
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: ls_recursive
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_LS)
static int ls_recursive(FAR struct nsh_vtbl_s *vtbl, FAR const char *dirpath,
                        FAR struct dirent *entryp, FAR void *pvarg)
{
  int ret = OK;

  /* Is this entry a directory (not a special directories, e.g. . and ..)? */

  if (DIRENT_ISDIRECTORY(entryp->d_type) && !ls_specialdir(entryp->d_name))
    {
      /* Yes.. */

      FAR char *newpath;
      newpath = nsh_getdirpath(vtbl, dirpath, entryp->d_name);

      /* List the directory contents */

      nsh_output(vtbl, "%s:\n", newpath);

      /* Traverse the directory  */

      ret = nsh_foreach_direntry(vtbl, "ls", newpath, ls_handler, pvarg);
      if (ret == 0)
        {
          /* Then recurse to list each directory within the directory */

          ret = nsh_foreach_direntry(vtbl, "ls", newpath, ls_recursive,
                                     pvarg);
        }

      free(newpath);
    }

  return ret;
}

#endif /* !CONFIG_NSH_DISABLE_LS */

/****************************************************************************
 * Name: fdinfo_callback
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_FDINFO)
static int fdinfo_callback(FAR struct nsh_vtbl_s *vtbl,
                           FAR const char *dirpath,
                           FAR struct dirent *entryp, FAR void *pvarg)
{
  FAR char *filepath;
  int ret;
  int i;

  UNUSED(pvarg);

  if (!DIRENT_ISDIRECTORY(entryp->d_type))
    {
      /* Not a directory, let's skip it */

      return OK;
    }

  /* Check name */

  for (i = 0; entryp->d_name[i] != '\0'; i++)
    {
      if (!isdigit(entryp->d_name[i]))
        {
          /* Name contains something other than a numeric character */

          return OK;
        }
    }

  /* Let's initialize all the information */

  ret = asprintf(&filepath, "%s/%s/group/fd", dirpath, entryp->d_name);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "fdinfo", "asprintf", NSH_ERRNO);
    }

  nsh_output(vtbl, "\npid:%s", entryp->d_name);
  ret = nsh_catfile(vtbl, "fdinfo", filepath);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "fdinfo", "nsh_catfaile", NSH_ERRNO);
    }

  free(filepath);
  return ret;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_basename
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_BASENAME
int cmd_basename(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *filename;

  /* Usage: basename <path> [<suffix>]
   *
   * NOTE:  basename() may modify path.
   */

  filename = basename(argv[1]);
  if (argc > 2)
    {
      FAR char *suffix = argv[2];
      int nndx;
      int sndx;

      /* Check for any trailing sub-string */

      nndx  = strlen(filename);
      sndx  = strlen(suffix);
      nndx -= sndx;

      if (nndx > 0 && strcmp(&filename[nndx], suffix) == 0)
        {
          filename[nndx] = '\0';
        }
    }

  /* And output the resulting basename */

  nsh_output(vtbl, "%s\n", filename);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_dirname
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_DIRNAME
int cmd_dirname(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *filename;

  /* Usage: dirname <path>
   *
   * NOTE:  basename() may modify path.
   */

  filename = dirname(argv[1]);
  nsh_output(vtbl, "%s\n", filename);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_cat
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_CAT
int cmd_cat(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *fullpath;
  int i;
  int ret = OK;

  /* Loop for each file name on the command line */

  for (i = 1; i < argc && ret == OK; i++)
    {
      /* Get the fullpath to the file */

      fullpath = nsh_getfullpath(vtbl, argv[i]);
      if (fullpath == NULL)
        {
          nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
          ret = ERROR;
        }
      else
        {
          /* Dump the file to the console */

          ret = nsh_catfile(vtbl, argv[0], fullpath);

          /* Free the allocated full path */

          nsh_freefullpath(fullpath);
        }
    }

  if (argc == 1)
    {
      char *buf = malloc(BUFSIZ);

      /* Dump from input */

      while (true)
        {
          ret = nsh_read(vtbl, buf, BUFSIZ);
          if (ret == 0)
            {
              break;
            }
          else if (ret < 0)
            {
              if (errno == EINTR)
                {
                  continue;
                }

              break;
            }

          nsh_write(vtbl, buf, ret);
        }

      free(buf);
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_dmesg
 ****************************************************************************/

#if defined(CONFIG_SYSLOG_DEVPATH) && !defined(CONFIG_NSH_DISABLE_DMESG)
int cmd_dmesg(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  int ret = ERROR;
  int fd;
  int option;

  if (argc > 1 && (option = getopt(argc, argv, "cC:")) != ERROR)
    {
      switch (option)
      {
        case 'c':
          ret = nsh_catfile(vtbl, argv[0], CONFIG_SYSLOG_DEVPATH);

          /* Go through */

        case 'C':
          fd = open(CONFIG_SYSLOG_DEVPATH, O_RDONLY);
          if (fd < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
              return fd;
            }

          ret = ioctl(fd, BIOC_FLUSH, 0);
          if (ret < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
            }

          close(fd);
          break;
      }
    }
  else
    {
      ret = nsh_catfile(vtbl, argv[0], CONFIG_SYSLOG_DEVPATH);
    }

  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_cp
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_CP
int cmd_cp(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *srcpath  = NULL;
  FAR char *destpath = NULL;
  bool recursive = false;
  int ret = ERROR;
  int option;

  /* Get the cp flags */

  while ((option = getopt(argc, argv, "r")) != ERROR)
    {
      switch (option)
        {
          case 'r':
            recursive = true;
            break;
        }
    }

  /* Get the full path to the source file */

  srcpath = nsh_getfullpath(vtbl, argv[optind]);
  if (srcpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      goto errout;
    }

  destpath = nsh_getfullpath(vtbl, argv[optind + 1]);
  if (destpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      goto errout_with_srcpath;
    }

  /* Check if the destination does not match the source */

  if (strcmp(destpath, srcpath) == 0)
    {
      nsh_error(vtbl, g_fmtsyntax, argv[0]);
      goto errout_with_destpath;
    }

  /* Now open the destination */

  if (recursive)
    {
      ret = cp_recursive(vtbl, srcpath, destpath);
    }
  else
    {
      ret = cp_handler(vtbl, srcpath, destpath);
    }

errout_with_destpath:
  nsh_freefullpath(destpath);

errout_with_srcpath:
  nsh_freefullpath(srcpath);

errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_losetup
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
#   if defined(CONFIG_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSETUP)
int cmd_losetup(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *loopdev = NULL;
  FAR char *filepath = NULL;
  struct losetup_s setup;
  bool teardown = false;
  bool readonly = false;
  off_t offset = 0;
  int sectsize = 512;
  bool badarg = false;
  int ret = ERROR;
  int option;
  int fd;

  /* Get the losetup options:  Two forms are supported:
   *
   *   losetup -d <loop-device>
   *   losetup [-o <offset>] [-r] [-b <sectsize> ] <loop-device> <filename>
   *
   * NOTE that the -o and -r options are accepted with the -d option, but
   * will be ignored.
   */

  while ((option = getopt(argc, argv, "d:o:rb:")) != ERROR)
    {
      switch (option)
        {
        case 'd':
          loopdev  = nsh_getfullpath(vtbl, optarg);
          teardown = true;
          break;

        case 'o':
          offset = atoi(optarg);
          break;

        case 'r':
          readonly = true;
          break;

        case 'b':
          sectsize = atoi(optarg);
          break;

        case '?':
        default:
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          badarg = true;
          break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      goto errout_with_paths;
    }

  /* If this is not a tear down operation, then additional command line
   * parameters are required.
   */

  if (!teardown)
    {
      /* There must be two arguments on the command line after the options */

      if (optind + 1 < argc)
        {
          loopdev = nsh_getfullpath(vtbl, argv[optind]);
          optind++;

          filepath = nsh_getfullpath(vtbl, argv[optind]);
          optind++;
        }
      else
        {
          nsh_error(vtbl, g_fmtargrequired, argv[0]);
          goto errout_with_paths;
        }
    }

  /* There should be nothing else on the command line */

  if (optind < argc)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      goto errout_with_paths;
    }

  /* Open the loop device */

  fd = open("/dev/loop", O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      goto errout_with_paths;
    }

  /* Perform the teardown operation */

  if (teardown)
    {
      /* Tear down the loop device. */

      ret = ioctl(fd, LOOPIOC_TEARDOWN, (unsigned long)((uintptr_t)loopdev));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }
  else
    {
      /* Set up the loop device */

      setup.devname  = loopdev;   /* The loop block device to be created */
      setup.filename = filepath;  /* The file or character device to use */
      setup.sectsize = sectsize;  /* The sector size to use with the block device */
      setup.offset   = offset;    /* An offset that may be applied to the device */
      setup.readonly = readonly;  /* True: Read access will be supported only */

      ret = ioctl(fd, LOOPIOC_SETUP, (unsigned long)((uintptr_t)&setup));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }

  ret = OK;

  /* Free resources */

errout_with_fd:
  close(fd);

errout_with_paths:
  if (loopdev)
    {
      free(loopdev);
    }

  if (filepath)
    {
      free(filepath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_losmart
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
#   if defined(CONFIG_SMART_DEV_LOOP) && !defined(CONFIG_NSH_DISABLE_LOSMART)
int cmd_losmart(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *loopdev = NULL;
  FAR char *filepath = NULL;
  struct smart_losetup_s setup;
  bool teardown = false;
  bool readonly = false;
  int minor = -1;
  int erasesize = -1;
  int sectsize = -1;
  off_t offset = 0;
  bool badarg = false;
  int ret = ERROR;
  int option;
  int fd;

  /* Get the losetup options:  Two forms are supported:
   *
   *   losmart -d <loop-device>
   *   losmart [-m minor-number] [-o <offset>] [-e erasesize] [-s sectsize]
   *           [-r] <filename>
   *
   * NOTE that the -o and -r options are accepted with the -d option, but
   * will be ignored.
   */

  while ((option = getopt(argc, argv, "d:e:m:o:rs:")) != ERROR)
    {
      switch (option)
        {
        case 'd':
          loopdev  = nsh_getfullpath(vtbl, optarg);
          teardown = true;
          break;

        case 'e':
          erasesize = atoi(optarg);
          break;

        case 'm':
          minor = atoi(optarg);
          break;

        case 'o':
          offset = atoi(optarg);
          break;

        case 'r':
          readonly = true;
          break;

        case 's':
          sectsize = atoi(optarg);
          break;

        case '?':
        default:
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          badarg = true;
          break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      goto errout_with_paths;
    }

  /* If this is not a tear down operation, then additional command line
   * parameters are required.
   */

  if (!teardown)
    {
      /* There must be two arguments on the command line after the options */

      if (optind < argc)
        {
          filepath = nsh_getfullpath(vtbl, argv[optind]);
          optind++;
        }
      else
        {
          nsh_error(vtbl, g_fmtargrequired, argv[0]);
          goto errout_with_paths;
        }
    }

  /* There should be nothing else on the command line */

  if (optind < argc)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      goto errout_with_paths;
    }

  /* Open the loop device */

  fd = open("/dev/smart", O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      goto errout_with_paths;
    }

  /* Perform the teardown operation */

  if (teardown)
    {
      /* Tear down the loop device. */

      ret = ioctl(fd, SMART_LOOPIOC_TEARDOWN,
                 (unsigned long)((uintptr_t) loopdev));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }
  else
    {
      /* Set up the loop device */

      setup.minor     = minor;      /* The loop block device to be created */
      setup.filename  = filepath;   /* The file or character device to use */
      setup.sectsize  = sectsize;   /* The sector size to use with the block device */
      setup.erasesize = erasesize;  /* The sector size to use with the block device */
      setup.offset    = offset;     /* An offset that may be applied to the device */
      setup.readonly  = readonly;   /* True: Read access will be supported only */

      ret = ioctl(fd, SMART_LOOPIOC_SETUP,
                  (unsigned long)((uintptr_t)&setup));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }

  ret = OK;

  /* Free resources */

errout_with_fd:
  close(fd);

errout_with_paths:
  if (loopdev)
    {
      free(loopdev);
    }

  if (filepath)
    {
      free(filepath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_lomtd
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
#  if defined(CONFIG_MTD_LOOP) && !defined(CONFIG_NSH_DISABLE_LOMTD)
int cmd_lomtd(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *loopdev = NULL;
  FAR char *filepath = NULL;
  struct mtd_losetup_s setup;
  bool teardown = false;
  int erasesize = -1;
  int sectsize = -1;
  off_t offset = 0;
  bool badarg = false;
  int ret = ERROR;
  int option;
  int fd;

  /* Get the lomtd options:  Two forms are supported:
   *
   *   lomtd -d <loop-device>
   *   lomtd [-o <offset>] [-e erasesize] [-b sectsize]
   *         <loop-device> <filename>
   *
   * NOTE that the -o and -r options are accepted with the -d option, but
   * will be ignored.
   */

  while ((option = getopt(argc, argv, "d:o:e:b:")) != ERROR)
    {
      switch (option)
        {
        case 'd':
          loopdev  = nsh_getfullpath(vtbl, optarg);
          teardown = true;
          break;

        case 'e':
          erasesize = atoi(optarg);
          break;

        case 'o':
          offset = atoi(optarg);
          break;

        case 'b':
          sectsize = atoi(optarg);
          break;

        case '?':
        default:
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          badarg = true;
          break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      goto errout_with_paths;
    }

  /* If this is not a tear down operation, then additional command line
   * parameters are required.
   */

  if (!teardown)
    {
      /* There must be two arguments on the command line after the options */

      if (optind + 1 < argc)
        {
          loopdev = nsh_getfullpath(vtbl, argv[optind]);
          optind++;

          filepath = nsh_getfullpath(vtbl, argv[optind]);
          optind++;
        }
      else
        {
          nsh_error(vtbl, g_fmtargrequired, argv[0]);
          goto errout_with_paths;
        }
    }

  /* There should be nothing else on the command line */

  if (optind < argc)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      goto errout_with_paths;
    }

  /* Open the loop device */

  fd = open("/dev/loopmtd", O_RDONLY);
  if (fd < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      goto errout_with_paths;
    }

  /* Perform the teardown operation */

  if (teardown)
    {
      /* Tear down the loop device. */

      ret = ioctl(fd, MTD_LOOPIOC_TEARDOWN,
                  (unsigned long)((uintptr_t)loopdev));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }
  else
    {
      /* Set up the loop device */

      setup.devname   = loopdev;    /* The loop block device to be created */
      setup.filename  = filepath;   /* The file or character device to use */
      setup.sectsize  = sectsize;   /* The sector size to use with the block device */
      setup.erasesize = erasesize;  /* The sector size to use with the block device */
      setup.offset    = offset;     /* An offset that may be applied to the device */

      ret = ioctl(fd, MTD_LOOPIOC_SETUP,
                  (unsigned long)((uintptr_t)&setup));
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
          goto errout_with_fd;
        }
    }

  ret = OK;

  /* Free resources */

errout_with_fd:
  close(fd);

errout_with_paths:
  if (loopdev)
    {
      free(loopdev);
    }

  if (filepath)
    {
      free(filepath);
    }

  return ret;
}
#  endif
#endif

/****************************************************************************
 * Name: cmd_ln
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_LN) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
int cmd_ln(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *linkpath;
  FAR char *tgtpath;
  int ndx;
  int ret;

  /* ln [-s] <target> <link> */

  if (argc == 4)
    {
      if (strcmp(argv[1], "-s") != 0)
        {
          nsh_error(vtbl, g_fmtarginvalid, argv[0]);
          return ERROR;
        }

      ndx = 2;
    }
  else
    {
      ndx = 1;
    }

  /* Get the full path to the link target  */

  tgtpath = nsh_getfullpath(vtbl, argv[ndx]);
  if (tgtpath == NULL)
    {
      goto errout_with_nomemory;
    }

  /* Get the full path to the location where the link will be created */

  linkpath = nsh_getfullpath(vtbl, argv[ndx + 1]);
  if (linkpath == NULL)
    {
      goto errout_with_tgtpath;
    }

  if (ndx == 1)
    {
      ret = link(tgtpath, linkpath);
    }
  else
    {
      ret = symlink(tgtpath, linkpath);
    }

  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "link", NSH_ERRNO);
      ret = ERROR;
    }

  nsh_freefullpath(linkpath);
  nsh_freefullpath(tgtpath);
  return ret;

errout_with_tgtpath:
  nsh_freefullpath(tgtpath);
errout_with_nomemory:
  nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_ls
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_LS
int cmd_ls(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct stat st;
  FAR const char *relpath;
  unsigned int lsflags = 0;
  FAR char *fullpath;
  bool badarg = false;
  int len;
  int ret;

  /* Get the ls options */

  int option;
  while ((option = getopt(argc, argv, "lRsh")) != ERROR)
    {
      switch (option)
        {
          case 'l':
            lsflags |= (LSFLAGS_SIZE | LSFLAGS_LONG | LSFLAGS_UID_GID);
            break;

          case 'R':
            lsflags |= LSFLAGS_RECURSIVE;
            break;

          case 's':
            lsflags |= LSFLAGS_SIZE;
            break;

#ifdef CONFIG_HAVE_FLOAT
          case 'h':
            lsflags |= LSFLAGS_HUMANREADBLE;
            break;
#endif
          case '?':
          default:
            nsh_error(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There may be one argument after the options */

  if (optind + 1 < argc)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      return ERROR;
    }
  else if (optind >= argc)
    {
      relpath = nsh_getcwd(vtbl);
    }
  else
    {
      relpath = argv[optind];
    }

  /* Get the fullpath to the directory */

  fullpath = nsh_getfullpath(vtbl, relpath);
  if (fullpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      return ERROR;
    }

  /* Trim any trailing '/' characters */

  len = strlen(fullpath) - 1;
  while (len > 0 && fullpath[len] == '/')
    {
      fullpath[len] = '\0';
      len--;
    }

  /* See if it is a single file */

  if (stat(fullpath, &st) < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "stat", NSH_ERRNO);
      ret = ERROR;
    }
  else if (!S_ISDIR(st.st_mode))
    {
      /* Pass a null dirent to ls_handler to signify that this is a single
       * file
       */

      ret = ls_handler(vtbl, fullpath, NULL,
                       (FAR void *)((uintptr_t)lsflags));
    }
  else
    {
      /* List the directory contents */

      nsh_output(vtbl, "%s:\n", fullpath);

      ret = nsh_foreach_direntry(vtbl, "ls", fullpath, ls_handler,
                                 (FAR void *)((uintptr_t)lsflags));
      if (ret == OK && (lsflags & LSFLAGS_RECURSIVE) != 0)
        {
          /* Then recurse to list each directory within the directory */

          ret = nsh_foreach_direntry(vtbl, "ls", fullpath, ls_recursive,
                                     (FAR void *)((uintptr_t)lsflags));
        }
    }

  nsh_freefullpath(fullpath);
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_mkdir
 ****************************************************************************/

#ifdef NSH_HAVE_DIROPTS
#ifndef CONFIG_NSH_DISABLE_MKDIR
int cmd_mkdir(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *fullpath = NULL;
  bool parent = false;
  int ret = ERROR;
  int option;

  while ((option = getopt(argc, argv, "p")) != ERROR)
    {
      switch (option)
        {
          case 'p':
            parent = true;
            break;
        }
    }

  if (optind < argc)
    {
      fullpath = nsh_getfullpath(vtbl, argv[optind]);
    }

  if (fullpath != NULL)
    {
      FAR char *slash = parent ? fullpath : "";

      for (; ; )
        {
          slash = strchr(slash, '/');
          if (slash != NULL)
            {
              *slash = '\0';
            }

          ret = mkdir(fullpath, 0777);

          if (ret < 0 && (errno != EEXIST || !parent))
            {
              nsh_error(vtbl, g_fmtcmdfailed,
                        fullpath, "mkdir", NSH_ERRNO);
              break;
            }

          if (slash != NULL)
            {
              *slash++ = '/';
            }
          else
            {
              break;
            }
         }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_mkfatfs
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FSUTILS_MKFATFS)
#ifndef CONFIG_NSH_DISABLE_MKFATFS
int cmd_mkfatfs(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  struct fat_format_s fmt = FAT_FORMAT_INITIALIZER;
  FAR char *fullpath;
  bool badarg;
  int option;
  int rootdirentries;
  int ret = ERROR;

  /* mkfatfs [-F <fatsize>] <block-driver> */

  badarg = false;
  while ((option = getopt(argc, argv, ":F:r:")) != ERROR)
    {
      switch (option)
        {
          case 'F':
            fmt.ff_fattype = atoi(optarg);
            if (fmt.ff_fattype != 0  && fmt.ff_fattype != 12 &&
                fmt.ff_fattype != 16 && fmt.ff_fattype != 32)
              {
                nsh_error(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            break;

         case 'r':
            rootdirentries = atoi(optarg);
            if (rootdirentries >= 0)
              {
                fmt.ff_rootdirentries = rootdirentries;
              }
            else
              {
                nsh_error(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            break;

         case ':':
            nsh_error(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_error(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There should be exactly one parameter left on the command-line */

  if (optind == argc - 1)
    {
      fullpath = nsh_getfullpath(vtbl, argv[optind]);
      if (fullpath == NULL)
        {
          nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
          return ERROR;
        }
    }
  else if (optind < argc)
    {
      nsh_error(vtbl, g_fmttoomanyargs, argv[0]);
      return ERROR;
    }
  else
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

  /* Now format the FAT file system */

  ret = mkfatfs(fullpath, &fmt);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "mkfatfs", NSH_ERRNO);
    }

  nsh_freefullpath(fullpath);
  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_mkfifo
 ****************************************************************************/

#if defined(CONFIG_PIPES) && CONFIG_DEV_FIFO_SIZE > 0 && \
    !defined(CONFIG_NSH_DISABLE_MKFIFO)
int cmd_mkfifo(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *fullpath = nsh_getfullpath(vtbl, argv[1]);
  int ret = ERROR;

  if (fullpath != NULL)
    {
      ret = mkfifo(fullpath, 0777);
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "mkfifo", NSH_ERRNO);
        }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif /* CONFIG_PIPES && CONFIG_DEV_FIFO_SIZE > 0 && !CONFIG_NSH_DISABLE_MKFIFO */

/****************************************************************************
 * Name: cmd_mkrd
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_MKRD
int cmd_mkrd(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR const char *fmt;
  struct boardioc_mkrd_s desc;
  uint32_t nsectors;
  bool badarg = false;
  int sectsize = 512;
  int minor = 0;
  int ret;

  /* Get the mkrd options */

  int option;
  while ((option = getopt(argc, argv, ":m:s:")) != ERROR)
    {
      switch (option)
        {
          case 'm':
            minor = atoi(optarg);
            if (minor < 0 || minor > 255)
              {
                nsh_error(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            break;

          case 's':
            sectsize = atoi(optarg);
            if (minor < 0 || minor > 16384)
              {
                nsh_error(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            break;

         case ':':
            nsh_error(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_error(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered,
   * then return without processing the command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There should be exactly one parameter left on the command-line */

  if (optind == argc - 1)
    {
      nsectors = (uint32_t)atoi(argv[optind]);
    }
  else if (optind < argc)
    {
      fmt = g_fmttoomanyargs;
      goto errout_with_fmt;
    }
  else
    {
      fmt = g_fmtargrequired;
      goto errout_with_fmt;
    }

  /* Then create the ramdisk */

  desc.minor    = minor;         /* Minor device number of the RAM disk. */
  desc.nsectors = nsectors;      /* The number of sectors in the RAM disk. */
  desc.sectsize = sectsize;      /* The size of one sector in bytes. */
  desc.rdflags  = RDFLAG_WRENABLED | RDFLAG_FUNLINK;

  ret = boardctl(BOARDIOC_MKRD, (uintptr_t)&desc);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "boardctl(BOARDIOC_MKRD)",
                NSH_ERRNO_OF(-ret));
      return ERROR;
    }

  return ret;

errout_with_fmt:
  nsh_output(vtbl, fmt, argv[0]);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: cmd_mksmartfs
 ****************************************************************************/

#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_FS_SMARTFS) && \
    defined(CONFIG_FSUTILS_MKSMARTFS)
#ifndef CONFIG_NSH_DISABLE_MKSMARTFS
int cmd_mksmartfs(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR char *fullpath = NULL;
  int ret = OK;
  uint16_t  sectorsize = 0;
  int force = 0;
  int opt;
#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
  int nrootdirs = 1;
#endif

  /* Process any options */

  optind = 0;
  while ((opt = getopt(argc, argv, "fs:")) != -1)
    {
      switch (opt)
        {
          case 'f':
            force = 1;
            break;

          case 's':
            sectorsize = atoi(optarg);
            if (sectorsize < 256 || sectorsize > 16384)
              {
                nsh_error(vtbl, "Sector size must be 256-16384\n");
                return EINVAL;
              }

            if (sectorsize & (sectorsize - 1))
              {
                nsh_error(vtbl, "Sector size must be power of 2\n");
                return EINVAL;
              }

            break;

          default:
            break;
        }
    }

  if (optind < argc)
    {
      fullpath = nsh_getfullpath(vtbl, argv[optind]);
    }

  if (fullpath)
    {
      /* Test if number of root directories was supplied */

#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
      if (optind + 1 < argc)
        {
          nrootdirs = atoi(argv[optind + 1]);
        }

      if (nrootdirs > 8 || nrootdirs < 1)
        {
          nsh_error(vtbl, "Invalid number of root directories specified\n");
        }
      else
#endif
      if (force || issmartfs(fullpath) != OK)
        {
#ifdef CONFIG_SMARTFS_MULTI_ROOT_DIRS
          ret = mksmartfs(fullpath, sectorsize, nrootdirs);
#else
          ret = mksmartfs(fullpath, sectorsize);
#endif
          if (ret < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "mksmartfs",
                        NSH_ERRNO);
            }
        }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_mv
 ****************************************************************************/

#ifdef NSH_HAVE_DIROPTS
#ifndef CONFIG_NSH_DISABLE_MV
int cmd_mv(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *oldpath;
  FAR char *newpath;
  int ret;

  /* Get the full path to the old and new file paths */

  oldpath = nsh_getfullpath(vtbl, argv[1]);
  if (oldpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      return ERROR;
    }

  newpath = nsh_getfullpath(vtbl, argv[2]);
  if (newpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      ret = ERROR;
      goto errout_with_oldpath;
    }

  /* Perform the mount */

  ret = rename(oldpath, newpath);
  if (ret < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "rename", NSH_ERRNO);
    }

  /* Free the file paths */

  nsh_freefullpath(newpath);

errout_with_oldpath:
  nsh_freefullpath(oldpath);
  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_readlink
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_READLINK) && defined(CONFIG_PSEUDOFS_SOFTLINKS)
int cmd_readlink(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *fullpath;
  ssize_t len;

  /* readlink <link> */

  /* Get the fullpath to the directory */

  fullpath = nsh_getfullpath(vtbl, argv[1]);
  if (fullpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      return ERROR;
    }

  len = readlink(fullpath, vtbl->iobuffer, IOBUFFERSIZE);
  nsh_freefullpath(fullpath);

  if (len < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, "ls", "readlink", NSH_ERRNO);
      return ERROR;
    }

  nsh_output(vtbl, "%s\n", vtbl->iobuffer);
  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_rm
 ****************************************************************************/

#ifdef NSH_HAVE_DIROPTS
#ifndef CONFIG_NSH_DISABLE_RM

static int unlink_recursive(FAR char *path, FAR struct stat *stat)
{
  struct dirent *d;
  size_t len;
  int ret;
  DIR *dp;

  ret = lstat(path, stat);
  if (ret < 0)
    {
      return ret;
    }

  if (!S_ISDIR(stat->st_mode))
    {
      return unlink(path);
    }

  dp = opendir(path);
  if (dp == NULL)
    {
      return -1;
    }

  len = strlen(path);
  if (len > 0 && path[len - 1] == '/')
    {
      path[--len] = '\0';
    }

  while ((d = readdir(dp)) != NULL)
    {
      if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
        {
          continue;
        }

      snprintf(&path[len], PATH_MAX - len, "/%s", d->d_name);
      ret = unlink_recursive(path, stat);
      if (ret < 0)
        {
          closedir(dp);
          return ret;
        }
    }

  ret = closedir(dp);
  if (ret >= 0)
    {
      path[len] = '\0';
      ret = rmdir(path);
    }

  return ret;
}

int cmd_rm(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  bool recursive = false;
  bool force = false;
  FAR char *fullpath;
  struct stat stat;
  int ret = ERROR;
  int c;

  while ((c = getopt(argc, argv, "rf")) != ERROR)
    {
      switch (c)
        {
          case 'r':
            recursive = true;
            break;
          case 'f':
            force = true;
            break;
          case '?':
            nsh_output(vtbl, "Unknown option 0x%x\n", optopt);
            return ret;
          default:
            nsh_error(vtbl, g_fmtargrequired, argv[0]);
            return ret;
        }
    }

  if (optind >= argc)
    {
      if (force)
        {
          ret = OK;
        }
      else
        {
          nsh_error(vtbl, g_fmtargrequired, argv[0]);
        }

      return ret;
    }

  fullpath = nsh_getfullpath(vtbl, argv[optind]);
  if (fullpath != NULL)
    {
      if (recursive)
        {
          ret = unlink_recursive(fullpath, &stat);
        }
      else
        {
          ret = unlink(fullpath);
        }

      if (force && errno == ENOENT)
        {
          ret = 0;
        }

      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "unlink", NSH_ERRNO);
        }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_rmdir
 ****************************************************************************/

#ifdef NSH_HAVE_DIROPTS
#ifndef CONFIG_NSH_DISABLE_RMDIR
int cmd_rmdir(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *fullpath = nsh_getfullpath(vtbl, argv[1]);
  int ret = ERROR;

  if (fullpath != NULL)
    {
      ret = rmdir(fullpath);
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "rmdir", NSH_ERRNO);
        }

      nsh_freefullpath(fullpath);
    }

  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_source
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLESCRIPT) && !defined(CONFIG_NSH_DISABLE_SOURCE)
int cmd_source(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  return nsh_script(vtbl, argv[0], argv[1], true);
}
#endif

/****************************************************************************
 * Name: cmd_cmp
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_CMP
int cmd_cmp(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *path1 = NULL;
  FAR char *path2 = NULL;
  int fd1 = -1;
  int fd2 = -1;
  int ret = ERROR;

  /* Get the full path to the two files */

  path1 = nsh_getfullpath(vtbl, argv[1]);
  if (path1 == NULL)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      goto errout;
    }

  path2 = nsh_getfullpath(vtbl, argv[2]);
  if (path2 == NULL)
    {
      nsh_error(vtbl, g_fmtargrequired, argv[0]);
      goto errout_with_path1;
    }

  /* Open the files for reading */

  fd1 = open(path1, O_RDONLY);
  if (fd1 < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      goto errout_with_path2;
    }

  fd2 = open(path2, O_RDONLY);
  if (fd2 < 0)
    {
      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      goto errout_with_fd1;
    }

  /* The loop until we hit the end of file or find a difference in the two
   * files.
   */

  for (; ; )
    {
      char buf1[128];
      char buf2[128];

      /* Read the file data */

      ssize_t nbytesread1 = read(fd1, buf1, sizeof(buf1));
      ssize_t nbytesread2 = read(fd2, buf2, sizeof(buf2));

      if (nbytesread1 < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "read", NSH_ERRNO);
          goto errout_with_fd2;
        }

      if (nbytesread2 < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "read", NSH_ERRNO);
          goto errout_with_fd2;
        }

      /* Compare the file data */

      if (nbytesread1 != nbytesread2 ||
          memcmp(buf1, buf2, nbytesread1) != 0)
        {
          nsh_error(vtbl, "files differ: byte %zd\n",
                    nbytesread1 > nbytesread2 ? nbytesread2 : nbytesread1);
          goto errout_with_fd2;
        }

      /* A partial read indicates the end of file (usually) */

      if (nbytesread1 < (ssize_t)sizeof(buf1))
        {
          break;
        }
    }

  /* The files are the same, i.e., the end of file was encountered
   * without finding any differences.
   */

  ret = OK;

errout_with_fd2:
  close(fd2);
errout_with_fd1:
  close(fd1);
errout_with_path2:
  nsh_freefullpath(path2);
errout_with_path1:
  nsh_freefullpath(path1);
errout:
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_truncate
 ****************************************************************************/

#ifndef CONFIG_DISABLE_MOUNTPOINT
#ifndef CONFIG_NSH_DISABLE_TRUNCATE
int cmd_truncate(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  FAR char *fullpath;
  FAR char *endptr;
  struct stat buf;
  unsigned long length;
  int ret;

  /* truncate -s <length> <file-path> */

  if (strcmp(argv[1], "-s") != 0)
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  length = strtoul(argv[2], &endptr, 0);
  if (*endptr != '\0')
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  /* Get the full path to the file */

  fullpath = nsh_getfullpath(vtbl, argv[3]);
  if (fullpath == NULL)
    {
      nsh_error(vtbl, g_fmtcmdoutofmemory, argv[0]);
      return ERROR;
    }

  /* stat the file */

  ret = stat(fullpath, &buf);
  if (ret < 0)
    {
      int errval = errno;

      /* If stat() failed because the file does not exist, then try to
       * create it.
       */

      if (errval == ENOENT)
        {
          int fd = creat(fullpath, 0666);
          if (fd < 0)
            {
              nsh_error(vtbl, g_fmtcmdfailed, argv[0], "stat", NSH_ERRNO);
              ret = ERROR;
            }
          else
            {
              /* We successfully performed the create and now have a zero-
               * length file.  Perform the truncation to extend the
               * allocation (unless we really wanted a zero-length file).
               */

              ret = OK;
              if (length > 0)
                {
                  /* Extend the file to length */

                  ret = ftruncate(fd, length);
                  if (ret < 0)
                    {
                      nsh_error(vtbl, g_fmtcmdfailed, argv[0], "ftruncate",
                                 NSH_ERRNO);
                    }
                }

              close(fd);
            }
        }
      else
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "stat",
                     NSH_ERRNO_OF(errval));
          ret = ERROR;
        }
    }
  else if (!S_ISREG(buf.st_mode))
    {
      nsh_error(vtbl, g_fmtarginvalid, argv[0]);
      ret = ERROR;
    }
  else
    {
      /* Everything looks good... perform the truncation */

      ret = truncate(fullpath, length);
      if (ret < 0)
        {
          nsh_error(vtbl, g_fmtcmdfailed, argv[0], "truncate", NSH_ERRNO);
        }
    }

  nsh_freefullpath(fullpath);
  return ret;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_fdinfo
 ****************************************************************************/

#if defined(CONFIG_FS_PROCFS) && !defined(CONFIG_NSH_DISABLE_FDINFO)
int cmd_fdinfo(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  UNUSED(argc);

  if (argv[1] != NULL)
    {
      FAR char *fdpath = NULL;
      int ret;

      /* The directories of the processes are displayed numerically */

      if (!isdigit(argv[1][0]))
        {
          nsh_error(vtbl, g_fmtcmdfailed, "fdinfo",
                   "not process id", NSH_ERRNO);
          return ERROR;
        }

      ret = asprintf(&fdpath, "%s/%s/group/fd",
                     CONFIG_NSH_PROC_MOUNTPOINT, argv[1]);
      if (ret < 0)
        {
          return ret;
        }

      ret = nsh_catfile(vtbl, argv[0], fdpath);
      free(fdpath);
      return ret;
    }

  return nsh_foreach_direntry(vtbl, "fdinfo", CONFIG_NSH_PROC_MOUNTPOINT,
                              fdinfo_callback, NULL);
}
#endif
