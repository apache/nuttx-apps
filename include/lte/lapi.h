/****************************************************************************
 * apps/include/lte/lapi.h
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

#ifndef __APPS_INCLUDE_LTE_LAPI_H
#define __APPS_INCLUDE_LTE_LAPI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include "lte/lte_api.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototyppes
 ****************************************************************************/

/****************************************************************************
 * Name: lapi_evtinit
 ****************************************************************************/

int lapi_evtinit(FAR const char *mqname);

/****************************************************************************
 * Name: lapi_evtdestoy
 ****************************************************************************/

void lapi_evtdestoy(void);

/****************************************************************************
 * Name: lapi_evtyield
 ****************************************************************************/

int lapi_evtyield(int timeout_ms);

/****************************************************************************
 * Name: lapi_req
 ****************************************************************************/

int lapi_req(uint32_t cmdid, FAR void *inp, size_t insz, FAR void *outp,
             size_t outsz, FAR void *cb);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_LTE_LAPI_H */
