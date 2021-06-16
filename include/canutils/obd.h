/****************************************************************************
 * apps/include/canutils/obd.h
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

#ifndef __APPS_INCLUDE_CANUTILS_OBD_H
#define __APPS_INCLUDE_CANUTILS_OBD_H

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
