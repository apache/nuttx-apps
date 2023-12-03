/****************************************************************************
 * apps/examples/modbus/modbus_main.c
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
 ****************************************************************************
 * Leveraged from:
 *
 *   FreeModbus Library: Linux Demo Application
 *   Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "modbus/mb.h"
#include "modbus/mbport.h"

#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  include <nuttx/leds/userled.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_EXAMPLES_MODBUS_PORT
#  define CONFIG_EXAMPLES_MODBUS_PORT 0
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_BAUD
#  define CONFIG_EXAMPLES_MODBUS_BAUD B38400
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_PARITY
#  define CONFIG_EXAMPLES_MODBUS_PARITY MB_PAR_EVEN
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_REG_INPUT_START
#  define CONFIG_EXAMPLES_MODBUS_REG_INPUT_START 1000
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_REG_INPUT_NREGS
#  define CONFIG_EXAMPLES_MODBUS_REG_INPUT_NREGS 4
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START
#  define CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START 2000
#endif

#ifndef CONFIG_EXAMPLES_MODBUS_REG_HOLDING_NREGS
#  define CONFIG_EXAMPLES_MODBUS_REG_HOLDING_NREGS 130
#endif

#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
#  define CONFIG_USERLEDS_DEVPATH "/dev/userleds"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum modbus_threadstate_e
{
  STOPPED = 0,
  RUNNING,
  SHUTDOWN
};

struct modbus_state_s
{
  enum modbus_threadstate_e threadstate;
  uint16_t reginput[CONFIG_EXAMPLES_MODBUS_REG_INPUT_NREGS];
  uint16_t regholding[CONFIG_EXAMPLES_MODBUS_REG_HOLDING_NREGS];
  uint16_t regcoils[CONFIG_EXAMPLES_MODBUS_REG_COILS_NREGS];
#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
  int fd_leds;
#endif
  pthread_t threadid;
  pthread_mutex_t lock;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static inline int modbus_initialize(void);
static void *modbus_pollthread(void *pvarg);
static inline int modbus_create_pollthread(void);
static void modbus_showusage(FAR const char *progname, int exitcode);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct modbus_state_s g_modbus;
static const uint8_t g_slaveid[] =
{
  0xaa, 0xbb, 0xcc
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modbus_initialize
 *
 * Description:
 *   Called from the ModBus polling thread in order to initialized the
 *   FreeModBus interface.
 *
 ****************************************************************************/

static inline int modbus_initialize(void)
{
  eMBErrorCode mberr;
  int status;

  /* Verify that we are in the stopped state */

  if (g_modbus.threadstate == RUNNING)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: Bad state: %d\n", g_modbus.threadstate);
      return EINVAL;
    }

  /* Initialize the ModBus demo data structures */

  status = pthread_mutex_init(&g_modbus.lock, NULL);
  if (status != 0)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: pthread_mutex_init failed: %d\n",  status);
      return status;
    }

  status = ENODEV;

  /* Initialize the FreeModBus library.
   *
   * MB_RTU                        = RTU mode
   * 0x0a                          = Slave address
   * CONFIG_EXAMPLES_MODBUS_PORT   = port, default=0 (i.e., /dev/ttyS0)
   * CONFIG_EXAMPLES_MODBUS_BAUD   = baud, default=B38400
   * CONFIG_EXAMPLES_MODBUS_PARITY = parity, default=MB_PAR_EVEN
   */

  mberr = eMBInit(MB_RTU, 0x0a, CONFIG_EXAMPLES_MODBUS_PORT,
                  CONFIG_EXAMPLES_MODBUS_BAUD,
                  CONFIG_EXAMPLES_MODBUS_PARITY);
  if (mberr != MB_ENOERR)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: eMBInit failed: %d\n", mberr);
      goto errout_with_mutex;
    }

  /* Set the slave ID
   *
   * 0x34        = Slave ID
   * true        = Is running (run indicator status = 0xff)
   * g_slaveid   = Additional values to be returned with the slave ID
   * 3           = Length of additional values (in bytes)
   */

  mberr = eMBSetSlaveID(0x34, true, g_slaveid, 3);
  if (mberr != MB_ENOERR)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: eMBSetSlaveID failed: %d\n", mberr);
      goto errout_with_modbus;
    }

  /* Enable FreeModBus */

  mberr = eMBEnable();
  if (mberr != MB_ENOERR)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: eMBEnable failed: %d\n", mberr);
      goto errout_with_modbus;
    }

  /* Successfully initialized */

  g_modbus.threadstate = RUNNING;
  return OK;

errout_with_modbus:

  /* Release hardware resources. */

  eMBClose();

errout_with_mutex:

  /* Free/uninitialize data structures */

  pthread_mutex_destroy(&g_modbus.lock);

  g_modbus.threadstate = STOPPED;
  return status;
}

