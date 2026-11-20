/****************************************************************************
 * apps/testing/nettest/tun/test_tun_tcp.c
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
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <cmocka.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <nuttx/net/tun.h>

#include "test_tun.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TUN_ADDR_A     0x0a4e0001  /* 10.78.0.1: the TUN interface */
#define TUN_ADDR_P     0x0a4e0002  /* 10.78.0.2: the peer */
#define TUN_PORT_A     5025
#define TUN_ISN_P      0x52300000u
#define TUN_TOTAL      8192        /* Bytes that the server writes */
#define TUN_PKTSIZE    CONFIG_NET_TUN_PKTSIZE

#ifdef CONFIG_NET_TCP_RTO
#  define TUN_RTO_MS   (CONFIG_NET_TCP_RTO * 500)
#else
#  define TUN_RTO_MS   1500
#endif

/* Lossy rounds: more than TCP_MAXRTX */

#ifdef CONFIG_NET_TCP_MAXRTX
#  define TUN_ROUNDS   (CONFIG_NET_TCP_MAXRTX + 2)
#else
#  define TUN_ROUNDS   10
#endif

/* A round takes one timeout: at most two times the RTO after one backoff.
 * The rounds get two times that.
 */

#define TUN_ROUNDS_MS  (TUN_ROUNDS * 4 * TUN_RTO_MS)

#define TCP_FIN        0x01
#define TCP_SYN        0x02
#define TCP_RST        0x04
#define TCP_PSH        0x08
#define TCP_ACK        0x10

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct tun_seg_s
{
  uint32_t seq;
  uint32_t ack;
  uint8_t  flags;
  int      len;
  FAR const uint8_t *data;
};

struct tun_state_s
{
  int       fd;
  char      ifname[IFNAMSIZ];
  pthread_t server;
  bool      started;
  uint16_t  pport;          /* The peer's port: new in each run */
  uint32_t  pisn;           /* The peer's ISN */
  uint32_t  isn;            /* The server's ISN */
  uint32_t  pseq;           /* The peer's next sequence number */
  bool      synack;         /* The SYN-ACK came: isn is valid */
  int       mss;            /* The MSS of the server's SYN-ACK */
  bool      fin;
  uint32_t  finlen;         /* Stream offset of the FIN */
  bool      rst;
  int       wrong;          /* Bytes that differ from the stream */
  uint32_t  wrongoff;       /* The first one */
  uint8_t   have[TUN_TOTAL + TUN_PKTSIZE];
  uint8_t   pkt[TUN_PKTSIZE];

  /* Results of the server thread */

