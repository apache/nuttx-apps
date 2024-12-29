/****************************************************************************
 * apps/testing/testsuites/kernel/syscall/common/test_syscall_common.c
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
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include "SyscallTest.h"

#define CM_SYSCALL_TESTDIR "CM_syscall_testdir"
#define PATH_SIZE 128

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int rm_recursive(FAR char *path)
{
  struct dirent *d;
  struct stat stat;
  size_t len;
  int ret;
  DIR *dp;

  ret = lstat(path, &stat);
  if (ret < 0)
    {
      return ret;
    }

  if (!S_ISDIR(stat.st_mode))
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
      ret = rm_recursive(path);
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

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_nuttx_syscall_testgroupsetup
 ****************************************************************************/

int test_nuttx_syscall_test_group_setup(void **state)
{
  int res;
  struct stat buf;

  res = chdir(MOUNT_DIR);
  if (res != 0)
    {
      syslog(LOG_INFO, "ERROR: Failed to switch the mount dir\n");
      exit(1);
    }

  res = stat(CM_SYSCALL_TESTDIR, &buf);
  if (res == 0 && buf.st_mode == S_IFDIR)
    {
      res = chdir(CM_SYSCALL_TESTDIR);
      return res;
    }

  else
    {
      char testdir[PATH_MAX] =
      {
        0
      };

      sprintf(testdir, "%s/%s", MOUNT_DIR, CM_SYSCALL_TESTDIR);

      /* Delete the existing test directory */

      rm_recursive(testdir);
      res = mkdir(CM_SYSCALL_TESTDIR, 0777);
      if (res != 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to creat the test directory\n");
          exit(1);
        }

      res = chdir(CM_SYSCALL_TESTDIR);
    }

  return res;
}

/****************************************************************************
 * Name: test_nuttx_syscall_test_group_teardown
 ****************************************************************************/

int test_nuttx_syscall_test_group_teardown(void **state)
{
  int res;
  char testdir[PATH_MAX] =
  {
    0
  };

  sprintf(testdir, "%s", CM_SYSCALL_TESTDIR);

  res = chdir(MOUNT_DIR);
  if (res != 0)
    {
      syslog(LOG_INFO, "ERROR: Failed to switch the mount dir\n");
      exit(1);
    }

  /* call the recursive delete interface */

  rm_recursive(testdir);

  return 0;
}

/****************************************************************************
 * Name: cmtestfillfilewithfd
 ****************************************************************************/

static int cmtestfillfilewithfd(int fd, char pattern, size_t bs,
                                size_t bcount)
{
  size_t i;
  char *buf;

  /* Filling a memory buffer with provided pattern */

  buf = (char *)malloc(bs);
  if (buf == NULL)
      return -1;

  for (i = 0; i < bs; i++)
    buf[i] = pattern;

  /* Filling the file */

  for (i = 0; i < bcount; i++)
    {
      if (write(fd, buf, bs) != (ssize_t)bs)
        {
          free(buf);
          return -1;
        }
    }

  free(buf);

  return 0;
}

/****************************************************************************
 * Name: cmtestfillfile
 ****************************************************************************/

