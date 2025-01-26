/****************************************************************************
 * apps/system/resmonitor/fillmem.c
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

#include <errno.h>
#include <getopt.h>
#include <malloc.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PAYLOADSIZE 100

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
         "Usage: CMD [-f <fill size>] [-r <remain size>]\n"
         "\t\t-f: set fill mem size, can end with KkMm, (disabled if -r "
         "is set)\n"
         "\t\t-r: set remain size, can end with KkMm\n"
         "\t\t-p: set percentage occupancy\n");
  exit(1);
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

int main(int argc, char *argv[])
{
  struct mallinfo g_alloc_info;
  size_t fill_size      = 0;
  size_t remain_size    = 0;
  size_t free_mem       = 0;
  size_t largest_mem    = 0;
  size_t total_mem      = 0;
  size_t used_mem       = 0;
  float fill_percentage = 0;
  char *payloads[PAYLOADSIZE];
  int idx = 0;
  go      = 1;
  struct timeval sleeptime;
  memset(payloads, 0, sizeof(payloads));
  int mode = 0;
  if (argc == 1)
    {
      show_usages();
    }

  int o;
  while ((o = getopt(argc, argv, "r:f:p:")) != EOF)
    {
      switch (o)
        {
          case 'r':
            remain_size = (size_t)bytes(optarg);
            mode        = 1;
            break;
          case 'f':
            fill_size = (size_t)bytes(optarg);
            break;
          case 'p':
            fill_percentage = atof(optarg);
            mode            = 2;
            break;
          default:
            show_usages();
            break;
        }
    }

  g_alloc_info = mallinfo();
  total_mem    = (size_t)g_alloc_info.arena;
  free_mem     = (size_t)g_alloc_info.fordblks;
  largest_mem  = (size_t)g_alloc_info.mxordblk;
  used_mem     = (size_t)g_alloc_info.uordblks;
  if (mode == 2)
    {
      size_t per_fill = (size_t)total_mem * fill_percentage;
      syslog(LOG_INFO, "per fill is %zu\n", per_fill);
      fill_size = per_fill - used_mem;
      if (fill_size < 0)
        {
          syslog(LOG_WARNING,
                 "memory has used %zu, has exceed %f\n",
                 used_mem,
                 fill_percentage);
          return 0;
        }

      mode = 0;
    }

  if (remain_size != 0 || mode == 1)
    {
      if (remain_size > free_mem)
        {
          syslog(LOG_INFO,
                 "remain is greater than free, remain %zu, free %zu\n",
                 remain_size,
                 free_mem);
          return 0;
        }

      fill_size = free_mem - remain_size;
    }

  if (fill_size > free_mem)
    {
      syslog(LOG_INFO,
             "no enough mem, fill %zu, free %zu\n",
             fill_size,
             free_mem);
      return 0;
    }

  syslog(LOG_INFO,
         "fill size %zu, free %zu, largest %zu\n",
         fill_size,
         free_mem,
         largest_mem);
  if (fill_size <= largest_mem)
    {
      payloads[0] = malloc(fill_size - 100);
      if (payloads[0] == NULL)
        {
          syslog(LOG_ERR, "malloc fail, errno %d\n", errno);
          goto END;
        }

      syslog(LOG_INFO, "malloc size %zu\n", fill_size);
    }
  else
    {
      while ((fill_size > largest_mem) && (idx < PAYLOADSIZE - 1))
        {
          if (largest_mem <= 150)
            {
              break;
            }

          payloads[idx] = malloc(largest_mem - 100);
          if (payloads[idx] == NULL)
            {
              syslog(LOG_ERR, "malloc fail, errno %d\n", errno);
              goto END;
            }

          fill_size -= largest_mem;
          syslog(LOG_INFO, "idx: %d,  malloc size %zu\n", idx, largest_mem);
          g_alloc_info = mallinfo();
          largest_mem  = (size_t)g_alloc_info.mxordblk;
          idx++;
        }

      if (largest_mem > 100)
        {
          if (largest_mem - fill_size <= 100)
            {
              fill_size = largest_mem - 100;
            }

          payloads[idx] = malloc(fill_size);
          if (payloads[idx] == NULL)
            {
              syslog(LOG_ERR, "malloc fail, errno %d\n", errno);
              goto END;
            }

          syslog(LOG_INFO, "idx: %d,  malloc size %zu\n", idx, fill_size);
        }
    }

  signal(SIGINT, handler);
  signal(SIGKILL, handler);
  while (go)
    {
      sleeptime.tv_sec  = 600;
      sleeptime.tv_usec = 0;
      select(0, NULL, NULL, NULL, &sleeptime);
    }

  syslog(LOG_INFO, "program complete!\n");
END:
  for (int i = 0; i < PAYLOADSIZE; i++)
    {
      if (payloads[i] != NULL)
        {
          free(payloads[i]);
        }
    }

  return 0;
}
