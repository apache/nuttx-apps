/****************************************************************************
 * apps/lte/alt1250/alt1250_reset_seq.c
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

#include <sys/poll.h>
#include <sys/param.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include "alt1250_dbg.h"
#include "alt1250_devif.h"
#include "alt1250_devevent.h"
#include "alt1250_postproc.h"
#include "alt1250_container.h"
#include "alt1250_ioctl_subhdlr.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_reset_seq.h"
#include "alt1250_atcmd.h"
#include "alt1250_evt.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct reset_arg_s
{
  int seq_no;
  unsigned long arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct reset_arg_s reset_arg;

static postproc_hdlr_t ponreset_seq[] =
{
  postproc_fwgetversion,
};
#define PONRESET_SEQ_NUM  nitems(ponreset_seq)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: postproc_ponresetseq
 ****************************************************************************/

static int postproc_ponresetseq(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *reply,
                                FAR struct usock_s *usock,
                                FAR int32_t *usock_result,
                                FAR uint32_t *usock_xid,
                                FAR struct usock_ackinfo_s *ackinfo,
                                unsigned long arg)
{
  int ret = REP_NO_ACK_WOFREE;
  FAR struct reset_arg_s *rarg = (FAR struct reset_arg_s *)arg;
  ASSERT(rarg->seq_no < PONRESET_SEQ_NUM);

  ponreset_seq[rarg->seq_no](dev, reply, usock, usock_result, usock_xid,
                             ackinfo, rarg->arg);
  rarg->seq_no++;
  if (rarg->seq_no == PONRESET_SEQ_NUM)
    {
      /* On last postproc, container should be free */

      dev->recvfrom_processing = false;
      ret = REP_NO_ACK;
      MODEM_STATE_PON(dev);
    }

  return ret;
}

/****************************************************************************
 * name: send_getversion_onreset
 ****************************************************************************/

static int send_getversion_onreset(FAR struct alt1250_s *dev,
                                   FAR struct alt_container_s *container,
                                   FAR int32_t *usock_result)
{
  reset_arg.seq_no = 0;
  reset_arg.arg = 0;

  set_container_ids(container, -1, LTE_CMDID_GETVER);
  set_container_argument(container, NULL, 0);
  set_container_response(container, alt1250_getevtarg(LTE_CMDID_GETVER), 2);
  set_container_postproc(container, postproc_ponresetseq,
                         (unsigned long)&reset_arg);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: str_toupper_case
 ****************************************************************************/

static void str_toupper_case(FAR char *data, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      data[i] = (char)toupper(data[i]);
    }
}

/****************************************************************************
 * name: recv_atreply_onreset
 ****************************************************************************/

static int recv_atreply_onreset(atreply_parser_t parse,
                                FAR struct alt1250_s *dev,
                                FAR void *arg)
{
  int ret;
  uint64_t bitmap;
  struct pollfd fds;
  nfds_t nfds;
  FAR struct alt_container_s *rlist;
  FAR struct alt_container_s *container;
  FAR char *reply;
  int rlen;

  fds.fd = dev->altfd;
  fds.events = POLLIN;
  nfds = 1;

  ret = poll(&fds, nfds, -1);
  ASSERT(ret > 0);
  ASSERT(fds.revents & POLLIN);

  ret = altdevice_getevent(dev->altfd, &bitmap, &rlist);
  ASSERT(ret == OK);

  if (bitmap & ALT1250_EVTBIT_RESET)
    {
      /* Reset is happened again... */

      container_free_all(rlist);
      dev->sid = -1;

      ret = REP_MODEM_RESET;
    }
  else if (bitmap & ALT1250_EVTBIT_REPLY)
    {
      container = container_pick_listtop(&rlist);
      ASSERT(rlist == NULL);

      reply = (FAR char *)container->outparam[0];
      rlen = *(FAR int *)container->outparam[2];

      str_toupper_case(reply, rlen);
      ret = parse(reply, rlen, arg);
      ASSERT(ret == OK);

      ret = REP_NO_ACK;
    }
  else
    {
      ASSERT(0);  /* Should not be here */
    }

  return ret;
}

/****************************************************************************
 * name: alt1250_disable_scanplan
 ****************************************************************************/

