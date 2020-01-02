/****************************************************************************
 * apps/examples/modbusmaster/mbmaster_main.c
 *
 * Copyright (c) 2016 Vytautas Lukenskas <lukevyta@gmail.com>
 * Copyright (c) 2019 Alan Carvalho de Assis <acassis@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbport.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* modbus master port */

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_PORT
#  define MBMASTER_PORT CONFIG_EXAMPLES_MODBUSMASTER_PORT
#else
#  define MBMASTER_PORT 1             /* port, 1 -- /dev/ttyS1 */
#endif

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_BAUDRATE
#  define MBMASTER_BAUD CONFIG_EXAMPLES_MODBUSMASTER_BAUDRATE
#else
#  define MBMASTER_BAUD B115200       /* baudrate, 115200kbps */
#endif

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_PARITY
#  define MBMASTER_PARITY CONFIG_EXAMPLES_MODBUSMASTER_PARITY
#else
#  define MBMASTER_PARITY MB_PAR_EVEN /* parity, even */
#endif

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_SLAVEADDR
#  define SLAVE_ID CONFIG_EXAMPLES_MODBUSMASTER_SLAVEADDR
#else
#  define SLAVE_ID 1                  /* slave address */
#endif

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_SLAVEREG
#  define SLAVE_STATUS_REG CONFIG_EXAMPLES_MODBUSMASTER_SLAVEREG
#else
#  define SLAVE_STATUS_REG 1          /* slave status register part1 */
#endif

#ifdef CONFIG_EXAMPLES_MODBUSMASTER_TESTCOUT
#  define MBMASTER_REQUESTS_COUNT CONFIG_EXAMPLES_MODBUSMASTER_TESTCOUT
#else
#  define MBMASTER_REQUESTS_COUNT 100 /* number of requests to send  */
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum mbmaster_threadcmd_e
{
  STOPPED = 0,
  RUN,
  SHUTDOWN
};

struct mbmaster_statistics_s
{
  int reqcount;
  int rspcount;
  int errcount;
};

struct mbmaster_run_s
{
  pthread_t pollthreadid;
  enum mbmaster_threadcmd_e threadcmd;
  struct mbmaster_statistics_s statistics;
};

/****************************************************************************
 * Private Global Variables
 ****************************************************************************/

static struct mbmaster_run_s g_mbmaster;

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * Name: mbmaster_initialize
 *
 * Description:
 *   Should initialize modbus stack.
 *
 ****************************************************************************/

static inline int mbmaster_initialize(void)
{
  eMBErrorCode mberr;

  /* Initialize the FreeModBus library (master) */

  mberr = eMBMasterInit(MB_RTU, MBMASTER_PORT,
                        MBMASTER_BAUD, MBMASTER_PARITY);
  if (mberr != MB_ENOERR)
    {
      fprintf(stderr, "mbmaster_main: "
              "ERROR: eMBMasterInit failed: %d\n", mberr);
      return ENODEV;
    }

  /* Enable FreeModBus Master */

  mberr = eMBMasterEnable();
  if (mberr != MB_ENOERR)
    {
      fprintf(stderr, "mbmaster_main: "
              "ERROR: eMBMasterEnable failed: %d\n", mberr);
      return ENODEV;
    }

  return OK;
}

/****************************************************************************
 * Name: mbmaster_deinitialize
 *
 * Description:
 *   Should release resources taken by modbus stack.
 *
 ****************************************************************************/

static inline void mbmaster_deinitialize(void)
{
  /* Disable modbus stack */

  eMBMasterDisable();

  /* Release hardware resources */

  eMBMasterClose();
}

/****************************************************************************
 * Name: mbmaster_pollthread
 *
 * Description:
 *   This is the ModBus Master polling thread.
 *
 ****************************************************************************/

