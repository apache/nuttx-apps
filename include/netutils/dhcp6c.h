/****************************************************************************
 * apps/include/netutils/dhcp6c.h
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

#ifndef __APPS_INCLUDE_NETUTILS_DHCP6C_H
#define __APPS_INCLUDE_NETUTILS_DHCP6C_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct dhcp6c_state
{
  struct in6_addr addr;
  struct in6_addr pd;       /* prefix address */
  struct in6_addr dns;
  struct in6_addr netmask;
  uint8_t pl;               /* prefix len */
  uint32_t t1;
  uint32_t t2;
};

typedef void (*dhcp6c_callback_t)(FAR struct dhcp6c_state *presult);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

FAR void *dhcp6c_open(FAR const char *interface);
int dhcp6c_request(FAR void *handle, FAR struct dhcp6c_state *presult);
int dhcp6c_request_async(FAR void *handle, dhcp6c_callback_t callback);
void dhcp6c_cancel(FAR void *handle);
void dhcp6c_close(FAR void *handle);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_DHCP6C_H */
