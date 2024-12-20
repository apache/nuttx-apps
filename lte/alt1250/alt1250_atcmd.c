/****************************************************************************
 * apps/lte/alt1250/alt1250_atcmd.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_atcmd.h"
#include "alt1250_devif.h"
#include "alt1250_postproc.h"
#include "alt1250_container.h"
#include "alt1250_usockevent.h"

#include <lte/lte_lwm2m.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct atcmd_postprocarg_t
{
  atcmd_postproc_t proc;
  unsigned long arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR void *atcmd_oargs[3];
static int atcmd_reply_len;
static struct atcmd_postprocarg_t postproc_argument;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: postproc_internal_atcmd
 ****************************************************************************/

static int postproc_internal_atcmd(FAR struct alt1250_s *dev,
                                   FAR struct alt_container_s *reply,
                                   FAR struct usock_s *usock,
                                   FAR int32_t *usock_result,
                                   FAR uint32_t *usock_xid,
                                   FAR struct usock_ackinfo_s *ackinfo,
                                   unsigned long arg)
{
  int ret = REP_NO_ACK;
  FAR struct atcmd_postprocarg_t *parg =
    (FAR struct atcmd_postprocarg_t *)arg;

  dev->recvfrom_processing = false;

  err_alt1250("Internal ATCMD Resp : %s\n", (FAR char *)reply->outparam[0]);

  if (parg->proc != NULL)
    {
      ret = parg->proc(dev, reply,
        (FAR char *)reply->outparam[0], *(FAR int *)reply->outparam[2],
        parg->arg, usock_result);
    }

  return ret;
}

/****************************************************************************
 * name: get_m2mrespstr
 ****************************************************************************/