static int alt1250_disable_scanplan(FAR struct alt1250_s *dev,
                                    FAR struct alt_container_s *container)
{
  int recv_ret;
  struct atreply_truefalse_s t_or_f;

  t_or_f.target_str = "0";
  ltesp_send_getscanplan(dev, container);
  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (!t_or_f.result)
    {
      ltesp_send_setscanplan(dev, container, false);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }
    }

  return recv_ret;
}

/****************************************************************************
 * name: alt1250_lwm2m_ponreset
 ****************************************************************************/

static int alt1250_lwm2m_ponreset(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container)
{
  int ret = REP_NO_ACK;
  int recv_ret;
  struct atreply_truefalse_s t_or_f;
  int32_t usock_result = OK;

  /* Make sure LwM2M func enabled */

  t_or_f.target_str = "\nTRUE\r";
  lwm2mstub_send_getenable(dev, container, &usock_result);
  if (usock_result == -ENOTSUP)
    {
      return REP_NO_ACK;
    }

  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (!t_or_f.result)
    {
      lwm2mstub_send_setenable(dev, container, true);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }

      ret = REP_SEND_ACK;
    }

  /* Make sure LwM2M AutoConnect is disabled */

  t_or_f.target_str = "\nFALSE\r";
  lwm2mstub_send_getautoconnect(dev, container);
  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (!t_or_f.result)
    {
      lwm2mstub_send_setautoconnect(dev, container, false);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }

      ret = REP_SEND_ACK;
    }

  /* Make sure LwM2M Version is 1.1 */

  t_or_f.target_str = "\n1.1\r";
  lwm2mstub_send_getversion(dev, container);
  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (!t_or_f.result)
    {
      lwm2mstub_send_setversion(dev, container, true);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }

      ret = REP_SEND_ACK;
    }

  /* Make sure LwM2M NameMode is 0:UserName */

  t_or_f.target_str = "\n0\r";
  lwm2mstub_send_getnamemode(dev, container);
  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (!t_or_f.result)
    {
      lwm2mstub_send_setnamemode(dev, container, 0);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }

      ret = REP_SEND_ACK;
    }

  /* Make sure NWOPER is not DEFAULT */

  t_or_f.target_str = "DEFAULT";
  ltenwop_send_getnwop(dev, container);
  recv_ret = recv_atreply_onreset(check_atreply_truefalse, dev, &t_or_f);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (t_or_f.result)
    {
      ltenwop_send_setnwoptp(dev, container);
      recv_ret = recv_atreply_onreset(check_atreply_ok, dev, NULL);
      if (recv_ret == REP_MODEM_RESET)
        {
          return recv_ret;
        }

      ret = REP_SEND_ACK;
    }

  /* Make sure SCAN_PALAN_EN is 0 */

  recv_ret = alt1250_disable_scanplan(dev, container);
  if (recv_ret == REP_MODEM_RESET)
    {
      return recv_ret;
    }

  if (ret == REP_SEND_ACK)
    {
      /* Force Reset is needed. */

      altdevice_reset(dev->altfd);
    }

  dev->is_support_lwm2m = true;

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: handle_poweron_reset_stage2
 ****************************************************************************/

int handle_poweron_reset_stage2(FAR struct alt1250_s *dev)
{
  int ret;
  int32_t unused;
  FAR struct alt_container_s *container;

  container = container_alloc();
  ASSERT(container != NULL);

  ret = send_getversion_onreset(dev, container, &unused);

  if (IS_NEED_CONTAINER_FREE(ret))
    {
      container_free(container);
    }

  if (ret > 0)
    {
      /* for blocking next usrsock request */

      ret = REP_MODEM_RESET;
      dev->recvfrom_processing = true;
    }

  return ret;
}

/****************************************************************************
 * name: handle_poweron_reset
 ****************************************************************************/

int handle_poweron_reset(FAR struct alt1250_s *dev)
{
  int ret = REP_MODEM_RESET;
  FAR struct alt_container_s *container;

  container = container_alloc();
  ASSERT(container != NULL);

  while (ret == REP_MODEM_RESET)
    {
      ret = alt1250_lwm2m_ponreset(dev, container);
    }

  container_free(container);

  MODEM_STATE_B4PON_2ND(dev);

  if (ret == REP_NO_ACK)
    {
      /* In this sequence,
       * NO_ACK means no force reset.
       * In that case, get version is needed to send here.
       */

      handle_poweron_reset_stage2(dev);
      ret = REP_MODEM_RESET;
    }

  return ret;
}
