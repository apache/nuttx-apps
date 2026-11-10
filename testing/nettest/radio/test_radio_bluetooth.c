/****************************************************************************
 * apps/testing/nettest/radio/test_radio_bluetooth.c
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

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netpacket/bluetooth.h>
#include <cmocka.h>

#include "test_radio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* An HCI device index that no Bluetooth device has */

#define RADIO_HCI_NODEV 200

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_radio_bluetooth_l2cap
 *
 * Description:
 *   An L2CAP socket that is not bound: no Bluetooth device has its
 *   all-zero local address.
 *
 ****************************************************************************/

void test_radio_bluetooth_l2cap(FAR void **state)
{
  int fd;

  fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_L2CAP);
  assert_return_code(fd, errno);
  test_radio_recv_nodev(fd);
}

/****************************************************************************
 * Name: test_radio_bluetooth_hci
 *
 * Description:
 *   An HCI socket bound to a device index that no Bluetooth device has.
 *
 ****************************************************************************/

void test_radio_bluetooth_hci(FAR void **state)
{
  struct sockaddr_hci addr;
  int fd;

  fd = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
  assert_return_code(fd, errno);

  memset(&addr, 0, sizeof(addr));
  addr.hci_family  = AF_BLUETOOTH;
  addr.hci_dev     = RADIO_HCI_NODEV;
  addr.hci_channel = HCI_CHANNEL_RAW;
  assert_return_code(bind(fd, (FAR struct sockaddr *)&addr, sizeof(addr)),
                     errno);

  test_radio_recv_nodev(fd);
}
