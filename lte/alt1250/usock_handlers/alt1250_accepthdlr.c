/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_accepthdlr.c
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
#include "alt1250_container.h"
#include "alt1250_devif.h"
#include "alt1250_socket.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: postproc_accepterr
 ****************************************************************************/

static int postproc_accepterr(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *reply,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo,
                              unsigned long arg)
{
  int ret = REP_SEND_ACK;

  dbg_alt1250("%s start\n", __func__);

  /* resp[0]: ret code
   * resp[1]: error code
   */

  *usock_xid = USOCKET_XID(usock);
  *usock_result = -EBUSY;

  return ret;
}

/****************************************************************************
 * name: send_close_command
 ****************************************************************************/

static int send_close_command(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              FAR struct usock_s *usock,
                              int altsock,
                              FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[1];

  /* This member is referenced only when sending a command and
   * not when receiving a response, so local variable is used.
   */

  inparam[0] = &altsock;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_CLOSE);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_accepterr, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: postproc_accept
 ****************************************************************************/

static int postproc_accept(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg)
{
  int ret = REP_SEND_ACK;
  int altsock_res;
  FAR struct usock_s *accept_sock;
  FAR void **resp = CONTAINER_RESPONSE(reply);

  dbg_alt1250("%s start\n", __func__);

  /* resp[0]: socket fd
   * resp[1]: error code
   * resp[2]: addrlen
   * resp[3]: accepted address
   */

  altsock_res = COMBINE_ERRCODE(*(int *)(resp[0]), *(int *)(resp[1]));
  if (altsock_res >= 0)
    {
      USOCKET_SET_SELECTABLE(usock, SELECT_READABLE);

      accept_sock = usocket_alloc(dev, USOCKET_TYPE(usock));
      if (!accept_sock)
        {
          ret = send_close_command(dev, reply, usock, altsock_res,
                                   usock_result);
          *usock_xid = USOCKET_XID(usock);
        }
      else
        {
          USOCKET_SET_ALTSOCKID(accept_sock, altsock_res);
          USOCKET_SET_STATE(accept_sock, SOCKET_STATE_CONNECTED);

          *usock_result = sizeof(uint16_t);
          *usock_xid = USOCKET_XID(usock);

          ackinfo->valuelen = MIN(USOCKET_REQADDRLEN(usock),
                                  *(uint16_t *)(resp[2]));
          ackinfo->valuelen_nontrunc = *(uint16_t *)(resp[2]);
          ackinfo->value_ptr = resp[3];
          ackinfo->buf_ptr = (FAR uint8_t *)&USOCKET_USOCKID(accept_sock);

          ret = REP_SEND_DACK;
        }

      usocket_commitstate(dev);
    }
  else
    {
      *usock_result = altsock_res;
      *usock_xid = USOCKET_XID(usock);
    }

  return ret;
}

/****************************************************************************
 * name: send_accept_command
 ****************************************************************************/

static int send_accept_command(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               FAR struct usock_s *usock,
                               uint16_t addrlen,
                               FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[2];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &addrlen;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ADDRLEN(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ADDR(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_ACCEPT);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_accept, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_accept
 ****************************************************************************/

int usockreq_accept(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_accept_s *request = &req->request.accept_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;
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

  ret = send_accept_command(dev, container, usock, addrlen, usock_result);

  if (IS_NEED_CONTAINER_FREE(ret))
    {
      container_free(container);
    }

  return ret;
}
