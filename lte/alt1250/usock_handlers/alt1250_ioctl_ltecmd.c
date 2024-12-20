/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_ltecmd.c
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

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

extern int usockreq_ioctl_extend(FAR struct alt1250_s *dev,
                                 FAR struct usrsock_request_buff_s *req,
                                 FAR int32_t *usock_result,
                                 FAR uint32_t *usock_xid,
                                 FAR struct usock_ackinfo_s *ackinfo);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl_ltecmd
 ****************************************************************************/

int usockreq_ioctl_ltecmd(FAR struct alt1250_s *dev,
                          FAR struct usrsock_request_buff_s *req,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  int ret = REP_SEND_ACK_WOFREE;
  usrsock_reqhandler_t ioctl_subhdlr = NULL;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = -EINVAL;
  *usock_xid = request->head.xid;

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
  if (dev->is_resuming &&
      !IS_LTE_REPORT_EVENT(ltecmd->cmdid) &&
      ltecmd->cmdid != LTE_CMDID_SETEVTCTX &&
      ltecmd->cmdid != LTE_CMDID_RESUME)
    {
      dbg_alt1250("Don't allow to call any API in resuming "
                  "phase.(cmdid=%lx)\n",
                  ltecmd->cmdid);
      return ret;
    }
#endif

  if (LTE_ISCMDGRP_NORMAL(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_normal;
    }
  else if (LTE_ISCMDGRP_EVENT(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_event;
    }
  else if (LTE_ISCMDGRP_NOMDM(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_other;
    }
  else if (LTE_ISCMDGRP_POWER(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_power;
    }
  else if (LTE_ISCMDGRP_FWUPDATE(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_fwupdate;
    }
  else if (LTE_ISCMDGRP_LWM2M(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_lwm2m;
    }
#ifdef CONFIG_LTE_ALT1250_EXTEND_IOCTL
  else if (LTE_ISCMDGRP_EXTEND(ltecmd->cmdid))
    {
      ioctl_subhdlr = usockreq_ioctl_extend;
    }
#endif

  if (ioctl_subhdlr != NULL)
    {
      ret = ioctl_subhdlr(dev, req, usock_result, usock_xid, ackinfo);
    }

  return ret;
}

