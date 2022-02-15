/****************************************************************************
 * apps/netutils/iperf/iperf.h
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

#ifndef __APPS_NETUTILS_IPERF_IPERF_H
#define __APPS_NETUTILS_IPERF_IPERF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IPERF_FLAG_CLIENT (1)
#define IPERF_FLAG_SERVER (1 << 1)
#define IPERF_FLAG_TCP (1 << 2)
#define IPERF_FLAG_UDP (1 << 3)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct iperf_cfg_t
{
  uint32_t flag;
  uint32_t dip;
  uint32_t sip;
  uint16_t dport;
  uint16_t sport;
  uint32_t interval;
  uint32_t time;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: iperf_start
 *
 * Description:
 *   Start iperf task.
 *
 ****************************************************************************/

int iperf_start(struct iperf_cfg_t *cfg);

/****************************************************************************
 * Name: iperf_stop
 *
 * Description:
 *   Stop iperf task.
 *
 ****************************************************************************/

int iperf_stop(void);

#ifdef __cplusplus
}
#endif
#undef EXTERN

#endif /* __ASSEMBLY__ */
#endif /* __APPS_NETUTILS_IPERF_IPERF_H */
