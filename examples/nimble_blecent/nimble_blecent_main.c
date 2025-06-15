/****************************************************************************
 * apps/examples/nimble_blecent/nimble_blecent_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <string.h>

/* BLE */

#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "host/ble_hs.h"
#include "host/util/util.h"

/* Mandatory services. */

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/bas/ble_svc_bas.h"

/* Application-specified header. */

#include "blecent.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLECENT_BAS_UUID 0x180f
#define BLECENT_CHR_BAS_BL_UUID 0x2a19

/* Not used now */

#define TASK_DEFAULT_PRIORITY CONFIG_EXAMPLES_NIMBLE_BLECENT_PRIORITY
#define TASK_DEFAULT_STACK NULL
#define TASK_DEFAULT_STACK_SIZE 0

/****************************************************************************
 * External Functions Prototypes
 ****************************************************************************/

void ble_hci_sock_ack_handler(FAR void *param);
void ble_hci_sock_set_device(int dev);

/****************************************************************************
 * Private Functions Prototypes
 ****************************************************************************/

static int blecent_gap_event(FAR struct ble_gap_event *event, FAR void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: blecent_on_read
 ****************************************************************************/

static int
blecent_on_read(uint16_t conn_handle,
                FAR const struct ble_gatt_error *error,
                FAR struct ble_gatt_attr *attr,
                FAR void *arg)
{
  printf("Read complete; status=%d conn_handle=%d", error->status,
         conn_handle);
  if (error->status == 0)
    {
      printf(" attr_handle=%d value=", attr->handle);
      print_mbuf(attr->om);
    }

  printf("\n");

  return 0;
}

/****************************************************************************
 * Name: blecent_on_write
 ****************************************************************************/

static int
blecent_on_write(uint16_t conn_handle,
                 FAR const struct ble_gatt_error *error,
                 FAR struct ble_gatt_attr *attr,
                 FAR void *arg)
{
  printf("Write complete; status=%d conn_handle=%d attr_handle=%d\n",
         error->status, conn_handle, attr->handle);

  return 0;
}

/* Application callback. Called when the attempt to subscribe to
 * notifications for the ANS Unread Alert Status characteristic
 * has completed.
 */

static int
blecent_on_subscribe(uint16_t conn_handle,
                     FAR const struct ble_gatt_error *error,
                     FAR struct ble_gatt_attr *attr,
                     FAR void *arg)
{
  printf("Subscribe complete; status=%d conn_handle=%d "
         "attr_handle=%d\n",
         error->status, conn_handle, attr->handle);

  return 0;
}

/****************************************************************************
 * Name: blecent_read_write_subscribe
 ****************************************************************************/

static void blecent_read_write_subscribe(FAR const struct peer *peer)
{
  const struct peer_chr *chr;
  const struct peer_dsc *dsc;
  uint8_t value[2];
  int rc;

  /* Read the supported-new-alert-category characteristic. */

  chr = peer_chr_find_uuid(
      peer, BLE_UUID16_DECLARE(BLECENT_SVC_ALERT_UUID),
      BLE_UUID16_DECLARE(BLECENT_CHR_SUP_NEW_ALERT_CAT_UUID));
  if (chr == NULL)
    {
      printf("Error: Peer doesn't support the Supported New "
             "Alert Category characteristic\n");
      goto err;
    }

  rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle,
                      blecent_on_read, NULL);
  if (rc != 0)
    {
      printf("Error: Failed to read characteristic; rc=%d\n", rc);
      goto err;
    }

  /* Write two bytes (99, 100) to the alert-notification-control-point
   * characteristic.
   */

  chr = peer_chr_find_uuid(
      peer, BLE_UUID16_DECLARE(BLECENT_SVC_ALERT_UUID),
      BLE_UUID16_DECLARE(BLECENT_CHR_ALERT_NOT_CTRL_PT));
  if (chr == NULL)
    {
      printf("Error: Peer doesn't support the Alert "
             "Notification Control Point characteristic\n");
      goto err;
    }

  value[0] = 99;
  value[1] = 100;
  rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle, value,
                            sizeof value, blecent_on_write, NULL);
  if (rc != 0)
    {
      printf("Error: Failed to write characteristic; rc=%d\n", rc);
    }

  /* Subscribe to notifications for the Battery Level characteristic.
   * A central enables notifications by writing two bytes (1, 0) to the
   * characteristic's client-characteristic-configuration-descriptor (CCCD).
   */

  dsc = peer_dsc_find_uuid(
      peer, BLE_UUID16_DECLARE(BLE_SVC_BAS_UUID16),
      BLE_UUID16_DECLARE(BLE_SVC_BAS_CHR_UUID16_BATTERY_LEVEL),
      BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));
  if (dsc == NULL)
    {
      printf("Error: Peer lacks a CCCD for the \
          Battery Level characteristic\n");
      goto err;
    }

  value[0] = 1;
  value[1] = 0;
  rc       = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle, value,
                                  sizeof value, blecent_on_subscribe, NULL);
  if (rc != 0)
    {
      printf("Error: Failed to subscribe to characteristic; "
             "rc=%d\n",
             rc);
      goto err;
    }

  return;

