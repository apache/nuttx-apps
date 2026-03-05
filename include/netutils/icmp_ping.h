/****************************************************************************
 * apps/include/netutils/icmp_ping.h
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

#ifndef __APPS_INCLUDE_NETUTILS_ICMP_PING_H
#define __APPS_INCLUDE_NETUTILS_ICMP_PING_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <netinet/in.h>
#include "icmp_pub.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Positive number represent information */

#define ICMP_I_OK          0   /* extra: not used      */
#define ICMP_I_BEGIN       1   /* extra: not used      */
#define ICMP_I_ROUNDTRIP   2   /* extra: packet delay  */
#define ICMP_I_FINISH      3   /* extra: elapsed time  */

/* Negative odd number represent error(unrecoverable) */

#define ICMP_E_MEMORY      -3  /* extra: not used      */
#define ICMP_E_SOCKET      -5  /* extra: error code    */
#define ICMP_E_SENDTO      -7  /* extra: error code    */
#define ICMP_E_SENDSMALL   -9  /* extra: sent bytes    */
#define ICMP_E_POLL        -11 /* extra: error code    */
#define ICMP_E_RECVFROM    -13 /* extra: error code    */
#define ICMP_E_RECVSMALL   -15 /* extra: recv bytes    */
#define ICMP_E_BINDDEV     -17 /* extra: error bind    */
#define ICMP_E_FILTER      -19 /* extra: error filter  */

/* Negative even number represent warning(recoverable) */

#define ICMP_W_TIMEOUT     -2  /* extra: timeout value */
#define ICMP_W_IDDIFF      -4  /* extra: recv id       */
#define ICMP_W_SEQNOBIG    -6  /* extra: recv seqno    */
#define ICMP_W_SEQNOSMALL  -8  /* extra: recv seqno    */
#define ICMP_W_RECVBIG     -10 /* extra: recv bytes    */
#define ICMP_W_DATADIFF    -12 /* extra: not used      */
#define ICMP_W_TYPE        -14 /* extra: recv type     */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_INCLUDE_NETUTILS_ICMP_PING_H */
