/****************************************************************************
 * apps/examples/rmtchar/rmtchar_main.c
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

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <debug.h>

#include "rmtchar.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if !defined(CONFIG_EXAMPLES_RMTCHAR_RX) && \
    !defined(CONFIG_EXAMPLES_RMTCHAR_TX)
#  error "At least one of CONFIG_EXAMPLES_RMTCHAR_RX or \
          CONFIG_EXAMPLES_RMTCHAR_TX must be defined"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct rmtchar_state_s g_rmtchar;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rmtchar_devpath
 ****************************************************************************/

static void rmtchar_devpath(FAR struct rmtchar_state_s *rmtchar,
                            FAR const char *devpath,
                            bool is_tx)
{
  /* Get rid of any old device path */

  if (is_tx)
    {
      if (rmtchar->txdevpath)
        {
          free(rmtchar->txdevpath);
        }

      /* Then set-up the new device path by copying the string */

      rmtchar->txdevpath = strdup(devpath);
    }
  else
    {
      if (rmtchar->rxdevpath)
        {
          free(rmtchar->rxdevpath);
        }

      /* Then set-up the new device path by copying the string */

      rmtchar->rxdevpath = strdup(devpath);
    }
}

/****************************************************************************
 * Name: rmtchar_help
 ****************************************************************************/

static void rmtchar_help(FAR struct rmtchar_state_s *rmtchar)
{
  printf("Usage: rmtchar [OPTIONS]\n");
  printf("\nArguments are \"sticky\".\n");
  printf("For example, once the RMT character device is\n");
  printf("specified, that device will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-i items] selects the number of words (items) to be transmitted"
         " or received by the RMT character device. "
         "Default: %d Current: %d\n",
         CONFIG_EXAMPLES_RMTCHAR_ITEMS,
         rmtchar->rmtchar_items ? rmtchar->rmtchar_items :
         CONFIG_EXAMPLES_RMTCHAR_ITEMS);
#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  printf("  [-t devpath] selects the RMT transmitter character device path. "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_RMTCHAR_TX_DEVPATH,
         rmtchar->txdevpath ? rmtchar->txdevpath : "NONE");
#endif
#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
  printf("  [-r devpath] selects the RMT receiver character device path. "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_RMTCHAR_RX_DEVPATH,
         rmtchar->rxdevpath ? rmtchar->rxdevpath : "NONE");
#endif
  printf("  [-h] shows this message and exits\n");
}

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

static int arg_string(FAR char **arg, FAR char **value)
{
  FAR char *ptr = *arg;

  if (ptr[2] == '\0')
    {
      *value = arg[1];
      return 2;
    }
  else
    {
      *value = &ptr[2];
      return 1;
    }
}

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

static int arg_decimal(FAR char **arg, FAR int *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

static void parse_args(FAR struct rmtchar_state_s *rmtchar,
                       int argc,
                       FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  int value;
  int index;
  int nargs;

  for (index = 1; index < argc; )
    {
      ptr = argv[index];
      if (ptr[0] != '-')
        {
          printf("Invalid options format: %s\n", ptr);
          exit(0);
        }

      switch (ptr[1])
        {
          case 'i':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %d\n", value);
                exit(1);
              }

            rmtchar->rmtchar_items = value;
            index += nargs;
            break;
#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
          case 't':
            nargs = arg_string(&argv[index], &str);
            rmtchar_devpath(rmtchar, str, true);
            index += nargs;
            break;
#endif

#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
          case 'r':
            nargs = arg_string(&argv[index], &str);
            rmtchar_devpath(rmtchar, str, false);
            index += nargs;
            break;
#endif
          case 'h':
            rmtchar_help(rmtchar);
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            rmtchar_help(rmtchar);
            exit(1);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rmtchar_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  pthread_attr_t attr;
  pthread_addr_t result;
#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  pthread_t transmitter;
#endif
#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
  pthread_t receiver;
#endif
#if defined(CONFIG_EXAMPLES_RMTCHAR_RX) && defined(CONFIG_EXAMPLES_RMTCHAR_TX)
  struct sched_param param;
#endif
  int ret;

  UNUSED(ret);

  /* Check if we have initialized */

  if (!g_rmtchar.initialized)
    {
      /* Set the default values */

      g_rmtchar.rmtchar_items = CONFIG_EXAMPLES_RMTCHAR_ITEMS;

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
      rmtchar_devpath(&g_rmtchar, CONFIG_EXAMPLES_RMTCHAR_TX_DEVPATH, true);
#endif
#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
      rmtchar_devpath(&g_rmtchar, CONFIG_EXAMPLES_RMTCHAR_RX_DEVPATH, false);
#endif

      g_rmtchar.initialized = true;
    }

  /* Parse the command line */

  parse_args(&g_rmtchar, argc, argv);

#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
  /* Start the receiver thread */

  printf("rmtchar_main: Start receiver thread\n");
  pthread_attr_init(&attr);

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  /* Bump the receiver priority from the default so that it will be above
   * the priority of transmitter.  This is important if a loopback test is
   * being performed.
   */

  pthread_attr_getschedparam(&attr, &param);
  param.sched_priority++;
  pthread_attr_setschedparam(&attr, &param);
#endif

  /* Set the receiver stack size */

  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_RMTCHAR_RXSTACKSIZE);

  /* Start the receiver */

  ret = pthread_create(&receiver, &attr, rmtchar_receiver, &g_rmtchar);
  if (ret != OK)
    {
      printf("rmtchar_main: ERROR: failed to Start receiver thread: %d\n",
             ret);
      return EXIT_FAILURE;
    }

  pthread_setname_np(receiver, "receiver");
#endif

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  /* Start the transmitter thread */

  printf("rmtchar_main: Start transmitter thread\n");
  pthread_attr_init(&attr);

  /* Set the transmitter stack size */

  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_RMTCHAR_TXSTACKSIZE);

  /* Start the transmitter */

  ret = pthread_create(&transmitter, &attr, rmtchar_transmitter, &g_rmtchar);
  if (ret != OK)
    {
      printf("rmtchar_main: ERROR: failed to Start transmitter thread: %d\n",
             ret);
#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
      printf("rmtchar_main: Waiting for the receiver thread\n");
      pthread_join(receiver, &result);
#endif
      return EXIT_FAILURE;
    }

  pthread_setname_np(transmitter, "transmitter");
#endif

#ifdef CONFIG_EXAMPLES_RMTCHAR_TX
  printf("rmtchar_main: Waiting for the transmitter thread\n");
  ret = pthread_join(transmitter, &result);
  if (ret != OK)
    {
      printf("rmtchar_main: ERROR: pthread_join failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_EXAMPLES_RMTCHAR_RX
  printf("rmtchar_main: Waiting for the receiver thread\n");
  ret = pthread_join(receiver, &result);
  if (ret != OK)
    {
      printf("rmtchar_main: ERROR: pthread_join failed: %d\n", ret);
    }
#endif

  return EXIT_SUCCESS;
}
