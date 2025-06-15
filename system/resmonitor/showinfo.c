/****************************************************************************
 * apps/system/resmonitor/showinfo.c
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

#include <malloc.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/time.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PATH    "/proc"
#define CPULOAD "cpuload"
#define HEAP    "heap"
#define STACK   "stack"
#define LOADAVG "loadavg"
#define GROUP   "group/status"
#define FILELEN 100

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct t_info
{
  unsigned long mem_total;
  unsigned long mem_used;
  unsigned long mem_maxused;
  unsigned long mem_free;
  unsigned long mem_largest;
  unsigned long mem_nused;
  unsigned long mem_nfree;
  char total_cpu[8];
};

struct p_info
{
  unsigned long stack_used;
  unsigned long heap_used;
  char cpu[8];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int go     = 1;
static int p_head = 1;
static int d_head = 1;

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
         "Usage: CMD [-i <interval>] [-p <pid>] [-a] [-d <dir>] [-l "
         "<logpath>] [-k <logpath>]\n"
         "\t\t-i: set refresh interval (s), default 1s\n"
         "\t\t-p: set pid to monitor, show stack, head and cpu used by pid\n"
         "\t\t-a: work only if -p is set, show stack, head and cpu used by "
         "pid group\n"
         "\t\t-d: print the diskinfo of the dir\n"
         "\t\t-l: the memory and cpu info output to logpath\n"
         "\t\t-k: the diskinfo output to logpath\n");
  exit(1);
}

static void get_cpu(int pid, char *buf)
{
  char filepath[FILELEN];
  int ret;
  if (pid <= 0)
    {
      ret = snprintf(filepath, FILELEN, "%s/%s", PATH, CPULOAD);
    }
  else
    {
      ret = snprintf(filepath, FILELEN, "%s/%d/%s", PATH, pid, LOADAVG);
    }

  if (ret < 0)
    {
      /* syslog(LOG_ERR, "snprintf error\n"); */

      snprintf(buf, 8, "%.1f%%", 0.0);
      return;
    }

  FILE *fp = fopen(filepath, "r");
  if (!fp)
    {
      snprintf(buf, 8, "%.1f%%", 0.0);
      return;
    }

  fgets(buf, 8, fp);

  /* sscanf(buf, "%f", &cpu); */

  buf[strlen(buf) - 1] = '\0';
  fclose(fp);
}

static void add_cpu(char *a, char *b, char *buf)
{
  if (a == NULL || b == NULL)
    {
      return;
    }

  float fa = 0;
  float fb = 0;
  sscanf(a, "%f", &fa);
  sscanf(b, "%f", &fb);
  float sum = fa + fb;
  int ret   = snprintf(buf, 8, "%.1f%%", sum);
  if (ret < 0)
    {
      syslog(LOG_ERR, "add_cpu error\n");
    }
}

static unsigned long get_heap(int pid)
{
  unsigned long heap = 0;
  char filepath[FILELEN];
  int ret = snprintf(filepath, FILELEN, "%s/%d/%s", PATH, pid, HEAP);
  if (ret < 0)
    {
      return heap;
    }

  FILE *fp = fopen(filepath, "r");
  if (!fp)
    {
      return heap;
    }

  char buf[25];
  while (fgets(buf, 25, fp) != NULL)
    {
      if (strstr(buf, "AllocSize"))
        {
          int idx = strcspn(buf, "0123456789");
          heap    = strtoul(&buf[idx], NULL, 10);
          break;
        }
    }

  fclose(fp);
  return heap;
}

static unsigned long get_stack(int pid)
{
  unsigned long stack = 0;
  char filepath[FILELEN];
  int ret = snprintf(filepath, FILELEN, "%s/%d/%s", PATH, pid, STACK);
  if (ret < 0)
    {
      return stack;
    }

  FILE *fp = fopen(filepath, "r");
  if (!fp)
    {
      return stack;
    }

  char buf[25];
  while (fgets(buf, 25, fp) != NULL)
    {
      if (strstr(buf, "StackUsed"))
        {
          int idx = strcspn(buf, "0123456789");
          stack   = strtoul(&buf[idx], NULL, 10);
          break;
        }
    }

  fclose(fp);
  return stack;
}

static int get_total_info(struct t_info *t_info)
{
  memset(t_info, 0, sizeof(struct t_info));
  struct mallinfo g_alloc_info = mallinfo();
  t_info->mem_total            = g_alloc_info.arena;
  t_info->mem_used             = g_alloc_info.uordblks;
  t_info->mem_free             = g_alloc_info.fordblks;
  t_info->mem_largest          = g_alloc_info.mxordblk;
  t_info->mem_nused            = g_alloc_info.aordblks;
  t_info->mem_nfree            = g_alloc_info.ordblks;
  t_info->mem_maxused          = g_alloc_info.usmblks;
  get_cpu(0, t_info->total_cpu);
  return 0;
}

