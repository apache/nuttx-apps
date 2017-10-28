/****************************************************************************
 * canutils/libobd2/obd_sendrequest.c
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
