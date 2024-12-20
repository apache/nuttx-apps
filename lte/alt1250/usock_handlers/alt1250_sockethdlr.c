/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_sockethdlr.c
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
 * Pre-processor Definitions
 ****************************************************************************/

#define IS_SUPPORTED_INET_DOMAIN(d) (((d) == AF_INET) || ((d) == AF_INET6))

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int postproc_getfl(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *reply,
                          FAR struct usock_s *usock,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo,
                          unsigned long arg);

static int postproc_setfl(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *reply,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo,
                              unsigned long arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_fctl_command
 ****************************************************************************/

static int send_fctl_command(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             FAR struct usock_s *usock,
                             int32_t cmd,
                             int32_t val,
                             FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[3];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &USOCKET_ALTSOCKID(usock);
  inparam[1] = &cmd;
  inparam[2] = &val;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_FCNTL);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, cmd == ALTCOM_SETFL ? postproc_setfl :
                          cmd == ALTCOM_GETFL ? postproc_getfl : NULL, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: postproc_setfl
 ****************************************************************************/

static int postproc_setfl(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *reply,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo,
                              unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);

  /* resp[0]: command return code
   * resp[1]: alt1250 errno
   */

  dbg_alt1250("%s start\n", __func__);

  *usock_xid = USOCKET_XID(usock);
  *usock_result = COMBINE_ERRCODE(*(FAR int *)resp[0], *(FAR int *)resp[1]);

  if (*usock_result >= 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_OPENED);
      usocket_commitstate(dev);

      switch (USOCKET_REQID(usock))
        {
          case USRSOCK_REQUEST_SOCKET:
            *usock_result = USOCKET_USOCKID(usock);
            dbg_alt1250("allocated usockid: %d\n", *usock_result);
            break;

          /* Below cases are for SOCK_STREAM type socket
           * SOCK_STREAM type socket can not be opend on alt1250 side,
           * because of LAPI command. It is also using SOCK_STREAM
           * type socket to send ioctl() to communicate with daemon.
           * So, on BSD-socket spec, aftre socket() API succeeded,
           * connect(), bind(), etc, can be called. And on the call,
           * socket on alt1250 side should be fixed, therefore,
           * open_altsocket() will be called on those cases.
           */

          case USRSOCK_REQUEST_CONNECT:
            ret = nextstep_connect(dev, reply, usock, usock_result,
                                   usock_xid, ackinfo, arg);
            break;

          case USRSOCK_REQUEST_BIND:
            ret = nextstep_bind(dev, reply, usock, usock_result,
                                usock_xid, ackinfo, arg);
            break;

          case USRSOCK_REQUEST_LISTEN:
            ret = nextstep_listen(dev, reply, usock, usock_result,
                                  usock_xid, ackinfo, arg);
            break;

          case USRSOCK_REQUEST_SETSOCKOPT:
            ret = nextstep_setsockopt(dev, reply, usock, usock_result,
                                      usock_xid, ackinfo, arg);
            break;

          case USRSOCK_REQUEST_GETSOCKOPT:
            ret = nextstep_getsockopt(dev, reply, usock, usock_result,
                                      usock_xid, ackinfo, arg);
            break;

          case USRSOCK_REQUEST_GETSOCKNAME:
            ret = nextstep_getsockname(dev, reply, usock, usock_result,
                                       usock_xid, ackinfo, arg);
            break;

          default:
            dbg_alt1250("unexpected sequence. reqid:0x%02x\n",
                        USOCKET_REQID(usock));
            *usock_result = -EFAULT;
            break;
        }
    }
  else
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_PREALLOC);
      if (USOCKET_TYPE(usock) == SOCK_DGRAM)
        {
          usocket_free(usock);
        }

      usocket_commitstate(dev);
    }

  return ret;
}

/****************************************************************************
 * name: postproc_getfl
 ****************************************************************************/

static int postproc_getfl(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *reply,
                          FAR struct usock_s *usock,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo,
                          unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);

  /* resp[0]: command return code
   * resp[1]: alt1250 errno
   */

  dbg_alt1250("%s start\n", __func__);

  *usock_xid = USOCKET_XID(usock);
  *usock_result = COMBINE_ERRCODE(*(FAR int *)resp[0], *(FAR int *)resp[1]);

  if (*usock_result >= 0)
    {
      /* Set non-blocking flag of socket fd */

      *usock_result |= ALTCOM_O_NONBLOCK;

      ret = send_fctl_command(dev, reply, usock, ALTCOM_SETFL, *usock_result,
                              usock_result);
    }

  if (*usock_result < 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_PREALLOC);
      if (USOCKET_TYPE(usock) == SOCK_DGRAM)
        {
          /* In DGRAM case, it is a part of socket() API sequence.
           * The socket() API is not returned on application layer.
           * So, internal socket should be free here.
           */

          usocket_free(usock);
        }

      usocket_commitstate(dev);
    }

  return ret;
}

