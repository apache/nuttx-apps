/****************************************************************************
 * canutils/libobd2/obd_decodepid.c
 *
 *   Copyright (C) 2017 Alan Carvalho de Assis. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
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

#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <nuttx/can/can.h>

#include "canutils/obd.h"
#include "canutils/obd_pid.h"
#include "canutils/obd_frame.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXDATA 16

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_data[MAXDATA];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: obd_decode_pid
 *
 * Description:
 *   Decode the value returned for a determined PID.
 *
 *   It will return the data decode as text string or NULL if error.
 *
 ****************************************************************************/

FAR char *obd_decode_pid(FAR struct obd_dev_s *dev, uint8_t pid)
{
  uint32_t pids;
  int rpm;

  /* Verify if received data is valid */

  if (dev->data[2] != pid)
    {
      printf("Expecting PID %02x but received %02x!\n", pid, dev->data[2]);
      return NULL;
    }

  switch (dev->data[2])
    {
      case OBD_PID_SUPPORTED:
        pids = (dev->data[3] << 24) | (dev->data[4] << 16) | \
               (dev->data[5] << 8) | dev->data[6];
        snprintf(g_data, MAXDATA, "%08X", pids);
#ifdef CONFIG_DEBUG_INFO
        printf("Supported PIDs: %08X\n");
#endif
        break;

      case OBD_PID_ENGINE_TEMPERATURE:
        snprintf(g_data, MAXDATA, "%d", dev->data[3] - 40);
#ifdef CONFIG_DEBUG_INFO
        printf("Engine Temperature = %d\n", dev->data[3] - 40);
#endif
        break;

      case OBD_PID_RPM:
        rpm = ((256 * dev->data[3]) + dev->data[4])/4;
        snprintf(g_data, MAXDATA, "%d", rpm);
#ifdef CONFIG_DEBUG_INFO
        printf("RPM = %d\n", rpm);
#endif
        break;

      case OBD_PID_SPEED:
        snprintf(g_data, MAXDATA, "%d", dev->data[3]);
#ifdef CONFIG_DEBUG_INFO
        printf("SPEED = %d Km/h\n", dev->data[3]);
#endif
        break;

      case OBD_PID_THROTTLE_POSITION:
        snprintf(g_data, MAXDATA, "%d", (100 * dev->data[3])/255);
#ifdef CONFIG_DEBUG_INFO
        printf("Throttle position = %d\% \n", (100 * dev->data[3])/255);
#endif
        break;
    }

  return g_data;
}
