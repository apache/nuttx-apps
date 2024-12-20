/****************************************************************************
 * apps/lte/lapi/src/lapi_other.c
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

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nuttx/wireless/lte/lte_ioctl.h>
#include <nuttx/net/netconfig.h>

#include "lte/lte_api.h"
#include "lte/lapi.h"

#include "lapi_dbg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ATCMD_HEADER     "AT"
#define ATCMD_HEADER_LEN (2)
#define ATCMD_FOOTER     '\r'
#define ATCMD_FOOTER_LEN (1)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lapi_req
 ****************************************************************************/

int lapi_req(uint32_t cmdid, FAR void *inp, size_t ilen, FAR void *outp,
  size_t olen, FAR void *cb)
{
  int ret;
  int sock;
  struct lte_ioctl_data_s cmd;

  sock = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sock < 0)
    {
      ret = -errno;
      lapi_printf("failed to open socket:%d\n", errno);
    }
  else
    {
      cmd.cmdid = cmdid;
      cmd.inparam = inp;
      cmd.inparamlen = ilen;
      cmd.outparam = outp;
      cmd.outparamlen = olen;
      cmd.cb = cb;

      ret = ioctl(sock, SIOCLTECMD, (unsigned long)&cmd);
      if (ret < 0)
        {
          ret = -errno;
          lapi_printf("failed to ioctl:%d\n", errno);
        }

      close(sock);
    }

  return ret;
}

int lte_data_allow_sync(uint8_t session_id, uint8_t allow,
                        uint8_t roaming_allow)
{
  lapi_printf("lte_data_allow_sync() is not supported.\n");

  return -EOPNOTSUPP;
}

int lte_data_allow(uint8_t session_id, uint8_t allow,
                   uint8_t roaming_allow, data_allow_cb_t callback)
{
  lapi_printf("lte_data_allow() is not supported.\n");

  return -EOPNOTSUPP;
}

int lte_get_errinfo(FAR lte_errinfo_t *info)
{
  int ret;

  FAR void *outarg[] =
    {
      info
    };

  if (!info)
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_GETERRINFO,
                 NULL, 0,
                 (FAR void *)outarg, nitems(outarg),
                 NULL);

  return ret;
}

int lte_send_atcmd_sync(FAR const char *cmd, int cmdlen,
  FAR char *respbuff, int respbufflen, FAR int *resplen)
{
  int32_t ret;
  FAR void *inarg[] =
    {
      (FAR void *)cmd, (FAR void *)cmdlen
    };

  FAR void *outarg[] =
    {
      respbuff, (FAR void *)respbufflen, resplen
    };

  if (!cmd
    || (ATCMD_HEADER_LEN + ATCMD_FOOTER_LEN) > cmdlen
    || LTE_AT_COMMAND_MAX_LEN < cmdlen
    || !respbuff || !respbufflen || !resplen)
    {
      return -EINVAL;
    }

  if (0 != strncmp(cmd, ATCMD_HEADER, ATCMD_HEADER_LEN))
    {
      return -EINVAL;
    }

  if (ATCMD_FOOTER != cmd[cmdlen - ATCMD_FOOTER_LEN])
    {
      return -EINVAL;
    }

  ret = lapi_req(LTE_CMDID_SENDATCMD,
                 (FAR void *)inarg, nitems(inarg),
                 (FAR void *)outarg, nitems(outarg),
                 NULL);

  return ret;
}
