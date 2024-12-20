/****************************************************************************
 * apps/lte/alt1250/alt1250_usockevent.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_USOCKEVENT_H
#define __APPS_LTE_ALT1250_ALT1250_USOCKEVENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/net/usrsock.h>

#include "alt1250_usockif.h"
#include "alt1250_daemon.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NEED_CONTAINER_FREE  (1<<16)
#define W_CONTAINER_FREE(a)  ((a) | NEED_CONTAINER_FREE)
#define WO_CONTAINER_FREE(a) (a)

#define IS_NEED_CONTAINER_FREE(r) ((r) & NEED_CONTAINER_FREE)

#define REP_SEND_ACK          W_CONTAINER_FREE(1)
#define REP_SEND_ACK_WOFREE   WO_CONTAINER_FREE(2)
#define REP_SEND_INPROG       W_CONTAINER_FREE(3)
#define REP_SEND_DACK         W_CONTAINER_FREE(4)
#define REP_SEND_TERM         W_CONTAINER_FREE(5)
#define REP_NO_CONTAINER      WO_CONTAINER_FREE(6)
#define REP_NO_ACK            W_CONTAINER_FREE(7)
#define REP_NO_ACK_WOFREE     WO_CONTAINER_FREE(8)
#define REP_MODEM_RESET       WO_CONTAINER_FREE(9)
#define REP_SEND_ACK_TXREADY  W_CONTAINER_FREE(10)
#define REP_SEND_DACK_RXREADY W_CONTAINER_FREE(11)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef int (*usrsock_reqhandler_t)(FAR struct alt1250_s *dev,
                                    FAR struct usrsock_request_buff_s *req,
                                    FAR int32_t *usock_result,
                                    FAR uint32_t *usock_xid,
                                    FAR struct usock_ackinfo_s *ackinfo);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int usock_reply(int ufd, int action_code, int32_t result,
                uint32_t xid, FAR struct usock_ackinfo_s *ackinfo);

int perform_usockrequest(FAR struct alt1250_s *dev);

#endif  /* __APPS_LTE_ALT1250_ALT1250_USOCKEVENT_H */
