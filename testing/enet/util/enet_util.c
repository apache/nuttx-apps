/****************************************************************************
 * apps/testing/enet/util/enet_util.c
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

#include "enet_util.h"
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DATA_SIZE 200

#define ETH_PTP_TYPE 0x88f7

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: enet_test_setup
 ****************************************************************************/

int enet_test_setup(void **state)
{
  struct senet_testsuites_params *enet_p;
  int ret;

  enet_p = zalloc(sizeof(struct senet_testsuites_params));
  assert_true(enet_p != NULL);
  enet_p->enet_dev.if_name = strdup(CONFIG_ENET_TEST_DEV);
  *state = enet_p;

  /* ifup enet interface */

  ret = netlib_ifup(enet_p->enet_dev.if_name);
  assert_true(ret == 0);
  enet_p->enet_dev.fd = enet_test_init(enet_p->enet_dev.if_name);
  assert_true(enet_p->enet_dev.fd > 0);
  return 0;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: enet_test_init
 ****************************************************************************/

int enet_test_init(const char *enetif)
{
  int ret;
  int pkt_fd;
  struct sockaddr_ll sll;
  struct timeval timeout;
  struct ifreq ifr;
  int ifindex;
  int optval;

  /* Create raw socket */

  pkt_fd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, HTONS(ETH_PTP_TYPE));
  assert_true(pkt_fd > 0);

  /* Initialize socket address structure */

  memset(&sll, 0, sizeof(struct sockaddr_ll));

  /* Set socket timeout options */

  timeout.tv_sec = 0;
  timeout.tv_usec = 1;

  ret = setsockopt(pkt_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout));
  assert_true(ret == 0);

  ret = setsockopt(pkt_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                   sizeof(timeout));
  assert_true(ret == 0);

  /* Set don't route option */

  optval = 1;
  ret = setsockopt(pkt_fd, SOL_SOCKET, SO_DONTROUTE, &optval,
                   sizeof(optval));
  assert_true(ret == 0);

  /* Get interface index */

  strlcpy(ifr.ifr_name, enetif, IFNAMSIZ);
  ret = ioctl(pkt_fd, SIOCGIFINDEX, &ifr);
  assert_true(ret == 0);

  ifindex = ifr.ifr_ifindex;

  /* Get interface flags */

  ret = ioctl(pkt_fd, SIOCGIFFLAGS, &ifr);
  assert_true(ret == 0);

  /* Set interface broadcast flag */

  ifr.ifr_flags |= IFF_BROADCAST;
  ret = ioctl(pkt_fd, SIOCSIFFLAGS, &ifr);
  assert_true(ret == 0);

  /* Prepare and bind socket */

  sll.sll_family = AF_PACKET;
  sll.sll_protocol = HTONS(ETH_PTP_TYPE);
  sll.sll_ifindex = ifindex;

  ret = bind(pkt_fd, (struct sockaddr *)&sll,
             sizeof(struct sockaddr_ll));
  assert_true(ret == 0);

  return pkt_fd;

errout_with_socket:
  close(pkt_fd);
  return -1;
}

/****************************************************************************
 * Name: enet_loop_send_frame
 * Description:
 *   Test sending ethernet frames in a loop (20 times)
 ****************************************************************************/

void enet_loop_send_frame(void **state)
{
  struct senet_testsuites_params *enet_if = *state;
  uint8_t data[DATA_SIZE];
  const char *message = "Hello, World!";
  uint16_t ip_checksum;
  int ret;
  size_t message_len;
  int i;
  const int LOOP_COUNT = 20;  /* Number of frames to send */

  /* Initialize test data */

  memset(data, 0, DATA_SIZE);

  /* Setup ethernet header */

  /* Destination MAC address (broadcast) */

  data[0] = 0x01; data[1] = 0x00; data[2] = 0x5e;
  data[3] = 0x00; data[4] = 0x00; data[5] = 0x00;

  /* Source MAC address: 58:32:31:32:39:37 */

  data[6] = 0x58; data[7] = 0x32; data[8] = 0x31;
  data[9] = 0x32; data[10] = 0x39; data[11] = 0x37;

  /* Ethernet type (IPv4) */

  data[12] = 0x88; data[13] = 0xf7;

  /* Setup IP header */

  data[14] = 0x45;                    /* Version 4, IHL 5 (20 bytes) */
  data[15] = 0x00;                    /* DSCP/ECN */
  data[16] = 0x00; data[17] = 0x4e;   /* Total Length: 78 bytes */
  data[18] = 0x00; data[19] = 0x00;   /* Identification */
  data[20] = 0x00; data[21] = 0x00;   /* Flags and Fragment Offset */
  data[22] = 0x40;                    /* TTL: 64 */
  data[23] = 0x11;                    /* Protocol: UDP */
  data[24] = 0x00; data[25] = 0x00;   /* Header Checksum (initial 0) */

  /* Source IP: 10.0.0.2 */

  data[26] = 0x0a; data[27] = 0x00;
  data[28] = 0x00; data[29] = 0x02;

  /* Destination IP: 255.255.255.255 (broadcast) */

  data[30] = 0xff; data[31] = 0xff;
  data[32] = 0xff; data[33] = 0xff;

  /* Copy payload message */

  message_len = strlen(message);
  memcpy(data + 42, message, message_len);

  /* Calculate IP header checksum */

  ip_checksum = 0;
  for (int i = 14; i < 34; i += 2)
    {
      ip_checksum += (data[i] << 8) | data[i + 1];
    }

  if (ip_checksum >> 16)
    {
      ip_checksum = (ip_checksum & 0xffff) + (ip_checksum >> 16);
    }

  ip_checksum = ~ip_checksum;
  data[24] = (ip_checksum >> 8) & 0xff;
  data[25] = ip_checksum & 0xff;

  /* Send frames in a loop */

  for (i = 0; i < LOOP_COUNT; i++)
    {
      ret = send(enet_if->enet_dev.fd, data, DATA_SIZE, 0);
      assert_true(ret > 0);

      /* Add a small delay between sends */

      usleep(100000);  /* 100ms delay */
    }

  /* Verify we completed all sends */

  assert_true(i == LOOP_COUNT);
}

/****************************************************************************
 * Name: enet_test_teardown
 ****************************************************************************/

int enet_test_teardown(void **state)
{
  int ret;
  struct senet_testsuites_params *enet_p;

  if (state == NULL || *state == NULL)
    {
      return -1;
    }

  enet_p = *state;

  /* close enet interface */

  ret = close(enet_p->enet_dev.fd);
  assert_true(ret == 0);
  ret = netlib_ifdown(enet_p->enet_dev.if_name);
  assert_true(ret >= 0);

  /* free memory */

  free(enet_p->enet_dev.if_name);

  /* free structure */

  free(enet_p);
  *state = NULL;
  return 0;
}
