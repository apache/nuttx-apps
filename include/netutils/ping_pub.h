/****************************************************************************
 * apps/include/netutils/ping_pub.h
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

#ifndef __APPS_INCLUDE_NETUTILS_PING_PUB_H
#define __APPS_INCLUDE_NETUTILS_PING_PUB_H

struct ping_priv_s
{
  int code;                        /* Notice code ICMP_I/E/W_XXX */
  long tmin;                       /* Minimum round trip time */
  long tmax;                       /* Maximum round trip time */
  long long tsum;                  /* Sum of all times, for doing average */
  long long tsum2;                 /* Sum2 is the sum of the squares of sum ,for doing mean deviation */
};

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

void ping_result(FAR const struct ping_result_s *result);
void ping6_result(FAR const struct ping_result_s *result);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_PING_PUB_H */
