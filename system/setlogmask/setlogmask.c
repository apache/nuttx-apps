/****************************************************************************
 * apps/system/setlogmask/setlogmask.c
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

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <nuttx/syslog/syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

#ifdef CONFIG_SYSLOG_IOCTL
static int print_channels(void)
{
  struct syslog_channel_info_s info[CONFIG_SYSLOG_MAX_CHANNELS];
  int ret;
  int fd;
  int i;

  fd = open("/dev/log", O_WRONLY);
  if (fd < 0)
    {
      perror("Failed to open /dev/log");
      return EXIT_FAILURE;
    }

  memset(info, 0, sizeof(info));
  ret = ioctl(fd, SYSLOGIOC_GETCHANNELS, &info);
  close(fd);
  if (ret < 0)
    {
      perror("Failed to get channels");
      return EXIT_FAILURE;
    }

  printf("Channels:\n");
  for (i = 0; i < CONFIG_SYSLOG_MAX_CHANNELS; i++)
    {
      if (info[i].sc_name[0] == '\0')
        {
          break;
        }

      printf("  %s: %s\n", info[i].sc_name,
             info[i].sc_disable ? "disable" : "enable");
    }

  return ret;
}

static int disable_channel(FAR const char *name, bool disable)
{
  struct syslog_channel_info_s info;
  int ret;
  int fd;

  fd = open("/dev/log", O_WRONLY);
  if (fd < 0)
    {
      perror("Failed to open /dev/log");
      return EXIT_FAILURE;
    }

  info.sc_disable = disable;
  strlcpy(info.sc_name, name, sizeof(info.sc_name));
  ret = ioctl(fd, SYSLOGIOC_SETFILTER, (unsigned long)&info);
  if (ret < 0)
    {
      perror("Failed to set filter");
    }

  close(fd);
  return ret;
}
#endif

static void show_usage(FAR const char *progname, int exitcode)
{
  printf("\nUsage: %s <d|i|n|w|e|c|a|r>\n", progname);
#ifdef CONFIG_SYSLOG_IOCTL
  printf("       %s list\n", progname);
  printf("       %s <enable/disable> <channel>\n", progname);
#endif
  printf("       %s -h\n", progname);
  printf("\nWhere:\n");
  printf("  d=DEBUG\n");
  printf("  i=INFO\n");
  printf("  n=NOTICE\n");
  printf("  w=WARNING\n");
  printf("  e=ERROR\n");
  printf("  c=CRITICAL\n");
  printf("  a=ALERT\n");
  printf("  r=EMERG\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  if (argc < 2)
    {
      show_usage(argv[0], EXIT_FAILURE);
    }

#ifdef CONFIG_SYSLOG_IOCTL
  if (strcmp(argv[1], "list") == 0)
    {
      print_channels();
      return EXIT_SUCCESS;
    }
  else if (argc == 3)
    {
      if (strcmp(argv[1], "enable") == 0)
        {
          return disable_channel(argv[2], false);
        }
      else if (strcmp(argv[1], "disable") == 0)
        {
          return disable_channel(argv[2], true);
        }
      else
        {
          show_usage(argv[0], EXIT_FAILURE);
        }
    }
#endif

  switch (*argv[1])
    {
      case 'd':
        {
          setlogmask(LOG_UPTO(LOG_DEBUG));
        }
        break;
      case 'i':
        {
          setlogmask(LOG_UPTO(LOG_INFO));
        }
        break;
      case 'n':
        {
          setlogmask(LOG_UPTO(LOG_NOTICE));
        }
        break;
      case 'w':
        {
          setlogmask(LOG_UPTO(LOG_WARNING));
        }
        break;
      case 'e':
        {
          setlogmask(LOG_UPTO(LOG_ERR));
        }
        break;
      case 'c':
        {
          setlogmask(LOG_UPTO(LOG_CRIT));
        }
        break;
      case 'a':
        {
          setlogmask(LOG_UPTO(LOG_ALERT));
        }
        break;
      case 'r':
        {
          setlogmask(LOG_UPTO(LOG_EMERG));
        }
        break;
      default:
        {
          show_usage(argv[0], EXIT_FAILURE);
        }
        break;
    }

  return EXIT_SUCCESS;
}
