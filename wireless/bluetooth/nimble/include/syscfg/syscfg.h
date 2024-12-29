/****************************************************************************
 * apps/wireless/bluetooth/nimble/include/syscfg/syscfg.h
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

#ifndef __APPS_WIRELESS_BLUETOOTH_NIMBLE_INCLUDE_SYSCFG_SYSCFG_H
#define __APPS_WIRELESS_BLUETOOTH_NIMBLE_INCLUDE_SYSCFG_SYSCFG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Macros used by NimBLE ****************************************************/

#define MYNEWT_VAL(_name)               MYNEWT_VAL_ ## _name
#define MYNEWT_VAL_CHOICE(_name, _val)  MYNEWT_VAL_ ## _name ## __ ## _val

/* NimBLE NuttX specific configuration **************************************/

#define MYNEWT_VAL_BLE_SOCK_USE_NUTTX        (1)
#define MYNEWT_VAL_NEWT_FEATURE_LOGCFG       (1)

/* NimBLE user specific configuration ***************************************/

#define MYNEWT_VAL_MSYS_1_BLOCK_COUNT        (CONFIG_NIMBLE_MSYS_1_BLOCK_COUNT)
#define MYNEWT_VAL_MSYS_1_BLOCK_SIZE         (CONFIG_NIMBLE_MSYS_1_BLOCK_SIZE)
#define MYNEWT_VAL_MSYS_2_BLOCK_COUNT        (CONFIG_NIMBLE_MSYS_2_BLOCK_COUNT)
#define MYNEWT_VAL_MSYS_2_BLOCK_SIZE         (CONFIG_NIMBLE_MSYS_2_BLOCK_SIZE)

/* MSYS sanity check not supported */

#define MYNEWT_VAL_MSYS_1_SANITY_MIN_COUNT   (0)
#define MYNEWT_VAL_MSYS_2_SANITY_MIN_COUNT   (0)

/* Transport configuration */

#define MYNEWT_VAL_BLE_TRANSPORT_ACL_FROM_HS_COUNT     (10)
#define MYNEWT_VAL_BLE_TRANSPORT_ACL_FROM_LL_COUNT     (10)
#define MYNEWT_VAL_BLE_TRANSPORT_ACL_SIZE              (251)
#define MYNEWT_VAL_BLE_TRANSPORT_EVT_COUNT             (4)
#define MYNEWT_VAL_BLE_TRANSPORT_EVT_DISCARDABLE_COUNT (16)
#define MYNEWT_VAL_BLE_TRANSPORT_EVT_SIZE              (70)

/* L2CAP configuration */

#define MYNEWT_VAL_BLE_L2CAP_COC_MAX_NUM        (CONFIG_NIMBLE_L2CAP_COC_MAX_NUM)
#define MYNEWT_VAL_BLE_L2CAP_COC_MPS            (MYNEWT_VAL_MSYS_1_BLOCK_SIZE-8)
#define MYNEWT_VAL_BLE_L2CAP_COC_SDU_BUFF_COUNT (1)
#define MYNEWT_VAL_BLE_L2CAP_ENHANCED_COC       (0)
#define MYNEWT_VAL_BLE_L2CAP_JOIN_RX_FRAGS      (1)
#define MYNEWT_VAL_BLE_L2CAP_MAX_CHANS          (3*MYNEWT_VAL_BLE_MAX_CONNECTIONS)
#define MYNEWT_VAL_BLE_L2CAP_RX_FRAG_TIMEOUT    (30000)
#define MYNEWT_VAL_BLE_L2CAP_SIG_MAX_PROCS      (1)

/* Host configuration */

#define MYNEWT_VAL_BLE_HOST                     (1)
#define MYNEWT_VAL_BLE_HS_AUTO_START            (1)
#define MYNEWT_VAL_BLE_HS_DEBUG                 (0)

#ifdef CONFIG_NIMBLE_HS_FLOW_CTRL
#  define MYNEWT_VAL_BLE_HS_FLOW_CTRL                  (1)
#  define MYNEWT_VAL_BLE_HS_FLOW_CTRL_ITVL             (1000)
#  define MYNEWT_VAL_BLE_HS_FLOW_CTRL_THRESH           (2)
#  define MYNEWT_VAL_BLE_HS_FLOW_CTRL_TX_ON_DISCONNECT (0)
#else
#  define MYNEWT_VAL_BLE_HS_FLOW_CTRL                  (0)
#endif

