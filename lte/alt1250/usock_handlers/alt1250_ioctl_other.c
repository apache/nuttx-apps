/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_other.c
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
#include "alt1250_util.h"
#include "alt1250_ioctl_subhdlr.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: perform_setrestart
 ****************************************************************************/

static int perform_setrestart(FAR struct alt1250_s *dev,
                              FAR struct usrsock_request_buff_s *req,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;

  *usock_result = alt1250_regevtcb(ltecmd->cmdid, ltecmd->cb);

  return REP_SEND_ACK_WOFREE;
}

/****************************************************************************
 * name: perform_seteventctx
 ****************************************************************************/

static int perform_seteventctx(FAR struct alt1250_s *dev,
                               FAR struct usrsock_request_buff_s *req,
                               FAR int32_t *usock_result,
                               FAR uint32_t *usock_xid,
                               FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  FAR struct lte_evtctx_in_s *in = ltecmd->inparam[0];
  FAR struct lte_evtctx_out_s *out = ltecmd->outparam[0];

  if ((in != NULL) && (in->mqname != NULL) && (out != NULL))
    {
      /* No need to check strlen here.
       * On current NuttX(v10.1) checks strlen of mssage queue name
       * as MAX_MQUEUE_PATH.
       */

      *usock_result = alt1250_evttask_msgconnect(in->mqname, dev);
      out->handle = alt1250_execcb;
    }

  return REP_SEND_ACK_WOFREE;
}

/****************************************************************************
 * name: perform_geterrinfo
 ****************************************************************************/

static int perform_geterrinfo(FAR struct alt1250_s *dev,
                              FAR struct usrsock_request_buff_s *req,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;

  void **arg = alt1250_getevtarg(LTE_CMDID_GETERRINFO);

  if (arg && arg[0] && ltecmd->outparam && ltecmd->outparam[0])
    {
      memcpy(ltecmd->outparam[0], arg[0], sizeof(lte_errinfo_t));
      *usock_result = OK;
    }

  return REP_SEND_ACK_WOFREE;
}

/****************************************************************************
 * name: perform_saveapn
 ****************************************************************************/

static int perform_saveapn(FAR struct alt1250_s *dev,
                           FAR struct usrsock_request_buff_s *req,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;

  if (ltecmd->inparam && ltecmd->inparam[0])
    {
      alt1250_saveapn(dev,
                      (FAR lte_apn_setting_t *)(ltecmd->inparam[0]));
      *usock_result = OK;
    }

  return REP_SEND_ACK_WOFREE;
}

/****************************************************************************
 * name: perform_getapn
 ****************************************************************************/

static int perform_getapn(FAR struct alt1250_s *dev,
                          FAR struct usrsock_request_buff_s *req,
                          FAR int32_t *usock_result,
                          FAR uint32_t *usock_xid,
                          FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;

  if (ltecmd->outparam && ltecmd->outparam[0])
    {
      alt1250_getapn(dev,
                     (FAR lte_apn_setting_t *)(ltecmd->outparam[0]));
      *usock_result = OK;
    }

  return REP_SEND_ACK_WOFREE;
}

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
/****************************************************************************
 * name: perform_setctxcb
 ****************************************************************************/

static int perform_setctxcb(FAR struct alt1250_s *dev,
                            FAR struct usrsock_request_buff_s *req,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;

  *usock_result = alt1250_set_context_save_cb(dev, ltecmd->cb);

  return REP_SEND_ACK_WOFREE;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: alt1250_geterrinfo
 ****************************************************************************/

void alt1250_geterrinfo(FAR lte_errinfo_t *errinfo)
{
  void **arg = alt1250_getevtarg(LTE_CMDID_GETERRINFO);

  if (arg && arg[0])
    {
      memcpy(errinfo, arg[0], sizeof(lte_errinfo_t));
    }
}

/****************************************************************************
 * name: usockreq_ioctl_other
 ****************************************************************************/

int usockreq_ioctl_other(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  int ret = REP_SEND_ACK_WOFREE;
  usrsock_reqhandler_t func = NULL;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = -EINVAL;
  *usock_xid = request->head.xid;

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_SETRESTART:
        func = perform_setrestart;
        break;

      case LTE_CMDID_SETEVTCTX:
        func = perform_seteventctx;
        break;

      case LTE_CMDID_GETERRINFO:
        func = perform_geterrinfo;
        break;

      case LTE_CMDID_SAVEAPN:
        func = perform_saveapn;
        break;

      case LTE_CMDID_GETAPN:
        func = perform_getapn;
        break;

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
      case LTE_CMDID_SETCTXCB:
        func = perform_setctxcb;
        break;
#endif

      default:
        break;
    }

  if (func != NULL)
    {
      ret = func(dev, req, usock_result, usock_xid, ackinfo);
    }

  return ret;
}
