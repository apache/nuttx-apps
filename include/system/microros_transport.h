/****************************************************************************
 * apps/include/system/microros_transport.h
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

#ifndef __APPS_INCLUDE_SYSTEM_MICROROS_TRANSPORT_H
#define __APPS_INCLUDE_SYSTEM_MICROROS_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* microros_transport_init
 *
 * Register the NuttX-native transport (UDP or Serial, selected by Kconfig)
 * with the rmw_microxrcedds layer.  Must be called once before
 * rclc_support_init().
 *
 * Returns 0 on success, negated errno on failure.
 */

int microros_transport_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_SYSTEM_MICROROS_TRANSPORT_H */