#define MYNEWT_VAL_BLE_HS_PHONY_HCI_ACKS               (0)
#define MYNEWT_VAL_BLE_HS_REQUIRE_OS                   (1)
#define MYNEWT_VAL_BLE_HS_STOP_ON_SHUTDOWN             (1)
#define MYNEWT_VAL_BLE_HS_STOP_ON_SHUTDOWN_TIMEOUT     (2000)

/* LE Isochronous Channels not supported for now */

#define MYNEWT_BLE_ISO                       (0)
#define MYNEWT_VAL_BLE_ISO_MAX_BISES         (0)

/* BLE role configuration */

#ifdef CONFIG_NIMBLE_ROLE_BROADCASTER
#  define MYNEWT_VAL_BLE_ROLE_BROADCASTER    (1)
#else
#  define MYNEWT_VAL_BLE_ROLE_BROADCASTER    (0)
#endif

#ifdef CONFIG_NIMBLE_ROLE_CENTRAL
#  define MYNEWT_VAL_BLE_ROLE_CENTRAL        (1)
#else
#  define MYNEWT_VAL_BLE_ROLE_CENTRAL        (0)
#endif

#ifdef CONFIG_NIMBLE_ROLE_OBSERVER
#  define MYNEWT_VAL_BLE_ROLE_OBSERVER       (1)
#else
#  define MYNEWT_VAL_BLE_ROLE_OBSERVER       (0)
#endif

#ifdef CONFIG_NIMBLE_ROLE_PERIPHERAL
#  define MYNEWT_VAL_BLE_ROLE_PERIPHERAL     (1)
#else
#  define MYNEWT_VAL_BLE_ROLE_PERIPHERAL     (0)
#endif

/* BLE features */

#ifdef CONFIG_NIMBLE_MESH
#  define MYNEWT_VAL_BLE_MESH                       (1)
#else
#  define MYNEWT_VAL_BLE_MESH                       (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_CONN_SUBRATING
#  define MYNEWT_VAL_BLE_CONN_SUBRATING             (1)
#else
#  define MYNEWT_VAL_BLE_CONN_SUBRATING             (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_EXT_ADV
#  define MYNEWT_VAL_BLE_EXT_ADV                    (1)
#  define MYNEWT_VAL_BLE_EXT_ADV_MAX_SIZE           (CONFIG_NIMBLE_BLE_EXT_ADV_MAX_SIZE)
#else
#  define MYNEWT_VAL_BLE_EXT_ADV                    (0)
#endif

#define MYNEWT_VAL_BLE_MAX_CONNECTIONS              (CONFIG_NIMBLE_BLE_MAX_CONN)
#define MYNEWT_VAL_BLE_MAX_PERIODIC_SYNCS           (CONFIG_NIMBLE_BLE_MAX_PERIODIC_SYNCS)
#define MYNEWT_VAL_BLE_MULTI_ADV_INSTANCES          (CONFIG_NIMBLE_BLE_MULTI_ADV_INSTANCES)
#define MYNEWT_VAL_BLE_HS_EXT_ADV_LEGACY_INSTANCE   (0)

#ifdef CONFIG_NIMBLE_BLE_PERIODIC_ADV
#  define MYNEWT_VAL_BLE_PERIODIC_ADV               (1)
#else
#  define MYNEWT_VAL_BLE_PERIODIC_ADV               (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_PERIODIC_ADV_SYNC_TRANSFER
#  define MYNEWT_VAL_BLE_PERIODIC_ADV_SYNC_TRANSFER (1)
#else
#  define MYNEWT_VAL_BLE_PERIODIC_ADV_SYNC_TRANSFER (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_POWER_CONTROL
#  define MYNEWT_VAL_BLE_POWER_CONTROL              (1)
#else
#  define MYNEWT_VAL_BLE_POWER_CONTROL              (0)
#endif

#define MYNEWT_VAL_BLE_VERSION                      (CONFIG_NIMBLE_BLE_VERSION)

