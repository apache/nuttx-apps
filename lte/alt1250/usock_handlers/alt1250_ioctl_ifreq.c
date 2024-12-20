/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_ifreq.c
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
#include <unistd.h>

#include "alt1250_dbg.h"
#include "alt1250_container.h"
#include "alt1250_socket.h"
#include "alt1250_devif.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_netdev.h"

/****************************************************************************
 * Private Function Prototype
 ****************************************************************************/

static int postproc_radiooff(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *reply,
                             FAR struct usock_s *usock,
                             FAR int32_t *usock_result,
                             FAR uint32_t *usock_xid,
                             FAR struct usock_ackinfo_s *ackinfo,
                             unsigned long arg);

static int postproc_radioon(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg);

static int postproc_actpdn(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg);

static int postproc_reportnet(FAR struct alt1250_s *dev,
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
 * name: send_actpdn_command
 ****************************************************************************/

static int send_actpdn_command(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               FAR struct usock_s *usock,
                               FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[1];

  /* This member is referenced only when sending a command and
   * not when receiving a response, so local variable is used.
   */

  inparam[0] = &dev->apn;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));
  USOCKET_SET_RESPONSE(usock, idx++, &dev->o_pdn);

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_ACTPDN);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_actpdn, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: send_radio_command
 ****************************************************************************/

static int send_radio_command(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result, bool on)
{
  int idx = 0;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));

  set_container_ids(container, USOCKET_USOCKID(usock),
                    on ? LTE_CMDID_RADIOON : LTE_CMDID_RADIOOFF);
  set_container_argument(container, NULL, 0);
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container,
                    on ? postproc_radioon : postproc_radiooff, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: postproc_radiooff
 ****************************************************************************/

static int postproc_radiooff(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *reply,
                             FAR struct usock_s *usock,
                             FAR int32_t *usock_result,
                             FAR uint32_t *usock_xid,
                             FAR struct usock_ackinfo_s *ackinfo,
                             unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);
  int altcom_result = *((int *)(resp[0]));

  dbg_alt1250("%s start\n", __func__);

  *usock_result = (CONTAINER_RESPRES(reply) == 0) ? altcom_result :
    CONTAINER_RESPRES(reply);

  alt1250_netdev_ifdown(dev);

  return ret;
}

/****************************************************************************
 * name: postproc_actpdn
 ****************************************************************************/

static int postproc_actpdn(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);
  int altcom_result = *((int *)(resp[0]));
  FAR lte_pdn_t *pdn = resp[1];

  dbg_alt1250("%s start\n", __func__);

  *usock_result = (CONTAINER_RESPRES(reply) == 0) ? altcom_result :
     CONTAINER_RESPRES(reply);

  if (*usock_result == 0)
    {
      /* After connecting to the LTE network,
       * wait for the modem to register the network interface.
       */

      usleep(ALT1250_NETIF_READY_DELAY);
      alt1250_netdev_ifup(dev, pdn);
    }

  return ret;
}

/****************************************************************************
 * name: postproc_radioon
 ****************************************************************************/

static int postproc_radioon(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);
  int altcom_result = *((int *)(resp[0]));

  dbg_alt1250("%s start\n", __func__);

  *usock_result = (CONTAINER_RESPRES(reply) == 0) ? altcom_result :
    CONTAINER_RESPRES(reply);

  if (*usock_result == 0)
    {
      ret = send_reportnet_command(dev, reply, usock, postproc_reportnet, 0,
                                   usock_result);
    }

  return ret;
}

/****************************************************************************
 * name: postproc_reportnet
 ****************************************************************************/

static int postproc_reportnet(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *reply,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo,
                              unsigned long arg)
{
  int ret = REP_SEND_ACK;
  FAR void **resp = CONTAINER_RESPONSE(reply);
  int altcom_result = *((int *)(resp[0]));

  dbg_alt1250("%s start\n", __func__);

  *usock_result = (CONTAINER_RESPRES(reply) == 0) ? altcom_result :
    CONTAINER_RESPRES(reply);

  if (*usock_result == 0)
    {
      ret = send_actpdn_command(dev, reply, usock, usock_result);
    }

  return ret;
}

/****************************************************************************
 * name: do_ifup
 ****************************************************************************/

static int do_ifup(FAR struct alt1250_s *dev,
                   FAR struct usrsock_request_buff_s *req,
                   FAR int32_t *usock_result,
                   FAR uint32_t *usock_xid,
                   FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request =
                              &req->request.ioctl_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

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

  container = container_alloc();
  if (container == NULL)
    {
      dbg_alt1250("no container\n");
      return REP_NO_CONTAINER;
    }

  ret = send_radio_command(dev, container, usock, usock_result, true);

  if (IS_NEED_CONTAINER_FREE(ret))
    {
      container_free(container);
    }

  return ret;
}

/****************************************************************************
 * name: do_ifdown
 ****************************************************************************/

static int do_ifdown(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_buff_s *req,
                     FAR int32_t *usock_result,
                     FAR uint32_t *usock_xid,
                     FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request =
                              &req->request.ioctl_req;
  FAR struct usock_s *usock;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

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

  container = container_alloc();
  if (container == NULL)
    {
      dbg_alt1250("no container\n");
      return REP_NO_CONTAINER;
    }

  ret = send_radio_command(dev, container, usock, usock_result, false);

  if (IS_NEED_CONTAINER_FREE(ret))
    {
      container_free(container);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_reportnet_command
 ****************************************************************************/

int send_reportnet_command(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           FAR struct usock_s *usock,
                           FAR postproc_hdlr_t func,
                           unsigned long priv,
                           FAR int32_t *usock_result)
{
  int idx = 0;
  FAR void *inparam[1];
  uint8_t enable = 1;

  /* This member is referenced only when sending a command and
   * not when receiving a response, so local variable is used.
   */

  inparam[0] = &enable;

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REP_RESULT(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_REPNETINFO);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, func, priv);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: usockreq_ioctl_ifreq
 ****************************************************************************/

int usockreq_ioctl_ifreq(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *usock_result,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct ifreq *if_req = &req->req_ioctl.ifreq;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;
  *usock_xid = request->head.xid;

  if (!dev->usock_enable)
    {
      *usock_result = -ENOTTY;
    }
  else if (if_req->ifr_flags & IFF_UP)
    {
      ret = do_ifup(dev, req, usock_result, usock_xid, ackinfo);
    }
  else
    {
      ret = do_ifdown(dev, req, usock_result, usock_xid, ackinfo);
    }

  return ret;
}
