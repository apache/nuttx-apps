/****************************************************************************
 * apps/examples/nimble_blecent/blecent.h
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

#ifndef __APPS_EXAMPLES_NIMBLE_BLECENT_BLECENT_H
#define __APPS_EXAMPLES_NIMBLE_BLECENT_BLECENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLECENT_SVC_ALERT_UUID              0x1811
#define BLECENT_CHR_SUP_NEW_ALERT_CAT_UUID  0x2A47
#define BLECENT_CHR_NEW_ALERT               0x2A46
#define BLECENT_CHR_SUP_UNR_ALERT_CAT_UUID  0x2A48
#define BLECENT_CHR_UNR_ALERT_STAT_UUID     0x2A45
#define BLECENT_CHR_ALERT_NOT_CTRL_PT       0x2A44

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Peer. */

struct peer_dsc
{
  SLIST_ENTRY(peer_dsc) next;
  struct ble_gatt_dsc dsc;
};
SLIST_HEAD(peer_dsc_list, peer_dsc);

struct peer_chr
{
  SLIST_ENTRY(peer_chr) next;
  struct ble_gatt_chr chr;

  struct peer_dsc_list dscs;
};
SLIST_HEAD(peer_chr_list, peer_chr);

struct peer_svc
{
  SLIST_ENTRY(peer_svc) next;
  struct ble_gatt_svc svc;

  struct peer_chr_list chrs;
};
SLIST_HEAD(peer_svc_list, peer_svc);

struct peer;
typedef void peer_disc_fn(const struct peer *peer, int status, void *arg);

struct peer
{
  SLIST_ENTRY(peer) next;

  uint16_t conn_handle;

  /* List of discovered GATT services. */

  struct peer_svc_list svcs;

  /* Keeps track of where we are in the service discovery process. */

  uint16_t disc_prev_chr_val;
  struct peer_svc *cur_svc;

  /* Callback that gets executed when service discovery completes. */

  peer_disc_fn *disc_cb;
  void *disc_cb_arg;
};

/****************************************************************************
 * Public Type Definition
 ****************************************************************************/

/* Forward references */

struct ble_hs_adv_fields;
struct ble_gap_conn_desc;
struct ble_hs_cfg;
union ble_store_value;
union ble_store_key;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/* Misc. */

void print_mbuf(FAR const struct os_mbuf *om);
FAR char *addr_str(FAR const void *addr);
void print_uuid(FAR const ble_uuid_t *uuid);
void print_conn_desc(FAR const struct ble_gap_conn_desc *desc);
void print_adv_fields(FAR const struct ble_hs_adv_fields *fields);

int peer_disc_all(uint16_t conn_handle, FAR peer_disc_fn *disc_cb,
                  FAR void *disc_cb_arg);
FAR const struct peer_dsc *
peer_dsc_find_uuid(FAR const struct peer *peer,
                   FAR const ble_uuid_t *svc_uuid,
                   FAR const ble_uuid_t *chr_uuid,
                   FAR const ble_uuid_t *dsc_uuid);
FAR const struct peer_chr *
peer_chr_find_uuid(FAR const struct peer *peer,
                   FAR const ble_uuid_t *svc_uuid,
                   FAR const ble_uuid_t *chr_uuid);
FAR const struct peer_svc *
peer_svc_find_uuid(FAR const struct peer *peer,
                   FAR const ble_uuid_t *uuid);
int peer_delete(uint16_t conn_handle);
int peer_add(uint16_t conn_handle);
int peer_init(int max_peers, int max_svcs, int max_chrs, int max_dscs);

#ifdef __cplusplus
}
#endif

#endif  /* __APPS_EXAMPLES_NIMBLE_BLECENT_BLECENT_H */
