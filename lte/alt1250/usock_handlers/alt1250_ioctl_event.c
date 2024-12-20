/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_event.c
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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_eventnotice_command
 ****************************************************************************/

static int send_eventnotice_command(FAR struct alt1250_s *dev,
                                    FAR struct alt_container_s *container,
                                    FAR struct usock_s *usock,
                                    FAR struct lte_ioctl_data_s *ltecmd,
                                    FAR int32_t *usock_result)
{
  set_container_ids(container, USOCKET_USOCKID(usock), ltecmd->cmdid);
  set_container_argument(container, ltecmd->inparam, ltecmd->inparamlen);
  set_container_response(container, ltecmd->outparam, ltecmd->outparamlen);
  set_container_postproc(container, NULL, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl_event
 ****************************************************************************/

int usockreq_ioctl_event(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  FAR struct usock_s *usock = NULL;
  int ret = REP_SEND_ACK_WOFREE;
  uint32_t cmdid = LTE_PURE_CMDID(ltecmd->cmdid);
  FAR struct alt_container_s *container;

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

  ret = alt1250_regevtcb(cmdid, ltecmd->cb);
  if (ret < 0)
    {
      *usock_result = ret;
      return REP_SEND_ACK_WOFREE;
    }

#ifdef CONFIG_LTE_ALT1250_ENABLE_HIBERNATION_MODE
  if (dev->is_resuming)
    {
      /* In resuming phase, report API callback can be registered without
       * sending modem command. Modem command will be sent in calling
       * resume API.
       */

      if (ltecmd->cmdid == LTE_CMDID_REPQUAL)
        {
          dev->quality_report_period = *((uint32_t *)ltecmd->inparam[1]);
        }
      else if (ltecmd->cmdid == LTE_CMDID_REPCELL)
        {
          dev->cellinfo_report_period = *((uint32_t *)ltecmd->inparam[1]);
        }

      *usock_result = ret;
      return REP_SEND_ACK_WOFREE;
    }
#endif

  container = container_alloc();
  if (container == NULL)
    {
      dbg_alt1250("no container\n");
      alt1250_regevtcb(cmdid, NULL);
      return REP_NO_CONTAINER;
    }

  USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);

  if (IS_LWM2M_EVENT(cmdid))
    {
      ret = send_m2mnotice_command(cmdid, dev, container, usock, ltecmd,
                                   usock_result);
    }
  else
    {
      ret = send_eventnotice_command(dev, container, usock, ltecmd,
                                   usock_result);
    }

  if (IS_NEED_CONTAINER_FREE(ret))
    {
      container_free(container);
    }

  if (*usock_result == -ENOSYS)
    {
      /* -ENOSYS means that there is no composer.
       * There are cases where the LAPI command ID group is an event,
       * but no composer is needed.
       * In that case, it is necessary to send OK to usrsock after
       * registering the callback function.
       */

      ret = REP_SEND_ACK_WOFREE;
      *usock_result = OK;
    }
  else if (*usock_result < 0)
    {
      /* clear callback */

      alt1250_regevtcb(cmdid, NULL);
    }

  return ret;
}
