/****************************************************************************
 * apps/canutils/isotpperf/isotpperf_main.c
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
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <nuttx/can.h>
#include <nuttx/can/isotp.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ISOTPPERF_SEQLEN 4     /* Sequence number bytes in a message */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct isotpperf_s
{
  bool     tx;                  /* Sender, else receiver */
  FAR const char *ifname;       /* SocketCAN interface */
  uint32_t txid;                /* CAN ID of sent frames */
  uint32_t rxid;                /* CAN ID of received frames */
  uint32_t count;               /* Messages to send or receive */
  uint32_t len;                 /* Message length */
  uint32_t timeout;             /* RX idle timeout in ms */
  uint8_t  bs;                  /* RX flow control block size */
  uint8_t  stmin;               /* RX flow control STmin in ms */

  /* Results */

  uint32_t msgs;                /* Messages sent or received */
  uint64_t bytes;               /* Bytes sent or received */
  uint32_t lost;                /* Sequence numbers skipped */
  uint32_t ooo;                 /* Sequence went backwards */
  uint32_t bad;                 /* Wrong length or contents */
  uint64_t t0;                  /* First message, us */
  uint64_t t1;                  /* Last message, us */
  bool     haveseq;             /* nextseq is valid */
  uint32_t nextseq;             /* Expected sequence number */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_buf[ISOTP_MAX_MSG_LEN];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: isotpperf_now
 *
 * Description:
 *   Get the monotonic time.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Time in microseconds.
 *
 ****************************************************************************/

static uint64_t isotpperf_now(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/****************************************************************************
 * Name: isotpperf_fill
 *
 * Description:
 *   Fill a message: a little-endian sequence number in the first
 *   ISOTPPERF_SEQLEN bytes, then a pattern.
 *
 * Input Parameters:
 *   buf - Message buffer
 *   len - Message length
 *   seq - Sequence number
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void isotpperf_fill(FAR uint8_t *buf, uint32_t len, uint32_t seq)
{
  uint32_t i;

  for (i = 0; i < len; i++)
    {
      buf[i] = i < ISOTPPERF_SEQLEN ? (uint8_t)(seq >> (8 * i)) :
                                      (uint8_t)(seq + i);
    }
}

/****************************************************************************
 * Name: isotpperf_check
 *
 * Description:
 *   Check the pattern of a received message.
 *
 * Input Parameters:
 *   buf - Message
 *   len - Message length
 *   seq - Sequence number
 *
 * Returned Value:
 *   True if the pattern is correct.
 *
 ****************************************************************************/

static bool isotpperf_check(FAR const uint8_t *buf, uint32_t len,
                            uint32_t seq)
{
  uint32_t i;

  for (i = ISOTPPERF_SEQLEN; i < len; i++)
    {
      if (buf[i] != (uint8_t)(seq + i))
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: isotpperf_report
 *
 * Description:
 *   Print the results.
 *
 * Input Parameters:
 *   p - Test state
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void isotpperf_report(FAR struct isotpperf_s *p)
{
  uint64_t us = p->t1 - p->t0;
  uint64_t bps = us > 0 ? p->bytes * 1000000 / us : 0;

  printf("isotpperf: mode=%s msgs=%" PRIu32 " bytes=%" PRIu64
         " us=%" PRIu64 " Bps=%" PRIu64 " lost=%" PRIu32 " ooo=%" PRIu32
         " bad=%" PRIu32 "\n", p->tx ? "tx" : "rx", p->msgs, p->bytes, us,
         bps, p->lost, p->ooo, p->bad);
}

/****************************************************************************
 * Name: isotpperf_tx
 *
 * Description:
 *   Send messages.
 *
 * Input Parameters:
 *   p    - Test state
 *   sock - Bound ISO-TP socket
 *
 * Returned Value:
 *   Zero on success, a negated errno value on failure.
 *
 ****************************************************************************/

static int isotpperf_tx(FAR struct isotpperf_s *p, int sock)
{
  uint32_t seq;
  ssize_t ret;

  p->t0 = isotpperf_now();
  for (seq = 0; seq < p->count; seq++)
    {
      isotpperf_fill(g_buf, p->len, seq);
      ret = write(sock, g_buf, p->len);
      if (ret != (ssize_t)p->len)
        {
          fprintf(stderr, "isotpperf: write %" PRIu32 " failed: %d\n",
                  seq, ret < 0 ? errno : (int)ret);
          return ret < 0 ? -errno : -EIO;
        }

      p->msgs++;
      p->bytes += p->len;
    }

  p->t1 = isotpperf_now();
  return OK;
}

/****************************************************************************
 * Name: isotpperf_rx
 *
 * Description:
 *   Receive and check messages until the count or the idle timeout.
 *
 * Input Parameters:
 *   p    - Test state
 *   sock - Bound ISO-TP socket
 *
 * Returned Value:
 *   Zero.
 *
 ****************************************************************************/

static int isotpperf_rx(FAR struct isotpperf_s *p, int sock)
{
  struct pollfd pfd;
  ssize_t ret;
  uint32_t seq;

  pfd.fd     = sock;
  pfd.events = POLLIN;

  while (p->msgs + p->lost < p->count)
    {
      if (poll(&pfd, 1, p->timeout) <= 0)
        {
          break;
        }

      ret = read(sock, g_buf, sizeof(g_buf));
      if (ret < 0)
        {
          fprintf(stderr, "isotpperf: read failed: %d\n", errno);
          p->bad++;
          continue;
        }

      if (p->msgs == 0)
        {
          p->t0 = isotpperf_now();
        }

      p->t1 = isotpperf_now();
      p->msgs++;
      p->bytes += ret;

      if (ret != (ssize_t)p->len || ret < ISOTPPERF_SEQLEN)
        {
          p->bad++;
          continue;
        }

      seq = (uint32_t)g_buf[0] | (uint32_t)g_buf[1] << 8 |
            (uint32_t)g_buf[2] << 16 | (uint32_t)g_buf[3] << 24;
      if (!isotpperf_check(g_buf, p->len, seq))
        {
          p->bad++;
        }

      if (p->haveseq)
        {
          if (seq > p->nextseq)
            {
              p->lost += seq - p->nextseq;
            }
          else if (seq < p->nextseq)
            {
              p->ooo++;
            }
        }

      p->haveseq = true;
      p->nextseq = seq + 1;
    }

  if (p->haveseq && p->nextseq < p->count)
    {
      p->lost += p->count - p->nextseq;
    }

  return OK;
}

/****************************************************************************
 * Name: isotpperf_usage
 *
 * Description:
 *   Print the usage.
 *
 * Input Parameters:
 *   progname - Program name
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void isotpperf_usage(FAR const char *progname)
{
  fprintf(stderr,
          "Usage: %s -m tx|rx -i <ifname> -t <txid> -r <rxid> [options]\n"
          "  -n <count>   messages (default 100)\n"
          "  -l <len>     message length, %d..%d (default 256)\n"
          "  -T <ms>      RX idle timeout (default 3000)\n"
          "  -b <bs>      RX flow control block size (default 0)\n"
          "  -s <ms>      RX flow control STmin (default 0)\n",
          progname, ISOTPPERF_SEQLEN, ISOTP_MAX_MSG_LEN);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: isotpperf_main
 *
 * Description:
 *   Entry point of the isotpperf tool.
 *
 * Input Parameters:
 *   argc - Number of arguments
 *   argv - Arguments
 *
 * Returned Value:
 *   EXIT_SUCCESS or EXIT_FAILURE.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct isotpperf_s p;
  struct sockaddr_can addr;
  struct ifreq ifr;
  bool havemode = false;
  int option;
  int sock;
  int ret;

  memset(&p, 0, sizeof(p));
  p.count   = 100;
  p.len     = 256;
  p.timeout = 3000;
  p.txid    = CAN_SFF_MASK + 1;
  p.rxid    = CAN_SFF_MASK + 1;

  while ((option = getopt(argc, argv, "m:i:t:r:n:l:T:b:s:h")) != ERROR)
    {
      switch (option)
        {
          case 'm':
            p.tx = strcmp(optarg, "tx") == 0;
            havemode = p.tx || strcmp(optarg, "rx") == 0;
            break;

          case 'i':
            p.ifname = optarg;
            break;

          case 't':
            p.txid = strtoul(optarg, NULL, 0);
            break;

          case 'r':
            p.rxid = strtoul(optarg, NULL, 0);
            break;

          case 'n':
            p.count = strtoul(optarg, NULL, 0);
            break;

          case 'l':
            p.len = strtoul(optarg, NULL, 0);
            break;

          case 'T':
            p.timeout = strtoul(optarg, NULL, 0);
            break;

          case 'b':
            p.bs = strtoul(optarg, NULL, 0);
            break;

          case 's':
            p.stmin = strtoul(optarg, NULL, 0);
            break;

          default:
            isotpperf_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

  if (!havemode || p.ifname == NULL || p.txid > CAN_SFF_MASK ||
      p.rxid > CAN_SFF_MASK || p.len < ISOTPPERF_SEQLEN ||
      p.len > ISOTP_MAX_MSG_LEN)
    {
      isotpperf_usage(argv[0]);
      return EXIT_FAILURE;
    }

  sock = socket(PF_CAN, SOCK_DGRAM, CAN_ISOTP);
  if (sock < 0)
    {
      fprintf(stderr, "isotpperf: socket failed: %d\n", errno);
      return EXIT_FAILURE;
    }

  strlcpy(ifr.ifr_name, p.ifname, IFNAMSIZ);
  if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
    {
      fprintf(stderr, "isotpperf: no interface %s\n", p.ifname);
      ret = -errno;
      goto out;
    }

  if (!p.tx)
    {
      struct can_isotp_fc_options fc;

      memset(&fc, 0, sizeof(fc));
      fc.bs    = p.bs;
      fc.stmin = p.stmin;
      if (setsockopt(sock, SOL_CAN_ISOTP, CAN_ISOTP_RECV_FC, &fc,
                     sizeof(fc)) < 0)
        {
          fprintf(stderr, "isotpperf: RECV_FC failed: %d\n", errno);
        }
    }

  memset(&addr, 0, sizeof(addr));
  addr.can_family        = AF_CAN;
  addr.can_ifindex       = ifr.ifr_ifindex;
  addr.can_addr.tp.tx_id = p.txid;
  addr.can_addr.tp.rx_id = p.rxid;
  if (bind(sock, (FAR struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      fprintf(stderr, "isotpperf: bind failed: %d\n", errno);
      ret = -errno;
      goto out;
    }

  ret = p.tx ? isotpperf_tx(&p, sock) : isotpperf_rx(&p, sock);
  isotpperf_report(&p);

out:
  close(sock);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
