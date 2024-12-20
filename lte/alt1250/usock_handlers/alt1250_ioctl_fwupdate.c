/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_fwupdate.c
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
#include <nuttx/crc32.h>
#include <nuttx/net/usrsock.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_fwupdate.h"
#include "alt1250_container.h"
#include "alt1250_socket.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_devif.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DELTA_MAGICNO (('D') + ('T' << 8) + ('F' << 16) + ('W' << 24))
#define IS_IN_RANGE(a, l, h) (((a) >= (l)) && ((a) < (h)))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: postprochdlr_fwgetimglen
 ****************************************************************************/

static int postproc_fwgetimglen(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *reply,
                                FAR struct usock_s *usock,
                                FAR int32_t *usock_result,
                                FAR uint32_t *usock_xid,
                                FAR struct usock_ackinfo_s *ackinfo,
                                unsigned long arg)
{
  if (CONTAINER_RESPRES(reply) >= 0)
    {
      /* When modem returned non-zero value,
       * some data have already injected.
       * But additional header is not counted.
       * Here is adjustement code for it.
       */

      if (dev->fwup_info.act_injected == -1)
        {
          /* First call */

          dev->fwup_info.act_injected = CONTAINER_RESPRES(reply);
          dev->fwup_info.hdr_injected = (dev->fwup_info.act_injected > 0) ?
            sizeof(struct delta_header_s) : 0;
        }
      else if (dev->fwup_info.act_injected >= LTE_IMAGE_PERT_SIZE)
        {
          /* Injection is already done until pert area. */

          if (CONTAINER_RESPRES(reply) == 0)
            {
              /* Modem should returl non-zero value.
               * This is error case because of mis-matched.
               */

              /* Notice: This case can be happened in normal case
               * when injection done just until pert area.
               * But it can't be rescued.
               */

              dev->fwup_info.hdr_injected = 0;
              dev->fwup_info.act_injected = -1;
            }
          else
            {
              dev->fwup_info.act_injected = CONTAINER_RESPRES(reply);
            }
        }

      if (dev->fwup_info.act_injected == -1)
        {
          CONTAINER_RESPRES(reply) = -ENODATA;
        }
      else
        {
          /* Reply size should include header injected size */

          CONTAINER_RESPRES(reply) =
            dev->fwup_info.act_injected + dev->fwup_info.hdr_injected;
        }
    }
  else
    {
      /* In error case from modem, injection is reset. */

      dev->fwup_info.hdr_injected = 0;
      dev->fwup_info.act_injected = -1;
    }

  *usock_result = CONTAINER_RESPRES(reply);

  return REP_SEND_ACK;
}

/****************************************************************************
 * Name: postprochdlr_fwupdate_injection
 ****************************************************************************/

static int postproc_fwupdate_injection(FAR struct alt1250_s *dev,
                                       FAR struct alt_container_s *reply,
                                       FAR struct usock_s *usock,
                                       FAR int32_t *usock_result,
                                       FAR uint32_t *usock_xid,
                                       FAR struct usock_ackinfo_s *ackinfo,
                                       unsigned long arg)
{
  int app_injected = (int)arg;

  if (CONTAINER_RESPRES(reply) < 0)
    {
      dev->fwup_info.hdr_injected = 0;
      dev->fwup_info.act_injected = -1;
    }
  else if (app_injected >= 0)
    {
      CONTAINER_RESPRES(reply) = app_injected;
    }

  *usock_result = CONTAINER_RESPRES(reply);

  return REP_SEND_ACK;
}

/****************************************************************************
 * Name: fwupdate_dummy_postproc
 ****************************************************************************/

static int postproc_fwupdate_dummy(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *reply,
                                  FAR struct usock_s *usock,
                                  FAR int32_t *usock_result,
                                  FAR uint32_t *usock_xid,
                                  FAR struct usock_ackinfo_s *ackinfo,
                                  unsigned long arg)
{
  *usock_result = CONTAINER_RESPRES(reply);

  return REP_SEND_ACK;
}

/****************************************************************************
 * Name: store_injection_data
 ****************************************************************************/

