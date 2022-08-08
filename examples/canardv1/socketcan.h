/****************************************************************************
 * apps/examples/canardv1/socketcan.h
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

#ifndef SOCKETCAN_H_INCLUDED
#define SOCKETCAN_H_INCLUDED

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/time.h>
#include <sys/socket.h>

#include <nuttx/can.h>
#include <netpacket/can.h>

#include <canard.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct canardsocketinstance canardsocketinstance;
typedef int fd_t;

struct canardsocketinstance
{
  fd_t               s;
  bool               can_fd;

  /* Send msg structure */

  struct iovec       send_iov;
  struct canfd_frame send_frame;
  struct msghdr      send_msg;
  struct cmsghdr     *send_cmsg;
  struct timeval     *send_tv;   /* TX deadline timestamp */
  uint8_t            send_control[sizeof(struct cmsghdr)
                       + sizeof(struct timeval)];

  /* Receive msg structure */

  struct iovec       recv_iov;
  struct canfd_frame recv_frame;
  struct msghdr      recv_msg;
  struct cmsghdr     *recv_cmsg;
  uint8_t            recv_control[sizeof(struct cmsghdr)
                       + sizeof(struct timeval)];
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Creates a SocketCAN socket for corresponding iface can_iface_name
 * Also sets up the message structures required for transmit and receive
 * can_fd determines to use CAN FD frame when is 1, and classical CAN
 * frame when is 0. The return value is 0 on succes and -1 on error
 */

int16_t socketcanopen(canardsocketinstance *ins,
                      const char *const can_iface_name,
                      const bool can_fd);

/* Send a CanardFrame to the canardsocketinstance socket
 * This function is blocking
 * The return value is number of bytes transferred, negative value on error.
 */

int16_t socketcantransmit(canardsocketinstance *ins, const CanardFrame *txf);

/* Receive a CanardFrame from the canardsocketinstance socket
 * This function is blocking
 * The return value is number of bytes received, negative value on error.
 */

int16_t socketcanreceive(canardsocketinstance *ins, CanardFrame *rxf);

/* TODO implement ioctl for CAN filter */

int16_t socketcanconfigurefilter(const fd_t fd, const size_t num_filters,
                                 const struct can_filter *filters);

#ifdef __cplusplus
}
#endif

#endif //SOCKETCAN_H_INCLUDED
