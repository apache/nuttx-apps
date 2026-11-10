/****************************************************************************
 * apps/testing/nettest/radio/test_radio_ieee802154.c
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
#include <sys/socket.h>
#include <netpacket/ieee802154.h>
#include <cmocka.h>

#include "test_radio.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_radio_ieee802154
 *
 * Description:
 *   An IEEE 802.15.4 socket that is not bound: the device lookup finds no
 *   radio for a socket with no local address.
 *
 ****************************************************************************/

void test_radio_ieee802154(FAR void **state)
{
  int fd;

  fd = socket(AF_IEEE802154, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  assert_return_code(fd, errno);
  test_radio_recv_nodev(fd);
}
