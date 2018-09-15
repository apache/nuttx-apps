/****************************************************************************
 * include/canutils/obd.h
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

#ifndef __APPS_INCLUDE_CANUTILS_OBD_H
#define __APPS_INCLUDE_CANUTILS_OBD_H 1

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

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* CAN Modes */

enum
{
  CAN_STD = 0,
  CAN_EXT,
};

/* OBD-II structure */

struct obd_dev_s
{
  struct  can_msg_s can_txmsg;       /* TX Message                          */
  struct  can_msg_s can_rxmsg;       /* RX Message                          */
  struct  canioc_bittiming_s can_bt; /* Current bitrate                     */
  uint8_t can_mode;                  /* Current mode (Standard or Extended) */
  int     can_fd;                    /* File Descriptor of CAN Device       */
#ifdef CONFIG_MULTIFRAME_SUPPORT
  uint8_t data[4096];                /* Up to 4096 bytes                    */
#else
  uint8_t data[8];                   /* Single Frame = 8 bytes              */
#endif
};

/****************************************************************************
 * Name: obd_init
 *
 * Description:
 *   Initialize the OBD-II with initial baudrate
 *
 *   Returns a obd_dev_s with initial values or NULL if error.
 *
 ****************************************************************************/

FAR struct obd_dev_s *obd_init(FAR char *devfile, int baudate, int mode);

/****************************************************************************
 * Name: obd_sent_request
 *
 * Description:
 *   Send a "Request Message" to ECUs with requested PID.
 *
 *   It will return an error case the message fails to be sent.
 *
 ****************************************************************************/

int obd_send_request(FAR struct obd_dev_s *dev, uint8_t opmode, uint8_t pid);

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
                      int timeout);

/****************************************************************************
 * Name: obd_decode_pid
 *
 * Description:
 *   Decode the value returned for a determined PID.
 *
 *   It will return the decode PID as string or NULL if error.
 *
 ****************************************************************************/

FAR char *obd_decode_pid(FAR struct obd_dev_s *dev, uint8_t pid);

#endif /*__APPS_INCLUDE_CANUTILS_OBD_H */