  volatile int listening;   /* 1 listening, -1 failed */
  volatile int done;
  volatile int sent;        /* Return value of send() */
  volatile int senderr;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct tun_state_s g_tun;
static uint8_t g_txbuf[TUN_TOTAL];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* The byte of the stream at an offset: no two near offsets repeat */

static uint8_t tun_pattern(uint32_t off)
{
  return (uint8_t)(((off * 2654435761u) >> 13) ^ (off >> 8));
}

static uint32_t tun_now_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void tun_put16(FAR uint8_t *p, uint16_t v)
{
  p[0] = v >> 8;
  p[1] = v & 0xff;
}

static void tun_put32(FAR uint8_t *p, uint32_t v)
{
  p[0] = v >> 24;
  p[1] = (v >> 16) & 0xff;
  p[2] = (v >> 8) & 0xff;
  p[3] = v & 0xff;
}

static uint16_t tun_get16(FAR const uint8_t *p)
{
  return (uint16_t)((p[0] << 8) | p[1]);
}

static uint32_t tun_get32(FAR const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}

static uint16_t tun_csum(FAR const uint8_t *p, int n, uint32_t sum)
{
  int i;

  for (i = 0; i + 1 < n; i += 2)
    {
      sum += (p[i] << 8) | p[i + 1];
    }

  if ((n & 1) != 0)
    {
      sum += p[n - 1] << 8;
    }

  while ((sum >> 16) != 0)
    {
      sum = (sum & 0xffff) + (sum >> 16);
    }

  return (uint16_t)~sum;
}

/* Write one TCP segment of the peer (no data) to the TUN device */

static void tun_send_tcp(FAR struct tun_state_s *st, uint32_t ack,
                         uint8_t flags, uint16_t wnd)
{
  uint8_t pkt[44];
  FAR uint8_t *tcp = pkt + 20;
  int hl = (flags & TCP_SYN) != 0 ? 24 : 20;
  uint32_t sum = 0;
  ssize_t ret;

  memset(pkt, 0, sizeof(pkt));
  pkt[0] = 0x45;
  tun_put16(pkt + 2, 20 + hl);
  pkt[8] = 64;
  pkt[9] = IPPROTO_TCP;
  tun_put32(pkt + 12, TUN_ADDR_P);
  tun_put32(pkt + 16, TUN_ADDR_A);
  tun_put16(pkt + 10, tun_csum(pkt, 20, 0));

  tun_put16(tcp, st->pport);
  tun_put16(tcp + 2, TUN_PORT_A);
  tun_put32(tcp + 4, st->pseq - ((flags & TCP_SYN) != 0 ? 1 : 0));
  tun_put32(tcp + 8, ack);
  tcp[12] = (uint8_t)((hl / 4) << 4);
  tcp[13] = flags;
  tun_put16(tcp + 14, wnd);
  if (hl == 24)
    {
      tcp[20] = 2;              /* MSS option */
      tcp[21] = 4;
      tun_put16(tcp + 22, 1460);
    }

  /* The pseudo header: addresses, protocol and TCP length */

  sum += (TUN_ADDR_P >> 16) + (TUN_ADDR_P & 0xffff);
  sum += (TUN_ADDR_A >> 16) + (TUN_ADDR_A & 0xffff);
  sum += IPPROTO_TCP + hl;
  tun_put16(tcp + 16, tun_csum(tcp, hl, sum));

  ret = write(st->fd, pkt, 20 + hl);
  assert_int_equal(ret, 20 + hl);
}

/* The stream offset of the next byte that the peer has not received */

static uint32_t tun_contiguous(FAR struct tun_state_s *st)
{
  uint32_t off = 0;

  while (off < sizeof(st->have) && st->have[off] != 0)
    {
      off++;
    }

  return off;
}

/* Take the MSS option of a SYN-ACK */

static void tun_get_mss(FAR struct tun_state_s *st, FAR const uint8_t *tcp,
                        int hl)
{
  int i = 20;

  while (i < hl && tcp[i] != 0)
    {
      if (tcp[i] == 1)
        {
          i++;
        }
      else if (i + 1 >= hl || tcp[i + 1] < 2)
        {
          break;
        }
      else
        {
          if (tcp[i] == 2 && tcp[i + 1] == 4 && i + 4 <= hl)
            {
              st->mss = tun_get16(tcp + i + 2);
            }

          i += tcp[i + 1];
        }
    }
}

/* Read the next segment of the connection from the TUN device.  Returns
 * 1 with a segment, 0 if none came within timeout_ms.  Checks each data
 * byte against the stream; marks *again when all of its bytes came before
 * (a retransmission).
 */

static int tun_recv_seg(FAR struct tun_state_s *st,
                        FAR struct tun_seg_s *seg, int timeout_ms,
                        FAR bool *again)
{
  uint32_t end = tun_now_ms() + timeout_ms;
  FAR uint8_t *tcp;
  struct pollfd pfd;
  uint32_t off;
  int ihl;
  int tot;
  int hl;
  int n;
  int i;

  for (; ; )
    {
      int left = (int)(end - tun_now_ms());

      if (left <= 0)
        {
          return 0;
        }

      pfd.fd      = st->fd;
      pfd.events  = POLLIN;
      pfd.revents = 0;
      if (poll(&pfd, 1, left) <= 0)
        {
          continue;
        }

      n = read(st->fd, st->pkt, sizeof(st->pkt));
      if (n < 40 || (st->pkt[0] >> 4) != 4 || st->pkt[9] != IPPROTO_TCP)
        {
          continue;
        }

      ihl = (st->pkt[0] & 0x0f) * 4;
      tot = tun_get16(st->pkt + 2);
      tcp = st->pkt + ihl;
      if (tot > n || tun_get16(tcp) != TUN_PORT_A ||
          tun_get16(tcp + 2) != st->pport)
        {
          continue;
        }

      hl         = (tcp[12] >> 4) * 4;
      seg->seq   = tun_get32(tcp + 4);
      seg->ack   = tun_get32(tcp + 8);
      seg->flags = tcp[13];
      seg->len   = tot - ihl - hl;
      seg->data  = tcp + hl;
      if (seg->len < 0)
        {
          continue;
        }

      if ((seg->flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK))
        {
          tun_get_mss(st, tcp, hl);
        }

      break;
    }

  *again = false;
  if ((seg->flags & TCP_RST) != 0)
    {
      st->rst = true;
      return 1;
    }

  if ((seg->flags & TCP_SYN) != 0 || !st->synack)
    {
      return 1;
    }

  off = seg->seq - (st->isn + 1);
  if (seg->len > 0)
    {
      *again = true;
      for (i = 0; i < seg->len; i++)
        {
          if (off + i >= sizeof(st->have) || st->have[off + i] == 0)
            {
              *again = false;
            }

          if (off + i >= TUN_TOTAL ||
              seg->data[i] != tun_pattern(off + i))
            {
              if (st->wrong++ == 0)
                {
                  st->wrongoff = off + i;
                }
            }

          if (off + i < sizeof(st->have))
            {
              st->have[off + i] = 1;
            }
        }
    }

  if ((seg->flags & TCP_FIN) != 0)
    {
      st->fin    = true;
      st->finlen = off + seg->len;
    }

  return 1;
}

/* The server: accept one connection, write TUN_TOTAL bytes in one send(),
 * then close().
 */

static FAR void *tun_server(FAR void *arg)
{
  FAR struct tun_state_s *st = arg;
  struct sockaddr_in addr;
  struct timeval tv;
  int one = 1;
  int lsd;
  int sd;
  int i;

  lsd = socket(AF_INET, SOCK_STREAM, 0);
  tv.tv_sec  = 10;
  tv.tv_usec = 0;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(TUN_PORT_A);
  addr.sin_addr.s_addr = htonl(TUN_ADDR_A);
  if (lsd < 0 ||
      setsockopt(lsd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0 ||
      setsockopt(lsd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ||
      bind(lsd, (FAR struct sockaddr *)&addr, sizeof(addr)) < 0 ||
      listen(lsd, 1) < 0)
    {
      st->senderr   = errno;
      st->listening = -1;
      st->done      = 1;
      if (lsd >= 0)
        {
          close(lsd);
        }

      return NULL;
    }

  st->listening = 1;
  sd = accept(lsd, NULL, NULL);
  close(lsd);
  if (sd < 0)
    {
      st->senderr = errno;
      st->done    = 1;
      return NULL;
    }

  for (i = 0; i < TUN_TOTAL; i++)
    {
      g_txbuf[i] = tun_pattern(i);
    }

  /* One send(): the first write buffers are full */

  tv.tv_sec = TUN_ROUNDS_MS / 1000 + 30;
  if (setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
    {
      st->senderr = errno;
      close(sd);
      st->done    = 1;
      return NULL;
    }

  st->sent    = send(sd, g_txbuf, TUN_TOTAL, 0);
  st->senderr = st->sent < 0 ? errno : 0;
  close(sd);
  st->done    = 1;
  return NULL;
}

/* Open the TUN device and set up its interface 10.78.0.1/24.  Returns 0
 * or an errno value.
 */

static int tun_open(FAR struct tun_state_s *st)
{
  FAR struct sockaddr_in *sin;
  struct ifreq ifr;
  int err;
  int ret;
  int sd;

  st->fd = open("/dev/tun", O_RDWR | O_CLOEXEC);
  if (st->fd < 0)
    {
      return errno;
    }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN;
  if (ioctl(st->fd, TUNSETIFF, (unsigned long)&ifr) < 0)
    {
      return errno;
    }

  strlcpy(st->ifname, ifr.ifr_name, sizeof(st->ifname));

  /* 10.78.0.1/24 on the interface, then up */

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      return errno;
    }

  memset(&ifr, 0, sizeof(ifr));
  strlcpy(ifr.ifr_name, st->ifname, sizeof(ifr.ifr_name));
  sin                  = (FAR struct sockaddr_in *)&ifr.ifr_addr;
  sin->sin_family      = AF_INET;
  sin->sin_addr.s_addr = htonl(TUN_ADDR_A);
  ret = ioctl(sd, SIOCSIFADDR, (unsigned long)&ifr);
  if (ret == 0)
    {
      sin                  = (FAR struct sockaddr_in *)&ifr.ifr_netmask;
      sin->sin_family      = AF_INET;
      sin->sin_addr.s_addr = htonl(0xffffff00);
      ret = ioctl(sd, SIOCSIFNETMASK, (unsigned long)&ifr);
    }

  if (ret == 0)
    {
      ret = ioctl(sd, SIOCGIFFLAGS, (unsigned long)&ifr);
    }

  if (ret == 0)
    {
      ifr.ifr_flags |= IFF_UP;
      ret = ioctl(sd, SIOCSIFFLAGS, (unsigned long)&ifr);
    }

  err = ret < 0 ? errno : 0;
  close(sd);
  return err;
}

/* The SYN of the peer and the SYN-ACK of the server.  The case sends the
 * ACK with its window.
 */

static void tun_handshake(FAR struct tun_state_s *st)
{
  struct tun_seg_s seg;
  bool again;
  int i;

  /* A new port and ISN in each run: the connection of an earlier run
   * can still be in TIME_WAIT.
   */

  st->pport = 40000 + tun_now_ms() % 20000;
  st->pisn  = TUN_ISN_P + tun_now_ms() * 64;
  st->pseq  = st->pisn + 1;
  for (i = 0; i < 5 && !st->synack; i++)
    {
      tun_send_tcp(st, 0, TCP_SYN, 65535);
      while (tun_recv_seg(st, &seg, 1000, &again) == 1)
        {
          if ((seg.flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK) &&
              seg.ack == st->pisn + 1)
            {
              st->isn    = seg.seq;
              st->synack = true;
              break;
            }
        }
    }

  assert_true(st->synack);
  assert_true(st->mss > 0);
}

/* The window opens: acknowledge what came, until the FIN.  Then close the
 * connection and wait for the server.  pthread_join() makes the results
 * of the server visible.
 */

static void tun_finish(FAR struct tun_state_s *st)
{
  struct tun_seg_s seg;
  uint32_t end;
  bool again;
  int i;

  end = tun_now_ms() + 30000;
  while (!st->rst && st->wrong == 0 &&
         (!st->fin || tun_contiguous(st) < st->finlen) &&
         (int32_t)(end - tun_now_ms()) > 0)
    {
      tun_recv_seg(st, &seg, 300, &again);
      tun_send_tcp(st, st->isn + 1 + tun_contiguous(st), TCP_ACK, 65535);
    }

  if (st->fin && !st->rst)
    {
      tun_send_tcp(st, st->isn + 1 + st->finlen + 1, TCP_FIN | TCP_ACK,
                   65535);
      while (tun_recv_seg(st, &seg, 500, &again) == 1)
        {
        }
    }

  for (i = 0; i < 50 && !st->done; i++)
    {
      usleep(100000);
    }

  if (st->done)
    {
      pthread_join(st->server, NULL);
      st->started = false;
    }
}

/* Every byte at its own sequence number, no reset, all the rounds, the
 * whole stream and the FIN.
 */

static void tun_check(FAR struct tun_state_s *st, FAR const char *name,
                      int rounds)
{
  if (st->wrong != 0 || st->rst || !st->fin || rounds != TUN_ROUNDS)
    {
      printf("%s: %d rounds, %d wrong bytes (first at offset %" PRIu32
             "), rst %d, fin %d at %" PRIu32 ", %" PRIu32 " bytes in "
             "order, send() %d errno %d\n",
             name, rounds, st->wrong, st->wrongoff, st->rst, st->fin,
             st->finlen, tun_contiguous(st), st->sent, st->senderr);
    }

  assert_int_equal(st->wrong, 0);
  assert_false(st->rst);
  assert_int_equal(rounds, TUN_ROUNDS);
  assert_true(st->fin);
  assert_int_equal(st->finlen, TUN_TOTAL);
  assert_int_equal(tun_contiguous(st), TUN_TOTAL);
  assert_int_equal(st->done, 1);
  assert_int_equal(st->sent, TUN_TOTAL);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: test_tun_tcp_setup
 ****************************************************************************/

int test_tun_tcp_setup(FAR void **state)
{
  FAR struct tun_state_s *st = &g_tun;
  int ret;
  int i;

  memset(st, 0, sizeof(*st));
  st->fd   = -1;
  st->sent = -1;
  *state   = st;

  /* No assert here: cmocka does not run the teardown after a failed
   * setup.  Undo the setup instead.
   */

  ret = tun_open(st);
  if (ret == 0)
    {
      ret = pthread_create(&st->server, NULL, tun_server, st);
      st->started = ret == 0;
    }

  for (i = 0; ret == 0 && i < 100 && st->listening == 0; i++)
    {
      usleep(20000);
    }

  if (ret != 0 || st->listening != 1)
    {
      printf("test_tun_tcp_setup: error %d, server %d\n", ret,
             st->listening);
      test_tun_tcp_teardown(state);
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: test_tun_tcp_teardown
 ****************************************************************************/

int test_tun_tcp_teardown(FAR void **state)
{
  FAR struct tun_state_s *st = *state;
  int i;

  /* Closing the TUN device takes the interface away: a server that still
   * waits on its connection gets an error.
   */

  if (st->fd >= 0)
    {
      close(st->fd);
      st->fd = -1;
    }

  for (i = 0; i < 300 && st->started && !st->done; i++)
    {
      usleep(100000);
    }

  if (st->started && st->done)
    {
      pthread_join(st->server, NULL);
      st->started = false;
    }

  return 0;
}

/****************************************************************************
 * Name: test_tun_tcp_lossy
 *
 * Description:
 *   A link that loses the first transmission of every segment.  The peer
 *   (this program, 10.78.0.2) opens a window of mss / 8 bytes, ignores a
 *   segment the first time and acknowledges it when it comes again.  So
 *   each timeout of the server retransmits one segment and the peer
 *   acknowledges it: the connection makes progress at each timeout.  After
 *   TUN_ROUNDS rounds (more than TCP_MAXRTX), the window opens and the peer
 *   acknowledges everything.  The server must send every byte at its own
 *   sequence number, the whole stream and the FIN, and no RST.
 *
 ****************************************************************************/

void test_tun_tcp_lossy(FAR void **state)
{
  FAR struct tun_state_s *st = *state;
  struct tun_seg_s seg;
  uint32_t lastack = 0;
  uint32_t end;
  uint16_t wnd;
  bool again;
  int rounds = 0;

  tun_handshake(st);
  wnd = st->mss / 8 > 0 ? st->mss / 8 : 1;
  tun_send_tcp(st, st->isn + 1, TCP_ACK, wnd);

  /* The lossy rounds: acknowledge a segment only when it comes again */

  end = tun_now_ms() + TUN_ROUNDS_MS;
  while (rounds < TUN_ROUNDS && !st->rst && st->wrong == 0 &&
         (int32_t)(end - tun_now_ms()) > 0)
    {
      if (tun_recv_seg(st, &seg, 200, &again) == 1 && seg.len > 0 &&
          again)
        {
          uint32_t ack = tun_contiguous(st);

          if (ack > lastack)
            {
              rounds++;
              lastack = ack;
            }

          tun_send_tcp(st, st->isn + 1 + ack, TCP_ACK, wnd);
        }
    }

  tun_finish(st);
  tun_check(st, "test_tun_tcp_lossy", rounds);
}