int cmtestfillfile(const char *path, char pattern, size_t bs,
                   size_t bcount)
{
  int fd;

  fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0)
      return -1;

  if (cmtestfillfilewithfd(fd, pattern, bs, bcount))
    {
      close(fd);
      unlink(path);
      return -1;
    }

  if (close(fd) < 0)
    {
      unlink(path);

      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: checknames
 ****************************************************************************/

int checknames(char *pfilnames, DIR *ddir)
{
  struct dirent *dir;
  while ((dir = readdir(ddir)) != NULL)
    {
      /* cout << "file name=" << dir->d_name << endl; */

      if (strcmp(pfilnames, dir->d_name) == 0)
          return 0;
    }

  return -1;
}

/****************************************************************************
 * Name: check_and_report_ftruncatetest
 ****************************************************************************/

int check_and_report_ftruncatetest(int fd, off_t offset, char data,
                                   off_t trunc_len)
{
  int i;
  int file_length;
  char buf[1024];
  struct stat stat_buf;

  memset(buf, '*', sizeof(buf));

  fstat(fd, &stat_buf);
  file_length = stat_buf.st_size;

  if (file_length != trunc_len)
    {
      syslog(LOG_ERR, "FAIL, ftruncate() got incorrected size: %d\n",
             file_length);
      return -1;
    }

  lseek(fd, offset, SEEK_SET);
  read(fd, buf, sizeof(buf));

  for (i = 0; i < 256; i++)
    {
      if (buf[i] != data)
        {
          syslog(
              LOG_ERR,
              "FAIL, ftruncate() got incorrect data %i, expected %i\n",
              buf[i], data);
          return -1;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: tst_fill_fd
 ****************************************************************************/

int tst_fill_fd(int fd, char pattern, size_t bs, size_t bcount)
{
  size_t i;
  char *buf;

  /* Filling a memory buffer with provided pattern */

  buf = (char *)malloc(bs);
  if (buf == NULL)
      return -1;

  for (i = 0; i < bs; i++)
    buf[i] = pattern;

  /* Filling the file */

  for (i = 0; i < bcount; i++)
    {
      if (write(fd, buf, bs) != (ssize_t)bs)
        {
          free(buf);
          return -1;
        }
    }

  free(buf);

  return 0;
}

/****************************************************************************
 * Name: tst_fill_file
 ****************************************************************************/

int tst_fill_file(const char *path, char pattern, size_t bs,
                  size_t bcount)
{
  int fd;

  fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0)
      return -1;

  if (tst_fill_fd(fd, pattern, bs, bcount))
    {
      close(fd);
      unlink(path);
      return -1;
    }

  if (close(fd) < 0)
    {
      unlink(path);

      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: safe_open
 ****************************************************************************/

int safe_open(const char *pathname, int oflags, ...)
{
  va_list ap;
  int rval;
  mode_t mode;

  va_start(ap, oflags);
  mode = va_arg(ap, int);
  va_end(ap);

  rval = open(pathname, oflags, mode);
  if (rval == -1)
    {
      syslog(LOG_ERR, "open(%s,%d,0%o) failed\n", pathname, oflags,
             mode);
      fail_msg("test fail !");
    }

  return rval;
}

/****************************************************************************
 * Name: safe_write
 ****************************************************************************/

ssize_t safe_write(int fildes, const void *buf, size_t nbyte)
{
  ssize_t rval;

  rval = write(fildes, buf, nbyte);
  if (rval == -1 || ((size_t)rval != nbyte))
    {
      syslog(LOG_ERR, "write(%d,%p,%zu) failed\n", fildes, buf, rval);
    }

  return rval;
}

/****************************************************************************
 * Name: safe_lseek
 ****************************************************************************/

off_t safe_lseek(int fd, off_t offset, int whence)
{
  off_t rval;

  rval = lseek(fd, offset, whence);

  if (rval == (off_t)-1)
    {
      syslog(LOG_ERR, "lseek(%d, %ld, %d) failed\n", fd,
             (long int)offset, whence);
    }

  return rval;
}

/****************************************************************************
 * Name: safe_close
 ****************************************************************************/

int safe_close(int fildes)
{
  int rval;

  rval = close(fildes);
  if (rval == -1)
    {
      syslog(LOG_ERR, "close(%d) failed, errno %d\n", fildes, errno);
      fail_msg("test fail !");
    }

  return rval;
}

/****************************************************************************
 * Function Name: safe_touch
 *
 * Description:
 *  This Function is used to touch a file.
 *
 * Input Parameters:
 *   file
 *   lineno
 *   pathname     -  name of the file to be created
 *   mode         -  mode
 ****************************************************************************/

void safe_touch(const char *file, const int lineno, const char *pathname,
                mode_t mode)
{
  int ret;

  /* Open the file */

  ret = open(pathname, O_CREAT | O_WRONLY, mode);

  /* Open file fail */

  if (ret == -1)
    {
      syslog(LOG_ERR, "Failed to open file '%s' at %s:%d \n", pathname,
             file, lineno);
      return;
    }

  /* Close the file */

  ret = close(ret);

  /* Close file fail */

  if (ret == -1)
    {
      syslog(LOG_ERR, "Failed to close file '%s' at %s:%d \n", pathname,
             file, lineno);
      return;
    }
}

#ifdef CONFIG_NET

/****************************************************************************
 * Name: sock_addr
 ****************************************************************************/

char *sock_addr(const struct sockaddr *sa, socklen_t salen, char *res,
                size_t len)
{
  char portstr[8];

  switch (sa->sa_family)
    {
    case AF_INET:
      {
        struct sockaddr_in *sin = (struct sockaddr_in *)sa;

        if (!inet_ntop(AF_INET, &sin->sin_addr, res, len))
          return NULL;

        if (ntohs(sin->sin_port) != 0)
          {
            snprintf(portstr, sizeof(portstr), ":%d",
                     ntohs(sin->sin_port));
            strcat(res, portstr);
          }

        return res;
      }

    case AF_INET6:
      {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

        res[0] = '[';
        if (!inet_ntop(AF_INET6, &sin6->sin6_addr, res + 1, len - 1))
          return NULL;

        if (ntohs(sin6->sin6_port) != 0)
          {
            snprintf(portstr, sizeof(portstr), "]:%d",
                     ntohs(sin6->sin6_port));
            strcat(res, portstr);
            return res;
          }

        return res + 1;
      }

    case AF_UNIX:
      {
        struct sockaddr_un *unp = (struct sockaddr_un *)sa;

        if (unp->sun_path[0] == '\0')
          strcpy(res, "(no pathname bound)");
        else
          snprintf(res, len, "%s", unp->sun_path);

        return res;
      }

    default:
      {
        snprintf(res, len, "sock_ntop: unknown AF_xxx: %d, len: %d\n",
                 sa->sa_family, salen);

        return res;
      }
    }
}

/****************************************************************************
 * Name: get_unused_port
 * *
 * Description:
 *  return port in network byte order.
 ****************************************************************************/

unsigned short get_unused_port(void(cleanup_fn)(void),
                               unsigned short family, int type)
{
  int sock;
  socklen_t slen;
  struct sockaddr_storage _addr;
  struct sockaddr *addr = (struct sockaddr *)&_addr;
  struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
  struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;

  switch (family)
    {
    case AF_INET:
      addr4->sin_family = AF_INET;
      addr4->sin_port = 0;
      addr4->sin_addr.s_addr = INADDR_ANY;
      slen = sizeof(*addr4);
      break;

    case AF_INET6:
      addr6->sin6_family = AF_INET6;
      addr6->sin6_port = 0;
      addr6->sin6_addr = in6addr_any;
      slen = sizeof(*addr6);
      break;

    default:
      syslog(LOG_ERR, "nknown family\n");
      fail_msg("test fail !");
      return -1;
    }

  sock = socket(addr->sa_family, type, 0);
  if (sock < 0)
    {
      syslog(LOG_ERR, "socket failed\n");
      fail_msg("test fail !");
      return -1;
    }

  if (bind(sock, addr, slen) < 0)
    {
      syslog(LOG_ERR, "bind failed\n");
      fail_msg("test fail !");
      return -1;
    }

  if (getsockname(sock, addr, &slen) == -1)
    {
      syslog(LOG_ERR, "getsockname failed\n");
      fail_msg("test fail !");
      return -1;
    }

  if (close(sock) == -1)
    {
      syslog(LOG_ERR, "close failed\n");
      fail_msg("test fail !");
      return -1;
    }

  switch (family)
    {
    case AF_INET:
      return addr4->sin_port;
    case AF_INET6:
      return addr6->sin6_port;
    default:
      return -1;
    }
}

/****************************************************************************
 * Name: safe_socket
 ****************************************************************************/

int safe_socket(int domain, int type, int protocol)
{
  int rval;

  rval = socket(domain, type, protocol);

  if (rval < 0)
    {
      syslog(LOG_ERR, "socket(%d, %d, %d) failed\n", domain, type,
             protocol);
      fail_msg("test fail !");
    }

  return rval;
}

/****************************************************************************
 * Name: safe_connect
 ****************************************************************************/

int safe_connect(int sockfd, const struct sockaddr *addr,
                 socklen_t addrlen)
{
  int rval;
  char buf[128];

  rval = connect(sockfd, addr, addrlen);

  if (rval < 0)
    {
      syslog(LOG_ERR, "connect(%d, %s, %d) failed, errno %d\n", sockfd,
             sock_addr(addr, addrlen, buf, sizeof(buf)), addrlen, errno);
      fail_msg("test fail !");
    }

  return rval;
}

/****************************************************************************
 * Name: safe_getsockname
 ****************************************************************************/

int safe_getsockname(int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen)
{
  int rval;
  char buf[128];

  rval = getsockname(sockfd, addr, addrlen);

  if (rval < 0)
    {
      syslog(LOG_ERR, "getsockname(%d, %s, %d) failed\n", sockfd,
             sock_addr(addr, *addrlen, buf, sizeof(buf)), *addrlen);
    }

  return rval;
}

/****************************************************************************
 * Name: safe_bind
 ****************************************************************************/

int safe_bind(int socket, const struct sockaddr *address,
              socklen_t address_len)
{
  int i;
  char buf[128];

  for (i = 0; i < 120; i++)
    {
      if (!bind(socket, address, address_len))
          return 0;

      if (errno != EADDRINUSE)
        {
          syslog(LOG_ERR, "bind(%d, %s, %d) failed\n", socket,
                 sock_addr(address, address_len, buf, sizeof(buf)),
                 address_len);
          fail_msg("test fail !");
          return -1;
        }

      if ((i + 1) % 10 == 0)
        {
          syslog(LOG_INFO, "address is in use, waited %3i sec\n", i + 1);
        }

      sleep(1);
    }

  syslog(LOG_ERR, "Failed to bind(%d, %s, %d) after 120 retries\n",
         socket, sock_addr(address, address_len, buf, sizeof(buf)),
         address_len);
  fail_msg("test fail !");
  return -1;
}

/****************************************************************************
 * Name: safe_listen
 ****************************************************************************/

int safe_listen(int socket, int backlog)
{
  int rval;

  rval = listen(socket, backlog);

  if (rval < 0)
    {
      syslog(LOG_ERR, "listen(%d, %d) failed\n", socket, backlog);
    }

  return rval;
}

#endif
