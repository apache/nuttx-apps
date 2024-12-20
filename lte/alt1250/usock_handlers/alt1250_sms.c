/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_sms.c
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
#include <nuttx/net/sms.h>
#include <assert.h>

#include "alt1250_dbg.h"
#include "alt1250_container.h"
#include "alt1250_socket.h"
#include "alt1250_devif.h"
#include "alt1250_usockevent.h"
#include "alt1250_postproc.h"
#include "alt1250_usrsock_hdlr.h"
#include "alt1250_sms.h"
#include "alt1250_evt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_LTE_ALT1250_SMS_TOA)
#  if defined(CONFIG_LTE_ALT1250_SMS_NAI_UNKNOWN)
#    define ALT1250_SMS_NAI SMS_NAI_UNKNOWN
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_INTERNATIONAL)
#    define ALT1250_SMS_NAI SMS_NAI_INTERNATIONAL
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_NATIONAL)
#    define ALT1250_SMS_NAI SMS_NAI_NATIONAL
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_NETWORK_SPEC)
#    define ALT1250_SMS_NAI SMS_NAI_NETWORK_SPEC
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_SUBSCRIBER)
#    define ALT1250_SMS_NAI SMS_NAI_SUBSCRIBER
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_ALPANUMERIC)
#    define ALT1250_SMS_NAI SMS_NAI_ALPANUMERIC
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_ABBREVIATED)
#    define ALT1250_SMS_NAI SMS_NAI_ABBREVIATED
#  elif defined(CONFIG_LTE_ALT1250_SMS_NAI_RESERVED)
#    define ALT1250_SMS_NAI SMS_NAI_RESERVED
#  endif
#  if defined(CONFIG_LTE_ALT1250_SMS_NPI_UNKNOWN)
#    define ALT1250_SMS_NPI SMS_NPI_UNKNOWN
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_ISDN)
#    define ALT1250_SMS_NPI SMS_NPI_ISDN
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_DATA)
#    define ALT1250_SMS_NPI SMS_NPI_DATA
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_TELEX)
#    define ALT1250_SMS_NPI SMS_NPI_TELEX
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_SERVICE_CENTRE_SPEC)
#    define ALT1250_SMS_NPI SMS_NPI_SERVICE_CENTRE_SPEC
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_SERVICE_CENTRE_SPEC2)
#    define ALT1250_SMS_NPI SMS_NPI_SERVICE_CENTRE_SPEC2
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_NATIONAL)
#    define ALT1250_SMS_NPI SMS_NPI_NATIONAL
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_PRIVATE)
#    define ALT1250_SMS_NPI SMS_NPI_PRIVATE
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_ERMES)
#    define ALT1250_SMS_NPI SMS_NPI_ERMES
#  elif defined(CONFIG_LTE_ALT1250_SMS_NPI_RESERVED)
#    define ALT1250_SMS_NPI SMS_NPI_RESERVED
#  endif
#endif /* CONFIG_LTE_ALT1250_SMS_TOA */

/* RK_02_01_01_10xxx FW version that does not support SMS feature */

#define IS_SMS_UNAVAIL_FWVERSION(d) (!strncmp(MODEM_FWVERSION(d), \
                                              "RK_02_01_01", 11))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct alt_container_s g_sms_container;
static struct postproc_s g_sms_postproc;

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int postproc_smsinit(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg);

static int postproc_smsinit_reopen(FAR struct alt1250_s *dev,
                                   FAR struct alt_container_s *reply,
                                   FAR struct usock_s *usock,
                                   FAR int32_t *usock_result,
                                   FAR uint32_t *usock_xid,
                                   FAR struct usock_ackinfo_s *ackinfo,
                                   unsigned long arg);

static int postproc_smsfin(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg);

static int postproc_smsfin_reopen(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *reply,
                                  FAR struct usock_s *usock,
                                  FAR int32_t *usock_result,
                                  FAR uint32_t *usock_xid,
                                  FAR struct usock_ackinfo_s *ackinfo,
                                  unsigned long arg);

static int postproc_smssend(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg);

