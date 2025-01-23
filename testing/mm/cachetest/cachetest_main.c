/****************************************************************************
 * apps/testing/mm/cachetest/cachetest_main.c
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
#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CACHETEST_PREFIX "CACHE Test: "

#define OPTARG_TO_VALUE(value, type, base) \
  do \
  { \
    FAR char *ptr; \
    value = (type)strtoul(optarg, &ptr, base); \
    if (*ptr != '\0') \
      { \
        syslog(LOG_ERR, CACHETEST_PREFIX "Parameter error -%c %s\n", ch, \
               optarg); \
        cahcetest_show_usage(); \
      } \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct cachetest_s
{
  FAR char *wbuf;
  FAR char *rbuf;
  FAR char *waddr;
  uintptr_t offset;
  size_t size;
  int repeat_num;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cahcetest_show_usage
 ****************************************************************************/

static void cahcetest_show_usage(void)
{
  printf("\nUsage: %s -s [buffer-size] -f [offset] -n [repeat-number]\n",
         CONFIG_TESTING_CACHETEST_PROGNAME);
  printf("\nWhere:\n");
  printf("  -s [buffer-size] number of memory alloc for buffer"
         "(in bytes). [Default Size: 1MB] \n");
  printf("  -f [offset] offset to uncacheble address [Default: 0]\n");
  printf("  -n [repeat-number] number of repetitions"
         " [default value: unlimited].\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: cachetest_parse_commandline
 ****************************************************************************/

static void cachetest_parse_commandline(int argc, FAR char **argv,
                                        FAR struct cachetest_s *info)
{
  int ch;

  /* Default size: 1M */

  memset(info, 0, sizeof(struct cachetest_s));
  info->size = 1024 * 1024;

  while ((ch = getopt(argc, argv, "s::n::f::i")) != ERROR)
    {
      switch (ch)
        {
          case 's':
            OPTARG_TO_VALUE(info->size, size_t, 10);
            break;
          case 'n':
            OPTARG_TO_VALUE(info->repeat_num, int, 10);
            break;
          case 'f':
            OPTARG_TO_VALUE(info->offset, uintptr_t, 16);
            break;
          case '?':
            cahcetest_show_usage();
            break;
        }
    }

  info->wbuf = zalloc(info->size);
  if (info->wbuf == NULL)
    {
      syslog(LOG_ERR, CACHETEST_PREFIX "Alloc memory for info->wbuf"
             "failed:%d\n",
             ENOMEM);
      exit(EXIT_FAILURE);
    }

  info->rbuf = zalloc(info->size);
  if (info->rbuf == NULL)
    {
      free(info->wbuf);
      syslog(LOG_ERR, CACHETEST_PREFIX "Alloc memory for info->rbuf"
             "failed:%d\n",
             ENOMEM);
      exit(EXIT_FAILURE);
    }

  info->waddr = zalloc(info->size);
  if (info->waddr == NULL)
    {
      free(info->wbuf);
      free(info->rbuf);
      syslog(LOG_ERR, CACHETEST_PREFIX "Alloc memory for info->waddr"
             "failed:%d\n",
             ENOMEM);
      exit(EXIT_FAILURE);
    }

  syslog(LOG_INFO, CACHETEST_PREFIX "waddr:%p, uncacheble addr start:%p,"
         "size:%u\n", info->waddr,
         (FAR char *)((uintptr_t)info->waddr | info->offset), info->size);
}

/****************************************************************************
 * Name: cachetest_teardown
 ****************************************************************************/

static void cachetest_teardown(FAR struct cachetest_s *info)
{
  free(info->waddr);
  free(info->rbuf);
  free(info->wbuf);
}

/****************************************************************************
 * Name: cachetest_randchar
 ****************************************************************************/

static inline char cachetest_randchar(void)
{
  int value = random() % 63;
  if (value == 0)
    {
      return '0';
    }
  else if (value <= 10)
    {
      return value + '0' - 1;
    }
  else if (value <= 36)
    {
      return value + 'a' - 11;
    }
  else
    {
      return value + 'A' - 37;
    }
}

/****************************************************************************
 * Name: cachetest_randcontext
 ****************************************************************************/

static void cachetest_randcontext(uint32_t size, FAR void *input)
{
  /* Construct a buffer here and fill it with random characters */

  int i;
  FAR char *tmp;
  tmp = input;
  for (i = 0; i < size - 1; i++)
    {
      tmp[i] = cachetest_randchar();
    }

  tmp[i] = '\0';
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cachetest_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR char *ptr;
  struct cachetest_s info;

  /* Prepare parameters */

  cachetest_parse_commandline(argc, argv, &info);

  /* If there is no set number of times to repeat, it will loop continuously
   * by default.
   */

  for (int loop = 1; ((!info.repeat_num) || loop <= info.repeat_num);
       loop++)
    {
      cachetest_randcontext(info.size, info.wbuf);

      /* Get uncacheable address */

      ptr = (FAR char *)((uintptr_t)info.waddr | info.offset);

      memcpy(info.waddr, info.wbuf, info.size);

      up_flush_dcache((uintptr_t)info.waddr, (uintptr_t)info.waddr +
                      info.size);

      for (size_t i = 0; i < info.size; i++)
        {
          info.rbuf[i] = *ptr++;
        }

      if (memcmp(info.wbuf, info.rbuf, info.size) != 0)
        {
          syslog(LOG_ERR, CACHETEST_PREFIX "comparison failed!\n");
        }
      else
        {
          syslog(LOG_INFO, CACHETEST_PREFIX "comparsion success!\n");
        }

      /* To prevent tasks from being occupied all the time, switch
       * scheduling through usleep.
       */

      usleep(1);
    }

  cachetest_teardown(&info);

  return 0;
}
