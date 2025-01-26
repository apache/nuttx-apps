/****************************************************************************
 * apps/system/resmonitor/filldisk.c
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

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/statfs.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PATHSIZE 70
#define FILESIZE 100
#define FILENAME "testpayload"
#define INTERVAL 0

/****************************************************************************
 * Private Data
 ****************************************************************************/

static time_t interval = INTERVAL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int getdiv(uintmax_t count)
{
  int div = 1000;
  if (count > 1000)
    {
      div = 1000;
    }
  else if (count > 100)
    {
      div = 100;
    }
  else if (count > 10)
    {
      div = 10;
    }
  else
    {
      div = 1;
    }

  return div;
}

static uintmax_t
get_fillsize(char *filepath, int mode, uintmax_t remain, uintmax_t fill)
{
  struct statfs diskinfo;
  int ret = statfs(filepath, &diskinfo);
  if (ret != 0)
    {
      syslog(LOG_ERR, "statfs fail!\n");
      return -1;
    }

  uintmax_t bsize    = (uintmax_t)diskinfo.f_bsize;
  uintmax_t bavail   = (uintmax_t)diskinfo.f_bavail;
  uintmax_t t_size   = bsize * bavail;
  uintmax_t fillsize = 0;
  if (mode == 1)
    {
      fillsize = t_size - remain;
    }
  else if (mode == 2)
    {
      fillsize = (fill < t_size) ? fill : t_size;
    }

  return fillsize;
}

static clock_t fill_disk(char *file, uintmax_t bufsize, int mode,
                         uintmax_t remain, uintmax_t fill)
{
  FILE *fp;
  clock_t start, finish;
  struct timeval sleeptime;
  int ret;
  if ((fp = fopen(file, "a+")) == NULL)
    {
      syslog(LOG_ERR, "Fail to open file!\n");
      return -1;
    }

  uintmax_t fillsize = get_fillsize(file, mode, remain, fill);
  if (fillsize <= 0)
    {
      syslog(LOG_WARNING, "out of space !!\n");
      fclose(fp);
      return -1;
    }

  char *buf = (char *)malloc((size_t)bufsize);
  if (!buf)
    {
      syslog(LOG_ERR, "malloc fail, errno %d\n", errno);
      fclose(fp);
      return -1;
    }

  memset(buf, 'a', (size_t)bufsize);
  uintmax_t count = fillsize / bufsize;
  int div         = getdiv(count);
  start           = clock();
  while (fillsize > bufsize)
    {
      ret = fwrite(buf, 1, bufsize, fp);
      if (ret > 0)
        {
          fillsize -= ret;
          count--;
        }
      else
        {
          syslog(LOG_ERR, "write fail \n");
          fclose(fp);
          free(buf);
          return -1;
        }

      sleeptime.tv_sec  = interval / 1000000;
      sleeptime.tv_usec = interval % 1000000;
      select(0, NULL, NULL, NULL, &sleeptime);
      if (count % div == 0)
        {
          syslog(LOG_INFO, "remain %ju B\n", fillsize);
          fillsize = get_fillsize(file, mode, remain, fillsize);
          if (fillsize < 0)
            {
              syslog(LOG_WARNING, "out of space !!\n");
              fclose(fp);
              free(buf);
              return -1;
            }

          count = fillsize / bufsize;
          div   = getdiv(count);
        }
    }

  fwrite(buf, 1, fillsize, fp);
  fflush(fp);
  finish = clock();
  syslog(LOG_INFO, "write complete !!!\n");
  fclose(fp);
  free(buf);
  return finish - start;
}

static void show_usages(void)
{
  syslog(LOG_WARNING,
        "Usage: CMD [-d <dir>] [-f <fill size>] [-r <remain size>] [-b "
        "<write buf size>] [-n <file nums>] [-i <write interval (us)>]\n"
        "\t\t-d: set dir to fill (e.g. /data)\n"
        "\t\t-f: set fill file size, can end with KkMm, (disabled if -r is "
        "set)\n"
        "\t\t-n: works only if -f is set, create n files of size -f set, "
        "default 1\n"
        "\t\t-r: set remain size, can end with KkMm\n"
        "\t\t-b: set write buf size, can end with kKMm, default is f_bsize\n"
        "\t\t-i: set write intervals (us), default 0 us, set only if the "
        "program killed by wdt\n");
  exit(1);
}

static void double2str(double f, char *s, int len)
{
  int ret = snprintf(s, len, "%.2lf", f);
  if (ret < 0)
    {
      syslog(LOG_ERR, "double to str fail, ret %d\n", ret);
    }
}

static uintmax_t bytes(char *s)
{
  uintmax_t n;
  if (sscanf(s, "%ju", &n) < 1)
    {
      return 0;
    }

  if ((s[strlen(s) - 1] == 'k') || (s[strlen(s) - 1] == 'K'))
    {
      n *= 1024;
    }

  if ((s[strlen(s) - 1] == 'm') || (s[strlen(s) - 1] == 'M'))
    {
      n *= (1024 * 1024);
    }

  return n;
}

