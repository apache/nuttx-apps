/****************************************************************************
 * apps/wireless/bluetooth/zblue/port/src/hci.c
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

/* The HCI device of the host: H4 packets over a NuttX /dev/ttyHCIx
 * character device (drivers/serial/uart_bth4.c).  It is the zephyr,bt-hci
 * chosen node of port/include/zephyr/devicetree_generated.h.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <fcntl.h>
#include <unistd.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "port.h"

LOG_MODULE_REGISTER(zblue_hci, CONFIG_LOG_DEFAULT_LEVEL);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct hci_data
{
  struct bt_hci_driver_data common; /* Must be first */
  int fd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct hci_data g_hci_data =
{
  .fd = -1,
};

static K_KERNEL_STACK_DEFINE(g_rx_stack, CONFIG_ZBLUE_HCI_RX_STACKSIZE);
static struct k_thread g_rx_thread;
static uint8_t g_rx_frame[BT_BUF_RX_SIZE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int read_full(int fd, uint8_t *buf, size_t len)
{
  ssize_t ret;
  int depth;
  int err;

  while (len > 0)
    {
      depth = z_zblue_block_begin();
      ret = read(fd, buf, len);
      err = errno;
      z_zblue_block_end(depth);
      if (ret < 0)
        {
          if (err == EINTR)
            {
              continue;
            }

          return -err;
        }

      buf += ret;
      len -= ret;
    }

  return 0;
}

static bool evt_discardable(const uint8_t *hdr)
{
  const struct bt_hci_evt_hdr *evt = (const void *)hdr;
  const struct bt_hci_evt_le_meta_event *meta;

  if (IS_ENABLED(CONFIG_BT_CLASSIC) &&
      (evt->evt == BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI ||
       evt->evt == BT_HCI_EVT_EXTENDED_INQUIRY_RESULT))
    {
      return true;
    }

  if (evt->evt == BT_HCI_EVT_LE_META_EVENT)
    {
      meta = (const void *)evt->data;
      return meta->subevent == BT_HCI_EVT_LE_ADVERTISING_REPORT;
    }

  return false;
}

static struct net_buf *rx_alloc(uint8_t type, const uint8_t *hdr)
{
  struct net_buf *buf = NULL;
  bool discardable;

  switch (type)
    {
      case BT_HCI_H4_EVT:
        discardable = evt_discardable(hdr);
        do
          {
            buf = bt_buf_get_evt(hdr[0], discardable,
                                 discardable ? K_NO_WAIT : K_SECONDS(1));
          }
        while (buf == NULL && !discardable);
        break;

      case BT_HCI_H4_ACL:
        buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
        break;

      case BT_HCI_H4_ISO:
        if (IS_ENABLED(CONFIG_BT_ISO))
          {
            buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);
          }
        break;
    }

  return buf;
}

static void rx_thread(void *p1, void *p2, void *p3)
{
  const struct device *dev = p1;
  struct hci_data *hci = dev->data;
  struct net_buf *buf;
  uint8_t *frame = g_rx_frame;
  size_t hdrlen;
  size_t len;
  int ret;

  UNUSED(p2);
  UNUSED(p3);

  for (; ; )
    {
      ret = read_full(hci->fd, frame, 1);
      if (ret < 0)
        {
          LOG_ERR("read failed: %d", ret);
          return;
        }

      switch (frame[0])
        {
          case BT_HCI_H4_EVT:
            hdrlen = BT_HCI_EVT_HDR_SIZE;
            break;

          case BT_HCI_H4_ACL:
            hdrlen = BT_HCI_ACL_HDR_SIZE;
            break;

          case BT_HCI_H4_ISO:
            hdrlen = BT_HCI_ISO_HDR_SIZE;
            break;

          default:
            LOG_ERR("unknown H4 type 0x%02x", frame[0]);
            continue;
        }

      ret = read_full(hci->fd, frame + 1, hdrlen);
      if (ret < 0)
        {
          LOG_ERR("read failed: %d", ret);
          return;
        }

      switch (frame[0])
        {
          case BT_HCI_H4_EVT:
            len = frame[2];
            break;

          case BT_HCI_H4_ACL:
            len = sys_get_le16(&frame[3]);
            break;

          default:
            len = bt_iso_hdr_len(sys_get_le16(&frame[3]));
            break;
        }

      if (1 + hdrlen + len > sizeof(g_rx_frame))
        {
          LOG_ERR("packet too long: %zu", len);
          return;
        }

      ret = read_full(hci->fd, frame + 1 + hdrlen, len);
      if (ret < 0)
        {
          LOG_ERR("read failed: %d", ret);
          return;
        }

      buf = rx_alloc(frame[0], frame + 1);
      if (buf == NULL)
        {
          LOG_WRN("dropped packet type %u", frame[0]);
          continue;
        }

      if (net_buf_tailroom(buf) < hdrlen + len)
        {
          LOG_ERR("no room for %zu bytes", hdrlen + len);
          net_buf_unref(buf);
          continue;
        }

      net_buf_add_mem(buf, frame + 1, hdrlen + len);
      bt_hci_recv(dev, buf);
    }
}

static int hci_open(const struct device *dev)
{
  struct hci_data *hci = dev->data;

  hci->fd = open(CONFIG_ZBLUE_HCI_DEVICE, O_RDWR | O_CLOEXEC);
  if (hci->fd < 0)
    {
      LOG_ERR("cannot open %s: %d", CONFIG_ZBLUE_HCI_DEVICE, errno);
      return -errno;
    }

  k_thread_create(&g_rx_thread, g_rx_stack,
                  K_KERNEL_STACK_SIZEOF(g_rx_stack), rx_thread,
                  (void *)dev, NULL, NULL,
                  K_PRIO_COOP(CONFIG_BT_DRIVER_RX_HIGH_PRIO), 0, K_NO_WAIT);
  k_thread_name_set(&g_rx_thread, "zblue_hci_rx");
  return 0;
}

/* buf starts with the H4 packet type, as the upstream drivers expect.
 *
 * There is no close(): the RX thread cannot be stopped while it waits in
 * read(), so bt_disable() returns -ENOSYS, as for upstream drivers
 * without close().
 */

static int hci_send(const struct device *dev, struct net_buf *buf)
{
  struct hci_data *hci = dev->data;
  const uint8_t *data = buf->data;
  size_t len = buf->len;
  ssize_t ret;
  int depth;
  int err;

  /* The controller driver may block in write(): let the other
   * cooperative threads, the RX thread among them, run meanwhile.
   */

  while (len > 0)
    {
      depth = z_zblue_block_begin();
      ret = write(hci->fd, data, len);
      err = errno;
      z_zblue_block_end(depth);
      if (ret < 0)
        {
          if (err == EINTR)
            {
              continue;
            }

          LOG_ERR("write failed: %d", err);
          return -err;
        }

      data += ret;
      len -= ret;
    }

  net_buf_unref(buf);
  return 0;
}

static DEVICE_API(bt_hci, g_hci_api) =
{
  .open = hci_open,
  .send = hci_send,
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct device __device_dts_ord_1 =
{
  .name = "zblue_hci",
  .api  = &g_hci_api,
  .data = &g_hci_data,
};
