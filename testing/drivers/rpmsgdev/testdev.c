/****************************************************************************
 * apps/testing/drivers/rpmsgdev/testdev.c
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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#if defined(__NuttX__) && (defined(CONFIG_DEV_RPMSG) || defined(CONFIG_DEV_RPMSG_SERVER))
#include <nuttx/drivers/rpmsgdev.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef __ANDROID__
#define syslog(l,...) printf(__VA_ARGS__)
#endif

/* set a random open flag to check whether the open api behave correctly */

#define OPEN_FLAG O_RDWR
#define RET ENOENT
#define OFFSET 1234l
#define WHENCE SEEK_SET
#ifdef __NuttX__
#define IOCTL FIONSPACE
#define IOCTL_ARG 0x55AA
#endif
static int hour __attribute__((unused));

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef __NuttX__
static int     testdev_open(FAR struct file *filep);
static int     testdev_close(FAR struct file *filep);
static ssize_t testdev_read(FAR struct file *filep, FAR char *buffer,
                            size_t buflen);
static ssize_t testdev_write(FAR struct file *filep, FAR const char *buffer,
                             size_t buflen);
static off_t   testdev_seek(FAR struct file *filep, off_t offset,
                            int whence);
static int     testdev_ioctl(FAR struct file *filep, int cmd,
                             unsigned long arg);
static int     testdev_poll(FAR struct file *filep, FAR struct pollfd *fds,
                            bool setup);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_testdev_ops =
{
  .open = testdev_open,   /* open */
  .close = testdev_close, /* close */
  .read = testdev_read,   /* read */
  .write = testdev_write, /* write */
  .seek = testdev_seek,   /* seek */
  .ioctl = testdev_ioctl, /* ioctl */
  .poll = testdev_poll    /* poll */
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
  ,
  .unlink = NULL /* unlink */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int testdev_open(FAR struct file *filep)
{
  if ((filep->f_oflags & (OPEN_FLAG | O_NONBLOCK)) ==
      (OPEN_FLAG | O_NONBLOCK))
    {
      filep->f_pos = 0;
      syslog(LOG_INFO, "test open success\n");
      return 0;
    }

  syslog(LOG_INFO, "test open fail, %d\n", filep->f_oflags);
  return -RET;
}

static int testdev_close(FAR struct file *filep)
{
  if ((filep->f_oflags & (OPEN_FLAG | O_NONBLOCK)) ==
      (OPEN_FLAG | O_NONBLOCK))
    {
      syslog(LOG_INFO, "test close success\n");
      return 0;
    }

  syslog(LOG_INFO, "test close fail, %d\n", filep->f_oflags);
  return -1;
}

static ssize_t testdev_read(FAR struct file *filep, FAR char *buffer,
                            size_t buflen)
{
  size_t i;

  for (i = 0; i < buflen; i++)
    {
      buffer[i] = 'r';
    }

  syslog(LOG_INFO, "test read success\n");
  return buflen;
}

static ssize_t testdev_write(FAR struct file *filep, FAR const char *buffer,
                             size_t buflen)
{
  struct timeval tv;
  size_t i;

  for (i = 0; i < buflen; i++)
    {
      if (buffer[i] != 'w')
        {
          syslog(LOG_INFO, "test write fail\n");
          return -1;
        }
    }

  tv.tv_usec = buflen;
  tv.tv_sec = 0;
  select(0, NULL, NULL, NULL, &tv);
  syslog(LOG_INFO, "test write success\n");
  return buflen;
}

static off_t testdev_seek(FAR struct file *filep, off_t offset, int whence)
{
  if (offset != OFFSET || whence != WHENCE)
    {
      syslog(LOG_ERR, "test seek fail, offset %ld(except %ld), "
             "whence %d(except %d)\n", (long)offset, OFFSET, whence, WHENCE);
      return -1;
    }

  syslog(LOG_INFO, "test seek success, offset %ld\n", (long)offset);
  filep->f_pos = offset;
  return offset;
}

static int testdev_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
  int ret = -1;

  switch (cmd)
    {
      case DIOC_SETKEY:
        if (arg != 0)
          {
            syslog(LOG_ERR, "!!!!!!!!!!!!!!!!!! "
                            "should not print this log "
                            "!!!!!!!!!!!!!!!!!!!!!\n");
            ret = -1;
          }
        break;
      case IOCTL:
        if (arg == 0)
          {
            syslog(LOG_ERR, "test ioctl fail, cmd %d(except %d), "
                   "whence 0(expect not 0)\n", cmd, IOCTL);
            ret = -1;
          }
        else
          {
           if (*(int *)arg == IOCTL_ARG)
             {
               ret = RET;
             }
           else
             {
               syslog(LOG_ERR, "test ioctl fail, cmd %d(except %d), "
                      "whence %d(expect %d)\n",
                      cmd, IOCTL, *(int *)arg, IOCTL_ARG);
               ret = -1;
             }
          }
        break;
      default:
        break;
    }

  syslog(LOG_INFO, "test ioctl success\n");
  return ret;
}

