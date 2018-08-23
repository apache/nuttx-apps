/****************************************************************************
 * examplex/ipforward/ipforward.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include <net/if.h>
#include <arpa/inet.h>

#include <nuttx/net/ip.h>
#include <nuttx/net/tcp.h>
#include <nuttx/net/icmpv6.h>
#include <nuttx/net/tun.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_DEVNAME     8
#define IPFWD_BUFSIZE   CONFIG_NET_TUN_MTU
#define NBYTES_PER_LINE 32
#define IPFWD_NPACKETS  3

#ifdef CONFIG_NET_IPv6
#  define IPADDR_TYPE   FAR const uint16_t *
#  define IP_HDRLEN     IPv6_HDRLEN
#else
#  define IPADDR_TYPE   uint32_t
#  define IP_HDRLEN     IPv4_HDRLEN
#endif

#if defined(CONFIG_NET_ETHERNET)
#  define MAC_ADDRLEN    6   /* IFHWADDRLEN */
#elif defined(CONFIG_NET_6LOWPAN)
#  ifdef CONFIG_NET_6LOWPAN_EXTENDEDADDR
#    define MAC_ADDRLEN  10  /* NET_6LOWPAN_EADDRSIZE */
#  else
#    define MAC_ADDRLEN  2   /* NET_6LOWPAN_SADDRSIZE */
#  endif
#else
#  define MAC_ADDRLEN    0   /* No link layer address */
#endif

/****************************************************************************
 * Name: Private Types
 ****************************************************************************/

struct ipfwd_tun_s
{
  int                it_fd;
  char               it_devname[MAX_DEVNAME];
};

struct ipfwd_state_s
{
  struct ipfwd_tun_s if_tun0;
  struct ipfwd_tun_s if_tun1;
  pthread_t          if_receiver;
  pthread_t          if_sender;
};

struct ipfwd_arg_s
{
  int                ia_fd;
  IPADDR_TYPE        ia_srcipaddr;
  IPADDR_TYPE        ia_destipaddr;
  uint8_t            ia_buffer[IPFWD_BUFSIZE];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
/* Network addresses:
 *
 * g_tun0_laddr is the address assigned to the tun0 device.  g_tun1_laddr
 * is the address assigned to tun1 device.  Both are loal addresses can the
 * target can received addresses on.
 *
 * g_netmask is the network mask that defines the networks:  tun0 is on
 * network 0; tun1 is on network 1.
 *
 * g_tun0_raddr and g_tun1_raddr are addresses that are not local to the
 * target, but should instead be forwarded via the correct network device.
 * g_tun0_raddr lies on network 0 and g_tun1_raddr lies on network 1 and so
 * should be forwarded from network 0 to network1 by the NuttX IP forwarding
 * logic.
 */

#ifdef CONFIG_NET_IPv6
static const uint16_t g_tun0_laddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),       /* Netork 0 */
  HTONS(0x0097),
};

static const uint16_t g_tun1_laddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x0001),  /* Netork 1 */
  HTONS(0x0139),
};

static const uint16_t g_tun0_raddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),       /* Netork 0 */
  HTONS(0x0062),
};

static const uint16_t g_tun1_raddr[8] =
{
  HTONS(0x7c00),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0),
  HTONS(0x0001),  /* Netork 1 */
  HTONS(0x0147),
};

static const uint16_t g_netmask[8] =
{
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0xffff),
  HTONS(0),
};
#else
static const uint32_t g_tun0_laddr = HTONL(0x0a000097);  /* Netork 0 */
static const uint32_t g_tun1_laddr = HTONL(0x0a000139);  /* Netork 1 */
static const uint32_t g_tun0_raddr = HTONL(0x0a000062);  /* Netork 0 */
static const uint32_t g_tun1_raddr = HTONL(0x0a000147);  /* Netork 1 */
static const uint32_t g_netmask    = HTONL(0xffffff00);
#endif

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
static const char g_payload[] = "Hi there, TUN receiver!";
#endif

