/****************************************************************************
 * apps/netutils/webserver/httpd_dirlist.c
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
#include <sys/utsname.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#include "netutils/httpd.h"

#include "httpd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_ARCH_BOARD_CUSTOM
#  ifndef CONFIG_ARCH_BOARD_CUSTOM_NAME
#    define BOARD_NAME g_unknown
#  else
#    define BOARD_NAME CONFIG_ARCH_BOARD_CUSTOM_NAME
#  endif
#else
#  ifndef CONFIG_ARCH_BOARD
#    define BOARD_NAME g_unknown
#  else
#    define BOARD_NAME CONFIG_ARCH_BOARD
#  endif
#endif

#define BUF_SIZE 1024

#define HEADER \
  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n" \
  "<html>\n" \
  "<head>\n"

#define TITLE \
  " <title>Index of /%s</title>\n" \
  "</head>\n" \
  "<body>\n" \
  "<h2>Index of /%s</h2>\n" \
  "<table>\n"

#define TABLE \
  " <tr><th valign=\"top\"></th><th>Name</a></th><th>Last modified</a>" \
  "</th><th>Size</a></th></tr>\n" \
  " <tr><th colspan=\"4\"><hr></th></tr>\n"

#define PARENT \
  " <tr><td valign=\"top\"></td><td><a href=\"..\">../</a></td>\n" \
  " <td align=\"right\"> - </td>" \
  "<td align=\"right\"> - </td><td>&nbsp; </td></tr>\n"

#define ENTRY                                                         \
  " <tr><td valign=\"top\"></td><td><a href=\"%s%s\">%s%s</a></td>\n" \
  " <td align=\"right\">%4d-%02d-%02d %02d:%02d</td>" \
  "<td align=\"right\"> %s </td><td>&nbsp; </td></tr>\n"

#define FOOTER \
  " <tr><th colspan=\"4\"><hr></th></tr>\n" \
  "</table>\n" \
  "<address>uIP web server (%s %s %s %s %s) </address>\n" \
  "</body>\n" \
  "</html>\n"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: httpd_is_file
 ****************************************************************************/

bool httpd_is_file(FAR const char *filename)
{
  char *path;
  int fd;
  bool ret = false;

  path = malloc(CONFIG_NAME_MAX);
  ASSERT(path);

  snprintf(path, CONFIG_NAME_MAX, "%s/%s",
           CONFIG_NETUTILS_HTTPD_PATH, filename);

  fd = open(path, O_RDONLY);

  if (-1 != fd)
    {
      close(fd);
      ret = true;
    }

  free(path);
  return ret;
}

/****************************************************************************
 * Name: httpd_dirlist
 ****************************************************************************/

ssize_t httpd_dirlist(int outfd, FAR struct httpd_fs_file *file)
{
  struct dirent *dent;
  struct utsname info;
  struct stat buf;
  struct tm tm;
  ssize_t total;
  ssize_t ret;
  char size[16];
  char *path;
  char *tmp;
  DIR *dir;

  total = 0;

  tmp = (char *)calloc(1, BUF_SIZE);
  ASSERT(tmp);

  /* exclude document root path */

  path = file->path + sizeof(CONFIG_NETUTILS_HTTPD_PATH);

  /* compose hdr/title/path/table and write them */

  if ('\0' == *path)
    {
      snprintf(tmp, BUF_SIZE, HEADER TITLE TABLE,
               "", "");
    }
  else
    {
      snprintf(tmp, BUF_SIZE, HEADER TITLE TABLE PARENT,
               path, path);
    }

  ret = write(outfd, tmp, strlen(tmp));

  if (-1 == ret)
    {
      goto errout;
    }

  total += ret;

  dir = opendir(file->path);

  if (NULL == dir)
    {
      goto errout_with_hdr;
    }

  while (true)
    {
      dent = readdir(dir);

      if (NULL == dent)
        {
          break;
        }

      ret = asprintf(&path, "%s/%s", file->path, dent->d_name);
      ASSERT(ret > 0 && path);

      /* call stat() to obtain modified time and size */

      ret = stat(path, &buf);
      ASSERT(0 == ret);

      free(path);
      localtime_r(&buf.st_mtime, &tm);

      /* compose an entry name for directory or file */

      if (dent->d_type == DTYPE_DIRECTORY)
        {
          snprintf(tmp, BUF_SIZE, ENTRY,
                   dent->d_name, "/", dent->d_name, "/",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min,
                   "-"
                   );
        }
      else
        {
          snprintf(size, sizeof(size), "%jd",
                   (uintmax_t)buf.st_size);

          snprintf(tmp, BUF_SIZE, ENTRY,
                   dent->d_name, "", dent->d_name, "",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min,
                   size
                   );
        }

      ret = write(outfd, tmp, strlen(tmp));

      if (-1 == ret)
        {
          break;
        }

      total += ret;
    }

  closedir(dir);

errout_with_hdr:

  memset(&info, 0, sizeof(info));
  uname(&info);

  snprintf(tmp, BUF_SIZE, FOOTER,
           info.sysname,
           info.release,
           info.version,
           info.machine,
           BOARD_NAME);

  ret = write(outfd, tmp, strlen(tmp));

  if (-1 != ret)
    {
      total += ret;
    }

  ret = total;

errout:

  free(tmp);
  return ret;
}
