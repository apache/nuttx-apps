/****************************************************************************
 * apps/testing/nettest/pkt/test_pkt_lo.c
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
#include <poll.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <cmocka.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <sys/socket.h>

#include "test_pkt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The IPv4 header has no options */

#define TEST_PKT_IPLEN    sizeof(struct iphdr)
#define TEST_PKT_PROTO    253     /* IP protocol for experiments, RFC 3692 */
#define TEST_PKT_TYPE     0x88b5  /* Ethernet type for experiments */
#define TEST_PKT_MF       0x2000  /* IPv4 flag "more fragments" */
#define TEST_PKT_TIMXCEED 11      /* ICMP Time Exceeded */
#define TEST_PKT_FRAGTIME 1       /* Its code: reassembly time exceeded */
#define TEST_PKT_WAIT_MS  2000    /* Time to wait for a frame */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* A packet socket of the given protocol, bound to lo */

static int test_pkt_open(int proto)
{
  struct sockaddr_ll addr;
  int sd;

  sd = socket(AF_PACKET, SOCK_RAW | SOCK_CLOEXEC, htons(proto));
  assert_return_code(sd, errno);

  memset(&addr, 0, sizeof(addr));
  addr.sll_family   = AF_PACKET;
  addr.sll_protocol = htons(proto);
  addr.sll_ifindex  = if_nametoindex("lo");
  assert_true(addr.sll_ifindex > 0);
  assert_return_code(bind(sd, (FAR struct sockaddr *)&addr, sizeof(addr)),
                     errno);
  return sd;
}

static uint16_t test_pkt_chksum(FAR const void *data, size_t len)
{
  FAR const uint8_t *p = data;
  uint32_t sum = 0;
  size_t i;

  for (i = 0; i < len; i += 2)
    {
      sum += (p[i] << 8) | p[i + 1];
    }

  while (sum >> 16)
    {
      sum = (sum & 0xffff) + (sum >> 16);
    }

  return ~sum & 0xffff;
}

/* An IPv4 packet from 127.0.0.1 to 127.0.0.1 with 'plen' bytes of data and
 * the given fragment field; returns its length.
 */

static size_t test_pkt_ipv4(FAR uint8_t *f, size_t plen, uint16_t id,
                            uint16_t frag)
{
  struct iphdr ip;
  size_t i;

  memset(&ip, 0, sizeof(ip));
  ip.version  = 4;
  ip.ihl      = sizeof(ip) / 4;
  ip.tot_len  = htons(TEST_PKT_IPLEN + plen);
  ip.id       = htons(id);
  ip.frag_off = htons(frag);
  ip.ttl      = 64;
  ip.protocol = TEST_PKT_PROTO;
  ip.saddr    = htonl(INADDR_LOOPBACK);
  ip.daddr    = htonl(INADDR_LOOPBACK);
  ip.check    = htons(test_pkt_chksum(&ip, sizeof(ip)));
  memcpy(f, &ip, sizeof(ip));

  for (i = 0; i < plen; i++)
    {
      f[TEST_PKT_IPLEN + i] = 0xa0 + i;
    }

  return TEST_PKT_IPLEN + plen;
}

/* Read one frame; wait at most 'ms' milliseconds */

static ssize_t test_pkt_read(int sd, FAR uint8_t *buf, size_t len, int ms)
{
  struct pollfd pfd;

  pfd.fd      = sd;
  pfd.events  = POLLIN;
  pfd.revents = 0;
  if (poll(&pfd, 1, ms) <= 0)
    {
      return -1;
    }

  return recv(sd, buf, len, MSG_DONTWAIT);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_pkt_lo_send
 *
 * Description:
 *   send() of an IPv4 packet on a packet socket bound to lo returns its
 *   length, and an ETH_P_ALL socket bound to lo reads the same bytes.  The
 *   sender uses another protocol, so that it does not read the frame.
 *
 ****************************************************************************/

void test_pkt_lo_send(FAR void **state)
{
  uint8_t frame[64];
  uint8_t buf[256];
  size_t len;
  ssize_t ret;
  int rsd;
  int ssd;

  rsd = test_pkt_open(ETH_P_ALL);
  ssd = test_pkt_open(TEST_PKT_TYPE);

  len = test_pkt_ipv4(frame, 8, 1, 0);
  assert_int_equal(send(ssd, frame, len, 0), len);

  ret = test_pkt_read(rsd, buf, sizeof(buf), TEST_PKT_WAIT_MS);
  assert_int_equal(ret, len);
  assert_memory_equal(buf, frame, len);

  close(ssd);
  close(rsd);
}

/****************************************************************************
 * Name: test_pkt_lo_frag
 *
 * Description:
 *   The first fragment of an IPv4 packet, sent on lo by a packet socket.
 *   The reassembly times out, and the stack sends an ICMP Time Exceeded
 *   message on lo.  The kernel must go on, and the ETH_P_ALL socket must
 *   read the ICMP message.
 *
 ****************************************************************************/

void test_pkt_lo_frag(FAR void **state)
{
#if defined(CONFIG_NET_IPFRAG) && defined(CONFIG_NET_ICMP) && \
    !defined(CONFIG_NET_ICMP_NO_STACK)
  struct iphdr ip;
  uint8_t frame[64];
  uint8_t buf[256];
  bool found = false;
  size_t len;
  ssize_t ret;
  int rsd;
  int ssd;

  rsd = test_pkt_open(ETH_P_ALL);
  ssd = test_pkt_open(TEST_PKT_TYPE);

  /* Fragment offset 0 with "more fragments" */

  len = test_pkt_ipv4(frame, 16, 2, TEST_PKT_MF);
  assert_int_equal(send(ssd, frame, len, 0), len);

  /* Wait for the reassembly timeout (deciseconds) */

  sleep(CONFIG_NET_IPFRAG_REASS_MAXAGE / 10 + 3);

  /* The ICMP header follows the IP header: type, then code */

  while ((ret = test_pkt_read(rsd, buf, sizeof(buf), 200)) > 0)
    {
      memcpy(&ip, buf, sizeof(ip));
      if (ret >= TEST_PKT_IPLEN + 8 && ip.protocol == IPPROTO_ICMP &&
          buf[TEST_PKT_IPLEN] == TEST_PKT_TIMXCEED &&
          buf[TEST_PKT_IPLEN + 1] == TEST_PKT_FRAGTIME)
        {
          found = true;
        }
    }

  close(ssd);
  close(rsd);
  assert_true(found);
#else
  skip();
#endif
}