static int get_pid_info(struct p_info *p_info, int pid, bool all)
{
  memset(p_info, 0, sizeof(struct p_info));
  if (!all)
    {
      get_cpu(pid, p_info->cpu);
      p_info->heap_used  = get_heap(pid);
      p_info->stack_used = get_stack(pid);
    }
  else
    {
      char filepath[FILELEN];
      int ret = snprintf(filepath, FILELEN, "%s/%d/%s", PATH, pid, GROUP);
      if (ret < 0)
        {
          return -1;
        }

      FILE *fp = fopen(filepath, "r");
      if (!fp)
        {
          return -1;
        }

      char buf[100];
      while (fgets(buf, 100, fp) != NULL)
        {
          if (strstr(buf, "Member IDs"))
            {
              int idx = strcspn(buf, ":");
              char *p = strtok(&buf[idx], " ");
              while ((p = strtok(NULL, " ")) != NULL)
                {
                  int gpid     = strtoul(p, NULL, 10);
                  char gcpu[8] = "0.0%%";
                  if (gpid > 0)
                    {
                      get_cpu(gpid, gcpu);
                    }

                  add_cpu(p_info->cpu, gcpu, p_info->cpu);
                  p_info->heap_used += get_heap(gpid);
                  p_info->stack_used += get_stack(gpid);
                }

              break;
            }
        }

      fclose(fp);
    }

  return 0;
}

static void
print_result(struct t_info *t_info, struct p_info *p_info, char *logpath)
{
  if (strlen(logpath) != 0)
    {
      time_t rawtime = 0;
      struct tm info;
      char date[30];
      time(&rawtime);
      localtime_r(&rawtime, &info);
      strftime(date, 30, "[%Y-%m-%d %H:%M:%S] ", &info);
      FILE *f;
      if ((f = fopen(logpath, "a+")) != NULL)
        {
          if (p_head == 1)
            {
              p_head = 0;
              if (p_info == NULL)
                {
                  fprintf(f,
                          "%22s%13s%11s%11s%11s%11s%7s%7s%7s\n",
                          "",
                          "total",
                          "used",
                          "free",
                          "maxused",
                          "largest",
                          "nused",
                          "nfree",
                          "cpu");
                }
              else
                {
                  fprintf(f,
                          "%22s%13s%11s%11s%11s%11s%7s%7s%7s%7s%11s%7s\n",
                          "",
                          "total",
                          "used",
                          "free",
                          "maxused",
                          "largest",
                          "nused",
                          "nfree",
                          "cpu",
                          "pstack",
                          "pheap",
                          "pcpu");
                }
            }

          if (p_info == NULL)
            {
              fprintf(f,
                      "%-22s%13lu%11lu%11lu%11lu%11lu%7lu%7lu%7s\n",
                      date,
                      t_info->mem_total,
                      t_info->mem_used,
                      t_info->mem_free,
                      t_info->mem_maxused,
                      t_info->mem_largest,
                      t_info->mem_nused,
                      t_info->mem_nfree,
                      t_info->total_cpu);
            }
          else
            {
              fprintf(
                  f,
                  "%-22s%13lu%11lu%11lu%11lu%11lu%7lu%7lu%7s%7lu%11lu%7s\n",
                  date,
                  t_info->mem_total,
                  t_info->mem_used,
                  t_info->mem_free,
                  t_info->mem_maxused,
                  t_info->mem_largest,
                  t_info->mem_nused,
                  t_info->mem_nfree,
                  t_info->total_cpu,
                  p_info->stack_used,
                  p_info->heap_used,
                  p_info->cpu);
            }

          fclose(f);
        }
      else
        {
          syslog(LOG_ERR, "fopen logpath %s error \n", logpath);
        }

      return;
    }

  if (p_info == NULL)
    {
      syslog(LOG_INFO,
             "%11s%11s%11s%11s%11s%7s%7s%7s\n",
             "total",
             "used",
             "free",
             "maxused",
             "largest",
             "nused",
             "nfree",
             "cpu");
      syslog(LOG_INFO,
             "%11lu%11lu%11lu%11lu%11lu%7lu%7lu%7s\n\n",
             t_info->mem_total,
             t_info->mem_used,
             t_info->mem_free,
             t_info->mem_maxused,
             t_info->mem_largest,
             t_info->mem_nused,
             t_info->mem_nfree,
             t_info->total_cpu);
    }
  else
    {
      syslog(LOG_INFO,
             "%11s%11s%11s%11s%11s%7s%7s%7s%7s%11s%7s\n",
             "total",
             "used",
             "free",
             "maxused",
             "largest",
             "nused",
             "nfree",
             "cpu",
             "pstack",
             "pheap",
             "pcpu");
      syslog(LOG_INFO,
             "%11lu%11lu%11lu%11lu%11lu%7lu%7lu%7s%7lu%11lu%7s\n\n",
             t_info->mem_total,
             t_info->mem_used,
             t_info->mem_free,
             t_info->mem_maxused,
             t_info->mem_largest,
             t_info->mem_nused,
             t_info->mem_nfree,
             t_info->total_cpu,
             p_info->stack_used,
             p_info->heap_used,
             p_info->cpu);
    }
}

