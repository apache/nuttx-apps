/****************************************************************************
 * apps/system/resmonitor/fillcpu.c
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

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PATH    "/proc"
#define CPULOAD "cpuload"
#define LOADAVG "loadavg"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int go = 1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void handler(int sig)
{
  go = 0;
}

static void show_usages(void)
{
  syslog(LOG_WARNING,
        "Usage: CMD [-c <cpu>] [-f <filepath>] [-a]\n"
        "\t\t-c: set cpu occupation that you except, default 80\n"
        "\t\t-f: set write payload path, eg. /tmp/payload, program will "
        "write to memory if -f not set\n"
        "\t\t-a: the cpu (set by -c) is the cpu occupied by this program\n");
  exit(1);
}

static float get_cpu(int pid)
{
  float cpu = 0;
  char filepath[20];
  int ret;
  if (pid <= 0)
    {
      ret = snprintf(filepath, 20, "%s/%s", PATH, CPULOAD);
    }
  else
    {
      ret = snprintf(filepath, 20, "%s/%d/%s", PATH, pid, LOADAVG);
    }

  if (ret < 0)
    {
      /* syslog(LOG_ERR, "snprintf error\n"); */

      return cpu;
    }

  FILE *fp = fopen(filepath, "r");
  if (!fp)
    {
      return cpu;
    }

  char buf[8];
  fgets(buf, 8, fp);
  sscanf(buf, "%f", &cpu);
  fclose(fp);
  return cpu;
}

static int writefile(char *filepath, char *buffer1, char *buffer2)
{
  if (strlen(filepath) == 0)
    {
      memset(buffer2, '*', 1024);
      memcpy(buffer1, buffer2, 1024);
      return 0;
    }
  else
    {
      int fd;
      memset(buffer1, '*', 1024);
      if ((fd = open(filepath, O_WRONLY | O_CREAT, 0700)) <= 0)
        {
          syslog(LOG_ERR, "open file error\n");
          return -1;
        }

      if (write(fd, buffer1, 1024) <= 0)
        {
          syslog(LOG_ERR, "write file error\n");
          close(fd);
          return -1;
        }

      return close(fd);
    }
}

int main(int argc, char *argv[])
{
  char buf1[1024];
  char buf2[1024];
  char filepath[40];
  struct timeval sleeptime;
  int n        = 0;
  int lowcount = 0;
  int cpu      = 80;
  bool ispid   = false;
  int time     = 10000;
  float fcpu;
  int o;

  memset(filepath, 0, 40);
  go       = 1;
  if (argc == 1)
    {
      show_usages();
    }

  while ((o = getopt(argc, argv, "c:f:a")) != EOF)
    {
      switch (o)
        {
          case 'c':
            cpu = atoi(optarg);
            break;
          case 'f':
            snprintf(filepath, 40, "%s", optarg);
            break;
          case 'a':
            ispid = true;
            break;
          default:
            show_usages();
            break;
        }
    }

  signal(SIGINT, handler);
  signal(SIGKILL, handler);

  while (go)
    {
      if (time < 1000)
        {
          time = 1000;
          lowcount++;
          if (lowcount > 4)
            {
              lowcount = 0;
              n += 2;
            }
        }

      else if (time > 10000)
        {
          time = 10000;
          n -= 1;
          n = (n < 0 ? 0 : n);
        }
      else
        {
          lowcount = 0;
        }

      sleeptime.tv_sec  = 0;
      sleeptime.tv_usec = time;
      select(0, NULL, NULL, NULL, &sleeptime);
      if (ispid)
        {
          fcpu = get_cpu(getpid());
        }
      else
        {
          fcpu = get_cpu(0);
        }

      if (fcpu > cpu)
        {
          time += 1000;
        }
      else
        {
          time -= 1000;
        }

      for (int i = 0; i < n; i++)
        {
          if (writefile(filepath, buf1, buf2) != 0)
            {
              break;
            }
        }
    }

  syslog(LOG_INFO, "program complete!\n");
  return 0;
}
