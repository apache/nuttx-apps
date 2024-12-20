/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_denyinetsock.c
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
#include "alt1250_netdev.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl_denyinetsock
 ****************************************************************************/

int usockreq_ioctl_denyinetsock(FAR struct alt1250_s *dev,
                                FAR struct usrsock_request_buff_s *req,
                                FAR int32_t *usock_result,
                                FAR uint32_t *usock_xid,
                                FAR struct usock_ackinfo_s *ackinfo)
{
  uint8_t sock_type = req->req_ioctl.sock_type;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;

  if (sock_type == DENY_INET_SOCK_ENABLE)
    {
      /* Block to create INET socket */

      dev->usock_enable = FALSE;
    }
  else if (sock_type == DENY_INET_SOCK_DISABLE)
    {
      /* Allow to create INET socket */

      dev->usock_enable = TRUE;
    }
  else
    {
      dbg_alt1250("unexpected sock_type:0x%02x\n", sock_type);
      *usock_result = -EINVAL;
    }

  return ret;
}