/****************************************************************************
 * Name: modbus_pollthread
 *
 * Description:
 *   This is the ModBus polling thread.
 *
 ****************************************************************************/

static void *modbus_pollthread(void *pvarg)
{
  eMBErrorCode mberr;
  int ret;
#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
  int i;
  userled_set_t ledbit;
  userled_set_t ledset = 0;
  userled_set_t supported;
#endif

  /* Initialize the modbus */

  ret = modbus_initialize();
  if (ret != OK)
    {
      fprintf(stderr, "modbus_main: "
              "ERROR: modbus_initialize failed: %d\n", ret);
      return NULL;
    }

  srand(time(NULL));

  /* Open the USERLED device */

#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
  printf("Opening %s\n", CONFIG_USERLEDS_DEVPATH);
  g_modbus.fd_leds = open(CONFIG_USERLEDS_DEVPATH, O_WRONLY);
  if (g_modbus.fd_leds < 0)
    {
      int errcode = errno;
      printf("ERROR: Failed to open %s: %d\n",
             CONFIG_USERLEDS_DEVPATH, errcode);
      goto stop_modbus_exit;
    }

  /* Get the set of LEDs supported */

  ret = ioctl(g_modbus.fd_leds, ULEDIOC_SUPPORTED,
              (unsigned long)((uintptr_t)&supported));
  if (ret < 0)
    {
      int errcode = errno;
      printf("ERROR: ioctl(ULEDIOC_SUPPORTED) failed: %d\n",
             errcode);
      close(g_modbus.fd_leds);
      goto stop_modbus_exit;
    }
#endif

  /* Then loop until we are commanded to shutdown */

  do
    {
      /* Poll */

      mberr = eMBPoll();
      if (mberr != MB_ENOERR)
        {
           break;
        }

      /* Generate some random input */

      g_modbus.reginput[0] = (uint16_t)rand();

      /* If Reg Coils controls USERLEDs, update it! */

#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
      ledset = 0;

      for (i = 0; i < CONFIG_EXAMPLES_MODBUS_REG_COILS_NREGS && i < 32; i++)
        {
          ledbit = g_modbus.regcoils[i] << i;
          ledset |= ledbit & supported;
        }

      ret = ioctl(g_modbus.fd_leds, ULEDIOC_SETALL, ledset);
      if (ret < 0)
        {
          int errcode = errno;
          printf("ERROR: ioctl(ULEDIOC_SUPPORTED) failed: %d\n",
                 errcode);
          close(g_modbus.fd_leds);
          goto stop_modbus_exit;
        }
#endif
    }
  while (g_modbus.threadstate != SHUTDOWN);

#ifdef CONFIG_EXAMPLES_MODBUS_REG_COILS_USERLEDS
stop_modbus_exit:
#endif

  /* Disable */

  eMBDisable();

  /* Release hardware resources. */

  eMBClose();

  /* Free/uninitialize data structures */

  pthread_mutex_destroy(&g_modbus.lock);
  g_modbus.threadstate = STOPPED;
  return NULL;
}

/****************************************************************************
 * Name: modbus_create_pollthread
 *
 * Description:
 *   Start the ModBus polling thread
 *
 ****************************************************************************/

static inline int modbus_create_pollthread(void)
{
  int ret;

  if (g_modbus.threadstate != RUNNING)
    {
      ret = pthread_create(&g_modbus.threadid, NULL,
                           modbus_pollthread, NULL);
    }
    else
    {
      ret = EINVAL;
    }

  return ret;
}

/****************************************************************************
 * Name: modbus_showusage
 *
 * Description:
 *   Show usage of the demo program and exit
 *
 ****************************************************************************/

