/****************************************************************************
 * netutils/webserver/httpd_dirlist.c
 *
 *   Copyright 2019 Sony Home Entertainment & Sound Products Inc.
 *   Author: Masayuki Ishikawa <Masayuki.Ishikawa@jp.sony.com>
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
 * 3. Neither the name NuttX nor the names of its contributors may be
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

/****************************************************************************
 * Included Header Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
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

      path = malloc(CONFIG_NAME_MAX);
      ASSERT(path);

      snprintf(path, CONFIG_NAME_MAX, "%s/%s",
               file->path, dent->d_name);

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
          snprintf(size, sizeof(size), "%d",
                   buf.st_size);

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
