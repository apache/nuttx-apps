/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_connecthdlr.c
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
 * Private Data
 ****************************************************************************/

static int16_t g_request_level = SOL_SOCKET;
static int16_t g_request_option = SO_ERROR;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: postproc_connect
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
      *usock_result = *(FAR int32_t *)(resp[3]);
      *usock_xid = USOCKET_XID(usock);

      dbg_alt1250("connect result = %d\n", *usock_result);

      if (*usock_result > 0)
        {
          *usock_result = -(*usock_result);
        }

      if (*usock_result == 0)
        {
          USOCKET_SET_STATE(usock, SOCKET_STATE_CONNECTED);
        }
      else
        {
          USOCKET_SET_STATE(usock, SOCKET_STATE_OPENED);
        }
    }
  else
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_OPENED);
    }

  USOCKET_SET_SELECTABLE(usock, SELECT_WRITABLE);
  usocket_commitstate(dev);

  return ret;
}

/****************************************************************************
 * name: postproc_connect
 ****************************************************************************/

static int postproc_connect(FAR struct alt1250_s *dev,
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
   */

  *usock_xid = USOCKET_XID(usock);
  *usock_result = COMBINE_ERRCODE(*(FAR int *)resp[0], *(FAR int *)resp[1]);

  dbg_alt1250("%s connect result:%d\n", __func__, *usock_result);

  if (*usock_result >= 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_CONNECTED);
    }
  else if (*usock_result == -EINPROGRESS)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_WAITCONN);
      ret = REP_NO_ACK;
    }
  else
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_OPENED);
    }

  usocket_commitstate(dev);

  return ret;
}

/****************************************************************************
 * name: send_connect_command
 ****************************************************************************/

static int send_connect_command(FAR struct alt1250_s *dev,
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

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_CONNECT);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_connect, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: nextstep_check_connectres
 ****************************************************************************/

int nextstep_check_connectres(FAR struct alt1250_s *dev,
                              FAR struct usock_s *usock)
{
  int ret = REP_NO_CONTAINER;
  int32_t usock_result;
  FAR struct alt_container_s *container;

  dbg_alt1250("%s start\n", __func__);

  container = container_alloc();
  if (container != NULL)
    {
      ret = send_getsockopt_command(dev, container, usock,
                                    g_request_level, g_request_option,
                                    sizeof(int), &g_request_level,
                                    &g_request_option, postproc_getsockopt,
                                    0, &usock_result);
    }
  else
    {
      /* If there is no container, the state is not
       * updated. This guarantees that a select request
       * will be sent. When the response is received,
       * it can try to acquire a container. There is a
       * possibility that the container is free at that
       * timing.
       */

      dbg_alt1250("no container\n");
    }

  return ret;
}

/****************************************************************************
 * name: nextstep_connect
 ****************************************************************************/

int nextstep_connect(FAR struct alt1250_s *dev,
                     FAR struct alt_container_s *reply,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR uint32_t *usock_xid,
                     FAR struct usock_ackinfo_s *ackinfo,
                     unsigned long arg)
{
  int ret = REP_SEND_ACK;

  dbg_alt1250("%s start\n", __func__);

  ret = send_connect_command(dev, reply, usock, usock_result);

  if  (*usock_result >= 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_CONNECTING);
      usocket_commitstate(dev);
    }

  return ret;
}

/****************************************************************************
 * usockreq_connect
 ****************************************************************************/

int usockreq_connect(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_buff_s *req,
                     FAR int32_t *usock_result,
                     FAR uint32_t *usock_xid,
                     FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_connect_s *request = &req->request.conn_req;
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

        ret = nextstep_connect(dev, container, usock, usock_result,
                               usock_xid, ackinfo, 0);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }
        break;
    }

  return ret;
}