static bool store_injection_data(FAR char *dist, FAR int *ofst, int sz,
    FAR uint8_t **src, FAR int *len)
{
  bool ret = false;
  int rest;

  if (IS_IN_RANGE(*ofst, 0, sz))
    {
      rest = sz - *ofst;
      rest = (rest > *len) ? *len : rest;
      memcpy(&dist[*ofst], *src, rest);
      *len -= rest;
      *ofst += rest;
      *src += rest;
      if (*ofst == sz)
        {
          ret = true;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: verify_header_crc32
 ****************************************************************************/

static bool verify_header_crc32(FAR struct alt1250_s *dev)
{
  uint32_t crc_result = crc32((uint8_t *)&dev->fwup_info.hdr,
      sizeof(struct delta_header_s));

  return (crc_result == 0);
}

/****************************************************************************
 * Name: verify_pert_crc32
 ****************************************************************************/

static bool verify_pert_crc32(FAR struct alt1250_s *dev)
{
  uint32_t crc_result = crc32((uint8_t *)dev->fwup_info.img_pert,
                              LTE_IMAGE_PERT_SIZE);
  return (crc_result == dev->fwup_info.hdr.pert_crc);
}

/****************************************************************************
 * Name: fwupdate_header_injection
 ****************************************************************************/

static int fwupdate_header_injection(FAR struct alt1250_s *dev,
                                    FAR struct lte_ioctl_data_s *cmd,
                                    FAR bool *send_pert)
{
  FAR uint8_t *data = (FAR uint8_t *)cmd->inparam[0];
  int len = *(FAR int *)cmd->inparam[1];
  bool init = *(FAR bool *)cmd->inparam[2];
  bool filled_all;

  if (init)
    {
      dev->fwup_info.act_injected = 0;
      dev->fwup_info.hdr_injected = 0;
    }

  *send_pert = false;

  /* Make sure lte_get_version() and
   * ltefwupdate_injected_datasize() have been already called.
   */

  if ((dev->fw_version[0] == 'R') && (dev->fw_version[1] == 'K')
      && (dev->fwup_info.act_injected != -1) && (len > 0))
    {
      /* Check if under header injection phase */

      if (dev->fwup_info.act_injected == 0)
        {
          filled_all
            = store_injection_data((char *)&dev->fwup_info.hdr,
                                   &dev->fwup_info.hdr_injected,
                                   sizeof(struct delta_header_s), &data,
                                   &len);
          if (filled_all)
            {
              if (!verify_header_crc32(dev) ||
                  (dev->fwup_info.hdr.chunk_code != DELTA_MAGICNO) ||
                  strncmp(dev->fw_version, dev->fwup_info.hdr.np_package,
                    LTE_VER_NP_PACKAGE_LEN))
                {
                  /* CRC error or Version mis-match */

                  dbg_alt1250("FWUP: Error CRC Header\n");
                  dev->fwup_info.hdr_injected = 0;
                  dev->fwup_info.act_injected = -1;
                  return -EINVAL;
                }
            }
        }

      /* Check if under pert injection phase */

      if ((dev->fwup_info.hdr_injected >= sizeof(struct delta_header_s)) &&
          (len > 0))
        {
          filled_all
            = store_injection_data(dev->fwup_info.img_pert,
                                   &dev->fwup_info.act_injected,
                                   LTE_IMAGE_PERT_SIZE, &data, &len);
          if (filled_all)
            {
              if (!verify_pert_crc32(dev))
                {
                  /* CRC error */

                  dbg_alt1250("FWUP: Error CRC Pertition\n");
                  dev->fwup_info.hdr_injected = 0;
                  dev->fwup_info.act_injected = -1;
                  return -EINVAL;
                }
              else
                {
                  *send_pert = true;
                }
            }
        }

      return len;
    }
  else
    {
      dbg_alt1250("FWUP: Not initialized.\n");
      return -ENODATA;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: usockreq_ioctl_fwupdate
 ****************************************************************************/

int usockreq_ioctl_fwupdate(FAR struct alt1250_s *dev,
                            FAR struct usrsock_request_buff_s *req,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  FAR struct usock_s *usock = NULL;
  int ret = REP_SEND_ACK_WOFREE;
  FAR struct alt_container_s *container;
  postproc_hdlr_t postproc_hdlr = NULL;
  bool send_cmd = true;
  int postproc_priv = -1;

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

  /* FW Update APIs are all synchronouse. */

  if ((ltecmd->cmdid & LTE_CMDOPT_ASYNC_BIT) || (ltecmd->cb != NULL))
    {
      *usock_result = -EINVAL;
      return REP_SEND_ACK_WOFREE;
    }

  container = container_alloc();
  if (container == NULL)
    {
      dbg_alt1250("no container\n");
      return REP_NO_CONTAINER;
    }

  /* Special operation for each commands */

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_INJECTIMAGE:
        {
          *usock_result = fwupdate_header_injection(dev, ltecmd, &send_cmd);
          if (*usock_result < 0)
            {
              container_free(container);
              return REP_SEND_ACK_WOFREE;
            }
          else
            {
              /* That send_cmd is set to true means
               * that pert image should be sent.
               */

              if (send_cmd)
                {
                  /* Need to care for reply value as actual injected
                   * size from application point of view
                   */

                  postproc_priv
                    = *(FAR int *)ltecmd->inparam[1] - *usock_result;
                  ltecmd->inparam[0] = dev->fwup_info.img_pert;
                  *(FAR int *)ltecmd->inparam[1] = LTE_IMAGE_PERT_SIZE;
                  *(FAR bool *)ltecmd->inparam[2] = true;
                }
              else if (*usock_result > 0)
                {
                  send_cmd = true;
                }
              else  /* In case of *result == 0 */
                {
                  /* No send injecting data to modem,
                   * but the data is all accepted.
                   */

                  *usock_result = *(FAR int *)ltecmd->inparam[1];
                  container_free(container);
                }

              postproc_hdlr = postproc_fwupdate_injection;
            }
        }
        break;

      case LTE_CMDID_GETIMAGELEN:
        {
          postproc_hdlr = postproc_fwgetimglen;
        }
        break;

      case LTE_CMDID_EXEUPDATE:
        {
          /* When execution starts, injection is reset */

          reset_fwupdate_info(dev);
          postproc_hdlr = postproc_fwupdate_dummy;
        }
        break;

      default:
        {
          *usock_result = -EINVAL;
          container_free(container);
          return REP_SEND_ACK_WOFREE;
        }
        break;
    }

  if (send_cmd)
    {
      USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);

      set_container_ids(container, USOCKET_USOCKID(usock), ltecmd->cmdid);
      set_container_argument(container, ltecmd->inparam, ltecmd->inparamlen);
      set_container_response(container, ltecmd->outparam,
                             ltecmd->outparamlen);
      set_container_postproc(container, postproc_hdlr, postproc_priv);

      ret = altdevice_send_command(dev, dev->altfd, container, usock_result);

      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }
    }

  return ret;
}

void reset_fwupdate_info(FAR struct alt1250_s *dev)
{
  dev->fwup_info.hdr_injected = 0;
  dev->fwup_info.act_injected = -1;
  dev->fw_version[0] = '\0';
}
