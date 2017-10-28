/****************************************************************************
 * canutils/libobd2/obd_waitresponse.c
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
