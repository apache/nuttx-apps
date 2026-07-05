/****************************************************************************
 * apps/examples/lely_master/candev_char.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#include <nuttx/can/can.h>

#include "candev.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_candev_fd;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: comaster_candev_init
 ****************************************************************************/

int comaster_candev_init(void)
{
  struct canioc_bittiming_s bt;
  int ret;

  /* Non-blocking so the main loop keeps servicing the CANopen timers */

  g_candev_fd = open(CONFIG_EXAMPLES_LELYMASTER_DEVPATH,
                     O_RDWR | O_NONBLOCK);
  if (g_candev_fd < 0)
    {
      ret = -errno;
      printf("ERROR: open %s failed %d\n",
             CONFIG_EXAMPLES_LELYMASTER_DEVPATH, errno);
      return ret;
    }

  ret = ioctl(g_candev_fd, CANIOC_GET_BITTIMING, &bt);
  if (ret < 0)
    {
      printf("Bit timing not available: %d\n", errno);
    }
  else
    {
      printf("Bit timing:\n");
      printf("   Baud: %lu\n", (unsigned long)bt.bt_baud);
      printf("  TSEG1: %u\n", bt.bt_tseg1);
      printf("  TSEG2: %u\n", bt.bt_tseg2);
      printf("    SJW: %u\n", bt.bt_sjw);
    }

  return OK;
}

/****************************************************************************
 * Name: comaster_candev_send
 ****************************************************************************/

int comaster_candev_send(FAR const struct can_msg *msg)
{
  struct can_msg_s txmsg;
  ssize_t          nwritten;
  int              i;

  txmsg.cm_hdr.ch_id    = msg->id;
  txmsg.cm_hdr.ch_rtr   = (msg->flags & CAN_FLAG_RTR) != 0;
  txmsg.cm_hdr.ch_dlc   = msg->len;
#ifdef CONFIG_CAN_EXTID
  txmsg.cm_hdr.ch_extid = (msg->flags & CAN_FLAG_IDE) != 0;
#endif
#ifdef CONFIG_CAN_ERRORS
  txmsg.cm_hdr.ch_error = 0;
#endif
  txmsg.cm_hdr.ch_tcf   = 0;

  for (i = 0; i < msg->len; i++)
    {
      txmsg.cm_data[i] = msg->data[i];
    }

  nwritten = write(g_candev_fd, &txmsg, CAN_MSGLEN(txmsg.cm_hdr.ch_dlc));
  if (nwritten != CAN_MSGLEN(txmsg.cm_hdr.ch_dlc))
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: comaster_candev_recv
 ****************************************************************************/

int comaster_candev_recv(FAR struct can_msg *msg)
{
  struct can_msg_s rxmsg;
  ssize_t          nread;
  int              i;

  nread = read(g_candev_fd, &rxmsg, sizeof(struct can_msg_s));
  if (nread < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return 0;
        }

      return -1;
    }

  if (nread < CAN_MSGLEN(0))
    {
      return 0;
    }

  msg->id    = rxmsg.cm_hdr.ch_id;
  msg->flags = 0;

#ifdef CONFIG_CAN_EXTID
  if (rxmsg.cm_hdr.ch_extid)
    {
      msg->flags |= CAN_FLAG_IDE;
    }
#endif

  if (rxmsg.cm_hdr.ch_rtr)
    {
      msg->flags |= CAN_FLAG_RTR;
    }

  msg->len = rxmsg.cm_hdr.ch_dlc;

  for (i = 0; i < rxmsg.cm_hdr.ch_dlc; i++)
    {
      msg->data[i] = rxmsg.cm_data[i];
    }

  return 1;
}
