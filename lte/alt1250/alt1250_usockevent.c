/****************************************************************************
 * apps/lte/alt1250/alt1250_usockevent.c
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
#include <assert.h>
#include <nuttx/net/usrsock.h>

#include "alt1250_dbg.h"
#include "alt1250_usockevent.h"
#include "alt1250_usrsock_hdlr.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const usrsock_reqhandler_t handlers[USRSOCK_REQUEST__MAX] =
{
  [USRSOCK_REQUEST_SOCKET]      = usockreq_socket,
  [USRSOCK_REQUEST_CLOSE]       = usockreq_close,
  [USRSOCK_REQUEST_CONNECT]     = usockreq_connect,
  [USRSOCK_REQUEST_SENDTO]      = usockreq_sendto,
  [USRSOCK_REQUEST_RECVFROM]    = usockreq_recvfrom,
  [USRSOCK_REQUEST_SETSOCKOPT]  = usockreq_setsockopt,
  [USRSOCK_REQUEST_GETSOCKOPT]  = usockreq_getsockopt,
  [USRSOCK_REQUEST_GETSOCKNAME] = usockreq_getsockname,
  [USRSOCK_REQUEST_GETPEERNAME] = usockreq_getpeername,
  [USRSOCK_REQUEST_BIND]        = usockreq_bind,
  [USRSOCK_REQUEST_LISTEN]      = usockreq_listen,
  [USRSOCK_REQUEST_ACCEPT]      = usockreq_accept,
  [USRSOCK_REQUEST_IOCTL]       = usockreq_ioctl,
  [USRSOCK_REQUEST_SHUTDOWN]    = usockreq_shutdown
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usock_reply
 ****************************************************************************/

int usock_reply(int ufd, int action_code, int32_t result,
                uint32_t xid, FAR struct usock_ackinfo_s *ackinfo)
{
  int ret = OK;

  dbg_alt1250("action code: %08x\n", action_code);

  switch (action_code)
    {
      case REP_SEND_ACK:
      case REP_SEND_ACK_WOFREE:
      case REP_SEND_INPROG:
      case REP_SEND_TERM:
        ret = usockif_sendack(ufd, result, xid,
                    (action_code == REP_SEND_INPROG));
        break;

      case REP_SEND_DACK:
        ret = usockif_senddataack(ufd, result, xid, ackinfo);
        break;

    case REP_SEND_ACK_TXREADY:
        ret = usockif_sendack(ufd, result, xid, false);
        usockif_sendevent(ufd, ackinfo->usockid, USRSOCK_EVENT_SENDTO_READY);
        break;

    case REP_SEND_DACK_RXREADY:
        ret = usockif_senddataack(ufd, result, xid, ackinfo);
        usockif_sendevent(ufd, ackinfo->usockid,
                          USRSOCK_EVENT_RECVFROM_AVAIL);
        break;
    }

  return ret;
}

/****************************************************************************
 * name: perform_usockrequest
 ****************************************************************************/

int perform_usockrequest(FAR struct alt1250_s *dev)
{
  int ret = OK;
  int32_t usock_result;
  uint32_t usock_xid;
  struct usock_ackinfo_s ackinfo;

  if (!IS_USOCKREQ_RECEIVED(dev))
    {
      ret = usockif_readrequest(dev->usockfd, &dev->usockreq);
      ASSERT(ret >= 0);
      if (IS_IOCTLREQ(dev->usockreq))
        {
          ret = usockif_readreqioctl(dev->usockfd, &dev->usockreq);
          if (ret < 0)
            {
              /* In unsupported ioctl command case */

              usock_reply(dev->usockfd, REP_SEND_ACK_WOFREE, ret,
                          USOCKREQXID(dev->usockreq), NULL);
              return OK;
            }
        }

      dev->is_usockrcvd = true;
    }

  if (!IS_REQID_VALID(dev->usockreq))
    {
      ret = REP_SEND_ACK;
      usock_result = -EINVAL;
      usock_xid = USOCKREQXID(dev->usockreq);
    }
  else
    {
      ret = handlers[USOCKREQID(dev->usockreq)](dev, &dev->usockreq,
                                                &usock_result, &usock_xid,
                                                &ackinfo);
    }

  ASSERT(ret >= 0);

  usock_reply(dev->usockfd, ret, usock_result, usock_xid, &ackinfo);

  return ret;
}