static void modbus_showusage(FAR const char *progname, int exitcode)
{
  printf("USAGE: %s [-d|e|s|q|h]\n\n", progname);
  printf("Where:\n");
  printf("  -d : Disable protocol stack\n");
  printf("  -e : Enable the protocol stack\n");
  printf("  -s : Show current status\n");
  printf("  -q : Quit application\n");
  printf("  -h : Show this information\n");
  printf("\n");
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: modbus_main
 *
 * Description:
 *   This is the main entry point to the demo program
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  bool quit = true;
  int option;
  int ret;

  /* Handle command line arguments */

  while ((option = getopt(argc, argv, "desqh")) != ERROR)
    {
      switch (option)
        {
          case 'd': /* Disable protocol stack */
            g_modbus.threadstate = SHUTDOWN;
            break;

          case 'e': /* Enable the protocol stack */
            {
              /* Keep running, otherwise the thread will die */

              quit = false;

              ret = modbus_create_pollthread();
              if (ret != OK)
                {
                  fprintf(stderr, "modbus_main: "
                          "ERROR: modbus_create_pollthread failed: %d\n",
                          ret);
                  exit(EXIT_FAILURE);
                }
            }
            break;

          case 's': /* Show current status */
            switch (g_modbus.threadstate)
              {
                case RUNNING:
                  printf("modbus_main: Protocol stack is running\n");
                  break;

                case STOPPED:
                  printf("modbus_main: Protocol stack is stopped\n");
                  break;

                case SHUTDOWN:
                  printf("modbus_main: Protocol stack is shutting down\n");
                  break;

                default:
                  fprintf(stderr, "modbus_main: "
                          "ERROR: Invalid thread state: %d\n",
                          g_modbus.threadstate);
                  break;
              }
            break;

          case 'q': /* Quit application */
            pthread_kill(g_modbus.threadid, 9);
            break;

          case 'h': /* Show help info */
            modbus_showusage(argv[0], EXIT_SUCCESS);
            break;

          default:
            fprintf(stderr, "modbus_main: "
                    "ERROR: Unrecognized option: '%c'\n", option);
            modbus_showusage(argv[0], EXIT_FAILURE);
            break;
        }
    }

  /* Don't exit until the thread finishes */

  if (!quit)
    {
      pthread_join(g_modbus.threadid, NULL);
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * Name: eMBRegInputCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegInputCB(uint8_t *buffer, uint16_t address, uint16_t nregs)
{
  eMBErrorCode mberr = MB_ENOERR;
  int          index;

  if ((address >= CONFIG_EXAMPLES_MODBUS_REG_INPUT_START) &&
      (address + nregs <=
      CONFIG_EXAMPLES_MODBUS_REG_INPUT_START +
      CONFIG_EXAMPLES_MODBUS_REG_INPUT_NREGS))
    {
      index = (int)(address - CONFIG_EXAMPLES_MODBUS_REG_INPUT_START);
      while (nregs > 0)
        {
          *buffer++ = (uint8_t)(g_modbus.reginput[index] & 0xff);
          *buffer++ = (uint8_t)(g_modbus.reginput[index] >> 8);
          index++;
          nregs--;
        }
    }
  else
    {
      mberr = MB_ENOREG;
    }

  return mberr;
}

/****************************************************************************
 * Name: eMBRegHoldingCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegHoldingCB(uint8_t *buffer, uint16_t address,
                             uint16_t nregs, eMBRegisterMode mode)
{
  eMBErrorCode    mberr = MB_ENOERR;
  int             index;

  if ((address >= CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START) &&
      (address + nregs <=
       CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START +
       CONFIG_EXAMPLES_MODBUS_REG_HOLDING_NREGS))
    {
      index = (int)(address - CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START);
      switch (mode)
        {
          /* Pass current register values to the protocol stack. */

          case MB_REG_READ:
            while (nregs > 0)
              {
                *buffer++ = (uint8_t)(g_modbus.regholding[index] & 0xff);
                *buffer++ = (uint8_t)(g_modbus.regholding[index] >> 8);
                index++;
                nregs--;
              }
            break;

          /* Update current register values with new values from the
           * protocol stack.
           */

          case MB_REG_WRITE:
            while (nregs > 0)
              {
                g_modbus.regholding[index] = *buffer++;
                g_modbus.regholding[index] |= *buffer++ << 8;
                index++;
                nregs--;
              }
            break;
        }
    }
  else
    {
      mberr = MB_ENOREG;
    }

  return mberr;
}

/****************************************************************************
 * Name: eMBRegCoilsCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegCoilsCB(uint8_t *buffer, uint16_t address,
                           uint16_t ncoils, eMBRegisterMode mode)
{
  eMBErrorCode    mberr = MB_ENOERR;
  int             index;

  if ((address >= CONFIG_EXAMPLES_MODBUS_REG_COILS_START) &&
      (address + ncoils <=
       CONFIG_EXAMPLES_MODBUS_REG_COILS_START +
       CONFIG_EXAMPLES_MODBUS_REG_COILS_NREGS))
    {
      index = (int)(address - CONFIG_EXAMPLES_MODBUS_REG_COILS_START);
      switch (mode)
        {
          /* Pass current register values to the protocol stack. */

          case MB_REG_READ:
            while (ncoils > 0)
              {
                *buffer++ = (uint8_t)(g_modbus.regcoils[index] & 0xff);
                *buffer++ = (uint8_t)(g_modbus.regcoils[index] >> 8);
                index++;
                ncoils--;
              }
            break;

          /* Update current register values with new values from the
           * protocol stack.
           */

          case MB_REG_WRITE:
            while (ncoils > 0)
              {
                g_modbus.regcoils[index] = *buffer++;
                g_modbus.regcoils[index] |= *buffer++ << 8;
                index++;
                ncoils--;
              }
            break;
        }
    }
  else
    {
      mberr = MB_ENOREG;
    }

  return mberr;
}

/****************************************************************************
 * Name: eMBRegDiscreteCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBRegDiscreteCB(uint8_t *buffer, uint16_t address,
                              uint16_t ndiscrete)
{
  return MB_ENOREG;
}
