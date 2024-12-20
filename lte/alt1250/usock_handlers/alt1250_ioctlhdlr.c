/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctlhdlr.c
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
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"
#include "alt1250_ioctl_subhdlr.h"
#include "alt1250_sms.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl
 ****************************************************************************/

int usockreq_ioctl(FAR struct alt1250_s *dev,
                   FAR struct usrsock_request_buff_s *req,
                   FAR int32_t *usock_result,
                   FAR uint32_t *usock_xid,
                   FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request =
                              &req->request.ioctl_req;
  FAR struct usock_s *usock;
  int ret = REP_SEND_ACK_WOFREE;
  usrsock_reqhandler_t ioctl_subhdlr = NULL;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = -EBADFD;
  *usock_xid = request->head.xid;

  usock = usocket_search(dev, request->usockid);
  if (usock)
    {
      USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);

      switch (request->cmd)
        {
          case FIONBIO:
          case SIOCGIFFLAGS:

            /* ALT1250 doesn't use this command. Only return OK. */

            *usock_result = OK;
            break;
          case SIOCLTECMD:
            ioctl_subhdlr = usockreq_ioctl_ltecmd;
            break;
          case SIOCSIFFLAGS:
            ioctl_subhdlr = usockreq_ioctl_ifreq;
            break;
          case SIOCDENYINETSOCK:
            ioctl_subhdlr = usockreq_ioctl_denyinetsock;
            break;
          case SIOCSMSENSTREP:
          case SIOCSMSGREFID:
          case SIOCSMSSSCA:
            ioctl_subhdlr = usockreq_ioctl_sms;
          default:
            *usock_result = -EINVAL;
            break;
        }

      if (ioctl_subhdlr != NULL)
        {
          ret = ioctl_subhdlr(dev, req, usock_result, usock_xid, ackinfo);
        }
    }

  return ret;
}
