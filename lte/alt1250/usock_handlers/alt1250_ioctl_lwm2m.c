/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_ioctl_lwm2m.c
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

#include <stdlib.h>
#include <string.h>

#include "alt1250_dbg.h"
#include "alt1250_container.h"
#include "alt1250_atcmd.h"
#include "alt1250_ioctl_subhdlr.h"
#include "alt1250_usrsock_hdlr.h"

#include <lte/lte_lwm2m.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: atcmdreply_getepname
 ****************************************************************************/

static int atcmdreply_getepname(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *reply,
                                FAR char *rdata, int len, unsigned long arg,
                                FAR int32_t *usock_result)
{
  char *name;
  void **param = (void **)arg;
  char *outbuf = (char *)param[0];
  int  outbuflen = (int)param[1];

  *usock_result = check_atreply_ok(rdata, len, NULL);
  if (*usock_result == OK)
    {
      name = strtok_r(rdata, " \r\n", &rdata);
      if (name)
        {
          memset(outbuf, 0, outbuflen);
          strncpy(outbuf, name, outbuflen);
          *usock_result = outbuf[outbuflen - 1] == 0
                        ? strlen(outbuf) : outbuflen;
        }
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: strcpy_until
 ****************************************************************************/

static char strcpy_until(char *dst, int n, char **src, char *delim)
{
  char *tmp = *src;

  if (dst)
    {
      dst[n - 1] = '\0';
      n--;
    }

  while (*tmp && !strchr(delim, *tmp))
    {
      if (dst && (n > 0))
        {
          *dst++ = *tmp;
          n--;
        }

      tmp++;
    }

  if (dst && (n > 0))
    {
      *dst = '\0';
    }

  *src = tmp + 1;

  return *tmp;
}

/****************************************************************************
 * name: atcmdreply_getsrvinfo_step4
 ****************************************************************************/

static int atcmdreply_getsrvinfo_step4(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char *tmp;
  int ret = REP_SEND_ACK;
  FAR struct lwm2mstub_serverinfo_s *info
            = (FAR struct lwm2mstub_serverinfo_s *)(((void **)arg)[0]);

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      info->object_inst = 0;

      if ((tmp = strstr(rdata, "%LWM2MCMD: ")) != NULL)
        {
          tmp += strlen("%LWM2MCMD: ");
          info->nonip = strstr(tmp, "\"N\"") ? true : false;

          *usock_result = 0;
        }
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_getsrvinfo_step3
 ****************************************************************************/

static int atcmdreply_getsrvinfo_step3(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char *tmp;
  int ret = REP_SEND_ACK;
  FAR struct lwm2mstub_serverinfo_s *info
            = (FAR struct lwm2mstub_serverinfo_s *)(((void **)arg)[0]);

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      info->object_inst = 0;

      if ((tmp = strstr(rdata, "%LWM2MCMD: ")) != NULL)
        {
          tmp += strlen("%LWM2MCMD: ");
          strcpy_until(NULL, 0, &tmp, ",");
          strcpy_until(NULL, 0, &tmp, ",");
          strcpy_until(NULL, 0, &tmp, ",");
          strcpy_until(NULL, 0, &tmp, ",");
          info->security_mode = atoi(tmp);

          ret = lwm2mstub_send_getresource(dev, reply,
                                           CONTAINER_SOCKETID(reply),
                                           atcmdreply_getsrvinfo_step4,
                                           arg, usock_result, "1,0,22");
        }
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_getsrvinfo_step2
 ****************************************************************************/

static int atcmdreply_getsrvinfo_step2(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char *tmp;
  int ret = REP_SEND_ACK;
  FAR struct lwm2mstub_serverinfo_s *info
            = (FAR struct lwm2mstub_serverinfo_s *)(((void **)arg)[0]);

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      info->object_inst = 0;

      if ((tmp = strstr(rdata, "%LWM2MCMD: ")) != NULL)
        {
          tmp += strlen("%LWM2MCMD: ");
          info->bootstrap = strstr(tmp, "\"false\"") ? false : true;

          ret = lwm2mstub_send_getresource(dev, reply,
                                           CONTAINER_SOCKETID(reply),
                                           atcmdreply_getsrvinfo_step3,
                                           arg, usock_result, "0,0,2");
        }
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_getsrvinfo_step1
 ****************************************************************************/

static int atcmdreply_getsrvinfo_step1(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char *tmp;
  int ret = REP_SEND_ACK;

  FAR struct lwm2mstub_serverinfo_s *info
            = (FAR struct lwm2mstub_serverinfo_s *)(((void **)arg)[0]);

  *usock_result = -EIO;

  /* Expected reply format
   *  %LWM2MCMD: <ServerURL>,<void>,<lifetime>,<binding>,<status>
   */

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      info->object_inst = 0;
      info->device_id[0] = '\0';
      info->security_key[0] = '\0';

      if ((tmp = strstr(rdata, "%LWM2MCMD: ")) != NULL)
        {
          tmp += strlen("%LWM2MCMD: ");

          strcpy_until(info->server_uri, LWM2MSTUB_MAX_SERVER_NAME,
                             &tmp, ",\r\n");
          strcpy_until(NULL, 0, &tmp, ",\r\n"); /* Skip Server ID */
          strcpy_until(NULL, 0, &tmp, ",\r\n"); /* Skip Life time */
          strcpy_until(NULL, 0, &tmp, ",\r\n"); /* Skip Binding */
          info->state = atoi(tmp);

          ret = lwm2mstub_send_getresource(dev, reply,
                                           CONTAINER_SOCKETID(reply),
                                           atcmdreply_getsrvinfo_step2,
                                           arg, usock_result, "0,0,1");
        }
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_getenobjnum
 ****************************************************************************/

static int atcmdreply_getenobjnum(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      *usock_result = 1;

      rdata += 2; /* avoid "\r\n" */
      while (*rdata != '\r')
        {
          if (*rdata == ';')
            {
              (*usock_result)++;
            }

          rdata++;
        }
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: atcmdreply_getenobjects
 ****************************************************************************/

static int atcmdreply_getenobjects(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  int i;
  FAR uint16_t *objids = (FAR uint16_t *)(((void **)arg)[0]);
  int objnum = (int)(((void **)arg)[1]);

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      i = 0;
      rdata += 2; /* avoid "\r\n" */
      objids[i++] = atoi(rdata);
      while (strcpy_until(NULL, 0, &rdata, ";\r\n") == ';' && i < objnum)
        {
          objids[i++] = atoi(rdata);
        }

      *usock_result = i;
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: atcmdreply_getobjresnum
 ****************************************************************************/

static int atcmdreply_getobjresnum(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char term;

  *usock_result = -ENODEV;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      *usock_result = -EIO;
      if ((rdata = strstr(rdata, "%OBJECTDEF: ")) != NULL)
        {
          *usock_result = 0;
          rdata += strlen("%OBJECTDEF: ");
          term = ',';
          while (term == ',')
            {
              if ((term = strcpy_until(NULL, 0, &rdata, ",\r\n")) == ','
               && (term = strcpy_until(NULL, 0, &rdata, ",\r\n")) == ','
               && (term = strcpy_until(NULL, 0, &rdata, ",\r\n")) == ','
               && (term = strcpy_until(NULL, 0, &rdata, ",\r\n")) == ',')
                {
                  /* the commas must be 4 for one resource definitions */

                  (*usock_result)++;
                }
            }
        }
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: resoperation_code
 ****************************************************************************/

static int resoperation_code(const char *s)
{
  return s[0] == 'W' ? LWM2MSTUB_RESOP_WRITE :
         s[0] == 'X' ? LWM2MSTUB_RESOP_EXEC  :
         s[0] != 'R' ? -1  :
         s[1] == 'W' ? LWM2MSTUB_RESOP_RW : LWM2MSTUB_RESOP_READ;
}

/****************************************************************************
 * name: resdatatype_code
 ****************************************************************************/

static int resdatatype_code(const char *s)
{
  int ret = -1;

  if (!strcmp(s, "\"none\""))
    {
      ret = LWM2MSTUB_RESDATA_NONE;
    }
  else if (!strcmp(s, "str"))
    {
      ret = LWM2MSTUB_RESDATA_STRING;
    }
  else if (!strcmp(s, "int"))
    {
      ret = LWM2MSTUB_RESDATA_INT;
    }
  else if (!strcmp(s, "uint"))
    {
      ret = LWM2MSTUB_RESDATA_UNSIGNED;
    }
  else if (!strcmp(s, "flt"))
    {
      ret = LWM2MSTUB_RESDATA_FLOAT;
    }
  else if (!strcmp(s, "bool"))
    {
      ret = LWM2MSTUB_RESDATA_BOOL;
    }
  else if (!strcmp(s, "opq"))
    {
      ret = LWM2MSTUB_RESDATA_OPAQUE;
    }
  else if (!strcmp(s, "time"))
    {
      ret = LWM2MSTUB_RESDATA_TIME;
    }
  else if (!strcmp(s, "ol"))
    {
      ret = LWM2MSTUB_RESDATA_OBJLINK;
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_getobjdef
 ****************************************************************************/

static int atcmdreply_getobjdef(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  char tmp[8];
  char term;
  FAR struct lwm2mstub_resource_s *reses
              = (FAR struct lwm2mstub_resource_s *)(((void **)arg)[0]);
  int resnum = (int)(((void **)arg)[1]);

  *usock_result = -ENODEV;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      *usock_result = -EIO;
      if ((rdata = strstr(rdata, "%OBJECTDEF: ")) != NULL)
        {
          *usock_result = 0;
          rdata += strlen("%OBJECTDEF: ");
          term = strcpy_until(NULL, 0, &rdata, ",\r\n");

          /* <objID>,<resID>,<Type>,<Operation>,<Type> */

          while (term == ',' && *usock_result < resnum)
            {
              reses[*usock_result].res_id = atoi(rdata);
              term = strcpy_until(NULL, 0, &rdata, ",\r\n");

              term = strcpy_until(tmp, 8, &rdata, ",\r\n");
              reses[*usock_result].operation = resoperation_code(tmp);

              reses[*usock_result].inst_type = atoi(rdata);
              term = strcpy_until(NULL, 0, &rdata, ",\r\n");

              term = strcpy_until(tmp, 8, &rdata, ",\r\n");
              reses[*usock_result].data_type = resdatatype_code(tmp);

              (*usock_result)++;
            }
        }
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: atcmdreply_getrat
 ****************************************************************************/

static int atcmdreply_getrat(FAR struct alt1250_s *dev,
                             FAR struct alt_container_s *reply,
                             FAR char *rdata, int len, unsigned long arg,
                             FAR int32_t *usock_result)
{
  *usock_result = -ENODEV;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      *usock_result = -EIO;
      if (strstr(rdata, "CATM") != NULL)
        {
          *usock_result = LTE_RAT_CATM;
        }
      else if (strstr(rdata, "NBIOT") != NULL)
        {
          *usock_result = LTE_RAT_NBIOT;
        }
    }

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: atcmdreply_setsrvinfo_step4
 ****************************************************************************/

static int atcmdreply_setsrvinfo_step4(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK;

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      ret = lwm2mstub_send_bsdone(dev, reply, CONTAINER_SOCKETID(reply),
                                  usock_result);
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_setsrvinfo_step3
 ****************************************************************************/

static int atcmdreply_setsrvinfo_step3(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK;

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      ret = lwm2mstub_send_bscreateobj1(dev, reply,
                                  CONTAINER_SOCKETID(reply),
                                  atcmdreply_setsrvinfo_step4, arg,
                                  usock_result,
                                  (FAR struct lwm2mstub_serverinfo_s *)arg);
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_setsrvinfo_step2
 ****************************************************************************/

static int atcmdreply_setsrvinfo_step2(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK;

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      ret = lwm2mstub_send_bscreateobj0(dev, reply,
                                  CONTAINER_SOCKETID(reply),
                                  atcmdreply_setsrvinfo_step3, arg,
                                  usock_result,
                                  (FAR struct lwm2mstub_serverinfo_s *)arg);
    }

  return ret;
}

/****************************************************************************
 * name: atcmdreply_setsrvinfo_step1
 ****************************************************************************/

static int atcmdreply_setsrvinfo_step1(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *reply,
                               FAR char *rdata, int len, unsigned long arg,
                               FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK;

  *usock_result = -EIO;

  if (check_atreply_ok(rdata, len, NULL) == OK)
    {
      ret = lwm2mstub_send_bsdelete(dev, reply, CONTAINER_SOCKETID(reply),
                             atcmdreply_setsrvinfo_step2, arg, usock_result);
    }

  return ret;
}

/****************************************************************************
 * name: perform_m2m_applysetting
 ****************************************************************************/

static int perform_m2m_applysetting(FAR struct alt1250_s *dev,
                                    FAR int32_t *usock_result,
                                    uint32_t xid)
{
  dbg_alt1250("%s called\n", __func__);

  if (dev->lwm2m_apply_xid >= 0)
    {
      *usock_result = -EINPROGRESS;
      dbg_alt1250("Apply is in progress\n");
      return REP_SEND_ACK;
    }

  *usock_result = OK;
  dev->lwm2m_apply_xid = (int64_t)xid;

  MODEM_STATE_INTENTRST(dev);
  altdevice_reset(dev->altfd);

  return REP_NO_ACK;  /* Ack is replied after received reset event */
}

/****************************************************************************
 * name: commands_on_any_state
 ****************************************************************************/

static int commands_on_any_state(FAR struct alt1250_s *dev,
                                 FAR struct alt_container_s *container,
                                 FAR struct usock_s *usock,
                                 FAR struct lte_ioctl_data_s *ltecmd,
                                 FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK;

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_LWM2M_GETEP:
        ret = lwm2mstub_send_getepname(dev, container,
                                       USOCKET_USOCKID(usock),
                                       atcmdreply_getepname,
                                       (unsigned long)ltecmd->outparam,
                                       usock_result);
        break;

      case LTE_CMDID_LWM2M_GETSRVNUM:
        *usock_result = -EOPNOTSUPP;
        break;

      case LTE_CMDID_LWM2M_GETSRVINFO:
        ret = lwm2mstub_send_getsrvinfo(dev, container,
                                        USOCKET_USOCKID(usock),
                                        atcmdreply_getsrvinfo_step1,
                                        (unsigned long)ltecmd->outparam,
                                        usock_result);
        break;

      case LTE_CMDID_LWM2M_GETACTIVEOBJNUM:
        ret = lwm2mstub_send_getsupobjs(dev, container,
                                        USOCKET_USOCKID(usock),
                                        atcmdreply_getenobjnum, 0,
                                        usock_result);
        break;

      case LTE_CMDID_LWM2M_GETACTIVEOBJ:
        ret = lwm2mstub_send_getsupobjs(dev, container,
                                        USOCKET_USOCKID(usock),
                                        atcmdreply_getenobjects,
                                        (unsigned long)ltecmd->outparam,
                                        usock_result);
        break;

      case LTE_CMDID_LWM2M_GETOBJRESNUM:
        ret = lwm2mstub_send_getobjdef(dev, container,
                                       USOCKET_USOCKID(usock),
                                       atcmdreply_getobjresnum,
                                       0, usock_result,
                                       *((uint16_t *)ltecmd->inparam));
        break;

      case LTE_CMDID_LWM2M_GETOBJRESOURCE:
        ret = lwm2mstub_send_getobjdef(dev, container,
                                       USOCKET_USOCKID(usock),
                                       atcmdreply_getobjdef,
                                       (unsigned long)ltecmd->outparam,
                                       usock_result,
                                       *((uint16_t *)ltecmd->inparam));
        break;

      case LTE_CMDID_LWM2M_GETRAT:
        ret = lwm2mstub_send_getrat(dev, container,
                                    USOCKET_USOCKID(usock),
                                    atcmdreply_getrat,
                                    (unsigned long)ltecmd->outparam,
                                    usock_result);
        break;

      case LTE_CMDID_LWM2M_GETQMODE:
        ret = lwm2mstub_send_getqueuemode(dev, container,
                                          USOCKET_USOCKID(usock),
                                          usock_result);
        break;
    }

  return ret;
}

/****************************************************************************
 * name: commands_on_poweron_state
 ****************************************************************************/

static int commands_on_poweron_state(FAR struct alt1250_s *dev,
                                     FAR struct alt_container_s *container,
                                     FAR struct usock_s *usock,
                                     FAR struct lte_ioctl_data_s *ltecmd,
                                     FAR int32_t *usock_result)
{
  int ret;

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_LWM2M_SETEP:
          ret = lwm2mstub_send_setepname(
                  dev, container, USOCKET_USOCKID(usock),
                  usock_result, (const char *)ltecmd->inparam);
        break;

      case LTE_CMDID_LWM2M_SETSRVINFO:
          ret = lwm2mstub_send_bsstart(
                    dev, container, USOCKET_USOCKID(usock),
                    atcmdreply_setsrvinfo_step1,
                    (unsigned long)ltecmd->inparam[0],
                    usock_result);
        break;

      case LTE_CMDID_LWM2M_SETACTIVEOBJ:
          ret = lwm2mstub_send_setsupobjs(
                    dev, container, USOCKET_USOCKID(usock),
                    usock_result,
                    (uint16_t *)ltecmd->inparam[0], (int)ltecmd->inparam[1]);
        break;

      case LTE_CMDID_LWM2M_SETOBJRESOURCE:
          ret = lwm2mstub_send_setobjdef(
                    dev, container, USOCKET_USOCKID(usock),
                    usock_result, (uint16_t)(uint32_t)ltecmd->inparam[0],
                    (int)ltecmd->inparam[1],
                    (struct lwm2mstub_resource_s *)ltecmd->inparam[2]);
        break;

      case LTE_CMDID_LWM2M_CHANGERAT:
        ret = lwm2mstub_send_changerat(dev, container,
                                       USOCKET_USOCKID(usock),
                                       usock_result,
                                       *((int *)ltecmd->inparam));
        break;

      case LTE_CMDID_LWM2M_SETQMODE:
        ret = lwm2mstub_send_setqueuemode(dev, container,
                                       USOCKET_USOCKID(usock),
                                       usock_result,
                                       (int)ltecmd->inparam);
        break;

      case LTE_CMDID_LWM2M_APPLY_SETTING:
        ret = perform_m2m_applysetting(dev, usock_result,
                                       USOCKET_XID(usock));
        break;

      default:
        ret = commands_on_any_state(dev, container, usock, ltecmd,
                                    usock_result);
        break;
    }

  return ret;
}

/****************************************************************************
 * name: commands_on_radioon_state
 ****************************************************************************/

static int commands_on_radioon_state(FAR struct alt1250_s *dev,
                                     FAR struct alt_container_s *container,
                                     FAR struct usock_s *usock,
                                     FAR struct lte_ioctl_data_s *ltecmd,
                                     FAR int32_t *usock_result)
{
  int ret;

  switch (ltecmd->cmdid)
    {
      case LTE_CMDID_LWM2M_CONNECT:
          ret = lwm2mstub_send_registration(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (int)ltecmd->inparam
                );
        break;

      case LTE_CMDID_LWM2M_READRESP:
          ret = lwm2mstub_send_evrespwvalue(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (int)ltecmd->inparam[0],
                      (int)ltecmd->inparam[1],
                      (struct lwm2mstub_instance_s *)ltecmd->inparam[2],
                      (char *)ltecmd->inparam[3]
                );
        break;

      case LTE_CMDID_LWM2M_WRITERESP:
          ret = lwm2mstub_send_evresponse(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (int)ltecmd->inparam[0],
                      (int)ltecmd->inparam[1],
                      (struct lwm2mstub_instance_s *)ltecmd->inparam[2]
                );
        break;

      case LTE_CMDID_LWM2M_OBSERVERESP:
          ret = lwm2mstub_send_evrespwoinst(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (int)ltecmd->inparam[0],
                      (int)ltecmd->inparam[1]
                );
        break;

      case LTE_CMDID_LWM2M_EXECRESP:
          ret = lwm2mstub_send_evresponse(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (int)ltecmd->inparam[0],
                      (int)ltecmd->inparam[1],
                      (struct lwm2mstub_instance_s *)ltecmd->inparam[2]
                );
        break;

      case LTE_CMDID_LWM2M_OBSERVEUPDATE:
          ret = lwm2mstub_send_objevent(
                      dev, container, USOCKET_USOCKID(usock), usock_result,
                      (char *)ltecmd->inparam[0],
                      (struct lwm2mstub_instance_s *)ltecmd->inparam[1],
                      (char *)ltecmd->inparam[2]
                );
        break;

      default:
        ret = commands_on_any_state(dev, container, usock, ltecmd,
                                    usock_result);
        break;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: send_m2mnotice_command
 ****************************************************************************/

int send_m2mnotice_command(uint32_t cmdid,
                           FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *container,
                           FAR struct usock_s *usock,
                           FAR struct lte_ioctl_data_s *ltecmd,
                           FAR int32_t *ures)
{
  int ret = REP_SEND_ACK;
  *ures = OK;

  switch (cmdid)
    {
      case LTE_CMDID_LWM2M_READ_EVT:
      case LTE_CMDID_LWM2M_WRITE_EVT:
      case LTE_CMDID_LWM2M_EXEC_EVT:
      case LTE_CMDID_LWM2M_OVSTART_EVT:
      case LTE_CMDID_LWM2M_OVSTOP_EVT:

        /* TODO: Now unregister any events makes all event notify stop.
         *       This will be fixed in near future.
         */

        ret = lwm2mstub_send_m2mobjcmd(
                                dev, container, USOCKET_USOCKID(usock), ures,
                                (ltecmd->cb != NULL));
        break;

      case LTE_CMDID_LWM2M_SERVEROP_EVT:
        ret = lwm2mstub_send_m2mopev(
                                dev, container, USOCKET_USOCKID(usock), ures,
                                (ltecmd->cb != NULL));
        break;

      case LTE_CMDID_LWM2M_FWUP_EVT:
        ret = lwm2mstub_send_m2mev(
                                dev, container, USOCKET_USOCKID(usock), ures,
                                (ltecmd->cb != NULL));
        break;

      default:
        *ures = -EINVAL;
        break;
    }

  return ret;
}

/****************************************************************************
 * name: usockreq_ioctl_lwm2m
 ****************************************************************************/

int usockreq_ioctl_lwm2m(FAR struct alt1250_s *dev,
                         FAR struct usrsock_request_buff_s *req,
                         FAR int32_t *ures,
                         FAR uint32_t *usock_xid,
                         FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_ioctl_data_s *ltecmd = &req->req_ioctl.ltecmd;
  FAR struct usock_s *usock = NULL;
  FAR struct alt_container_s *container;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s called with : %08lx\n", __func__, ltecmd->cmdid);

  *ures = -EOPNOTSUPP;
  *usock_xid = request->head.xid;

  if (dev->is_support_lwm2m)
    {
      usock = usocket_search(dev, request->usockid);
      if (usock == NULL)
        {
          *ures = -EBADFD;
          return ret;
        }

      container = container_alloc();
      if (container == NULL)
        {
          return REP_NO_CONTAINER;
        }

      *ures = -EINVAL;
      ret = REP_SEND_ACK;
      USOCKET_SET_REQUEST(usock, request->head.reqid, request->head.xid);

      if (MODEM_STATE_IS_PON(dev))
        {
          /* These commands is available just in power on state */

          ret = commands_on_poweron_state(dev, container, usock, ltecmd,
                                          ures);
        }
      else if (MODEM_STATE_IS_RON(dev))
        {
          ret = commands_on_radioon_state(dev, container, usock, ltecmd,
                                          ures);
        }

      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }
    }

  return ret;
}