static FAR const char *get_m2mrespstr(int resp)
{
  FAR const char *ret = NULL;

  switch (resp)
    {
      case LWM2MSTUB_RESP_CHANGED:
        ret = "2.04";
        break;
      case LWM2MSTUB_RESP_CONTENT:
        ret = "2.05";
        break;
      case LWM2MSTUB_RESP_BADREQ:
        ret = "4.00";
        break;
      case LWM2MSTUB_RESP_UNAUTH:
        ret = "4.01";
        break;
      case LWM2MSTUB_RESP_NOURI:
        ret = "4.04";
        break;
      case LWM2MSTUB_RESP_NOTALLOW:
        ret = "4.05";
        break;
      case LWM2MSTUB_RESP_NOTACCEPT:
        ret = "4.06";
        break;
      case LWM2MSTUB_RESP_UNSUPPORT:
        ret = "4.15";
        break;
      case LWM2MSTUB_RESP_INTERNALERROR:
        ret = "5.00";
        break;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: atcmdreply_true_false
 ****************************************************************************/

int atcmdreply_true_false(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *reply,
                          FAR char *rdata, int len, unsigned long arg,
                          FAR int32_t *usock_result)
{
  *usock_result = 0;

  if (strcasestr(rdata, "\nTRUE\r"))
    {
      *usock_result = 1;
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: atcmdreply_ok_error
 ****************************************************************************/

int atcmdreply_ok_error(FAR struct alt1250_s *dev,
                        FAR struct alt_container_s *reply,
                        FAR char *rdata, int len, unsigned long arg,
                        FAR int32_t *usock_result)
{
  *usock_result = check_atreply_ok(rdata, len, NULL);
  return REP_SEND_ACK;
}

/****************************************************************************
 * name: check_atreply_ok
 ****************************************************************************/

int check_atreply_ok(FAR char *reply, int len, FAR void *arg)
{
  int ret = ERROR;

  if (strstr(reply, "\nOK\r"))
    {
      ret = OK;
    }

  return ret;
}

/****************************************************************************
 * name: check_atreply_truefalse
 ****************************************************************************/

int check_atreply_truefalse(FAR char *reply, int len, FAR void *arg)
{
  int ret = ERROR;
  FAR struct atreply_truefalse_s *result =
    (FAR struct atreply_truefalse_s *)arg;

  if (check_atreply_ok(reply, len, NULL) == OK)
    {
      ret = OK;
      if (strstr(reply, result->target_str))
        {
          result->result = true;
        }
      else
        {
          result->result = false;
        }
    }

  return ret;
}

/****************************************************************************
 * name: send_internal_at_command
 ****************************************************************************/

static int send_internal_at_command(FAR struct alt1250_s *dev,
      FAR struct alt_container_s *container, int16_t usockid,
      atcmd_postproc_t proc, unsigned long arg, FAR int32_t *usock_result)
{
  int ret;

  FAR void *inparam[2];

  inparam[0] = dev->tx_buff;
  inparam[1] = (FAR void *)strlen((FAR const char *)dev->tx_buff);

  atcmd_oargs[0] = dev->rx_buff;
  atcmd_oargs[1] = (FAR void *)_RX_BUFF_SIZE;
  atcmd_oargs[2] = &atcmd_reply_len;

  postproc_argument.proc = proc;
  postproc_argument.arg = arg;

  set_container_ids(container, usockid, LTE_CMDID_SENDATCMD);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, atcmd_oargs, nitems(atcmd_oargs));
  set_container_postproc(container, postproc_internal_atcmd,
                         (unsigned long)&postproc_argument);

  err_alt1250("Internal ATCMD : %s\n", dev->tx_buff);

  ret = altdevice_send_command(dev, dev->altfd, container, usock_result);
  if (ret == REP_NO_ACK)
    {
      /* In case of no error */

      dev->recvfrom_processing = true;
    }

  return ret;
}

/****************************************************************************
 * name: lwm2mstub_send_reset
 ****************************************************************************/

int lwm2mstub_send_reset(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE, "ATZ\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_setenable
 ****************************************************************************/

int lwm2mstub_send_setenable(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container, bool en)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETACFG=modem_apps.LWM2M.AppEnable,\"%s\"\r",
           en ? "true" : "false");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_getenable
 ****************************************************************************/

int lwm2mstub_send_getenable(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             FAR int32_t *usock_result)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=modem_apps.LWM2M.AppEnable\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, usock_result);
}

/****************************************************************************
 * name: lwm2mstub_send_getnamemode
 ****************************************************************************/

int lwm2mstub_send_getnamemode(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.Config.NameMode\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_setnamemode
 ****************************************************************************/

int lwm2mstub_send_setnamemode(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               int mode)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETACFG=LWM2M.Config.NameMode,%d\r", mode);
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_getversion
 ****************************************************************************/

int lwm2mstub_send_getversion(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.Config.Version\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_setversion
 ****************************************************************************/

int lwm2mstub_send_setversion(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              bool is_v1_1)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
      "AT%%SETACFG=LWM2M.Config.Version,\"%s\"\r", is_v1_1 ? "1.1" : "1.0");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_getwriteattr
 ****************************************************************************/

int lwm2mstub_send_getwriteattr(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.HostObjects.HostEnableWriteAttrURCMode\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_setwriteattr
 ****************************************************************************/

int lwm2mstub_send_setwriteattr(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                bool en)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
      "AT%%SETACFG=LWM2M.HostObjects.HostEnableWriteAttrURCMode,\"%s\"\r",
      en ? "true" : "false");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_getautoconnect
 ****************************************************************************/

int lwm2mstub_send_getautoconnect(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.Config.AutoConnect\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_setautoconnect
 ****************************************************************************/

int lwm2mstub_send_setautoconnect(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container,
                                  bool en)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETACFG=LWM2M.Config.AutoConnect,\"%s\"\r",
           en ? "true" : "false");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: ltenwop_send_getnwop
 ****************************************************************************/

int ltenwop_send_getnwop(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%NWOPER?\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: ltenwop_send_setnwoptp
 ****************************************************************************/

int ltenwop_send_setnwoptp(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%NWOPER=\"TRUPHONE\"\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: ltesp_send_getscanplan
 ****************************************************************************/

int ltesp_send_getscanplan(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETCFG=\"SCAN_PLAN_EN\"\r");
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: ltesp_send_setscanplan
 ****************************************************************************/

int ltesp_send_setscanplan(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           bool enable)
{
  int32_t dummy;
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETCFG=\"SCAN_PLAN_EN\",\"%s\"\r", (enable ? "1" : "0"));
  return send_internal_at_command(dev, container, -1, NULL, 0, &dummy);
}

/****************************************************************************
 * name: lwm2mstub_send_getqueuemode
 ****************************************************************************/

int lwm2mstub_send_getqueuemode(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.TransportBindings_1_1.Queue\r");
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_true_false, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_setqueuemode
 ****************************************************************************/

int lwm2mstub_send_setqueuemode(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures, int en)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETACFG=LWM2M.TransportBindings_1_1.Queue,%s\r",
           (en == 1) ? "true" : "false");
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_m2mopev
 ****************************************************************************/

int lwm2mstub_send_m2mopev(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           int16_t usockid, FAR int32_t *ures, bool en)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MOPEV=%c,100\r", en ? '1' : '0');
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_m2mev
 ****************************************************************************/

int lwm2mstub_send_m2mev(FAR struct alt1250_s *dev,
                         FAR struct alt_container_s *container,
                         int16_t usockid, FAR int32_t *ures, bool en)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MEV=%c\r", en ? '1' : '0');
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_m2mobjcmd
 ****************************************************************************/

int lwm2mstub_send_m2mobjcmd(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures, bool en)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MOBJCMD=%c\r", en ? '1' : '0');
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getepname
 ****************************************************************************/

int lwm2mstub_send_getepname(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, atcmd_postproc_t proc,
                             unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.Config.Name\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getsrvinfo
 ****************************************************************************/

int lwm2mstub_send_getsrvinfo(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, atcmd_postproc_t proc,
                              unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MCMD=SERVERSINFO\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getresource
 ****************************************************************************/

int lwm2mstub_send_getresource(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               int16_t usockid, atcmd_postproc_t proc,
                               unsigned long arg, FAR int32_t *ures,
                               FAR char *resource)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MCMD=GET_RESOURCE,%s\r", resource);
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getsupobjs
 ****************************************************************************/

int lwm2mstub_send_getsupobjs(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, atcmd_postproc_t proc,
                              unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%GETACFG=LWM2M.Config.SupportedObjects\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getobjdef
 ****************************************************************************/

int lwm2mstub_send_getobjdef(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, atcmd_postproc_t proc,
                             unsigned long arg, FAR int32_t *ures,
                             uint16_t objid)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MOBJDEF=GET,%d\r", objid);
  return send_internal_at_command(dev, container, usockid, proc,
                                  arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_changerat
 ****************************************************************************/

int lwm2mstub_send_changerat(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures, int rat)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%setacfg=radiom.config.preferred_rat_list,\"%s\"\r",
           (rat == LTE_RAT_CATM) ? "CATM" : "NBIOT");

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_getrat
 ****************************************************************************/

int lwm2mstub_send_getrat(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *container,
                          int16_t usockid, atcmd_postproc_t proc,
                          unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%getacfg=radiom.config.preferred_rat_list\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_setepname
 ****************************************************************************/

int lwm2mstub_send_setepname(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures,
                             FAR const char *epname)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%SETACFG=\"LWM2M.Config.Name\",\"%s\"\r", epname);
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_bsstart
 ****************************************************************************/

int lwm2mstub_send_bsstart(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           int16_t usockid, atcmd_postproc_t proc,
                           unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MBSCMD=\"START\"\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_bsdelete
 ****************************************************************************/

int lwm2mstub_send_bsdelete(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            int16_t usockid, atcmd_postproc_t proc,
                            unsigned long arg, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MBSCMD=\"DELETE\"\r");
  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_bscreateobj0
 ****************************************************************************/

int lwm2mstub_send_bscreateobj0(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, atcmd_postproc_t proc,
                                unsigned long arg, FAR int32_t *ures,
                                FAR struct lwm2mstub_serverinfo_s *info)
{
  int i;
  int pos;

  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%LWM2MBSCMD=\"CREATE\",0,0,0,\"%s\",1,\"%s\",2,%d",
                 info->server_uri, info->bootstrap ? "true" : "false",
                 info->security_mode);

  if (info->security_mode != LWM2MSTUB_SECUREMODE_NOSEC)
    {
      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      ",3,\"");
      for (i = 0; i < LWM2MSTUB_MAX_DEVID && info->device_id[i]; i++)
        {
          pos += snprintf((FAR char *)&dev->tx_buff[pos],
                          _TX_BUFF_SIZE - pos,
                          "%02x", info->device_id[i]);
        }

      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      "\"");

      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      ",5,\"");
      for (i = 0; i < LWM2MSTUB_MAX_SEQKEY && info->security_key[i]; i++)
        {
          pos += snprintf((FAR char *)&dev->tx_buff[pos],
                          _TX_BUFF_SIZE - pos,
                          "%02x", info->security_key[i]);
        }

      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      "\"");
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos, ",10,0\r");

  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_bscreateobj1
 ****************************************************************************/

int lwm2mstub_send_bscreateobj1(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, atcmd_postproc_t proc,
                                unsigned long arg, FAR int32_t *ures,
                                FAR struct lwm2mstub_serverinfo_s *info)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MBSCMD=\"CREATE\",1,0,0,0,1,%lu%s\r", info->lifetime,
           info->nonip ? ",7,\"N\",22,\"N\"" : "");

  return send_internal_at_command(dev, container, usockid, proc, arg, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_bsdone
 ****************************************************************************/

int lwm2mstub_send_bsdone(FAR struct alt1250_s *dev,
                          FAR struct alt_container_s *container,
                          int16_t usockid, FAR int32_t *ures)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MBSCMD=\"DONE\"\r");
  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_setsupobjs
 ****************************************************************************/

int lwm2mstub_send_setsupobjs(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, FAR int32_t *ures,
                              FAR uint16_t *objids, int objnum)
{
  int pos;
  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%SETACFG=LWM2M.Config.SupportedObjects,0;1");

  while (objnum > 0)
    {
      /* Object 0 and Object 1 is mandatory and default */

      if (*objids != 0 && *objids != 1)
        {
          pos += snprintf((FAR char *)&dev->tx_buff[pos],
                          _TX_BUFF_SIZE - pos,
                          ";%d", *objids);
        }

      objids++;
      objnum--;
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos, "\r");

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_setobjdef
 ****************************************************************************/

int lwm2mstub_send_setobjdef(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *container,
                             int16_t usockid, FAR int32_t *ures,
                             uint16_t objid, int resnum,
                             FAR struct lwm2mstub_resource_s *resucs)
{
  int pos;

  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%LWM2MOBJDEF=SET,%d", objid);

  while (resnum > 0)
    {
      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
              ",%d,\"%s\",%d,\"%s\"", resucs->res_id,
              resucs->operation == LWM2MSTUB_RESOP_READ ? "R" :
              resucs->operation == LWM2MSTUB_RESOP_WRITE ? "W" :
              resucs->operation == LWM2MSTUB_RESOP_RW ? "RW" : "X",
              resucs->inst_type,
              resucs->data_type == LWM2MSTUB_RESDATA_NONE ? "NONE" :
              resucs->data_type == LWM2MSTUB_RESDATA_STRING ? "STR" :
              resucs->data_type == LWM2MSTUB_RESDATA_INT  ? "INT" :
              resucs->data_type == LWM2MSTUB_RESDATA_UNSIGNED ? "UINT" :
              resucs->data_type == LWM2MSTUB_RESDATA_FLOAT  ? "FLT" :
              resucs->data_type == LWM2MSTUB_RESDATA_BOOL ? "BOOL" :
              resucs->data_type == LWM2MSTUB_RESDATA_OPAQUE ? "OPQ" :
              resucs->data_type == LWM2MSTUB_RESDATA_TIME ? "TIME" : "OL");
      resucs++;
      resnum--;
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos, "\r");

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_registration
 ****************************************************************************/

int lwm2mstub_send_registration(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures, int cmd)
{
  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MCMD=%s\r",
           cmd == LWM2MSTUB_CONNECT_REGISTER   ? "REGISTER" :
           cmd == LWM2MSTUB_CONNECT_DEREGISTER ? "DEREGISTER" :
           cmd == LWM2MSTUB_CONNECT_REREGISTER ? "REGISTERUPD" :
                                                 "BOOTSTARP");

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_evrespwvalue
 ****************************************************************************/

int lwm2mstub_send_evrespwvalue(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures,
                                int seq_no, int resp,
                                FAR struct lwm2mstub_instance_s *inst,
                                FAR char *retval)
{
  int pos;
  FAR const char *resp_str = get_m2mrespstr(resp);

  if (resp_str == NULL)
    {
      *ures = -EINVAL;
      return REP_SEND_ACK;
    }

  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%LWM2MOBJRSP=%d,\"%s\",\"/%d/%d/%d", seq_no, resp_str,
                 inst->object_id, inst->object_inst, inst->res_id);

  if (inst->res_inst >= 0)
    {
      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      "/%d", inst->res_inst);
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
           "\",\"%s\"\r", retval);

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_evresponse
 ****************************************************************************/

int lwm2mstub_send_evresponse(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *container,
                              int16_t usockid, FAR int32_t *ures, int seq_no,
                              int resp,
                              FAR struct lwm2mstub_instance_s *inst)
{
  int pos;
  FAR const char *resp_str = get_m2mrespstr(resp);

  if (resp_str == NULL)
    {
      *ures = -EINVAL;
      return REP_SEND_ACK;
    }

  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%LWM2MOBJRSP=%d,\"%s\",\"/%d/%d/%d", seq_no, resp_str,
                 inst->object_id, inst->object_inst, inst->res_id);

  if (inst->res_inst >= 0)
    {
      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      "/%d", inst->res_inst);
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos, "\"\r");

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_evrespwoinst
 ****************************************************************************/

int lwm2mstub_send_evrespwoinst(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int16_t usockid, FAR int32_t *ures,
                                int seq_no, int resp)
{
  FAR const char *resp_str = get_m2mrespstr(resp);

  if (resp_str == NULL)
    {
      *ures = -EINVAL;
      return REP_SEND_ACK;
    }

  snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
           "AT%%LWM2MOBJRSP=%d,\"%s\"\r", seq_no, resp_str);

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}

/****************************************************************************
 * name: lwm2mstub_send_objevent
 ****************************************************************************/

int lwm2mstub_send_objevent(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *container,
                            int16_t usockid, FAR int32_t *ures,
                            FAR char *token,
                            FAR struct lwm2mstub_instance_s *inst,
                            FAR char *retval)
{
  int pos;

  pos = snprintf((FAR char *)dev->tx_buff, _TX_BUFF_SIZE,
                 "AT%%LWM2MOBJEV=\"%s\",,,0,\"/%d/%d",
                 token, inst->object_id, inst->object_inst);

  if (inst->res_id >= 0)
    {
      pos += snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
                      "/%d", inst->res_id);

      if (inst->res_inst >= 0)
        {
          pos += snprintf((FAR char *)&dev->tx_buff[pos],
                          _TX_BUFF_SIZE - pos,
                          "/%d", inst->res_id);
        }
    }

  snprintf((FAR char *)&dev->tx_buff[pos], _TX_BUFF_SIZE - pos,
           "\",\"%s\"\r", retval);

  return send_internal_at_command(dev, container, usockid,
                                  atcmdreply_ok_error, 0, ures);
}