err:

  /* Terminate the connection. */

  ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

/****************************************************************************
 * Name: blecent_on_disc_complete
 ****************************************************************************/

static void
blecent_on_disc_complete(FAR const struct peer *peer, int status,
                         FAR void *arg)
{
  if (status != 0)
    {
      /* Service discovery failed.  Terminate the connection. */

      printf("Error: Service discovery failed; status=%d "
             "conn_handle=%d\n",
             status, peer->conn_handle);
      ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
      return;
    }

  /* Service discovery has completed successfully.  Now we have a complete
   * list of services, characteristics, and descriptors that the peer
   * supports.
   */

  printf("Service discovery complete; status=%d "
         "conn_handle=%d\n",
         status, peer->conn_handle);

  /* Now perform three concurrent GATT procedures against the peer: read,
   * write, and subscribe to notifications.
   */

  blecent_read_write_subscribe(peer);
}

/****************************************************************************
 * Name: blecent_scan
 ****************************************************************************/

static void blecent_scan(void)
{
  struct ble_gap_disc_params disc_params;
  uint8_t own_addr_type;
  int rc;

  /* Figure out address to use while advertising (no privacy for now) */

  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0)
    {
      printf("error determining address type; rc=%d\n", rc);
      return;
    }

  /* Tell the controller to filter duplicates; we don't want to process
   * repeated advertisements from the same device.
   */

  disc_params.filter_duplicates = 1;

  /* Perform a passive scan.  I.e., don't send follow-up scan requests to
   * each advertiser.
   */

  disc_params.passive = 1;

  /* Use defaults for the rest of the parameters. */

  disc_params.itvl          = 0;
  disc_params.window        = 0;
  disc_params.filter_policy = 0;
  disc_params.limited       = 0;

  rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                    blecent_gap_event, NULL);
  if (rc != 0)
    {
      printf("Error initiating GAP discovery procedure; rc=%d\n", rc);
    }
}

/****************************************************************************
 * Name: blecent_should_connect
 ****************************************************************************/