static int testdev_poll(FAR struct file *filep, FAR struct pollfd *fds,
                        bool setup)
{
  if (!setup)
    {
      syslog(LOG_INFO, "test poll teardown\n");
      return 0;
    }

  syslog(LOG_INFO, "test poll, events %lu\n", (unsigned long)fds->events);
  poll_notify(&fds, 1, fds->events);
  syslog(LOG_INFO, "test poll notify\n");
  return 0;
}

static void _register_driver(int mode, char *cpu, char *remotepath,
                             char *localpath)
{
  int ret = -EINVAL;

  switch (mode)
    {
      case 0:
        if (remotepath == NULL)
          {
            syslog(LOG_ERR, "please set -r\n");
            exit(1);
          }

        ret = register_driver(remotepath, &g_testdev_ops, 0666, NULL);
      break;
#ifdef CONFIG_DEV_RPMSG
      case 1:
        if (cpu == NULL || remotepath == NULL || localpath == NULL)
          {
            syslog(LOG_ERR, "please set -c, -r, -l\n");
            exit(1);
          }

        ret = rpmsgdev_register(cpu, remotepath, localpath, 0);
      break;
#endif
      case 2:
#ifdef CONFIG_DEV_RPMSG_SERVER
        if (cpu == NULL || localpath == NULL)
          {
            syslog(LOG_ERR, "please set -c, -l\n");
            exit(1);
          }

        ret = rpmsgdev_export(cpu, localpath);
#else
      syslog(LOG_WARNING, "feature of case %d not enabled\n", mode);
#endif
      break;
      default:
      syslog(LOG_ERR, "-d must between 0 and 2\n");
        exit(1);
      break;
    }

  if (ret < 0)
    {
      syslog(LOG_ERR, "register driver failed, ret=%d, errno=%d\n",
             ret, errno);
      exit(1);
    }

  syslog(LOG_INFO, "register driver success\n");
}
#endif

#if defined(CONFIG_DEV_RPMSG) || defined(TEST_RPMSGDEV)
static int test_open(char *path, int flags)
{
  int fd;

  fd = open(path, flags);
  if (fd <= 0)
    {
      syslog(LOG_ERR, "open %s fail, ret %d, errno %d\n", path, fd, errno);
      return -1;
    }

  return fd;
}

static int test_close(int fd)
{
  int ret;

  ret = close(fd);
  if (ret < 0)
    {
      syslog(LOG_ERR, "close fail, ret %d, errno %d\n", ret, errno);
    }

  syslog(LOG_INFO, "close success, ret %d\n", ret);
  return ret;
}

static int test_read(int fd, size_t len)
{
  ssize_t rd = 0;
  ssize_t ret;
  char *buf;

  buf = malloc(len);
  if (buf == NULL)
    {
      syslog(LOG_ERR, "read malloc fail\n");
      return -1;
    }

  while (rd < len)
    {
      ret = read(fd, buf + rd, len - rd);
      rd += ret;
      if (ret <= 0)
        {
          break;
        }
    }

  if (rd != len)
    {
      syslog(LOG_ERR, "read fail, len %zd, except %zu, errno %d\n",
             rd, len, errno);
      free(buf);
      return -1;
    }

  for (ret = 0; ret < len; ret++)
    {
      if (buf[ret] != 'r')
        {
          syslog(LOG_ERR, "check data error, ret[%d] %d, except 'r'\n",
                 ret, buf[ret]);
          free(buf);
          return -1;
        }
    }

  free(buf);
  return 0;
}

static int test_write(int fd, size_t len)
{
  ssize_t wt = 0;
  ssize_t ret;
  char *buf;

  buf = malloc(len);
  if (buf == NULL)
    {
      syslog(LOG_ERR, "read malloc fail\n");
      return -1;
    }

  for (ret = 0; ret < len; ret++)
    {
      buf[ret] = 'w';
    }

  while (wt < len)
    {
      ret = write(fd, buf + wt, len - wt);
      wt += ret;
      if (ret <= 0)
        {
          break;
        }
    }

  if (wt != len)
    {
      free(buf);
      syslog(LOG_ERR, "write fail, len %zd, except %zu, errno %d\n",
             wt, len, errno);
      return -1;
    }

  free(buf);
  return 0;
}

static off_t test_seek(int fd, off_t offset, int whence)
{
  off_t ret;

  ret = lseek(fd, offset, whence);
  if (ret == -1)
    {
      syslog(LOG_ERR, "seek fail, errno %d\n", errno);
    }

  syslog(LOG_INFO, "test seek return %ld\n", (long)ret);
  return ret;
}