#ifdef CONFIG_NIMBLE_BLE_WHITELIST
#  define MYNEWT_VAL_BLE_WHITELIST                  (1)
#else
#  define MYNEWT_VAL_BLE_WHITELIST                  (0)
#endif

/* ATT configuration */

#define MYNEWT_VAL_BLE_ATT_PREFERRED_MTU        (CONFIG_NIMBLE_BLE_ATT_PREFFERED_MTU)
#define MYNEWT_VAL_BLE_ATT_SVR_FIND_INFO        (1)
#define MYNEWT_VAL_BLE_ATT_SVR_FIND_TYPE        (1)
#define MYNEWT_VAL_BLE_ATT_SVR_INDICATE         (1)
#define MYNEWT_VAL_BLE_ATT_SVR_MAX_PREP_ENTRIES (64)
#define MYNEWT_VAL_BLE_ATT_SVR_NOTIFY           (1)
#define MYNEWT_VAL_BLE_ATT_SVR_NOTIFY_MULTI     (1)
#define MYNEWT_VAL_BLE_ATT_SVR_QUEUED_WRITE     (1)
#define MYNEWT_VAL_BLE_ATT_SVR_QUEUED_WRITE_TMO (30000)
#define MYNEWT_VAL_BLE_ATT_SVR_READ             (1)
#define MYNEWT_VAL_BLE_ATT_SVR_READ_BLOB        (1)
#define MYNEWT_VAL_BLE_ATT_SVR_READ_GROUP_TYPE  (1)
#define MYNEWT_VAL_BLE_ATT_SVR_READ_MULT        (1)
#define MYNEWT_VAL_BLE_ATT_SVR_READ_TYPE        (1)
#define MYNEWT_VAL_BLE_ATT_SVR_SIGNED_WRITE     (1)
#define MYNEWT_VAL_BLE_ATT_SVR_WRITE            (1)
#define MYNEWT_VAL_BLE_ATT_SVR_WRITE_NO_RSP     (1)

/* EATT disabled for now */

#define MYNEWT_VAL_BLE_EATT_CHAN_NUM            (0)

/* GAP configuration */

#define MYNEWT_VAL_BLE_GAP_MAX_PENDING_CONN_PARAM_UPDATE (1)

/* GATT configuration */

#define MYNEWT_VAL_BLE_GATT_DISC_ALL_CHRS   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_DISC_ALL_DSCS   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_DISC_ALL_SVCS   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_DISC_CHR_UUID   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_DISC_SVC_UUID   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_FIND_INC_SVCS   (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_INDICATE        (1)
#define MYNEWT_VAL_BLE_GATT_MAX_PROCS       (4)
#define MYNEWT_VAL_BLE_GATT_NOTIFY          (1)
#define MYNEWT_VAL_BLE_GATT_READ            (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_READ_LONG       (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_READ_MAX_ATTRS  (8)
#define MYNEWT_VAL_BLE_GATT_READ_MULT       (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_READ_UUID       (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_RESUME_RATE     (1000)
#define MYNEWT_VAL_BLE_GATT_SIGNED_WRITE    (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_WRITE           (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_WRITE_LONG      (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_WRITE_MAX_ATTRS (4)
#define MYNEWT_VAL_BLE_GATT_WRITE_NO_RSP    (MYNEWT_VAL_BLE_ROLE_CENTRAL)
#define MYNEWT_VAL_BLE_GATT_WRITE_RELIABLE  (MYNEWT_VAL_BLE_ROLE_CENTRAL)

/* Security configuration */

#define MYNEWT_VAL_BLE_RPA_TIMEOUT          (CONFIG_NIMBLE_BLE_RPA_TIMEOUT)

#ifdef CONFIG_NIMBLE_BLE_SM_BONDING
#  define MYNEWT_VAL_BLE_SM_BONDING         (1)
#else
#  define MYNEWT_VAL_BLE_SM_BONDING         (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_SM_LEGACY
#  define MYNEWT_VAL_BLE_SM_LEGACY          (1)
#else
#  define MYNEWT_VAL_BLE_SM_LEGACY          (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_SM_SC
#  define MYNEWT_VAL_BLE_SM_SC              (1)
#else
#  define MYNEWT_VAL_BLE_SM_SC              (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_SM_SC_DEBUG
#  define MYNEWT_VAL_BLE_SM_SC_DEBUG_KEYS   (1)
#else
#  define MYNEWT_VAL_BLE_SM_SC_DEBUG_KEYS   (0)
#endif

