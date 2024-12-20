/****************************************************************************
 * apps/lte/alt1250/callback_handlers/alt1250_evt.h
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

#ifndef __APPS_LTE_ALT1250_CALLBACK_HANDLERS_ALT1250_EVT_H
#define __APPS_LTE_ALT1250_CALLBACK_HANDLERS_ALT1250_EVT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "alt1250_daemon.h"
#include "alt1250_util.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Represents when to clear the callback function */

/* Clear callbacks at all */

#define ALT1250_CLRMODE_ALL        (0)

/* Clear callbacks without restart callback */

#define ALT1250_CLRMODE_WO_RESTART (1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR struct alt_evtbuffer_s *init_event_buffer(void);
int alt1250_evtdatadestroy(void);
int alt1250_regevtcb(uint32_t id, FAR void *cb);
void alt1250_execcb(uint64_t evtbitmap);
FAR void **alt1250_getevtarg(uint32_t cmdid);
bool alt1250_checkcmdid(uint32_t cmdid, uint64_t evtbitmap,
                        FAR uint64_t *bit);
void alt1250_setevtarg_writable(uint32_t cmdid);
int alt1250_clrevtcb(uint8_t mode);

int alt1250_evttask_start(void);
void alt1250_evttask_stop(FAR struct alt1250_s *dev);
int alt1250_evttask_sendmsg(FAR struct alt1250_s *dev, uint64_t msg);
void alt1250_evttask_msgclose(FAR struct alt1250_s *dev);
int alt1250_evttask_msgconnect(FAR const char *qname,
                               FAR struct alt1250_s *dev);
uint32_t alt1250_search_registered_callback(FAR int *index);
int alt1250_get_report_ltecmd(FAR struct alt1250_s *dev, uint32_t cmdid,
                              FAR struct lte_ioctl_data_s *ltecmd);

#endif /* __APPS_LTE_ALT1250_CALLBACK_HANDLERS_ALT1250_EVT_H */
