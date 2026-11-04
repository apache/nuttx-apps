/****************************************************************************
 * apps/testing/testsuites/kernel/msg/cmocka_msg_test.c
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

#include "MsgTest.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MSG_TEST(f) cmocka_unit_test_setup_teardown(f, NULL, msg_test_teardown)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest nuttx_msg_test_suites[] =
  {
    MSG_TEST(test_nuttx_msg_get_private),
    MSG_TEST(test_nuttx_msg_get_key),
    MSG_TEST(test_nuttx_msg_type_zero),
    MSG_TEST(test_nuttx_msg_type_positive),
    MSG_TEST(test_nuttx_msg_type_negative),
    MSG_TEST(test_nuttx_msg_noerror),
    MSG_TEST(test_nuttx_msg_e2big),
    MSG_TEST(test_nuttx_msg_nowait),
    MSG_TEST(test_nuttx_msg_einval),
    MSG_TEST(test_nuttx_msg_ctl_stat),
    MSG_TEST(test_nuttx_msg_ctl_fields),
    MSG_TEST(test_nuttx_msg_ctl_set),
    MSG_TEST(test_nuttx_msg_ctl_rmid),
    MSG_TEST(test_nuttx_msg_wait_rcv),
    MSG_TEST(test_nuttx_msg_wait_types),
    MSG_TEST(test_nuttx_msg_wait_snd),
    MSG_TEST(test_nuttx_msg_eidrm_rcv),
    MSG_TEST(test_nuttx_msg_eidrm_snd),
    MSG_TEST(test_nuttx_msg_eintr_rcv),
    MSG_TEST(test_nuttx_msg_eintr_snd),
    MSG_TEST(test_nuttx_msg_pool_nowait),
    MSG_TEST(test_nuttx_msg_pool_wait),
    MSG_TEST(test_nuttx_msg_pool_eintr),
    MSG_TEST(test_nuttx_msg_pool_eidrm),
    MSG_TEST(test_nuttx_msg_get_many),
  };

  return cmocka_run_group_tests(nuttx_msg_test_suites, NULL, NULL);
}