static void print_disk_info(struct statfs *diskinfo)
{
  syslog(LOG_INFO, "\tfs block size : %zu\n", diskinfo->f_bsize);
  syslog(LOG_INFO,
         "\tfs block nums : %" PRIu64 "\n",
         (uint64_t)diskinfo->f_blocks);
  syslog(LOG_INFO,
         "\tfs free blocks : %" PRIu64 "\n",
         (uint64_t)diskinfo->f_bfree);
  syslog(LOG_INFO,
         "\tfs free blocks available : %" PRIu64 "\n",
         (uint64_t)diskinfo->f_bavail);
  syslog(LOG_INFO,
         "\tfs total file nodes : %" PRIu64 "\n",
         (uint64_t)diskinfo->f_files);
  syslog(LOG_INFO,
         "\tfs free file nodes : %" PRIu64 "\n",
         (uint64_t)diskinfo->f_ffree);
}

int main(int argc, FAR char *argv[])
{
  if (argc == 1)
    {
      show_usages();
      return -1;
    }

  char filepath[PATHSIZE];
  char sduration_t[15];
  char sduration_w[15];
  char sspeed_t[15];
  char sspeed_w[15];
  uintmax_t fill    = 0;
  uintmax_t remain  = 0;
  uintmax_t bufsize = 0;
  int nf            = 1;
  int mode          = 0;
  clock_t write_t   = 0;
  struct statfs diskinfo_old;
  struct statfs diskinfo_new;
  uintmax_t fillsize;
  double duration_t;
  double speed_t;
  double duration_w;
  double speed_w;
  clock_t start;
  clock_t finish;
  clock_t tmp;
  int ret;
  int o;

  interval = INTERVAL;
  memset(filepath, 0, PATHSIZE);
  while ((o = getopt(argc, argv, "d:f:r:b:i:n:")) != EOF)
    {
      switch (o)
        {
          case 'd':
            snprintf(filepath, PATHSIZE, "%s", optarg);
            break;
          case 'f':
            fill = bytes(optarg);
            if (mode != 1)
              {
                mode = 2;
              }

            break;
          case 'r':
            remain = bytes(optarg);
            mode   = 1;
            break;
          case 'i':
            interval = (time_t)bytes(optarg);
            break;
          case 'b':
            bufsize = bytes(optarg);
            break;
          case 'n':
            nf = atoi(optarg);
            break;
          default:
            show_usages();
            break;
        }
    }

  if (strlen(filepath) == 0)
    {
      syslog(LOG_ERR, "please set dir \n");
      return -1;
    }

  if (mode == 0)
    {
      syslog(LOG_ERR, "please set -f or -r \n");
      return -1;
    }
  else if (mode != 2)
    {
      nf = 1;
    }

  ret = statfs(filepath, &diskinfo_old);
  if (ret != 0)
    {
      syslog(LOG_ERR, "statfs fail!\n");
      return -1;
    }

  if (bufsize == 0)
    {
      bufsize = diskinfo_old.f_bsize;
    }

  fillsize = get_fillsize(filepath, mode, remain, fill) * nf;
  syslog(LOG_INFO, "outputfilepath: %s\n", filepath);
  start = clock();
  for (int i = 1; i <= nf; i++)
    {
      syslog(LOG_INFO, "create file %d, total %d\n", i, nf);
      char file[FILESIZE];
      ret = snprintf(file, FILESIZE, "%s/%s%d", filepath, FILENAME, i);
      if (ret < 0)
        {
          syslog(LOG_ERR, "snprintf err, ret %d\n", ret);
          return -1;
        }

      if ((tmp = fill_disk(file, bufsize, mode, remain, fill)) > 0)
        {
          write_t += tmp;
        }
      else
        {
          syslog(LOG_ERR, "fill disk error\n");
          return -1;
        }
    }

  finish = clock();
  ret    = statfs(filepath, &diskinfo_new);
  if (ret != 0)
    {
      syslog(LOG_ERR, "statfs fail!\n");
      return -1;
    }

  syslog(LOG_INFO, "***diskinfo before filldisk***\n");
  print_disk_info(&diskinfo_old);

  syslog(LOG_INFO, "***diskinfo after filldisk***\n");
  print_disk_info(&diskinfo_new);
  duration_t = (double)(finish - start) / CLOCKS_PER_SEC;
  speed_t    = fillsize / 1024.0 / duration_t;
  duration_w = (double)(write_t) / CLOCKS_PER_SEC;
  speed_w    = fillsize / 1024.0 / duration_w;
  double2str(speed_t, sspeed_t, 15);
  double2str(speed_w, sspeed_w, 15);
  double2str(duration_t, sduration_t, 15);
  double2str(duration_w, sduration_w, 15);
  syslog(LOG_INFO, "fill buf size: %ju\n", bufsize);
  syslog(LOG_INFO, "total fill size: %ju\n", fillsize);
  syslog(LOG_INFO,
         "write speed (include open and close) %s KB/s, duration %s s\n",
         sspeed_t,
         sduration_t);
  syslog(LOG_INFO,
         "write speed (only write) %s KB/s, duration %s s\n",
         sspeed_w,
         sduration_w);
  return 0;
}
