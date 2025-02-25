/****************************************************************************
 * apps/testing/enet/src/test_enet_loopwr_datacheck.c
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

#include "enettest.h"
#include <sys/socket.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DATA_SIZE 200

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_enet_loopwr_datacheck
 * Description:
 *   This function tests the ENET loop write function and reads the frame
 *   from the ENET interface without using poll.
 ****************************************************************************/

void test_enet_loopwr_datacheck(void **state)
{
  struct senet_testsuites_params *enet_if = *state;
  uint8_t recv_data[DATA_SIZE];
  ssize_t bytes_received;
  int loop_count = 0;
  const int LOOP_MAX = 20;
  const uint8_t expected_mac[6] =
  {
    0x58,
    0x32,
    0x31,
    0x32,
    0x39,
    0x37
  };

  /* Send frames */

  sleep(10);
  enet_loop_send_frame(state);

  /* Read loop */

  while (loop_count < LOOP_MAX)
    {
      /* Directly read data */

      bytes_received = read(enet_if->enet_dev.fd, recv_data, DATA_SIZE);
      assert_true(bytes_received >= 0);

      if (bytes_received > 0)
        {
          /* Verify ethernet type (0x88f7) */

          uint16_t eth_type = (recv_data[12] << 8) | recv_data[13];
          assert_true(eth_type == 0x88f7);

          /* Verify frame length matches expected size */

          assert_true(bytes_received == DATA_SIZE);

          /* Verify source MAC address */

          assert_memory_equal(&recv_data[6], expected_mac, 6);
          loop_count++;
        }
    }

  /* Verify we received expected number of frames */

  assert_true(loop_count == LOOP_MAX);
}
