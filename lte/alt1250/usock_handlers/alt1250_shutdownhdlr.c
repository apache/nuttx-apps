/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_shutdownhdlr.c
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
 * name: send_shutdown_command
 ****************************************************************************/

static int send_shutdown_command(FAR struct alt1250_s *dev,
                                 FAR struct alt_container_s *container,
                                 FAR struct usock_s *usock, int32_t how,
                                 FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[2];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &how;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_SHUTDOWN);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_sockcommon, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_shutdown
 ****************************************************************************/

int usockreq_shutdown(FAR struct alt1250_s *dev,
                      FAR struct usrsock_request_buff_s *req,
                      FAR int32_t *usock_result,
                      FAR uint32_t *usock_xid,
                      FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_shutdown_s *request =
    &req->request.shutdown_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;
  *usock_xid = request->head.xid;

  if (request->how != SHUT_RD &&
      request->how != SHUT_WR &&
      request->how != SHUT_RDWR)
    {
      dbg_alt1250("Invalid parameter: %d\n", request->how);
      *usock_result = -EINVAL;
      return REP_SEND_ACK_WOFREE;
    }

  usock = usocket_search(dev, request->usockid);
  if (usock == NULL)
    {
      dbg_alt1250("Failed to get socket context: %u\n",
                  request->usockid);
      *usock_result = -EBADFD;
      return REP_SEND_ACK_WOFREE;
    }

  USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);

  switch (USOCKET_STATE(usock))
    {
      case SOCKET_STATE_CLOSED:
      case SOCKET_STATE_CLOSING:
        dbg_alt1250("Unexpected state: %d\n", USOCKET_STATE(usock));
        *usock_result = -EBADFD;
        ret = REP_SEND_ACK_WOFREE;
        break;

      case SOCKET_STATE_PREALLOC:
        *usock_result = OK;
        ret = REP_SEND_ACK_WOFREE;
        break;

      default:
        container = container_alloc();
        if (container == NULL)
          {
            dbg_alt1250("no container\n");
            return REP_NO_CONTAINER;
          }

        ret = send_shutdown_command(dev, container, usock,
                                    request->how, usock_result);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }
        break;
    }

  return ret;
}
