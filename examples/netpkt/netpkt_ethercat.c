/****************************************************************************
 * apps/examples/netpkt/netpkt_ethercat.c
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

#include <stdio.h>
#include <time.h>

#include <net/if.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <sys/socket.h>

#include <nuttx/clock.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(void)
{
  printf("usage: netpkt_ethercat [<ifname> [times]]\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR const char *argv[])
{
  struct timespec send_time;
  struct timespec recv_time;
  struct sockaddr_ll addr;
  uint8_t sendbuff[60] =
  {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9c, 0x7f, 0x81, 0x3c, 0x1e, 0x8e,
    0x88, 0xa4, 0x0e, 0x10, 0x07, 0x4a, 0x00, 0x00, 0x30, 0x01, 0x02, 0x00,
  };

  uint8_t recvbuff[100];
  FAR const char *ifname = "eth0";
  int repeat = 10;
  int num_packets = 0;
  int len;
  int ifindex;
  int sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);

  if (sockfd == -1)
    {
      perror("ERROR: creating socket failed");
      return -errno;
    }

  if (argc >= 2)
    {
      ifname = argv[1];
    }

  ifindex = if_nametoindex(ifname);

  if (argc >= 3)
    {
      repeat = atoi(argv[2]);
    }

  if (ifindex == 0)
    {
      printf("Failed to get index of device %s\n", ifname);
      close(sockfd);
      usage();
      return -errno;
    }

  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = ifindex;
  if (bind(sockfd, (FAR const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("ERROR: binding socket failed");
      close(sockfd);
      usage();
      return -errno;
    }

  for (num_packets = 0; num_packets < repeat; num_packets++)
    {
      clock_gettime(CLOCK_REALTIME, &send_time);
      if (sendto(sockfd, sendbuff, 60, 0, NULL, 0) < 0)
        {
          perror("ERROR: sendto socket failed");
          continue;
        }

      len = recvfrom(sockfd, recvbuff, 100, 0, NULL, NULL);
      clock_gettime(CLOCK_REALTIME, &recv_time);
      printf("Data recv: %d bytes, spent time %ld ns\n", len,
              (recv_time.tv_sec - send_time.tv_sec) * NSEC_PER_SEC +
              recv_time.tv_nsec - send_time.tv_nsec);
      usleep(1000);
    }

  close(sockfd);

  return 0;
}