static int postproc_smsdelete(FAR struct alt1250_s *dev,
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
 * name: send_smsinit_command
 ****************************************************************************/

static int send_smsinit_command(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                int usockid,
                                FAR postproc_hdlr_t func,
                                FAR int32_t *usock_result)
{
  /* Dummy variable to receive replies by the container.
   * No one refers to this address.
   */

  FAR void *dummy_output = NULL;

  clear_container(container);
  set_container_ids(container, usockid, LTE_CMDID_SMS_INIT);
  set_container_argument(container, NULL, 0);
  set_container_response(container, &dummy_output, 1);
  set_container_postproc(container, func, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: send_smsfin_command
 ****************************************************************************/

static int send_smsfin_command(FAR struct alt1250_s *dev,
                               FAR struct alt_container_s *container,
                               int usockid,
                               FAR postproc_hdlr_t func,
                               FAR int32_t *usock_result)
{
  /* Dummy variable to receive replies by the container.
   * No one refers to this address.
   */

  FAR void *dummy_output = NULL;

  clear_container(container);
  set_container_ids(container, usockid, LTE_CMDID_SMS_FIN);
  set_container_argument(container, NULL, 0);
  set_container_response(container, &dummy_output, 1);
  set_container_postproc(container, func, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: send_smssend_command
 ****************************************************************************/

static int send_smssend_command(FAR struct alt1250_s *dev,
                                FAR struct alt_container_s *container,
                                FAR struct usock_s *usock,
                                FAR struct sms_send_msg_s *msg,
                                FAR uint16_t msg_len,
                                FAR int32_t *usock_result)
{
  int idx = 0;

  FAR void *inparam[6];
  uint8_t chset = SMS_CHSET_UCS2;

  /* Thease members are referenced only when sending a command and
   * not when receiving a response, so local variables are used.
   */

  inparam[0] = msg;
  inparam[1] = &msg_len;
  inparam[2] = &dev->sms_info.en_status_report;
  inparam[3] = &dev->sms_info.dest_scaddr;
  inparam[4] = &chset;
#if defined(CONFIG_LTE_ALT1250_SMS_TOA)
  inparam[5] = &dev->sms_info.dest_toa;
#else
  inparam[5] = NULL;
#endif

  USOCKET_SET_RESPONSE(usock, idx++, USOCKET_REFID(usock));
  USOCKET_SET_RESPONSE(usock, idx++, &USOCKET_REQBUFLEN(usock));

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_SMS_SEND);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, USOCKET_REP_RESPONSE(usock), idx);
  set_container_postproc(container, postproc_smssend, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: send_smsdelete_command
 ****************************************************************************/

static int send_smsdelete_command(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *container,
                                  FAR struct usock_s *usock,
                                  uint16_t msg_index,
                                  FAR int32_t *usock_result)
{
  /* Dummy variable to receive replies by the container.
   * No one refers to this address.
   */

  FAR void *dummy_output = NULL;
  FAR void *inparam[1];

  /* This member is referenced only when sending a command and
   * not when receiving a response, so local variables is used.
   */

  inparam[0] = &msg_index;

  set_container_ids(container, USOCKET_USOCKID(usock), LTE_CMDID_SMS_DELETE);
  set_container_argument(container, inparam, nitems(inparam));
  set_container_response(container, &dummy_output, 1);
  set_container_postproc(container, postproc_smsdelete, 0);

  return altdevice_send_command(dev, dev->altfd, container, usock_result);
}

/****************************************************************************
 * name: send_smsreportrecv_command
 ****************************************************************************/

static int send_smsreportrecv_command(FAR struct alt1250_s *dev,
                                      FAR int32_t *usock_result)
{
  struct alt_container_s container = {
    0
  };

  set_container_ids(&container, 0, LTE_CMDID_SMS_REPORT_RECV);

  return altdevice_send_command(dev, dev->altfd, &container, usock_result);
}

/****************************************************************************
 * name: reset_sms_info
 ****************************************************************************/

void reset_sms_info(FAR struct alt1250_s *dev)
{
  SMS_SET_STATE(&dev->sms_info, SMS_STATE_UNINIT);
  dev->sms_info.msg_index = 0;
  dev->sms_info.read_msglen = 0;
  dev->sms_info.total_msglen = 0;
  dev->sms_info.is_first_msg = false;
#if defined(CONFIG_LTE_ALT1250_SMS_TOA)
  dev->sms_info.dest_toa = SMS_SET_TOA(ALT1250_SMS_NAI, ALT1250_SMS_NPI);
#endif
}

/****************************************************************************
 * name: fill_recv_ackinfo
 ****************************************************************************/

static bool fill_recv_ackinfo(FAR struct alt1250_s *dev,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR struct usock_ackinfo_s *ackinfo)
{
  FAR void **arg;
  FAR uint8_t *msg_head;
  uint16_t remain_msglen;

  /* arg[0]: sms msg index
   * arg[1]: sms msg length
   * arg[2]: Maximum number of msg for concatenate sms.
   * arg[3]: Current number of msg for concatenate sms.
   * arg[4]: sms msg
   */

  arg = alt1250_getevtarg(LTE_CMDID_SMS_REPORT_RECV);

  assert(arg && arg[4]);

  msg_head = (FAR uint8_t *)arg[4];
  remain_msglen = dev->sms_info.msglen - dev->sms_info.read_msglen;

  if (!dev->sms_info.is_first_msg)
    {
      /* In the case of a concatenated SMS, the header information is only
       * given to the first SMS. Therefore, if the SMS is not the first
       * one, the header information is shifted and read.
       */

      msg_head += sizeof(struct sms_recv_msg_header_s);
      remain_msglen -= sizeof(struct sms_recv_msg_header_s);
    }

  *usock_result = MIN(USOCKET_REQBUFLEN(usock), remain_msglen);

  ackinfo->valuelen = 0;
  ackinfo->valuelen_nontrunc = 0;
  ackinfo->value_ptr = NULL;
  ackinfo->buf_ptr = msg_head + dev->sms_info.read_msglen;

  /* return is empty */

  return (USOCKET_REQBUFLEN(usock) < remain_msglen) ? false : true;
}

/****************************************************************************
 * name: notify_read_ready
 ****************************************************************************/

static void notify_read_ready(FAR struct alt1250_s *dev, uint16_t msg_index,
                              uint16_t msg_len)
{
  dev->sms_info.msg_index = msg_index;
  dev->sms_info.msglen = msg_len;
  dev->sms_info.read_msglen = 0;

  usocket_smssock_readready(dev);
}

/****************************************************************************
 * name: notify_abort
 ****************************************************************************/

static void notify_abort(FAR struct alt1250_s *dev)
{
  reset_sms_info(dev);
  usocket_smssock_abort(dev);
}

/****************************************************************************
 * name: update_concat_size
 ****************************************************************************/

static void update_concat_size(FAR struct alt1250_s *dev, uint16_t msg_index,
                               uint16_t msg_len, uint8_t max_num,
                               uint8_t seq_num,
                               FAR struct sms_recv_msg_header_s *sms_msg)
{
  int32_t usock_result = OK;

  dev->sms_info.total_msglen += sms_msg->datalen;

  if (max_num == seq_num)
    {
      /* In case of last of concatenated sms */

      /* Send only SMS FIN command to avoid receiving the next SMS REPORT
       * command. If SMS REPORT response is sent before SMS FIN command
       * is sent, unexpected SMS REPORT command may be received.
       */

      send_smsfin_command(dev, &g_sms_container, 0,
                          postproc_smsfin_reopen, &usock_result);
      if (usock_result < 0)
        {
          notify_abort(dev);
        }
      else
        {
          SMS_SET_STATE(&dev->sms_info, SMS_STATE_REOPEN);

          dev->sms_info.is_first_msg = true;
        }
    }
  else
    {
      send_smsreportrecv_command(dev, &usock_result);
      if (usock_result < 0)
        {
          notify_abort(dev);
        }
    }
}

/****************************************************************************
 * name: handle_recvmsg
 ****************************************************************************/

static void handle_recvmsg(FAR struct alt1250_s *dev, uint16_t msg_index,
                           uint16_t msg_len, uint8_t max_num,
                           uint8_t seq_num,
                           FAR struct sms_recv_msg_header_s *sms_msg)
{
  if (max_num == 0)
    {
      /* In case of not concatenated sms */

      dev->sms_info.total_msglen = sms_msg->datalen;
      dev->sms_info.is_first_msg = true;

      SMS_SET_STATE(&dev->sms_info, SMS_STATE_READ_READY);

      /* Notify usrsock of the read ready event. */

      notify_read_ready(dev, msg_index, msg_len);
    }
  else
    {
      /* In case of concatenated sms */

      SMS_SET_STATE(&dev->sms_info, SMS_STATE_CALC_SIZE);

      dev->sms_info.msg_index = msg_index;

      update_concat_size(dev, msg_index, msg_len, max_num, seq_num, sms_msg);
    }
}

/****************************************************************************
 * name: postproc_smsinit
 ****************************************************************************/

static int postproc_smsinit(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg)
{
  int ret = REP_SEND_ACK_TXREADY;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = CONTAINER_RESPRES(reply);
  if (*usock_result < 0)
    {
      ret = REP_SEND_ACK;
    }
  else
    {
      *usock_result = USOCKET_USOCKID(usock);
      ackinfo->usockid = USOCKET_USOCKID(usock);
      SMS_SET_STATE(&dev->sms_info, SMS_STATE_WAITMSG);
    }

  return ret;
}

/****************************************************************************
 * name: postproc_smsinit_reopen
 ****************************************************************************/

static int postproc_smsinit_reopen(FAR struct alt1250_s *dev,
                                   FAR struct alt_container_s *reply,
                                   FAR struct usock_s *usock,
                                   FAR int32_t *usock_result,
                                   FAR uint32_t *usock_xid,
                                   FAR struct usock_ackinfo_s *ackinfo,
                                   unsigned long arg)
{
  dbg_alt1250("%s start\n", __func__);

  *usock_result = CONTAINER_RESPRES(reply);
  if (*usock_result >= 0)
    {
      SMS_SET_STATE(&dev->sms_info, SMS_STATE_WAITMSG_CONCAT);
    }
  else
    {
      notify_abort(dev);
    }

  return REP_NO_ACK_WOFREE;
}

/****************************************************************************
 * name: postproc_smsfin
 ****************************************************************************/

static int postproc_smsfin(FAR struct alt1250_s *dev,
                           FAR struct alt_container_s *reply,
                           FAR struct usock_s *usock,
                           FAR int32_t *usock_result,
                           FAR uint32_t *usock_xid,
                           FAR struct usock_ackinfo_s *ackinfo,
                           unsigned long arg)
{
  dbg_alt1250("%s start\n", __func__);

  *usock_xid = USOCKET_XID(usock);
  *usock_result = CONTAINER_RESPRES(reply);

  usocket_free(usock);

  return REP_SEND_ACK;
}

/****************************************************************************
 * name: postproc_smsfin_reopen
 ****************************************************************************/

static int postproc_smsfin_reopen(FAR struct alt1250_s *dev,
                                  FAR struct alt_container_s *reply,
                                  FAR struct usock_s *usock,
                                  FAR int32_t *usock_result,
                                  FAR uint32_t *usock_xid,
                                  FAR struct usock_ackinfo_s *ackinfo,
                                  unsigned long arg)
{
  dbg_alt1250("%s start\n", __func__);

  if (SMS_STATE(&dev->sms_info) == SMS_STATE_UNINIT)
    {
      dbg_alt1250("All sms sockets are closed\n");
    }
  else
    {
      send_smsinit_command(dev, reply, 0, postproc_smsinit_reopen,
                           usock_result);
      if (*usock_result < 0)
        {
          notify_abort(dev);
        }
    }

  return REP_NO_ACK_WOFREE;
}

/****************************************************************************
 * name: postproc_smssend
 ****************************************************************************/

static int postproc_smssend(FAR struct alt1250_s *dev,
                            FAR struct alt_container_s *reply,
                            FAR struct usock_s *usock,
                            FAR int32_t *usock_result,
                            FAR uint32_t *usock_xid,
                            FAR struct usock_ackinfo_s *ackinfo,
                            unsigned long arg)
{
  int ret = REP_SEND_ACK_TXREADY;

  dbg_alt1250("%s start\n", __func__);

  /* resp[0]: referese ID list */

  *usock_xid = USOCKET_XID(usock);
  *usock_result = CONTAINER_RESPRES(reply);

  if (*usock_result == -EPROTO)
    {
      lte_errinfo_t errinfo;
      alt1250_geterrinfo(&errinfo);
      *usock_result = (errinfo.err_indicator & LTE_ERR_INDICATOR_ERRCODE) ?
                       -errinfo.err_result_code : *usock_result;
    }

  ackinfo->usockid = USOCKET_USOCKID(usock);

  return ret;
}

/****************************************************************************
 * name: postproc_smsdelete
 ****************************************************************************/

static int postproc_smsdelete(FAR struct alt1250_s *dev,
                              FAR struct alt_container_s *reply,
                              FAR struct usock_s *usock,
                              FAR int32_t *usock_result,
                              FAR uint32_t *usock_xid,
                              FAR struct usock_ackinfo_s *ackinfo,
                              unsigned long arg)
{
  int ret = REP_SEND_ACK;

  dbg_alt1250("%s start\n", __func__);

  *usock_xid = USOCKET_XID(usock);
  *usock_result = CONTAINER_RESPRES(reply);
  if (*usock_result >= 0)
    {
      ret = send_smsreportrecv_command(dev, usock_result);
      if (*usock_result >= 0)
        {
          /* In case of success */

          ret = REP_SEND_DACK;
          fill_recv_ackinfo(dev, usock, usock_result, ackinfo);

          dev->sms_info.is_first_msg = false;
        }
    }

  return ret;
}

/****************************************************************************
 * name: sms_report_event
 ****************************************************************************/

static void sms_report_event(FAR struct alt1250_s *dev, uint16_t msg_index,
                             uint16_t msg_len, uint8_t max_num,
                             uint8_t seq_num,
                             FAR struct sms_recv_msg_header_s *sms_msg)
{
  dbg_alt1250("%s start\n", __func__);

  assert(msg_len >= sizeof(struct sms_recv_msg_header_s));

  switch (SMS_STATE(&dev->sms_info))
    {
      case SMS_STATE_UNINIT:
        dbg_alt1250("All sms sockets are closed\n");
        break;

      case SMS_STATE_REOPEN:
        dbg_alt1250("Receive report msg in REOPEN state\n");
        break;

      case SMS_STATE_READ_READY:

        /* Notify usrsock of the read ready event. */

        notify_read_ready(dev, msg_index, msg_len);
        break;

      case SMS_STATE_WAITMSG:
        handle_recvmsg(dev, msg_index, msg_len, max_num, seq_num, sms_msg);
        break;

      case SMS_STATE_WAITMSG_CONCAT:
        if (dev->sms_info.msg_index != msg_index)
          {
            /* In case of unexpected SMS received.
             * Therefore, start over from the beginning.
             */

            handle_recvmsg(dev, msg_index, msg_len, max_num, seq_num,
                           sms_msg);
          }
        else
          {
            /* In case of expected concatenated SMS received. */

            sms_msg->datalen = dev->sms_info.total_msglen;

            SMS_SET_STATE(&dev->sms_info, SMS_STATE_READ_READY);

            /* Notify usrsock of the read ready event. */

            notify_read_ready(dev, msg_index, msg_len);
          }
        break;

      case SMS_STATE_CALC_SIZE:
        update_concat_size(dev, msg_index, msg_len, max_num, seq_num,
                           sms_msg);
        break;

      default:
        dbg_alt1250("Receive report msg in unexpected state: %d\n",
                    SMS_STATE(&dev->sms_info));
        break;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: alt1250_sms_init
 ****************************************************************************/

int alt1250_sms_init(FAR struct alt1250_s *dev, FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR struct usock_ackinfo_s *ackinfo)
{
  int ret = REP_SEND_ACK_WOFREE;
  FAR struct alt_container_s *container;

  dbg_alt1250("%s start\n", __func__);

  if (IS_SMS_UNAVAIL_FWVERSION(dev))
    {
      dbg_alt1250("This ALT1250 FW version does not support SMS.\n");
      *usock_result = -EAFNOSUPPORT;
      return REP_SEND_ACK_WOFREE;
    }

  if (SMS_STATE(&dev->sms_info) == SMS_STATE_UNINIT)
    {
      container = container_alloc();
      if (container == NULL)
        {
          dbg_alt1250("no container\n");
          return REP_NO_CONTAINER;
        }

      ret = send_smsinit_command(dev, container, USOCKET_USOCKID(usock),
                                 postproc_smsinit, usock_result);

      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }
    }
  else
    {
      ret = REP_SEND_ACK_TXREADY;
      ackinfo->usockid = USOCKET_USOCKID(usock);
    }

  return ret;
}

/****************************************************************************
 * name: alt1250_sms_fin
 ****************************************************************************/

int alt1250_sms_fin(FAR struct alt1250_s *dev, FAR struct usock_s *usock,
                    FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK_WOFREE;
  FAR struct alt_container_s *container;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = OK;

  if (usocket_smssock_num(dev) == 1)
    {
      if (SMS_STATE(&dev->sms_info) == SMS_STATE_REOPEN)
        {
          return REP_NO_CONTAINER;
        }

      container = container_alloc();
      if (container == NULL)
        {
          dbg_alt1250("no container\n");
          return REP_NO_CONTAINER;
        }

      ret = send_smsfin_command(dev, container, USOCKET_USOCKID(usock),
                                postproc_smsfin, usock_result);

      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }

      alt1250_reset_sms_info(dev);

      if (*usock_result < 0)
        {
          usocket_free(usock);
        }
    }
  else
    {
      usocket_free(usock);
    }

  return ret;
}

/****************************************************************************
 * name: alt1250_sms_send
 ****************************************************************************/

int alt1250_sms_send(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_sendto_s *req,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result)
{
  int ret = REP_SEND_ACK_WOFREE;
  FAR struct alt_container_s *container;

  dbg_alt1250("%s start\n", __func__);

  if (SMS_STATE(&dev->sms_info) == SMS_STATE_REOPEN)
    {
      /* Sending the SMS send command now will fail because the
       * SMS fin command has already been sent.
       * Therefore, return REP_NO_CONTAINER and wait for the status to
       * become ready to send.
       */

      return REP_NO_CONTAINER;
    }

  container = container_alloc();
  if (container == NULL)
    {
      dbg_alt1250("no container\n");
      return REP_NO_CONTAINER;
    }

  if (req->addrlen > 0)
    {
      /* Ignore destination address. */

      usockif_discard(dev->usockfd, req->addrlen);
    }

  if (req->buflen > 0 &&
      req->buflen <= sizeof(struct sms_send_msg_s) + (SMS_MAX_DATALEN * 2))
    {
      ret = usockif_readreqsendbuf(dev->usockfd, dev->tx_buff, req->buflen);
      if (ret < 0)
        {
          *usock_result = ret;
          container_free(container);
          return REP_SEND_ACK_WOFREE;
        }

      ret = send_smssend_command(dev, container, usock,
                                 (FAR struct sms_send_msg_s *)dev->tx_buff,
                                 req->buflen,
                                 usock_result);
      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }
    }
  else
    {
      /* In case of invalid buffer length */

      *usock_result = -EINVAL;
      container_free(container);
      ret = REP_SEND_ACK_WOFREE;
    }

  return ret;
}

/****************************************************************************
 * name: alt1250_sms_recv
 ****************************************************************************/

int alt1250_sms_recv(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_recvfrom_s *req,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR struct usock_ackinfo_s *ackinfo)
{
  int ret = REP_SEND_ACK_WOFREE;
  FAR struct alt_container_s *container;

  dbg_alt1250("%s start\n", __func__);

  if (SMS_STATE(&dev->sms_info) == SMS_STATE_REOPEN)
    {
      return REP_NO_CONTAINER;
    }

  if (fill_recv_ackinfo(dev, usock, usock_result, ackinfo))
    {
      /* In case of buffer is empty */

      container = container_alloc();
      if (container == NULL)
        {
          dbg_alt1250("no container\n");
          return REP_NO_CONTAINER;
        }

      /* If the application has read all data,
       * change the status to SMS_STATE_WAITMSG.
       */

      dev->sms_info.total_msglen -= (dev->sms_info.msglen -
                                       sizeof(struct sms_recv_msg_header_s));
      if (dev->sms_info.total_msglen == 0)
        {
          SMS_SET_STATE(&dev->sms_info, SMS_STATE_WAITMSG);
        }

      /* Delete the SMS in the ALT1250 because one SMS was read. */

      ret = send_smsdelete_command(dev, container, usock,
                                   dev->sms_info.msg_index, usock_result);
      if (IS_NEED_CONTAINER_FREE(ret))
        {
          container_free(container);
        }
    }
  else
    {
      /* In case of buffer is not empty */

      ret = REP_SEND_DACK_RXREADY;
      dev->sms_info.read_msglen += req->max_buflen;
      ackinfo->usockid = USOCKET_USOCKID(usock);
    }

  return ret;
}

/****************************************************************************
 * name: usockreq_ioctl_sms
 ****************************************************************************/

int usockreq_ioctl_sms(FAR struct alt1250_s *dev,
                       FAR struct usrsock_request_buff_s *req,
                       FAR int32_t *usock_result,
                       FAR uint32_t *usock_xid,
                       FAR struct usock_ackinfo_s *ackinfo)
{
  FAR struct usrsock_request_ioctl_s *request = &req->request.ioctl_req;
  FAR struct lte_smsreq_s *smsreq = &req->req_ioctl.smsreq;
  FAR struct usock_s *usock;
  int ret = REP_SEND_ACK_WOFREE;

  dbg_alt1250("%s start\n", __func__);

  *usock_result = -EBADFD;

  usock = usocket_search(dev, request->usockid);
  if (usock)
    {
      switch (request->cmd)
        {
          case SIOCSMSENSTREP:
            *usock_result = OK;
            dev->sms_info.en_status_report = smsreq->smsru.enable;
            break;

          case SIOCSMSGREFID:
            *usock_result = OK;
            ret = REP_SEND_DACK;
            ackinfo->valuelen = sizeof(struct lte_smsreq_s);
            ackinfo->valuelen_nontrunc = sizeof(struct lte_smsreq_s);
            ackinfo->value_ptr = (FAR uint8_t *)USOCKET_REFID(usock);
            ackinfo->buf_ptr = NULL;
            break;

          case SIOCSMSSSCA:
            *usock_result = OK;
            memcpy(&dev->sms_info.dest_scaddr, &smsreq->smsru.scaddr,
                   sizeof(struct sms_sc_addr_s));
            break;

          default:
            *usock_result = -EINVAL;
            break;
        }
    }

  return ret;
}

/****************************************************************************
 * name: perform_sms_report_event
 ****************************************************************************/

uint64_t perform_sms_report_event(FAR struct alt1250_s *dev, uint64_t bitmap)
{
  uint64_t bit = 0ULL;
  FAR void **arg;

  if (alt1250_checkcmdid(LTE_CMDID_SMS_REPORT_RECV, bitmap, &bit))
    {
      arg = alt1250_getevtarg(LTE_CMDID_SMS_REPORT_RECV);
      if (arg && arg[0] && arg[1] && arg[2] && arg[3] && arg[4])
        {
          /* arg[0]: sms msg index
           * arg[1]: sms msg length
           * arg[2]: Maximum number of msg for concatenate sms.
           * arg[3]: Current number of msg for concatenate sms.
           * arg[4]: sms msg
           */

          sms_report_event(dev, *((FAR uint16_t *)arg[0]),
                           *((FAR uint16_t *)arg[1]),
                           *((FAR uint8_t *)arg[2]),
                           *((FAR uint8_t *)arg[3]),
                           (FAR struct sms_recv_msg_header_s *)arg[4]);
        }

      alt1250_setevtarg_writable(LTE_CMDID_SMS_REPORT_RECV);
    }

  return bit;
}

/****************************************************************************
 * name: alt1250_sms_initcontainer
 ****************************************************************************/

void alt1250_sms_initcontainer(FAR struct alt1250_s *dev)
{
  g_sms_container.priv = (unsigned long)&g_sms_postproc;
  clear_container(&g_sms_container);
}

/****************************************************************************
 * name: alt1250_reset_sms_info
 ****************************************************************************/

void alt1250_reset_sms_info(FAR struct alt1250_s *dev)
{
  notify_abort(dev);
}
