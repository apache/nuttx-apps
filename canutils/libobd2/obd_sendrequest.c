/****************************************************************************
 * apps/canutils/libobd2/obd_sendrequest.c
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
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: obd_sent_request
 *
 * Description:
 *   Send a "Request Message" to ECUs with requested PID.
 *
 *   It will return an error case the message fails to be sent.
 *
 ****************************************************************************/

int obd_send_request(FAR struct obd_dev_s *dev, uint8_t opmode, uint8_t pid)
{
  int nbytes;
  int msgdlc;
  int msgsize;
  uint8_t extended;

#ifdef CONFIG_DEBUG_INFO
  printf("Going SendRequest opmode=%d pid=%d\n", opmode, pid);
#endif

  /* Verify what is the current mode */

  if (dev->can_mode == CAN_EXT)
    {
      extended = 1;
    }
  else
    {
      extended = 0;
    }

  /* Define the CAN Data Length */

  msgdlc = 8;

  /* Construct the TX message header */

  if (extended)
    {
      dev->can_txmsg.cm_hdr.ch_id     = OBD_PID_STD_REQUEST; /* MSG ID for PID Request */
    }
  else
    {
#ifdef CONFIG_CAN_EXTID
      dev->can_txmsg.cm_hdr.ch_id     = OBD_PID_EXT_REQUEST; /* MSG ID for PID Request */
#endif
    }

  dev->can_txmsg.cm_hdr.ch_rtr    = false;               /* Not a Remote Frame     */
  dev->can_txmsg.cm_hdr.ch_dlc    = msgdlc;              /* Data length is 8 bytes */
#ifdef CONFIG_CAN_EXTID
  dev->can_txmsg.cm_hdr.ch_extid  = extended;            /* Standard/Extend mode   */
#endif
  dev->can_txmsg.cm_hdr.ch_unused = 0;                   /* Unused                 */

  /* Single Frame with two bytes data */

  dev->can_txmsg.cm_data[0] = OBD_SINGLE_FRAME | OBD_SF_DATA_LEN(2);

  /* Setup the Operation Mode */

  dev->can_txmsg.cm_data[1] = opmode;

  /* Setup the PID we are requesting */

  dev->can_txmsg.cm_data[2] = pid;

  /* Padding */

  dev->can_txmsg.cm_data[3] = 0;
  dev->can_txmsg.cm_data[4] = 0;
  dev->can_txmsg.cm_data[5] = 0;
  dev->can_txmsg.cm_data[6] = 0;
  dev->can_txmsg.cm_data[7] = 0;

  /* Send the TX message */

  msgsize = CAN_MSGLEN(msgdlc);
  nbytes = write(dev->can_fd, &dev->can_txmsg, msgsize);
  if (nbytes != msgsize)
    {
      printf("ERROR: write(%ld) returned %ld\n",
             (long)msgsize, (long)nbytes);
      return -EAGAIN;
    }

#ifdef CONFIG_DEBUG_INFO
  printf("PID Request sent correctly!\n");
  fflush(stdout);
#endif

  return OK;
}
