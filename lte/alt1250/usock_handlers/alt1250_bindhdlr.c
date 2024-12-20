/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_bindhdlr.c
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
#include "alt1250_socket.h"
#include "alt1250_devif.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"
#include "alt1250_usrsock_hdlr.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_bind_command
 ****************************************************************************/

static int send_bind_command(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             FAR struct usock_s *usock,
                             FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[3];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &USOCKET_REQADDRLEN(usock);
  inparam[2] = &USOCKET_REQADDR(usock);

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_BIND);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_sockcommon, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: nextstep_bind
 ****************************************************************************/

int nextstep_bind(FAR struct alt1250_s *dev,
                  FAR struct alt_container_s *reply,
                  FAR struct usock_s *usock,
                  FAR int32_t *usock_result,
                  FAR uint32_t *usock_xid,
                  FAR struct usock_ackinfo_s *ackinfo,
                  unsigned long arg)
{
  dbg_alt1250("%s start\n", __func__);

  return send_bind_command(dev, reply, usock, usock_result);
}

/****************************************************************************
 * name: usockreq_bind
 ****************************************************************************/

int usockreq_bind(FAR struct alt1250_s *dev,
                  FAR struct usrsock_request_buff_s *req,
                  FAR int32_t *usock_result,
                  FAR uint32_t *usock_xid,
                  FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_bind_s *request = &req->request.bind_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

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
  USOCKET_SET_REQADDRLEN(usock, request->addrlen);

  switch (USOCKET_STATE(usock))
    {
      case SOCKET_STATE_CLOSED:
      case SOCKET_STATE_CLOSING:
        dbg_alt1250("Unexpected state: %d\n", USOCKET_STATE(usock));
        *usock_result = -EBADFD;
        ret = REP_SEND_ACK_WOFREE;
        break;

      case SOCKET_STATE_PREALLOC:
        container = container_alloc();
        if (container == NULL)
          {
            dbg_alt1250("no container\n");
            return REP_NO_CONTAINER;
          }

        *usock_result = usockif_readreqaddr(dev->usockfd,
                                            &USOCKET_REQADDR(usock),
                                            USOCKET_REQADDRLEN(usock));
        if (*usock_result < 0)
          {
            container_free(container);
            return REP_SEND_ACK;
          }

        ret = open_altsocket(dev, container, usock, usock_result);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }
        break;

      default:
        container = container_alloc();
        if (container == NULL)
          {
            dbg_alt1250("no container\n");
            return REP_NO_CONTAINER;
          }

        *usock_result = usockif_readreqaddr(dev->usockfd,
                                            &USOCKET_REQADDR(usock),
                                            USOCKET_REQADDRLEN(usock));
        if (*usock_result < 0)
          {
            container_free(container);
            return REP_SEND_ACK;
          }

        ret = nextstep_bind(dev, container, usock, usock_result, usock_xid,
                            ackinfo, 0);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }
        break;
    }

  return ret;
}
