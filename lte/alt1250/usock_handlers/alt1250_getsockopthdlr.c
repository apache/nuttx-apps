/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_getsockopthdlr.c
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
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int postproc_getsockopt(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR struct usock_s *usock,
                               FAR int32_t *usock_result,
                               FAR uint32_t *usock_xid,
                               FAR struct usock_ackinfo_s *ackinfo,
                               unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);

  dbg_alt1250("%s start\n", __func__);

  /* resp[0]: ret code
   * resp[1]: error code
   * resp[2]: optlen
   * resp[3]: optval
   */

  *usock_xid = USOCKET_XID(usock);
  *usock_result = COMBINE_ERRCODE(*(FAR int *)resp[0], *(FAR int *)resp[1]);

  if (*usock_result >= 0)
    {
      *usock_result = 0;
      *usock_xid = USOCKET_XID(usock);

      ackinfo->valuelen = MIN(USOCKET_REQOPTLEN(usock),
                              *(FAR uint16_t *)(resp[2]));
      ackinfo->valuelen_nontrunc = *(FAR uint16_t *)(resp[2]);
      ackinfo->value_ptr = resp[3];
      ackinfo->buf_ptr = NULL;

      if (ackinfo->valuelen != 0)
        {
          ret = REP_SEND_DACK;
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: nextstep_getsockopt
 ****************************************************************************/

int nextstep_getsockopt(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR struct usock_s *usock,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo,
                        unsigned long arg)
{
  dbg_alt1250("%s start\n", __func__);

  return send_getsockopt_command(dev, reply, usock,
                                 USOCKET_REQOPTLEVEL(usock),
                                 USOCKET_REQOPTOPT(usock),
                                 USOCKET_REQOPTLEN(usock),
                                 &USOCKET_REQOPTLEVEL(usock),
                                 &USOCKET_REQOPTOPT(usock),
                                 postproc_getsockopt, 0, usock_result);
}

/****************************************************************************
 * name: send_getsockopt_command
 ****************************************************************************/

int send_getsockopt_command(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            FAR struct usock_s *usock,
                            int16_t level,
                            int16_t option,
                            uint16_t valuelen,
                            FAR int16_t *requested_level,
                            FAR int16_t *requested_option,
                            FAR postproc_hdlr_t func,
                            unsigned long priv,
                            FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[4];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &level;
  inparam[2] = &option;
  inparam[3] = &valuelen;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_OPTLEN(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_OPTVAL(usock));
  USOCKET_SET_RESPONSE(usock, idx++, requested_level);
  USOCKET_SET_RESPONSE(usock, idx++, requested_option);

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_GETSOCKOPT);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, func, priv);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: usockreq_getsockopt
 ****************************************************************************/

int usockreq_getsockopt(FAR struct alt1250_s *dev,
                        FAR struct usrsock_request_buff_s *req,
                        FAR int32_t *usock_result,
                        FAR uint32_t *usock_xid,
                        FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_getsockopt_s *request = &req->request.gopt_req;
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

  if (request->max_valuelen > _OPTVAL_LEN_MAX)
    {
      *usock_result = -EINVAL;
      return REP_SEND_ACK_WOFREE;
    }

  USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);
  USOCKET_SET_REQSOCKOPT(usock, request->level, request->option,
                         request->max_valuelen);

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

        ret = nextstep_getsockopt(dev, container, usock, usock_result,
                                  usock_xid, ackinfo, 0);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }
        break;
    }

  return ret;
}