#ifdef __NuttX__
static int test_ioctl(int fd, unsigned long request, unsigned long arg)
{
  int ret;

  ret = ioctl(fd, request, arg);
  if (ret == -1)
    {
      syslog(LOG_ERR, "ioctl fail, errno %d\n", errno);
    }

  syslog(LOG_INFO, "test ioctl return %d\n", ret);
  return ret;
}
#endif

static int test_poll(int fd, int event, int timeout)
{
  struct pollfd pfd;
  int ret;

  memset(&pfd, 0, sizeof(struct pollfd));

  pfd.fd = fd;
  pfd.events = event;

  ret = poll(&pfd, 1, timeout);
  if (ret < 0)
    {
      syslog(LOG_ERR, "fd %d poll failure: %d\n", fd, errno);
      return -1;
    }

  return ret;
}

/* open fail -> open success -> read 100 bytes -> close */

static int testcase_1(char *path)
{
  size_t len = 100;
  int fd;

  fd = open(path, O_RDONLY);
  if (fd != -1 || errno != RET)
    {
      syslog(LOG_ERR, "open test fail, fd %d (expect -1), "
             "errno %d (expect %d)\n", fd, errno, RET);
      return -1;
    }

  if ((fd = test_open(path, OPEN_FLAG)) <= 0)
    {
      return -1;
    }

  if (test_read(fd, len) != 0)
    {
      return -1;
    }

  return test_close(fd);
}

/* open -> write 100 bytes -> close */

static int testcase_2(char *path)
{
  size_t len = 100;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG)) <= 0)
    {
      return -1;
    }

  if (test_write(fd, len) != 0)
    {
      return -1;
    }

  return test_close(fd);
}

/* open -> seek fail -> seek success -> close */

static int testcase_3(char *path)
{
  int fd;

  if ((fd = test_open(path, OPEN_FLAG)) <= 0)
    {
      return -1;
    }

  if (test_seek(fd, 123, SEEK_SET) != -1)
    {
      return -1;
    }

  syslog(LOG_INFO, "seek1 success\n");

  if (test_seek(fd, OFFSET, WHENCE) != OFFSET)
    {
      return -1;
    }

  syslog(LOG_INFO, "seek2 success\n");
  return test_close(fd);
}

#ifdef __NuttX__
/* open -> ioctl fail -> ioctl success -> close */

static int testcase_4(char *path)
{
  int args = 0;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG)) <= 0)
    {
      return -1;
    }

  /* cmd not in rpmsgdev_ioctl_arglen */

  if (test_ioctl(fd, DIOC_SETKEY, IOCTL_ARG) != -1)
    {
      return -1;
    }

  syslog(LOG_INFO, "ioctl0 success\n");

  if (test_ioctl(fd, IOCTL, (unsigned long)&args) != -1)
    {
      return -1;
    }

  syslog(LOG_INFO, "ioctl1 success\n");

  args = IOCTL_ARG;
  if (test_ioctl(fd, IOCTL, (unsigned long)&args) != RET)
    {
      return -1;
    }

  syslog(LOG_INFO, "ioctl2 success\n");
  return test_close(fd);
}
#endif

/* open -> poll -> close */

static int testcase_5(char *path)
{
  int ret;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG)) <= 0)
    {
      return -1;
    }

  /* TODO : change poll notify in the threaad */

  ret = test_poll(fd, POLLIN, 200);
  if (ret != 1)
    {
      syslog(LOG_ERR, "poll test1 fail, ret %d, expect 1\n", ret);
      return -1;
    }

  syslog(LOG_INFO, "poll1 success\n");

  ret = test_poll(fd, POLLIN, 2000);
  if (ret != 1)
    {
      syslog(LOG_ERR, "poll test2 fail, ret %d, expect 1\n", ret);
      return -1;
    }

  syslog(LOG_INFO, "poll2 success\n");

#if 0
  ret = test_poll(fd, POLLOUT, 0);
  if (ret != 1)
    {
      syslog(LOG_ERR, "poll test3 fail, ret %d, expect 1\n", ret);
      return -1;
    }

  syslog(LOG_INFO, "poll3 success\n");
#endif

  return test_close(fd);
}

/* open -> read 20000 bytes -> close */

static int testcase_6(char *path)
{
  size_t len = 20000;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG | O_NONBLOCK)) <= 0)
    {
      return -1;
    }

  if (test_read(fd, len) != 0)
    {
      return -1;
    }

  return test_close(fd);
}

/* open -> write 20000 bytes -> close */

