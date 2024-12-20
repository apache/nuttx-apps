/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_power.c
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
#include "alt1250_devif.h"
#include "alt1250_evt.h"
#include "alt1250_ioctl_subhdlr.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_netdev.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl_power
 ****************************************************************************/

int usockreq_ioctl_power(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;
  *usock_xid = request->head.xid;

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_POWERON:
      case LTE_CMDID_TAKEWLOCK:
      case LTE_CMDID_GIVEWLOCK:
      case LTE_CMDID_COUNTWLOCK:
        *usock_result = altdevice_powercontrol(dev->altfd, ltecmd->cmdid);
        if (MODEM_STATE_IS_POFF(dev) && ltecmd->cmdid == LTE_CMDID_POWERON)
          {
            /* For special reset sequence on power-on case */

            MODEM_STATE_B4PON(dev);
          }
        break;

      case LTE_CMDID_POWEROFF:
        alt1250_clrevtcb(ALT1250_CLRMODE_WO_RESTART);
        *usock_result = altdevice_powercontrol(dev->altfd, ltecmd->cmdid);
        MODEM_STATE_POFF(dev);
        alt1250_netdev_ifdown(dev);
        break;

      case LTE_CMDID_FIN:
        alt1250_clrevtcb(ALT1250_CLRMODE_ALL);
        ret = REP_SEND_TERM;
        MODEM_STATE_POFF(dev);
        break;

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
      case LTE_CMDID_RETRYDISABLE:
      case LTE_CMDID_GET_POWER_STAT:
        *usock_result = altdevice_powercontrol(dev->altfd, ltecmd->cmdid);
        break;
#endif

      default:
        *usock_result = -EINVAL;
        break;
    }

  return ret;
}