static FAR void *mbmaster_pollthread(FAR void *pvarg)
{
  eMBErrorCode mberr;

  do
    {
      /* Poll */

      mberr = eMBMasterPoll();
      if (mberr != MB_ENOERR)
        {
          break;
        }
    }
  while (g_mbmaster.threadcmd != SHUTDOWN);

  /* Free/uninitialize data structures */

  g_mbmaster.threadcmd = STOPPED;
  printf("mbmaster_main: "
         "Exiting poll thread.\n");
  return NULL;
}

/****************************************************************************
 * Name: mbmaster_showstatistics
 *
 * Description:
 *   Show final connection statistics
 *
 ****************************************************************************/

static void mbmaster_showstatistics(void)
{
  printf("Modbus master statistics: \n");
  printf("Requests count:  %d\n", g_mbmaster.statistics.reqcount);
  printf("Responses count: %d\n", g_mbmaster.statistics.rspcount);
  printf("Errors count:    %d\n", g_mbmaster.statistics.errcount);
}

/****************************************************************************
 * Name: eMBMasterRegHoldingCB
 *
 * Description:
 *   Required FreeModBus callback function
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegHoldingCB(FAR uint8_t *buffer,
                                   uint16_t address, uint16_t nregs,
                                   eMBRegisterMode mode)
{
  eMBErrorCode mberr = MB_ENOERR;

  switch (mode)
    {
    case MB_REG_READ:
      g_mbmaster.statistics.rspcount++;
      break;

    case MB_REG_WRITE:
      printf("mbmaster_main: writing holding register, value: %d\n",
             *(buffer));
      break;
    }

  return mberr;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main/mbmaster_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  eMBMasterReqErrCode mberr;
  int reqcounter = 0;
  int ret;

  g_mbmaster.statistics.reqcount = 0;
  g_mbmaster.statistics.rspcount = 0;
  g_mbmaster.statistics.errcount = 0;
  g_mbmaster.threadcmd = STOPPED;
  reqcounter = 0;

  /* Initialize modbus stack, etc */

  printf("Initializing modbus master...\n");
  ret = mbmaster_initialize();

  if (ret != OK)
    {
      fprintf(stderr, "mbmaster_main: ",
              "ERROR: mbmaster_initialize failed: %d\n", ret);
      goto errout;
    }

  printf("Creating poll thread.\n");
  g_mbmaster.threadcmd = RUN;
  ret = pthread_create(&g_mbmaster.pollthreadid, NULL,
                       mbmaster_pollthread, NULL);

  if (ret != OK)
    {
      fprintf(stderr, "mbmaster_main: ",
              "ERROR: mbmaster_pollthread create failed: %d\n", ret);
      goto errout_with_initialize;
    }

  printf("Sending %d requests to slave %d\n",
         MBMASTER_REQUESTS_COUNT, SLAVE_ID);

  /* modbus is initialized and polling thread is running */

  while (reqcounter < MBMASTER_REQUESTS_COUNT)
    {
      g_mbmaster.statistics.reqcount++;
      mberr = eMBMasterReqReadHoldingRegister(SLAVE_ID,
                                              SLAVE_STATUS_REG, 1, -1);
      if (mberr != MB_MRE_NO_ERR)
        {
          fprintf(stderr, "mbmaster_main: "
                  "ERROR: holding reg %d read failed: %d\n",
                  SLAVE_STATUS_REG, mberr);

          g_mbmaster.statistics.errcount++;
        }

      reqcounter++;
    }

  /* Stop polling thread */

  g_mbmaster.threadcmd = SHUTDOWN;
  pthread_join(g_mbmaster.pollthreadid, NULL);
  g_mbmaster.threadcmd = STOPPED;

  /* Display connection statistics */

  mbmaster_showstatistics();

  printf("Deinitializing modbus master...\n");
  mbmaster_deinitialize();

  return OK;

errout_with_initialize:

  /* Release hardware resources */

  mbmaster_deinitialize();

errout:
  exit(EXIT_FAILURE);
}
