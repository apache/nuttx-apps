/****************************************************************************
 * apps/canutils/libobd2/obd_waitresponse.c
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
#include <string.h>
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
 * Name: obd_wait_response
 *
 * Description:
 *   Wait for a message from ECUs with requested PID that was sent using
 *   obd_send_request().
 *
 *   It will return an error case it doesn't receive the msg after the elapsed
 *   "timeout" time.
 *
 ****************************************************************************/

int obd_wait_response(FAR struct obd_dev_s *dev, uint8_t opmode, uint8_t pid,
                      int timeout)
{
  int i;
  int nbytes;
  int msgdlc;
  int msgsize;
  uint8_t extended;

#ifdef CONFIG_DEBUG_INFO
  printf("Waiting Response for pid=%d\n", pid);
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

  for (; ; )
    {
      /* Read the RX message */

      msgsize = sizeof(struct can_msg_s);
      nbytes = read(dev->can_fd, &dev->can_rxmsg, msgsize);
      if (nbytes < CAN_MSGLEN(0) || nbytes > msgsize)
        {
          printf("ERROR: read(%ld) returned %ld\n",
                 (long)msgsize, (long)nbytes);
          return -EAGAIN;
        }

      #ifdef CONFIG_DEBUG_INFO
        printf("  ID: %4u DLC: %u\n",
               dev->can_rxmsg.cm_hdr.ch_id, dev->can_rxmsg.cm_hdr.ch_dlc);
      #endif

      msgdlc = dev->can_rxmsg.cm_hdr.ch_dlc;

#ifdef CONFIG_DEBUG_INFO
      printf("Data received:\n");
      for (i = 0; i < msgdlc; i++)
        {
          printf("  %d: 0x%02x\n", i, dev->can_rxmsg.cm_data[i]);
        }

      fflush(stdout);
#endif

      /* Check if we received a Response Message */

      if ((extended && dev->can_rxmsg.cm_hdr.ch_id == OBD_PID_EXT_RESPONSE) || \
          (!extended && dev->can_rxmsg.cm_hdr.ch_id == OBD_PID_STD_RESPONSE))
        {

          /* Check if the Response if for the PID we are interested! */

          if (dev->can_rxmsg.cm_data[1] == (opmode + OBD_RESP_BASE) && \
              dev->can_rxmsg.cm_data[2] == pid)
            {
              /* Save received data (important for multi-frame mode) */

              memcpy(dev->data, dev->can_rxmsg.cm_data, msgdlc);
              return OK;
            }
        }

      /* Verify if we timed out */

      timeout--;
      if (timeout < 0)
        {
          printf("Timeout trying to receive PID %d\n", pid);
          return -ETIMEDOUT;
        }

      /* Wait 10ms */

      usleep(10000);
    }

  /* Never should come here */

  return OK;
}
