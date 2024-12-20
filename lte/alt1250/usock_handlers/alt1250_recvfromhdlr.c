/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_recvfromhdlr.c
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
#include <nuttx/net/usrsock.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_container.h"
#include "alt1250_devif.h"
#include "alt1250_socket.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint16_t _rx_max_buflen;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: postproc_recvfrom
 ****************************************************************************/

static int postproc_recvfrom(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *reply,
                             FAR struct usock_s *usock,
                             FAR int32_t *usock_result,
                             FAR uint32_t *usock_xid,
                             FAR struct usock_ackinfo_s *ackinfo,
                             unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);
  int rsize;
  uint16_t addr_len = *(uint16_t *)(resp[2]);

  dbg_alt1250("%s start\n", __func__);

  dev->recvfrom_processing = false;

  /* resp[0]: recv size
   * resp[1]: error code
   * resp[2]: address length
   * resp[3]: address
   * resp[4]: buffer
   */

  rsize = COMBINE_ERRCODE(*(int *)(resp[0]), *(int *)(resp[1]));

  *usock_result = rsize;
  *usock_xid = USOCKET_XID(usock);

  dbg_alt1250("recv %d bytes\n", rsize);

  if (rsize >= 0)
    {
      ackinfo->valuelen = MIN(USOCKET_REQADDRLEN(usock), addr_len);
      ackinfo->valuelen_nontrunc = addr_len;  /* Total size of addrlen */
      ackinfo->value_ptr = resp[3];           /* Receive from address */
      ackinfo->buf_ptr = resp[4];             /* Actual received data */

      if ((rsize == 0) && (_rx_max_buflen != 0))
        {
          usockif_sendclose(dev->usockfd, USOCKET_USOCKID(usock));
        }

      if ((rsize != 0) || (ackinfo->valuelen != 0))
        {
          ret = REP_SEND_DACK;
        }
    }

  USOCKET_SET_SELECTABLE(usock, SELECT_READABLE);
  usocket_commitstate(dev);

  return ret;
}

/****************************************************************************
 * name: send_recvfrom_command
 ****************************************************************************/

static int send_recvfrom_command(FAR struct alt1250_s *dev,
                                 FAR struct alt_container_s *container,
                                 FAR struct usock_s *usock,
                                 int32_t flags,
                                 uint16_t buflen,
                                 int16_t addrlen,
                                 FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[4];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &flags;
  inparam[2] = &buflen;
  inparam[3] = &addrlen;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ADDRLEN(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ADDR(usock));
  USOCKET_SET_RESPONSE(usock, idx++, dev->rx_buff);

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_RECVFROM);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx++);
  set_container_postproc(container, postproc_recvfrom, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_recvfrom
 ****************************************************************************/

int usockreq_recvfrom(FAR struct alt1250_s *dev,
                          FAR struct usrsock_request_buff_s *req,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_recvfrom_s *request = &req->request.recv_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret;
  socklen_t addrlen;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;
  *usock_xid = request->head.xid;

  usock = usocket_search(dev, request->usockid);
  if (usock == NULL)
    {
      dbg_alt1250("Failed to get socket context: %u\n",
                     request->usockid);
      *usock_result = -EBADFD;
      return REP_SEND_ACK_WOFREE;
    }

  USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);
  USOCKET_SET_REQADDRLEN(usock, request->max_addrlen);
  USOCKET_SET_REQBUFLEN(usock, request->max_buflen);

  if (IS_SMS_SOCKET(usock))
    {
      ret = alt1250_sms_recv(dev, request, usock, usock_result, ackinfo);
    }
  else
    {
      /* Check if this socket is connected. */

      if ((SOCK_STREAM == USOCKET_TYPE(usock)) &&
          (USOCKET_STATE(usock) != SOCKET_STATE_CONNECTED))
        {
          dbg_alt1250("Unexpected state: %d\n", USOCKET_STATE(usock));
          *usock_result = -ENOTCONN;
          return REP_SEND_ACK_WOFREE;
        }

      container = container_alloc();
      if (container == NULL)
        {
          dbg_alt1250("no container\n");
          return REP_NO_CONTAINER;
        }

      if (USOCKET_DOMAIN(usock) == AF_INET)
        {
          addrlen = sizeof(struct sockaddr_in);
        }
      else
        {
          addrlen = sizeof(struct sockaddr_in6);
        }

      _rx_max_buflen = MIN(request->max_buflen, _RX_BUFF_SIZE);

      ret = send_recvfrom_command(dev, container, usock, request->flags,
                                  _rx_max_buflen, addrlen, usock_result);
      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }

      if (*usock_result >= 0)
        {
          dev->recvfrom_processing = true;
        }
    }

  return ret;
}
