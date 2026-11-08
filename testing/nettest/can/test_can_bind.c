/****************************************************************************
 * apps/testing/nettest/can/test_can_bind.c
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

#include <nuttx/config.h>

#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>
#include <net/if.h>
#include <sys/socket.h>

#include <nuttx/can.h>

#include "test_can.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_CAN_NODEV   99            /* An interface index with no device */
#define TEST_CAN_WAIT_US (200 * 1000)  /* Time for the poll after a write */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_can_socket(void)
{
  int sd = socket(AF_CAN, SOCK_RAW | SOCK_CLOEXEC, CAN_RAW);

  assert_return_code(sd, errno);
  return sd;
}

/* bind() to 'ifindex'; the result, with errno saved in 'err' */

static int test_can_bindto(int sd, int ifindex, FAR int *err)
{
  struct sockaddr_can addr;
  int ret;

  memset(&addr, 0, sizeof(addr));
  addr.can_family  = AF_CAN;
  addr.can_ifindex = ifindex;
  ret  = bind(sd, (FAR struct sockaddr *)&addr, sizeof(addr));
  *err = ret < 0 ? errno : 0;
  return ret;
}

static void test_can_frame(FAR struct can_frame *frame)
{
  memset(frame, 0, sizeof(*frame));
  frame->can_id  = 0x123;
  frame->can_dlc = 1;
  frame->data[0] = 0x5a;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_can_bind_nodev
 ****************************************************************************/

void test_can_bind_nodev(FAR void **state)
{
  int sd = test_can_socket();
  int err;

  assert_int_equal(test_can_bindto(sd, TEST_CAN_NODEV, &err), -1);
  assert_int_equal(err, ENODEV);
  close(sd);
}

/****************************************************************************
 * Name: test_can_bind_any
 ****************************************************************************/

void test_can_bind_any(FAR void **state)
{
  int sd = test_can_socket();
  int err;

  assert_int_equal(test_can_bindto(sd, 0, &err), 0);
  close(sd);
}

/****************************************************************************
 * Name: test_can_bind_can0
 ****************************************************************************/

void test_can_bind_can0(FAR void **state)
{
  int ifindex = if_nametoindex("can0");
  int sd;
  int err;

  if (ifindex <= 0)
    {
      skip();
    }

  sd = test_can_socket();
  assert_int_equal(test_can_bindto(sd, ifindex, &err), 0);
  close(sd);
}

/****************************************************************************
 * Name: test_can_bind_todev
 *
 * Description:
 *   SO_BINDTODEVICE does not give a CAN socket a device: write() fails.
 *
 ****************************************************************************/

void test_can_bind_todev(FAR void **state)
{
#ifdef CONFIG_NET_BINDTODEVICE
  struct can_frame frame;
  int sd = test_can_socket();
  int ret;

  assert_return_code(setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, "lo", 3),
                     errno);
  test_can_frame(&frame);
  ret = write(sd, &frame, sizeof(frame));
  usleep(TEST_CAN_WAIT_US);
  assert_int_equal(ret, -1);
  close(sd);
#else
  skip();
#endif
}

/****************************************************************************
 * Name: test_can_bind_sendto
 *
 * Description:
 *   sendto() does not use its address: an unbound socket cannot send.
 *
 ****************************************************************************/

void test_can_bind_sendto(FAR void **state)
{
  struct sockaddr_can addr;
  struct can_frame frame;
  int sd = test_can_socket();
  int ret;

  memset(&addr, 0, sizeof(addr));
  addr.can_family  = AF_CAN;
  addr.can_ifindex = if_nametoindex("lo");
  test_can_frame(&frame);
  ret = sendto(sd, &frame, sizeof(frame), 0, (FAR struct sockaddr *)&addr,
               sizeof(addr));
  usleep(TEST_CAN_WAIT_US);
  assert_int_equal(ret, -1);
  close(sd);
}

/****************************************************************************
 * Name: test_can_bind_lo
 *
 * Description:
 *   bind() to the loopback device fails with ENODEV.  If it succeeds, a
 *   write follows, as a program would do.
 *
 ****************************************************************************/

void test_can_bind_lo(FAR void **state)
{
  struct can_frame frame;
  int ifindex = if_nametoindex("lo");
  int sd;
  int ret;
  int err;

  assert_true(ifindex > 0);
  sd  = test_can_socket();
  ret = test_can_bindto(sd, ifindex, &err);
  if (ret == 0)
    {
      /* The result does not matter: the kernel must not stop */

      test_can_frame(&frame);
      write(sd, &frame, sizeof(frame));
      usleep(TEST_CAN_WAIT_US);
    }

  close(sd);
  assert_int_equal(ret, -1);
  assert_int_equal(err, ENODEV);
}