static int blecent_should_connect(const struct ble_gap_disc_desc *disc)
{
  struct ble_hs_adv_fields fields;
  int rc;
  int i;

  /* The device has to be advertising connectability. */

  if (disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_ADV_IND
      && disc->event_type != BLE_HCI_ADV_RPT_EVTYPE_DIR_IND)
    {
      return 0;
    }

  rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
  if (rc != 0)
    {
      return 0;
    }

  /* The device has to advertise support for the Alert Notification
   * service (0x1811).
   */

  for (i = 0; i < fields.num_uuids16; i++)
    {
      if (ble_uuid_u16(&fields.uuids16[i].u) == BLECENT_SVC_ALERT_UUID)
        {
          return 1;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: blecent_connect_if_interesting
 ****************************************************************************/

static void
blecent_connect_if_interesting(FAR const struct ble_gap_disc_desc *disc)
{
  uint8_t own_addr_type;
  int rc;

  /* Don't do anything if we don't care about this advertiser. */

  if (!blecent_should_connect(disc))
    {
      return;
    }

  /* Scanning must be stopped before a connection can be initiated. */

  rc = ble_gap_disc_cancel();
  if (rc != 0)
    {
      printf("Failed to cancel scan; rc=%d\n", rc);
      return;
    }

  /* Figure out address to use for connect (no privacy for now) */

  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0)
    {
      printf("error determining address type; rc=%d\n", rc);
      return;
    }

  /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
   * timeout.
   */

  rc = ble_gap_connect(own_addr_type, &disc->addr, 30000, NULL,
                       blecent_gap_event, NULL);
  if (rc != 0)
    {
      printf("Error: Failed to connect to device; addr_type=%d "
             "addr=%s\n; rc=%d",
             disc->addr.type, addr_str(disc->addr.val), rc);
      return;
    }
}

/****************************************************************************
 * Name: blecent_gap_event
 ****************************************************************************/

static int blecent_gap_event(FAR struct ble_gap_event *event, FAR void *arg)
{
  struct ble_gap_conn_desc desc;
  struct ble_hs_adv_fields fields;
  int rc;

  switch (event->type)
    {
      case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0)
          {
            return 0;
          }

        /* An advertisment report was received during GAP discovery. */

        print_adv_fields(&fields);

        /* Try to connect to the advertiser if it looks interesting. */

        blecent_connect_if_interesting(&event->disc);
        return 0;

      case BLE_GAP_EVENT_CONNECT:

        /* A new connection was established or a connection attempt failed.
         */

        if (event->connect.status == 0)
          {
            /* Connection successfully established. */

            printf("Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);
            printf("\n");

            /* Remember peer. */

            rc = peer_add(event->connect.conn_handle);
            if (rc != 0)
              {
                printf("Failed to add peer; rc=%d\n", rc);
                return 0;
              }

            /* Perform service discovery. */

            rc = peer_disc_all(event->connect.conn_handle,
                               blecent_on_disc_complete, NULL);
            if (rc != 0)
              {
                printf("Failed to discover services; rc=%d\n", rc);
                return 0;
              }
          }
        else
          {
            /* Connection attempt failed; resume scanning. */

            printf("Error: Connection failed; status=%d\n",
                   event->connect.status);
            blecent_scan();
          }

        return 0;

      case BLE_GAP_EVENT_DISCONNECT:

        /* Connection terminated. */

        printf("disconnect; reason=%d ", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        printf("\n");

        /* Forget about peer. */

        peer_delete(event->disconnect.conn.conn_handle);

        /* Resume scanning. */

        blecent_scan();
        return 0;

      case BLE_GAP_EVENT_DISC_COMPLETE:
        printf("discovery complete; reason=%d\n",
               event->disc_complete.reason);
        return 0;

      case BLE_GAP_EVENT_ENC_CHANGE:

        /* Encryption has been enabled or disabled for this connection. */

        printf("encryption change event; status=%d ",
               event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        print_conn_desc(&desc);
        return 0;

      case BLE_GAP_EVENT_NOTIFY_RX:

        /* Peer sent us a notification or indication. */

        printf("received %s; conn_handle=%d attr_handle=%d "
               "attr_len=%d\n",
               event->notify_rx.indication ? "indication" : "notification",
               event->notify_rx.conn_handle, event->notify_rx.attr_handle,
               OS_MBUF_PKTLEN(event->notify_rx.om));

        uint8_t notif_data[100]; /* Size depending on the actual size of the
                                    notification data you have */
        uint16_t notif_len;
        int offset = 0;
        notif_len  = OS_MBUF_PKTLEN(event->notify_rx.om);
        os_mbuf_copydata(event->notify_rx.om, offset, notif_len, notif_data);

        printf("notification data: ");
        for (int i = 0; i < notif_len; i++)
          {
            printf("%02d ", notif_data[i]);
          }

        printf("\n");

        /* Attribute data is contained in event->notify_rx.attr_data. */

        return 0;

      case BLE_GAP_EVENT_MTU:
        printf("mtu update event; conn_handle=%d cid=%d mtu=%d\n",
               event->mtu.conn_handle, event->mtu.channel_id,
               event->mtu.value);
        return 0;

      case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */

        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host
         * should continue with the pairing operation.
         */

        return BLE_GAP_REPEAT_PAIRING_RETRY;

      default:
        return 0;
    }
}

static void blecent_on_reset(int reason)
{
  printf("Resetting state; reason=%d\n", reason);
}

/****************************************************************************
 * Name: blecent_on_sync
 ****************************************************************************/

static void blecent_on_sync(void)
{
  int rc;

  /* Make sure we have proper identity address set (public preferred) */

  rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  /* Begin scanning for a peripheral to connect to. */

  blecent_scan();
}

/****************************************************************************
 * Name: nimble_host_task
 ****************************************************************************/

static void nimble_host_task(FAR void *param)
{
  ble_svc_gap_device_name_set("NuttX NimBLE CENT");
  nimble_port_run();
}

/****************************************************************************
 * Name: ble_hci_sock_task
 ****************************************************************************/

static FAR void *ble_hci_sock_task(FAR void *param)
{
  ble_hci_sock_ack_handler(param);
  return NULL;
}

/****************************************************************************
 * Name: ble_host_task
 ****************************************************************************/

static FAR void *ble_host_task(FAR void *param)
{
  nimble_host_task(param);
  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nimble_main
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  struct ble_npl_task s_task_host;
  struct ble_npl_task s_task_hci;
  int ret = 0;

  /* allow to specify custom hci */

  if (argc > 1)
    {
      ble_hci_sock_set_device(atoi(argv[1]));
    }

#ifndef CONFIG_NSH_ARCHINIT
  /* Perform architecture-specific initialization */

  boardctl(BOARDIOC_INIT, 0);
#endif

#ifndef CONFIG_NSH_NETINIT
  /* Bring up the network */

  netinit_bringup();
#endif

  nimble_port_init();

  /* Configure the host. */

  ble_hs_cfg.reset_cb        = blecent_on_reset;
  ble_hs_cfg.sync_cb         = blecent_on_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  /* Initialize data structures to track connected peers. */

  ret = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
  assert(ret == 0);

  /* Create task which handles HCI socket */

  ret = ble_npl_task_init(&s_task_hci, "hci_sock", ble_hci_sock_task, NULL,
                          TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                          TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
  if (ret != 0)
    {
      printf("ERROR: starting hci task: %i\n", ret);
    }

  /* Create task which handles default event queue for host stack. */

  ret = ble_npl_task_init(&s_task_host, "ble_host", ble_host_task, NULL,
                          TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                          TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
  if (ret != 0)
    {
      printf("ERROR: starting ble task: %i\n", ret);
    }

  while (true)
    {
      sleep(1);
    }

  return 0;
}
