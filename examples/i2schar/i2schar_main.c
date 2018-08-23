/****************************************************************************
 * examples/i2schar/i2schar_main.c
 *
 *   Copyright (C) 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include "i2schar.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct i2schar_state_s g_i2schar;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2schar_devpath
 ****************************************************************************/

static void i2schar_devpath(FAR struct i2schar_state_s *i2schar,
                            FAR const char *devpath)
{
  /* Get rid of any old device path */

  if (i2schar->devpath)
    {
      free(i2schar->devpath);
    }

  /* Then set-up the new device path by copying the string */

  i2schar->devpath = strdup(devpath);
}

/****************************************************************************
 * Name: i2schar_help
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void i2schar_help(FAR struct i2schar_state_s *i2schar)
{
  printf("Usage: i2schar [OPTIONS]\n");
  printf("\nArguments are \"sticky\".  For example, once the I2C character device is\n");
  printf("specified, that device will be re-used until it is changed.\n");
  printf("\n\"sticky\" OPTIONS include:\n");
  printf("  [-p devpath] selects the I2C character device path.  "
         "Default: %s Current: %s\n",
         CONFIG_EXAMPLES_I2SCHAR_DEVPATH, g_i2schar.devpath ? g_i2schar.devpath : "NONE");
#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
  printf("  [-t count] selects the number of audio buffers to send.  "
         "Default: %d Current: %d\n",
         CONFIG_EXAMPLES_I2SCHAR_TXBUFFERS, i2schar->txcount);
#endif
#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
  printf("  [-r count] selects the number of audio buffers to receive.  "
         "Default: %d Current: %d\n",
         CONFIG_EXAMPLES_I2SCHAR_RXBUFFERS, i2schar->txcount);
#endif
  printf("  [-h] shows this message and exits\n");
}
#endif

/****************************************************************************
 * Name: arg_string
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
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
#endif

/****************************************************************************
 * Name: arg_decimal
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static int arg_decimal(FAR char **arg, FAR long *value)
{
  FAR char *string;
  int ret;

  ret = arg_string(arg, &string);
  *value = strtol(string, NULL, 10);
  return ret;
}
#endif

/****************************************************************************
 * Name: parse_args
 ****************************************************************************/

#ifdef CONFIG_NSH_BUILTIN_APPS
static void parse_args(FAR struct i2schar_state_s *i2schar, int argc, FAR char **argv)
{
  FAR char *ptr;
  FAR char *str;
  long value;
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
          case 'p':
            nargs = arg_string(&argv[index], &str);
            i2schar_devpath(i2schar, str);
            index += nargs;
            break;

          case 'r':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            i2schar->rxcount = (uint32_t)value;
            index += nargs;
            break;

          case 't':
            nargs = arg_decimal(&argv[index], &value);
            if (value < 0)
              {
                printf("Count must be non-negative: %ld\n", value);
                exit(1);
              }

            i2schar->txcount = (uint32_t)value;
            index += nargs;
            break;

          case 'h':
            i2schar_help(i2schar);
            exit(0);

          default:
            printf("Unsupported option: %s\n", ptr);
            i2schar_help(i2schar);
            exit(1);
        }
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i2schar_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int i2schar_main(int argc, char *argv[])
#endif
{
  pthread_attr_t attr;
  pthread_addr_t result;
#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
  pthread_t transmitter;
#endif
#ifdef CONFIG_EXAMPLES_I2SCHAR_RX
  pthread_t receiver;
#endif
#if defined(CONFIG_EXAMPLES_I2SCHAR_RX) & defined(CONFIG_EXAMPLES_I2SCHAR_TX)
  struct sched_param param;
#endif
  int ret;

  /* Check if we have initialized */

  if (!g_i2schar.initialized)
    {
#ifdef CONFIG_EXAMPLES_I2SCHAR_DEVINIT
      /* Initialization of the I2C character device is performed by logic
       * external to this test.
       */

      printf("i2schar_main: Initializing external I2C character device\n");
      ret = i2schar_devinit();
      if (ret != OK)
        {
          printf("i2schar_main: i2schar_devinit failed: %d\n", ret);
          return EXIT_FAILURE;
        }
#endif

      /* Set the default values */

      i2schar_devpath(&g_i2schar, CONFIG_EXAMPLES_I2SCHAR_DEVPATH);

#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
      g_i2schar.txcount = CONFIG_EXAMPLES_I2SCHAR_TXBUFFERS;
#else
      g_i2schar.rxcount = CONFIG_EXAMPLES_I2SCHAR_RXBUFFERS;
#endif

      g_i2schar.initialized = true;
    }

  /* Parse the command line */

#ifdef CONFIG_NSH_BUILTIN_APPS
  parse_args(&g_i2schar, argc, argv);
#endif

  sched_lock();
#ifdef CONFIG_EXAMPLES_I2SCHAR_RX
  /* Start the receiver thread */

  printf("i2schar_main: Start receiver thread\n");
  pthread_attr_init(&attr);

#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
  /* Bump the receiver priority from the default so that it will be above
   * the priority of transmitter.  This is important if a loopback test is
   * being performed; it improves the changes that a receiving audio buffer
   * is in place for each transmission.
    */

  (void)pthread_attr_getschedparam(&attr, &param);
  param.sched_priority++;
  (void)pthread_attr_setschedparam(&attr, &param);
#endif

  /* Set the receiver stack size */

  (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_I2SCHAR_RXSTACKSIZE);

  /* Start the receiver */

  ret = pthread_create(&receiver, &attr, i2schar_receiver, NULL);
  if (ret != OK)
    {
      sched_unlock();
      printf("i2schar_main: ERROR: failed to Start receiver thread: %d\n", ret);
      return EXIT_FAILURE;
    }

   pthread_setname_np(receiver, "receiver");
#endif

#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
  /* Start the transmitter thread */

  printf("i2schar_main: Start transmitter thread\n");
  pthread_attr_init(&attr);

  /* Set the transmitter stack size */

  (void)pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_I2SCHAR_TXSTACKSIZE);

  /* Start the transmitter */

  ret = pthread_create(&transmitter, &attr, i2schar_transmitter, NULL);
  if (ret != OK)
    {
      sched_unlock();
      printf("i2schar_main: ERROR: failed to Start transmitter thread: %d\n", ret);
#ifdef CONFIG_EXAMPLES_I2SCHAR_RX
      printf("i2schar_main: Waiting for the receiver thread\n");
      (void)pthread_join(receiver, &result);
#endif
      return EXIT_FAILURE;
    }

   pthread_setname_np(transmitter, "transmitter");
#endif

   sched_unlock();
#ifdef CONFIG_EXAMPLES_I2SCHAR_TX
   printf("i2schar_main: Waiting for the transmitter thread\n");
   ret = pthread_join(transmitter, &result);
   if (ret != OK)
     {
       printf("i2schar_main: ERROR: pthread_join failed: %d\n", ret);
     }
#endif

#ifdef CONFIG_EXAMPLES_I2SCHAR_RX
   printf("i2schar_main: Waiting for the receiver thread\n");
   ret = pthread_join(receiver, &result);
   if (ret != OK)
     {
       printf("i2schar_main: ERROR: pthread_join failed: %d\n", ret);
     }
#endif

  return EXIT_SUCCESS;
}
