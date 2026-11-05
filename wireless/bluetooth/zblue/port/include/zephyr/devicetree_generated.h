/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/include/zephyr/devicetree_generated.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_DEVICETREE_GENERATED_H
#define __APPS_WIRELESS_BLUETOOTH_ZBLUE_PORT_INCLUDE_ZEPHYR_DEVICETREE_GENERATED_H

/* Hand written replacement of the file the Zephyr build generates from the
 * devicetree.  It describes one node, the HCI device of the port
 * (port/src/hci.c), and makes it the zephyr,bt-hci chosen node.  Upstream
 * <zephyr/devicetree.h> works on top of it unchanged.
 */

#define DT_N_PATH "/"
#define DT_N_EXISTS 1
#define DT_N_ORD 0

#define DT_N_S_zblue_hci_PATH "/zblue_hci"
#define DT_N_S_zblue_hci_EXISTS 1
#define DT_N_S_zblue_hci_ORD 1
#define DT_N_S_zblue_hci_STATUS_okay 1
#define DT_N_S_zblue_hci_PARENT DT_N

#define DT_CHOSEN_zephyr_bt_hci DT_N_S_zblue_hci
#define DT_CHOSEN_zephyr_bt_hci_EXISTS 1

#define DT_FOREACH_HELPER(fn) fn(DT_N_S_zblue_hci)
#define DT_FOREACH_OKAY_HELPER(fn) fn(DT_N_S_zblue_hci)
#define DT_FOREACH_VARGS_HELPER(fn, ...) fn(DT_N_S_zblue_hci, __VA_ARGS__)
#define DT_FOREACH_OKAY_VARGS_HELPER(fn, ...) \
  fn(DT_N_S_zblue_hci, __VA_ARGS__)

#endif