#ifdef CONFIG_NET_IPv4
static uint16_t g_ipid;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ipfwd_tun_configure
 ****************************************************************************/

static int ipfwd_tun_configure(FAR struct ipfwd_tun_s *tun)
{
  struct ifreq ifr;
  int errcode;
  int ret;

  tun->it_fd = open("/dev/tun", O_RDWR);
  if (tun->it_fd < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: Failed to open /dev/tun: %d\n", errcode);
      return -errcode;
    }

  memset(&ifr, 0, sizeof(ifr));
  ifr.ifr_flags = IFF_TUN;

  ret = ioctl(tun->it_fd, TUNSETIFF, (unsigned long)&ifr);
  if (ret < 0)
    {
      errcode = errno;
      fprintf(stderr, "ERROR: ioctl TUNSETIFF failed: %d\n", errcode);
      close(tun->it_fd);
      return -errcode;
    }

  strncpy(tun->it_devname, ifr.ifr_name, MAX_DEVNAME);
  printf("Created TUN device: %s\n", tun->it_devname);
  return 0;
}

/****************************************************************************
 * Name: ipfwd_netconfig
 ****************************************************************************/

static int ipfwd_netconfig(FAR struct ipfwd_tun_s *tun, IPADDR_TYPE ipaddr,
                           IPADDR_TYPE netmask)
{
  int ret;

#ifdef CONFIG_NET_IPv6

  struct in6_addr addr;

  memcpy(addr.s6_addr16, ipaddr, 8 * sizeof(uint16_t));
  ret = netlib_set_ipv6addr(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv6addr() failed\n", ret);
      return ret;
    }

  memcpy(addr.s6_addr16, netmask, 8 * sizeof(uint16_t));
  ret = netlib_set_ipv6netmask(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv6netmask() failed\n", ret);
      return ret;
    }

#else /* CONFIG_NET_IPv4 */

  struct in_addr addr;

  addr.s_addr = ipaddr;
  ret = netlib_set_ipv4addr(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv4addr() failed\n", ret);
      return ret;
    }

  addr.s_addr = netmask;
  ret = netlib_set_ipv4netmask(tun->it_devname, &addr);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: netlib_set_ipv4netmask() failed\n", ret);
      return ret;
    }
#endif

  netlib_ifup(tun->it_devname);
  return 0;
}

/****************************************************************************
 * Name: Checksums
 ****************************************************************************/

static uint16_t chksum(uint16_t sum, FAR const uint8_t *data, uint16_t len)
{
  FAR const uint8_t *dataptr;
  FAR const uint8_t *last_byte;
  uint16_t t;

  dataptr = data;
  last_byte = data + len - 1;

  while (dataptr < last_byte)
    {
      /* At least two more bytes */

      t = ((uint16_t)dataptr[0] << 8) + dataptr[1];
      sum += t;
      if (sum < t)
        {
          sum++; /* carry */
        }

      dataptr += 2;
    }

  if (dataptr == last_byte)
    {
      t = (dataptr[0] << 8) + 0;
      sum += t;
      if (sum < t)
        {
          sum++; /* carry */
        }
    }

  /* Return sum in host byte order. */

  return sum;
}

#ifdef CONFIG_NET_IPv4
static uint16_t ipv4_chksum(FAR const uint8_t *buffer)
{
  uint16_t sum;

  sum = chksum(0, buffer, IPv4_HDRLEN);
  return (sum == 0) ? 0xffff : htons(sum);
}
#endif

