/****************************************************************************
 * apps/lte/alt1250/alt1250_usockif.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_USOCKIF_H
#define __APPS_LTE_ALT1250_ALT1250_USOCKIF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <net/if.h>

#include <nuttx/compiler.h>
#include <nuttx/net/usrsock.h>

#include <nuttx/wireless/lte/lte_ioctl.h>

#define DEV_USRSOCK  "/dev/usrsock"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IS_IOCTLREQ(r) ((r).request.head.reqid == USRSOCK_REQUEST_IOCTL)
#define IS_REQID_VALID(r) ((r).request.head.reqid < USRSOCK_REQUEST__MAX)
#define USOCKREQID(r) ((r).request.head.reqid)
#define USOCKREQXID(r) ((r).request.head.xid)

#define usockif_sendclose(fff, iii)  \
          usockif_sendevent((fff), (iii), USRSOCK_EVENT_REMOTE_CLOSED)

#define usockif_sendabort(fff, iii)  \
          usockif_sendevent((fff), (iii), USRSOCK_EVENT_ABORT)

#define usockif_sendtxready(fff, iii)  \
          usockif_sendevent((fff), (iii), USRSOCK_EVENT_SENDTO_READY)

#define usockif_sendrxready(fff, iii)  \
          usockif_sendevent((fff), (iii), USRSOCK_EVENT_RECVFROM_AVAIL)

/****************************************************************************
 * Public Types
 ****************************************************************************/

union usrsock_requests_u
{
  struct usrsock_request_common_s      head;

  struct usrsock_request_socket_s      sock_req;
  struct usrsock_request_close_s       close_req;
  struct usrsock_request_bind_s        bind_req;
  struct usrsock_request_connect_s     conn_req;
  struct usrsock_request_listen_s      listen_req;
  struct usrsock_request_accept_s      accept_req;
  struct usrsock_request_sendto_s      send_req;
  struct usrsock_request_recvfrom_s    recv_req;
  struct usrsock_request_setsockopt_s  sopt_req;
  struct usrsock_request_getsockopt_s  gopt_req;
  struct usrsock_request_getsockname_s name_req;
  struct usrsock_request_getpeername_s pname_req;
  struct usrsock_request_ioctl_s       ioctl_req;
  struct usrsock_request_shutdown_s    shutdown_req;
};
#define USOCK_HDR_SIZE sizeof(struct usrsock_request_common_s)

union usrsock_request_ioctl_u
{
  struct lte_ioctl_data_s ltecmd;
  struct ifreq ifreq;
  uint8_t sock_type;
  struct lte_smsreq_s smsreq;
};

struct usrsock_request_buff_s
{
  union usrsock_requests_u request;
  union usrsock_request_ioctl_u req_ioctl;
};

struct usock_ackinfo_s
{
  uint16_t valuelen;
  uint16_t valuelen_nontrunc;
  FAR uint8_t *value_ptr;
  FAR uint8_t *buf_ptr;
  int usockid;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int init_usock_device(void);
void finalize_usock_device(int fd);
int reset_usock_device(int fd);

int usockif_readrequest(int fd, FAR struct usrsock_request_buff_s *buf);
int usockif_readreqioctl(int fd, FAR struct usrsock_request_buff_s *buf);
int usockif_readreqaddr(int fd, FAR struct sockaddr_storage *addr,
                        size_t sz);
int usockif_readreqsendbuf(int fd, FAR uint8_t *sendbuf, size_t sz);
int usockif_readreqoption(int fd, FAR uint8_t *option, size_t sz);

void usockif_discard(int fd, size_t sz);
int usockif_sendack(int fd, int32_t usock_result, uint32_t usock_xid,
                    bool inprogress);
int usockif_senddataack(int fd, int32_t usock_result, uint32_t usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo);
int usockif_sendevent(int fd, int usockid, int event);

#endif  /* __APPS_LTE_ALT1250_ALT1250_USOCKIF_H */
