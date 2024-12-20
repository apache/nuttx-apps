/****************************************************************************
 * apps/lte/alt1250/alt1250_postproc.h
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

#ifndef __APPS_LTE_ALT1250_ALT1250_POSTPROC_H
#define __APPS_LTE_ALT1250_ALT1250_POSTPROC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <nuttx/modem/alt1250.h>

#include "alt1250_daemon.h"
#include "alt1250_usockif.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef CODE int (*postproc_hdlr_t)(FAR struct alt1250_s *dev,
  FAR struct alt_container_s *reply, FAR struct usock_s *usock,
  FAR int32_t *usock_result, uint32_t *usock_xid,
  FAR struct usock_ackinfo_s *ackinfo, unsigned long arg);

struct postproc_s
{
  postproc_hdlr_t hdlr;
  unsigned long priv;
};

#endif /* __APPS_LTE_ALT1250_ALT1250_POSTPROC_H */