static uint16_t common_chksum(FAR uint8_t *buffer, uint8_t proto)
{
#ifdef CONFIG_NET_IPv6
  FAR struct ipv6_hdr_s *ipv6 = (FAR struct ipv6_hdr_s *)buffer;
  uint16_t upperlen;
  uint16_t sum;

  /* The length reported in the IPv6 header is the length of the payload
   * that follows the header.
   */

  upperlen = ((uint16_t)ipv6->len[0] << 8) + ipv6->len[1];

  /* The checksum is calculated starting with a pseudo-header of IPv6 header
   * fields according to the IPv6 standard, which consists of the source
   * and destination addresses, the packet length and the next header field.
   */

  sum = upperlen + proto;

  /* Sum IP source and destination addresses. */

  sum = chksum(sum, (FAR uint8_t *)&ipv6->srcipaddr, 2 * sizeof(net_ipv6addr_t));

  /* Sum IP payload data. */

  sum = chksum(sum, &buffer[IPv6_HDRLEN], upperlen);
  return (sum == 0) ? 0xffff : htons(sum);
#else
  FAR struct ipv4_hdr_s *ipv4 = (FAR struct ipv4_hdr_s *)buffer;
  uint16_t upperlen;
  uint16_t sum;

  /* The length reported in the IPv4 header is the length of both the IPv4
   * header and the payload that follows the header.  We need to subtract
   * the size of the IPv4 header to get the size of the payload.
   */

  upperlen = (((uint16_t)(ipv4->len[0]) << 8) + ipv4->len[1]) - IPv4_HDRLEN;

  /* First sum pseudo-header. */
  /* IP protocol and length fields. This addition cannot carry. */

  sum = upperlen + proto;

  /* Sum IP source and destination addresses. */

  sum = chksum(sum, (FAR uint8_t *)&ipv4->srcipaddr, 2 * sizeof(in_addr_t));

  /* Sum IP payload data. */

  sum = chksum(sum, &buffer[IPv4_HDRLEN], upperlen);
  return (sum == 0) ? 0xffff : htons(sum);
#endif
}

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
static uint16_t tcp_chksum(FAR uint8_t *buffer)
{
  return common_chksum(buffer, IP_PROTO_TCP);
}
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_EXAMPLES_IPFORWARD_ICMPv6
static uint16_t icmpv6_chksum(FAR uint8_t *buffer)
{
  return common_chksum(buffer, IP_PROTO_ICMP6);
}
#endif

/****************************************************************************
 * Name: ipfwd_dumppkt (and friends)
 ****************************************************************************/

static char lib_nibble(unsigned char nibble)
{
  if (nibble < 10)
    {
      return '0' + nibble;
    }
  else
    {
      return 'a' + nibble - 10;
    }
}

static void ipfwd_dumpbuffer(FAR uint8_t *buffer, size_t buflen)
{
  unsigned int i;
  unsigned int j;
  unsigned int k;

  for (i = 0; i < buflen; i += NBYTES_PER_LINE)
    {
      putchar(' ');
      putchar(' ');

      /* Generate hex values:  2 * NBYTES_PER_LINE + 1 bytes */

      for (j = 0; j < NBYTES_PER_LINE; j++)
        {
          k = i + j;

          if (j == (NBYTES_PER_LINE / 2))
            {
              putchar(' ');
            }

          if (k < buflen)
            {
              putchar(lib_nibble((buffer[k] >> 4) & 0xf));
              putchar(lib_nibble(buffer[k] & 0xf));
            }
          else
            {
              putchar(' ');
              putchar(' ');
            }
        }

      putchar('\n');
   }
}