#ifdef CONFIG_NIMBLE_BLE_SM_SC_ONLY
#  define MYNEWT_VAL_BLE_SM_SC_ONLY         (1)
#else
#  define MYNEWT_VAL_BLE_SM_SC_ONLY         (0)
#endif

#define MYNEWT_VAL_BLE_SM_IO_CAP            (BLE_HS_IO_NO_INPUT_OUTPUT)
#define MYNEWT_VAL_BLE_SM_KEYPRESS          (0)
#define MYNEWT_VAL_BLE_SM_LVL               (0)
#define MYNEWT_VAL_BLE_SM_MAX_PROCS         (1)
#define MYNEWT_VAL_BLE_SM_MITM              (0)
#define MYNEWT_VAL_BLE_SM_OOB_DATA_FLAG     (0)
#define MYNEWT_VAL_BLE_SM_OUR_KEY_DIST      (0)
#define MYNEWT_VAL_BLE_SM_THEIR_KEY_DIST    (0)
#define MYNEWT_VAL_BLE_STORE_MAX_BONDS      (3)
#define MYNEWT_VAL_BLE_STORE_MAX_CCCDS      (8)

/* ANS service */

#define MYNEWT_VAL_BLE_SVC_ANS_NEW_ALERT_CAT (0)
#define MYNEWT_VAL_BLE_SVC_ANS_UNR_ALERT_CAT (0)

/* BAS service */

#define MYNEWT_VAL_BLE_SVC_BAS_BATTERY_LEVEL_NOTIFY_ENABLE (1)
#define MYNEWT_VAL_BLE_SVC_BAS_BATTERY_LEVEL_READ_PERM     (0)

/* DIS service */

#define MYNEWT_VAL_BLE_SVC_DIS_DEFAULT_READ_PERM           (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_FIRMWARE_REVISION_DEFAULT   (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_FIRMWARE_REVISION_READ_PERM (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_HARDWARE_REVISION_DEFAULT   (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_HARDWARE_REVISION_READ_PERM (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_MANUFACTURER_NAME_DEFAULT   (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_MANUFACTURER_NAME_READ_PERM (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_MODEL_NUMBER_DEFAULT        "Apache NuttX NimBLE"
#define MYNEWT_VAL_BLE_SVC_DIS_MODEL_NUMBER_READ_PERM      (0)
#define MYNEWT_VAL_BLE_SVC_DIS_SERIAL_NUMBER_DEFAULT       (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_SERIAL_NUMBER_READ_PERM     (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_SOFTWARE_REVISION_DEFAULT   (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_SOFTWARE_REVISION_READ_PERM (-1)
#define MYNEWT_VAL_BLE_SVC_DIS_SYSTEM_ID_DEFAULT           (NULL)
#define MYNEWT_VAL_BLE_SVC_DIS_SYSTEM_ID_READ_PERM         (-1)

/* GAP service */

#define MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE                  (0)
#define MYNEWT_VAL_BLE_SVC_GAP_APPEARANCE_WRITE_PERM       (-1)
#define MYNEWT_VAL_BLE_SVC_GAP_CENTRAL_ADDRESS_RESOLUTION  (-1)
#define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME                 "nimble"
#define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME_MAX_LENGTH      (31)
#define MYNEWT_VAL_BLE_SVC_GAP_DEVICE_NAME_WRITE_PERM      (-1)
#define MYNEWT_VAL_BLE_SVC_GAP_PPCP_MAX_CONN_INTERVAL      (0)
#define MYNEWT_VAL_BLE_SVC_GAP_PPCP_MIN_CONN_INTERVAL      (0)
#define MYNEWT_VAL_BLE_SVC_GAP_PPCP_SLAVE_LATENCY          (0)
#define MYNEWT_VAL_BLE_SVC_GAP_PPCP_SUPERVISION_TMO        (0)

#endif  /* __APPS_WIRELESS_BLUETOOTH_NIMBLE_INCLUDE_SYSCFG_SYSCFG_H */
