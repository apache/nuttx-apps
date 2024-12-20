/****************************************************************************
 * apps/lte/alt1250/usock_handlers/alt1250_sms.h
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

#ifndef __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SMS_H
#define __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SMS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include "alt1250_container.h"
#include "alt1250_postproc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SMS_STATE(info)     ((info)->sms_state)
#define SMS_MSG_INDEX(info) ((info)->msg_index)

#define SMS_STATE_STR(ss) \
  ((ss) == SMS_STATE_UNINIT ? "SMS_STATE_UNINIT" : \
   (ss) == SMS_STATE_WAITMSG ? "SMS_STATE_WAITMSG" : \
   (ss) == SMS_STATE_READ_READY ? "SMS_STATE_READ_READY" : \
   (ss) == SMS_STATE_CALC_SIZE ? "SMS_STATE_CALC_SIZE" : \
   (ss) == SMS_STATE_REOPEN ? "SMS_STATE_REOPEN" : \
   (ss) == SMS_STATE_WAITMSG_CONCAT ? "SMS_STATE_WAITMSG_CONCAT" : \
   "ERROR UNKOWN STATE")

#define SMS_SET_STATE(info, s) { \
  dbg_alt1250("[SMS stat] state: %s -> %s\n", \
              SMS_STATE_STR((info)->sms_state), SMS_STATE_STR(s)); \
 (info)->sms_state = (s); \
}
#define SMS_SET_MSG_INDEX(info, x) ((info)->msg_index = (x))

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum sms_state_e
{
  SMS_STATE_UNINIT = 0,
  SMS_STATE_WAITMSG,
  SMS_STATE_READ_READY,
  SMS_STATE_CALC_SIZE,
  SMS_STATE_REOPEN,
  SMS_STATE_WAITMSG_CONCAT
};

struct sms_info_s
{
  enum sms_state_e sms_state;
  uint16_t msg_index;
  unsigned short msglen;
  unsigned short read_msglen;
  unsigned short total_msglen;
  bool is_first_msg;

  bool en_status_report;
  struct sms_sc_addr_s dest_scaddr;
#if defined(CONFIG_LTE_ALT1250_SMS_TOA)
  uint8_t dest_toa;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int alt1250_sms_init(FAR struct alt1250_s *dev, FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR struct usock_ackinfo_s *ackinfo);
int alt1250_sms_fin(FAR struct alt1250_s *dev, FAR struct usock_s *usock,
                    FAR int32_t *usock_result);
int alt1250_sms_send(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_sendto_s *req,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result);
int alt1250_sms_recv(FAR struct alt1250_s *dev,
                     FAR struct usrsock_request_recvfrom_s *req,
                     FAR struct usock_s *usock,
                     FAR int32_t *usock_result,
                     FAR struct usock_ackinfo_s *ackinfo);
int usockreq_ioctl_sms(FAR struct alt1250_s *dev,
                       FAR struct usrsock_request_buff_s *req,
                       FAR int32_t *usock_result,
                       FAR uint32_t *usock_xid,
                       FAR struct usock_ackinfo_s *ackinfo);
uint64_t perform_sms_report_event(FAR struct alt1250_s *dev,
                                  uint64_t bitmap);
void alt1250_sms_initcontainer(FAR struct alt1250_s *dev);
void alt1250_reset_sms_info(FAR struct alt1250_s *dev);

#endif  /* __APPS_LTE_ALT1250_USOCK_HANDLERS_ALT1250_SMS_H */