static void ipfwd_dumppkt(FAR uint8_t *buffer, size_t buflen)
{
  size_t dumpsize;

  if (buflen <= 0)
    {
      return;
    }

  dumpsize = IP_HDRLEN;
  if (dumpsize > buflen)
    {
      printf("Truncated ");
      dumpsize = buflen;
    }

  printf("IP Header:\n");
  ipfwd_dumpbuffer(buffer, dumpsize);

  buffer += dumpsize;
  buflen -= dumpsize;
  if (buflen <= 0)
    {
      printf("Packet truncated\n");
      return;
    }

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
  dumpsize = TCP_HDRLEN;
  if (dumpsize > buflen)
    {
      printf("Truncated ");
      dumpsize = buflen;
    }

  printf("TCP Header:\n");
  ipfwd_dumpbuffer(buffer, dumpsize);

  buffer += dumpsize;
  buflen -= dumpsize;
  if (buflen <= 0)
    {
      printf("Packet truncated\n");
      return;
    }

  printf("Payload:\n");
  ipfwd_dumpbuffer(buffer, buflen);
#else
  dumpsize = SIZEOF_ICMPV6_NEIGHBOR_SOLICIT_S(MAC_ADDRLEN);
  if (dumpsize > buflen)
    {
      printf("Truncated ");
      dumpsize = buflen;
    }

  printf("ICMPv6 Neighbor Solicitation:\n");
  ipfwd_dumpbuffer(buffer, dumpsize);
#endif
}

/****************************************************************************
 * Name: ipfwd_receiver
 ****************************************************************************/

static FAR void *ipfwd_receiver(FAR void *arg)
{
  FAR struct ipfwd_arg_s *fwd = (FAR struct ipfwd_arg_s *)arg;
  ssize_t nread;
  int errcode;
  int i;

  for (i = 0; i < IPFWD_NPACKETS; i++)
    {
      nread = read(fwd->ia_fd, fwd->ia_buffer, IPFWD_BUFSIZE);
      if (nread < 0)
        {
          errcode = errno;
          fprintf(stderr, "ERROR: read() failed: %d\n", errcode);
          break;
        }

      printf("Received packet %d: size=%lu\n", i+1, (unsigned long)nread);
      ipfwd_dumppkt(fwd->ia_buffer, nread);
    }

  return NULL;
}

/****************************************************************************
 * Name: ipfwd_sender
 ****************************************************************************/

