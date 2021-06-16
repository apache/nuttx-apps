/****************************************************************************
 * apps/canutils/libobd2/obd_decodepid.c
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