static int testcase_7(char *path)
{
  size_t len = 20000;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG | O_NONBLOCK)) <= 0)
    {
      return -1;
    }

  if (test_write(fd, len) != 0)
    {
      return -1;
    }

  return test_close(fd);
}

/* open -> write 10000 bytes -> read 10000 bytes -> .... -> close */

static int testcase_8(char *path)
{
  size_t len = 10000;
  int cnt = 0;
  struct timeval t;
  time_t start;
  time_t finish;
  int fd;

  if ((fd = test_open(path, OPEN_FLAG | O_NONBLOCK)) <= 0)
    {
      return -1;
    }

  gettimeofday(&t, NULL);
  start = t.tv_sec;
  finish = t.tv_sec;

  while ((finish - start) < hour * 3600)
    {
      cnt++;
      gettimeofday(&t, NULL);
      finish = t.tv_sec;

      if (test_write(fd, len) != 0)
        {
          close(fd);
          return -1;
        }

      if (test_read(fd, len) != 0)
        {
          close(fd);
          return -1;
        }

      if (cnt % 50000)
        {
          cnt = 0;
          syslog(LOG_INFO, "[%s] time start: %ld finish: %ld now: %ld\n",
                 path, (long)start, (long)(start + hour * 3600),
                 (long)finish);
        }
    }

  return test_close(fd);
}
#endif

static void show_usage(void)
{
  syslog(LOG_WARNING,
         "Usage: CMD [-d <regist driver>] [-c <remote cpu>] "
         "[-l <localpath>] [-r <remotepath>] [-h <hour>] [-t <test>]\n"
         "\t\t-d: regist driver\n"
         "\t\t\t 0 for test driver\n"
         "\t\t\t 1 for rpmsgdev(client register)\n"
         "\t\t\t 2 for rpmsgdev(server export)\n"
         "\t\t-c: remote cpu which regists test driver\n"
         "\t\t-l: localpath, which means rpmsgdev's name\n"
         "\t\t-r: remotepath, which means test driver's name\n"
         "\t\t-h: set stability test hours\n"
         "\t\t-t: set testcases\n"
         "\t\t\t 1 for open fail -> open success -> read 100 bytes -> "
                   "close\n"
         "\t\t\t 2 for open -> write 100 bytes -> close\n"
         "\t\t\t 3 for open -> seek fail -> seek success -> close\n"
         "\t\t\t 4 for open -> ioctl fail -> ioctl success -> close\n"
         "\t\t\t 5 for open -> poll -> close\n"
         "\t\t\t 6 for open -> write 20000 bytes -> close\n"
         "\t\t\t 7 for open -> read 20000 bytes -> close\n"
         "\t\t\t 8 read write stability test\n");

  exit(1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int o;
  int mode = -1;
  char *cpu = NULL;
  char *remotepath = NULL;
  char *localpath = NULL;
  int test __attribute__((unused)) = -1;
  int ret __attribute__((unused));

  if (argc <= 1)
    {
      show_usage();
    }

  hour = 1;

  while ((o = getopt(argc, argv, "d:c:l:r:t:h:")) != EOF)
    {
      switch (o)
        {
          case 'd':
            mode = atoi(optarg);
          break;
          case 'c':
            cpu = optarg;
          break;
          case 'l':
            localpath = optarg;
          break;
          case 'r':
            remotepath = optarg;
          break;
          case 't':
            test = atoi(optarg);
          break;
          case 'h':
            hour = atoi(optarg);
          break;
          default:
            show_usage();
          break;
      }
  }

  if (mode >= 0)
    {
#ifdef __NuttX__
      _register_driver(mode, cpu, remotepath, localpath);
#endif
    }

#if defined(CONFIG_DEV_RPMSG) || defined(TEST_RPMSGDEV)
  if (test >= 0)
    {
      if (localpath == NULL)
        {
          syslog(LOG_ERR, "please set -l\n");
          exit(1);
        }

      switch (test)
        {
          case 1:
            ret = testcase_1(localpath);
          break;
          case 2:
            ret = testcase_2(localpath);
          break;
          case 3:
            ret = testcase_3(localpath);
          break;
#ifdef __NuttX__
          case 4:
            ret = testcase_4(localpath);
          break;
#endif
          case 5:
            ret = testcase_5(localpath);
          break;
          case 6:
            ret = testcase_6(localpath);
          break;
          case 7:
            ret = testcase_7(localpath);
          break;
          case 8:
            ret = testcase_8(localpath);
          break;
          default:
            syslog(LOG_ERR, "-t out of range\n");
            exit(1);
          break;
      }

      if (ret == 0)
        {
          syslog(LOG_INFO, "TEST PASS\n");
        }
      else
        {
          syslog(LOG_INFO, "TEST FAIL, expect 0 (ret %d)\n", ret);
        }
  }
#endif

  return 0;
}
