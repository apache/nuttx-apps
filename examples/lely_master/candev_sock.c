/****************************************************************************
 * apps/examples/lely_master/candev_sock.c
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
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/socket.h>

#include <nuttx/can.h>

#include "netutils/netlib.h"
#include "candev.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_candev_sock;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: comaster_candev_init
 ****************************************************************************/

int comaster_candev_init(void)
{
  struct sockaddr_can addr;
  struct ifreq ifr;
  int on = 1;
  int ret;

  g_candev_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (g_candev_sock < 0)
    {
      ret = -errno;
      printf("ERROR: socket failed %d\n", errno);
      return ret;
    }

  strlcpy(ifr.ifr_name, CONFIG_EXAMPLES_LELYMASTER_INTF, IFNAMSIZ);

  /* SocketCAN silently drops traffic while the interface is down, so bring
   * it up here.  This lets the example run without a separate "ifup" step.
   */

  if (netlib_ifup(ifr.ifr_name) < 0)
    {
      ret = -errno;
      printf("ERROR: netlib_ifup %s failed %d\n", ifr.ifr_name, errno);
      goto errout;
    }

  ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
  if (ifr.ifr_ifindex == 0)
    {
      ret = -errno;
      printf("ERROR: if_nametoindex %s failed %d\n", ifr.ifr_name, errno);
      goto errout;
    }

  setsockopt(g_candev_sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,
             &on, sizeof(on));

  memset(&addr, 0, sizeof(addr));
  addr.can_family  = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(g_candev_sock, (FAR struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      ret = -errno;
      printf("ERROR: bind failed %d\n", errno);
      goto errout;
    }

  ret = fcntl(g_candev_sock, F_SETFL, O_NONBLOCK);
  if (ret < 0)
    {
      ret = -errno;
      goto errout;
    }

  return OK;

errout:
  close(g_candev_sock);
  g_candev_sock = -1;
  return ret;
}

/****************************************************************************
 * Name: comaster_candev_send
 ****************************************************************************/

int comaster_candev_send(FAR const struct can_msg *msg)
{
  struct can_frame frame;
  ssize_t          nwritten;

  memset(&frame, 0, sizeof(frame));

  frame.can_id = msg->id;
  if (msg->flags & CAN_FLAG_IDE)
    {
      frame.can_id |= CAN_EFF_FLAG;
    }

  if (msg->flags & CAN_FLAG_RTR)
    {
      frame.can_id |= CAN_RTR_FLAG;
    }

  frame.can_dlc = msg->len;
  memcpy(frame.data, msg->data, msg->len);

  nwritten = write(g_candev_sock, &frame, sizeof(frame));
  if (nwritten != sizeof(frame))
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
  struct can_frame frame;
  ssize_t          nread;

  nread = read(g_candev_sock, &frame, sizeof(frame));
  if (nread < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return 0;
        }

      return -1;
    }

  if (nread < sizeof(frame))
    {
      return 0;
    }

  msg->id    = frame.can_id & CAN_EFF_MASK;
  msg->flags = 0;
  if (frame.can_id & CAN_EFF_FLAG)
    {
      msg->flags |= CAN_FLAG_IDE;
    }

  if (frame.can_id & CAN_RTR_FLAG)
    {
      msg->flags |= CAN_FLAG_RTR;
    }

  msg->len = frame.can_dlc;
  memcpy(msg->data, frame.data, frame.can_dlc);

  return 1;
}