/****************************************************************************
 * name: postproc_socket
 ****************************************************************************/

static int postproc_socket(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);

  /* resp[0]: altcom_sockfd
   * resp[1]: alt1250 errno
   */

  dbg_alt1250("%s start\n", __func__);

  *usock_xid = USOCKET_XID(usock);
  *usock_result = COMBINE_ERRCODE(*(FAR int *)resp[0], *(FAR int *)resp[1]);

  if (*usock_result >= 0)
    {
      /* In case of success socket allocation */

      USOCKET_SET_ALTSOCKID(usock, *usock_result);

      ret = send_fctl_command(dev, reply, usock, ALTCOM_GETFL, 0,
                              usock_result);
    }

  if (*usock_result < 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_PREALLOC);
      if (USOCKET_TYPE(usock) == SOCK_DGRAM)
        {
          /* In DGRAM case, it is a part of socket() API sequence.
           * The socket() API is not returned on application layer.
           * So, internal socket should be free here.
           */

          usocket_free(usock);
        }

      usocket_commitstate(dev);
    }

  return ret;
}

/****************************************************************************
 * name: send_socket_command
 ****************************************************************************/

static int send_socket_command(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               FAR struct usock_s *usock,
                               int16_t domain,
                               int16_t type,
                               int16_t protocol,
                               FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[3];

  /* These members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = &domain;
  inparam[1] = &type;
  inparam[2] = &protocol;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_ERRCODE(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_SOCKET);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_socket, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: open_altsocket
 ****************************************************************************/

int open_altsocket(FAR struct alt1250_s *dev,
                   FAR struct alt_container_s *container,
                   FAR struct usock_s *usock,
                   FAR int32_t *usock_result)
{
  int ret;

  dbg_alt1250("%s start\n", __func__);

  ret = send_socket_command(dev, container, usock,
                            USOCKET_DOMAIN(usock), USOCKET_TYPE(usock),
                            USOCKET_PROTOCOL(usock),
                            usock_result);
  if (*usock_result >= 0)
    {
      USOCKET_SET_STATE(usock, SOCKET_STATE_OPEN);
      usocket_commitstate(dev);
    }

  return ret;
}

/****************************************************************************
 * name: usockreq_socket
 ****************************************************************************/

int usockreq_socket(FAR struct alt1250_s *dev,
                    FAR struct usrsock_request_buff_s *req,
                    FAR int32_t *usock_result,
                    FAR uint32_t *usock_xid,
                    FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_socket_s *request = &req->request.sock_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start type=%d\n", __func__, request->type);

  *usock_xid = request->head.xid;

  if (!IS_SUPPORTED_INET_DOMAIN(request->domain))
    {
      dbg_alt1250("Not support this domain: %u\n", request->domain);
      *usock_result = -EAFNOSUPPORT;
      return REP_SEND_ACK_WOFREE;
    }
  else if (!dev->usock_enable && IS_SUPPORTED_INET_DOMAIN(request->domain) &&
           request->type != SOCK_CTRL)
    {
      /* If domain is AF_INET while usock_enable is false,
       * set usockid to -ENOTSUP to fallback kernel
       * network stack.
       */

      *usock_result = -ENOTSUP;
      return REP_SEND_ACK_WOFREE;
    }

  if (!dev->api_enable)
    {
      dbg_alt1250("Don't allow to call any API now.\n");
      *usock_result = -EBUSY;
      return REP_SEND_ACK_WOFREE;
    }

  usock = usocket_alloc(dev, request->type);
  if (usock == NULL)
    {
      dbg_alt1250("socket alloc faild\n");
      *usock_result = -EBUSY;
      return REP_SEND_ACK_WOFREE;
    }

  *usock_result = USOCKET_USOCKID(usock);

  USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);
  USOCKET_SET_SOCKTYPE(usock, request->domain, request->type,
                       request->protocol);

  switch (request->type)
    {
      case SOCK_STREAM:
      case SOCK_CTRL:
        dbg_alt1250("allocated usockid: %d\n", *usock_result);
        break;

      case SOCK_SMS:
        ret = alt1250_sms_init(dev, usock, usock_result, ackinfo);
        if (*usock_result < 0)
          {
            usocket_free(usock);
          }
        break;

      case SOCK_DGRAM:
      case SOCK_RAW:
        dbg_alt1250("allocated usockid: %d\n", *usock_result);
        container = container_alloc();
        if (container == NULL)
          {
            dbg_alt1250("no container\n");
            usocket_free(usock);
            return REP_NO_CONTAINER;
          }

        ret = open_altsocket(dev, container, usock, usock_result);
        if (IS_NEED_CONTAINER_FREE(ret))
          {
            container_free(container);
          }

        if (*usock_result < 0)
          {
            usocket_free(usock);
          }
        break;

      default:
        dbg_alt1250("Not support this type: %u\n", request->type);
        *usock_result = -EAFNOSUPPORT;
        usocket_free(usock);
        break;
    }

  return ret;
}