static void transfer(uintmax_t value, char *res, int len)
{
  memset(res, 0, len);
  if (value > 1024 * 1024)
    {
      int ret = snprintf(res, len, "%ju", value / 1024 / 1024);
      if (ret < 0)
        {
          return;
        }

      strcat(res, "M");
    }
  else if (value > 1024)
    {
      int ret = snprintf(res, len, "%ju", value / 1024);
      if (ret < 0)
        {
          return;
        }

      strcat(res, "K");
    }
  else
    {
      int ret = snprintf(res, len, "%ju", value);
      if (ret < 0)
        {
          return;
        }

      strcat(res, "B");
    }
}

static void print_diskinfo(char *path, char *logpath)
{
  struct statfs diskinfo;
  int ret = statfs(path, &diskinfo);
  if (ret != 0)
    {
      syslog(LOG_ERR, "statfs fail!\n");
      return;
    }

  char c_tsize[20];
  char c_asize[20];
  char c_bsize[20];
  char c_fsize[20];
  uintmax_t bsize  = (uintmax_t)diskinfo.f_bsize;
  uintmax_t bavail = (uintmax_t)diskinfo.f_bavail;
  uintmax_t blocks = (uintmax_t)diskinfo.f_blocks;
  uintmax_t bfree  = (uintmax_t)diskinfo.f_bfree;
  transfer(bsize * blocks, c_tsize, 20);
  transfer(bsize * bavail, c_asize, 20);
  transfer(bsize * bfree, c_fsize, 20);
  transfer(bsize, c_bsize, 20);
  if (strlen(logpath) != 0)
    {
      time_t rawtime = 0;
      struct tm info;
      char date[30];
      time(&rawtime);
      localtime_r(&rawtime, &info);
      strftime(date, 30, "[%Y-%m-%d %H:%M:%S] ", &info);
      FILE *f;
      if ((f = fopen(logpath, "a+")) != NULL)
        {
          if (d_head == 1)
            {
              d_head = 0;
              fprintf(f,
                      "%22s%11s%11s%11s%11s%11s\n",
                      "",
                      "dir",
                      "total size",
                      "free size",
                      "avail size",
                      "block size");
            }

          fprintf(f,
                  "%-22s%11s%11s%11s%11s%11s\n",
                  date,
                  path,
                  c_tsize,
                  c_fsize,
                  c_asize,
                  c_bsize);
          fclose(f);
        }
      else
        {
          syslog(LOG_ERR, "fopen logpath %s error \n", logpath);
        }
    }
  else
    {
      syslog(LOG_INFO,
             "%11s%11s%11s%11s%11s\n",
             "dir",
             "total size",
             "free size",
             "avail size",
             "block size");
      syslog(LOG_INFO,
             "%11s%11s%11s%11s%11s\n\n",
             path,
             c_tsize,
             c_fsize,
             c_asize,
             c_bsize);
    }
}

int main(int argc, char *argv[])
{
  if (argc == 1)
    {
      show_usages();
    }

  char filepath[FILELEN];
  char logpath_l[FILELEN];
  char logpath_k[FILELEN];
  struct timeval sleeptime;
  struct t_info total_info;
  struct p_info pid_info;
  int interval = 1;
  int pid      = -1;
  bool all     = false;
  int ret;
  int o;

  memset(filepath, 0, FILELEN);
  memset(logpath_l, 0, FILELEN);
  memset(logpath_k, 0, FILELEN);
  go           = 1;
  p_head       = 1;
  d_head       = 1;
  while ((o = getopt(argc, argv, "i:p:ad:l:k:")) != EOF)
    {
      switch (o)
        {
          case 'i':
            interval = atoi(optarg);
            break;
          case 'p':
            pid = atoi(optarg);
            break;
          case 'a':
            all = true;
            break;
          case 'd':
            snprintf(filepath, FILELEN, "%s", optarg);
            break;
          case 'l':
            snprintf(logpath_l, FILELEN, "%s", optarg);
            break;
          case 'k':
            snprintf(logpath_k, FILELEN, "%s", optarg);
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
      ret = get_total_info(&total_info);
      if (ret < 0)
        {
          syslog(LOG_ERR, "get total info fail\n");
          break;
        }

      if (pid > 0)
        {
          ret = get_pid_info(&pid_info, pid, all);
          if (ret < 0)
            {
              syslog(LOG_ERR, "get pid info fail\n");
              break;
            }
        }

      if (all || pid > 0)
        {
          print_result(&total_info, &pid_info, logpath_l);
        }
      else
        {
          print_result(&total_info, NULL, logpath_l);
        }

      if (strlen(filepath) != 0)
        {
          print_diskinfo(filepath, logpath_k);
        }

      sleeptime.tv_sec  = interval;
      sleeptime.tv_usec = 0;
      select(0, NULL, NULL, NULL, &sleeptime);
    }

  syslog(LOG_INFO, "program complete!\n");
  return 0;
}