static FAR void *ipfwd_sender(FAR void *arg)
{
  FAR struct ipfwd_arg_s *fwd = (FAR struct ipfwd_arg_s *)arg;
#ifdef CONFIG_NET_IPv6
  FAR struct ipv6_hdr_s *ipv6;
#else
  FAR struct ipv4_hdr_s *ipv4;
#endif
#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
  FAR struct tcp_hdr_s *tcp;
  FAR char *payload;
#endif
#ifdef CONFIG_EXAMPLES_IPFORWARD_ICMPv6
  FAR struct icmpv6_neighbor_solicit_s *sol;
#endif
  size_t paysize;
  size_t pktlen;
  ssize_t nwritten;
  uint16_t l3hdrlen;
  uint8_t proto;
  int errcode;
  int i;

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
  l3hdrlen = TCP_HDRLEN;
  paysize  = sizeof(g_payload);
  proto    = IP_PROTO_TCP;
#else
  l3hdrlen = SIZEOF_ICMPV6_NEIGHBOR_SOLICIT_S(MAC_ADDRLEN);
  paysize  = 0;
  proto    = IP_PROTO_ICMP6;
#endif

  for (i = 0; i < IPFWD_NPACKETS; i++)
    {
#ifdef CONFIG_NET_IPv6
      ipv6 = (FAR struct ipv6_hdr_s *)fwd->ia_buffer;

      /* Set up the IPv6 header */

      ipv6->vtc    = 0x60;                         /* Version/traffic class (MS) */
      ipv6->tcf    = 0;                            /* Traffic class (LS)/Flow label (MS) */
      ipv6->flow   = 0;                            /* Flow label (LS) */

      /* Length excludes the IPv6 header */

      pktlen       = l3hdrlen + paysize;
      ipv6->len[0] = (pktlen >> 8);
      ipv6->len[1] = (pktlen & 0xff);

      ipv6->proto  = proto;                 /* Next header */
      ipv6->ttl    = 255;                          /* Hop limit */

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
      /* Set the uniicast destination IP address */

      net_ipv6addr_copy(ipv6->destipaddr, fwd->ia_destipaddr);
#else
      /* Set the multicast destination IP address */

      ipv6->destipaddr[0] = HTONS(0xff02);
      ipv6->destipaddr[1] = HTONS(0x0000);
      ipv6->destipaddr[2] = HTONS(0x0000);
      ipv6->destipaddr[3] = HTONS(0x0000);
      ipv6->destipaddr[4] = HTONS(0x0000);
      ipv6->destipaddr[5] = HTONS(0x0001);
      ipv6->destipaddr[6] = fwd->ia_destipaddr[6] | HTONS(0xff00);
      ipv6->destipaddr[7] = fwd->ia_destipaddr[7];
#endif

      /* Set source IP address. */

      net_ipv6addr_copy(ipv6->srcipaddr,  fwd->ia_srcipaddr);

      pktlen       = IPv6_HDRLEN + l3hdrlen + paysize;
#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
      tcp          = (FAR struct tcp_hdr_s *)
                      &fwd->ia_buffer[IPv6_HDRLEN];
#else
      sol          = (FAR struct icmpv6_neighbor_solicit_s *)
                     &fwd->ia_buffer[IPv6_HDRLEN];
#endif
#else
      ipv4 = (FAR struct ipv4_hdr_s *)fwd->ia_buffer;

      /* Set up the IPv4 header */

      ipv4->vhl         = 0x45;
      ipv4->tos         = 0;

      pktlen            = IPv4_HDRLEN + l3hdrlen + paysize;
      ipv4->len[0]      = (pktlen >> 8);
      ipv4->len[1]      = (pktlen & 0xff);

      ++g_ipid;
      ipv4->ipid[0]     = g_ipid >> 8;
      ipv4->ipid[1]     = g_ipid & 0xff;

      ipv4->ipoffset[0] = IP_FLAG_DONTFRAG >> 8;
      ipv4->ipoffset[1] = IP_FLAG_DONTFRAG & 0xff;
      ipv4->ttl         = IP_TTL;
      ipv4->proto       = proto;

      net_ipv4addr_hdrcopy(ipv4->srcipaddr,  fwd->ia_srcipaddr);
      net_ipv4addr_hdrcopy(ipv4->destipaddr, fwd->ia_destipaddr);

      /* Calculate IP checksum. */

      ipv4->ipchksum    = 0;
      ipv4->ipchksum    = ~(ipv4_chksum(fwd->ia_buffer));

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
      tcp               = (FAR struct tcp_hdr_s *)
                           &fwd->ia_buffer[IPv4_HDRLEN];
#else
      sol               = (FAR struct icmpv6_neighbor_solicit_s *)
                           &fwd->ia_buffer[IPv4_HDRLEN];
#endif
#endif

#ifdef CONFIG_EXAMPLES_IPFORWARD_TCP
      /* Set up the TCP header.  NOTE:  Most of the elements are irrelevant
       * in this test. The forwarding is L2 layer only and the L3 header
       * content is not used in the forwarding.
       */

      memset(tcp, 0, sizeof(struct tcp_hdr_s));

      tcp->srcport     = HTONS(0x1234);
      tcp->destport    = HTONS(0xabcd);
      tcp->tcpoffset   = (TCP_HDRLEN / 4) << 4;

      payload          = (FAR char *)tcp + TCP_HDRLEN;
      memcpy(payload, g_payload, paysize);

      tcp->tcpchksum   = ~tcp_chksum(fwd->ia_buffer);
#else
      /* Set up the ICMPv6 Neighbor Solicitation message */

      sol->type     = ICMPv6_NEIGHBOR_SOLICIT; /* Message type */
      sol->code     = 0;                       /* Message qualifier */
      sol->flags[0] = 0;                       /* flags */
      sol->flags[1] = 0;
      sol->flags[2] = 0;
      sol->flags[3] = 0;

      /* Copy the target address into the Neighbor Solicitation message */

      net_ipv6addr_copy(sol->tgtaddr, fwd->ia_destipaddr);

      /* Set up the options */

      sol->opttype  = ICMPv6_OPT_SRCLLADDR;           /* Option type */
      sol->optlen   = ICMPv6_OPT_OCTECTS(MAC_ADDRLEN); /* Option length in octets */

      /* Copy our link layer address into the message */

      memset(sol->srclladdr, 0x88, MAC_ADDRLEN);

      /* Calculate the checksum over both the ICMP header and payload */

      sol->chksum   = 0;
      sol->chksum   = ~icmpv6_chksum(fwd->ia_buffer);
#endif

      printf("Sending packet %d: size=%lu\n", i+1, (unsigned long)pktlen);
      ipfwd_dumppkt(fwd->ia_buffer, pktlen);

      nwritten = write(fwd->ia_fd, fwd->ia_buffer, pktlen);
      if (nwritten < 0)
        {
          errcode = errno;
          fprintf(stderr, "ERROR: write() failed: %d\n", errcode);
          break;
        }

      printf("  %lu bytes sent\n", (unsigned long)nwritten);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fstest_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int ipfwd_main(int argc, char *argv[])
#endif
{
  struct ipfwd_state_s fwd;
  struct ipfwd_arg_s tun0arg;
  struct ipfwd_arg_s tun1arg;
  FAR void *value;
  int errcode = EXIT_SUCCESS;
  int ret;

  /* Initialize the first TUN device */

  ret = ipfwd_tun_configure(&fwd.if_tun0);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to create tun0: %d\n", ret);
      goto errout;
    }

  ret = ipfwd_netconfig(&fwd.if_tun0, g_tun0_laddr, g_netmask);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipfwd_netconfig for tun0 failed: %d\n", ret);
      goto errout_with_tun0;
    }

  /* Initialize the second TUN device */

  ret = ipfwd_tun_configure(&fwd.if_tun1);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to create tun1: %d\n", ret);
      goto errout_with_tun0;
    }

  ret = ipfwd_netconfig(&fwd.if_tun1, g_tun1_laddr, g_netmask);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: ipfwd_netconfig tun1 failed: %d\n", ret);
      errcode = EXIT_FAILURE;
      goto errout_with_tun1;
    }

  /* Start receiver thread on tun1 */

  tun1arg.ia_fd         = fwd.if_tun1.it_fd;
  tun1arg.ia_srcipaddr  = g_tun1_raddr;
  tun1arg.ia_destipaddr = g_tun0_raddr;

  ret = pthread_create(&fwd.if_receiver, NULL, ipfwd_receiver, &tun1arg);
  if (ret != 0)
    {
      fprintf(stderr, "ERROR: pthread_create() failed for receiver: %d\n", ret);
      errcode = EXIT_FAILURE;
      goto errout_with_tun1;
    }

  /* Start sender thread on tun0 */

  tun0arg.ia_fd         = fwd.if_tun0.it_fd;
  tun0arg.ia_srcipaddr  = g_tun0_raddr;
  tun0arg.ia_destipaddr = g_tun1_raddr;

  ret = pthread_create(&fwd.if_sender, NULL, ipfwd_sender, &tun0arg);
  if (ret != 0)
    {
      fprintf(stderr, "ERROR: pthread_create() failed for sender: %d\n", ret);
      errcode = EXIT_FAILURE;
      goto errout_with_receiver;
    }

  /* Wait for sender thread to terminate */

  ret = pthread_join(fwd.if_sender, &value);
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: pthread_join() failed for sender: %d\n", ret);
    }

errout_with_receiver:
  /* Wait for receiver thread to terminate */

  pthread_kill(fwd.if_receiver, 9);
  ret = pthread_join(fwd.if_receiver, &value);
  if (ret != OK)
    {
      fprintf(stderr, "ERROR: pthread_join() failed for receiver: %d\n", ret);
    }

errout_with_tun1:
  close(fwd.if_tun1.it_fd);
errout_with_tun0:
  close(fwd.if_tun0.it_fd);
errout:
  return errcode;
}
