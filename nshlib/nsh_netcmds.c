/****************************************************************************
 * apps/nshlib/nsh_netcmds.c
 *
 *   Copyright (C) 2007-2012, 2014-2015 Gregory Nutt. All rights reserved.
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
#ifdef CONFIG_NET

#include <sys/stat.h>    /* Needed for open */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>       /* Needed for open */
#include <libgen.h>      /* Needed for basename */
#include <errno.h>
#include <debug.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include <nuttx/clock.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/netstats.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/icmp.h>
#include <nuttx/net/icmpv6.h>

#if defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING) && \
   !defined(CONFIG_DISABLE_SIGNALS)
#  include <apps/netutils/netlib.h>
#  include <apps/netutils/dnsclient.h>
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#  include <apps/netutils/netlib.h>
#  include <apps/netutils/tftp.h>
#endif

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#  ifndef CONFIG_NSH_DISABLE_WGET
#    include <apps/netutils/netlib.h>
#    include <apps/netutils/webclient.h>
#  endif
#endif

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
#  ifdef CONFIG_HAVE_GETHOSTBYNAME
#    include <netdb.h>
#  else
#    include <apps/netutils/dnsclient.h>
#  endif
#  include <apps/netutils/dhcpc.h>
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef HAVE_PING
#undef HAVE_PING6

#if defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING) && \
   !defined(CONFIG_DISABLE_SIGNALS) && !defined(CONFIG_NSH_DISABLE_PING)
#  define HAVE_PING
#endif

#if defined(CONFIG_NET_ICMPv6) && defined(CONFIG_NET_ICMPv6_PING) && \
   !defined(CONFIG_DISABLE_SIGNALS) && !defined(CONFIG_NSH_DISABLE_PING6)
#  define HAVE_PING6
#endif

/* Size of the ECHO data */

#define DEFAULT_PING_DATALEN 56

/* Get the larger value */

#ifndef MAX
#  define MAX(a,b) (a > b ? a : b)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
struct tftpc_args_s
{
  bool        binary;    /* true:binary ("octet") false:text ("netascii") */
  bool        allocated; /* true: destpath is allocated */
  char       *destpath;  /* Path at destination */
  const char *srcpath;   /* Path at src */
  in_addr_t   ipaddr;    /* Host IP address */
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if defined(HAVE_PING) || defined(HAVE_PING6)
static uint16_t g_pingid = 0;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping_newid
 ****************************************************************************/

#if defined(HAVE_PING) || defined(HAVE_PING6)
static uint16_t ping_newid(void)
{
  irqstate_t save = irqsave();
  uint16_t ret = ++g_pingid;
  irqrestore(save);
  return ret;
}
#endif /* HAVE_PING || HAVE_PINg */

/****************************************************************************
 * Name: ping_options
 ****************************************************************************/

#if defined(HAVE_PING) || defined(HAVE_PING6)
static int ping_options(FAR struct nsh_vtbl_s *vtbl,
                        int argc, FAR char **argv,
                        FAR int *count, FAR uint32_t *dsec, FAR char **staddr)
{
  FAR const char *fmt = g_fmtarginvalid;
  bool badarg = false;
  int option;
  int tmp;

  /* Get the ping options */

  while ((option = getopt(argc, argv, ":c:i:")) != ERROR)
    {
      switch (option)
        {
          case 'c':
            tmp = atoi(optarg);
            if (tmp < 1 || tmp > 10000)
              {
                nsh_output(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            else
              {
                *count = tmp;
              }
            break;

          case 'i':
            tmp = atoi(optarg);
            if (tmp < 1 || tmp >= 4294)
              {
                nsh_output(vtbl, g_fmtargrange, argv[0]);
                badarg = true;
              }
            else
              {
                *dsec = 10 * tmp;
              }
            break;

          case ':':
            nsh_output(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_output(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered, then return without processing the
   * command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There should be exactly on parameter left on the command-line */

  if (optind == argc-1)
    {
      *staddr = argv[optind];
    }
  else if (optind >= argc)
    {
      fmt = g_fmttoomanyargs;
      goto errout;
    }
  else
    {
      fmt = g_fmtargrequired;
      goto errout;
    }

  return OK;

errout:
  nsh_output(vtbl, fmt, argv[0]);
  return ERROR;
}
#endif /* HAVE_PING || HAVE_PING6 */

/****************************************************************************
 * Name: net_statistics
 ****************************************************************************/

#if defined(CONFIG_NET_STATISTICS) && !defined(CONFIG_NSH_DISABLE_IFCONFIG)
static inline void net_statistics(FAR struct nsh_vtbl_s *vtbl)
{
  /* Headings */

  nsh_output(vtbl, "           ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  IPv4");
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  IPv6");
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "   TCP");
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "   UDP");
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  ICMP");
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  ICMPv6");
#endif
  nsh_output(vtbl, "\n");

  /* Received packets */

  nsh_output(vtbl, "Received   ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  %04x", g_netstats.ipv4.recv);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  %04x", g_netstats.ipv6.recv);
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  %04x", g_netstats.tcp.recv);
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  %04x", g_netstats.udp.recv);
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  %04x", g_netstats.icmp.recv);
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  %04x", g_netstats.icmpv6.recv);
#endif
  nsh_output(vtbl, "\n");

  /* Dropped packets */

  nsh_output(vtbl, "Dropped    ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  %04x", g_netstats.ipv4.drop);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  %04x", g_netstats.ipv6.drop);
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  %04x", g_netstats.tcp.drop);
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  %04x", g_netstats.udp.drop);
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  %04x", g_netstats.icmp.drop);
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  %04x", g_netstats.icmpv6.drop);
#endif
  nsh_output(vtbl, "\n");

  /* Dropped IP packets */

#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  IPv4        VHL: %04x   Frg: %04x\n",
             g_netstats.ipv4.vhlerr, g_netstats.ipv4.fragerr);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  IPv6        VHL: %04x\n",
             g_netstats.ipv6.vhlerr);
#endif

  /* Checksum errors */

  nsh_output(vtbl, "  Checksum ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  %04x", g_netstats.ipv4.chkerr);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  %04x", g_netstats.tcp.chkerr);
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  %04x", g_netstats.udp.chkerr);
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  ----");
#endif
  nsh_output(vtbl, "\n");

#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  TCP         ACK: %04x   SYN: %04x\n",
            g_netstats.tcp.ackerr, g_netstats.tcp.syndrop);
  nsh_output(vtbl, "              RST: %04x  %04x\n",
            g_netstats.tcp.rst, g_netstats.tcp.synrst);
#endif

  /* Prototype errors */

  nsh_output(vtbl, "  Type     ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  %04x", g_netstats.ipv4.protoerr);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  %04x", g_netstats.ipv6.protoerr);
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  %04x", g_netstats.icmp.typeerr);
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  %04x", g_netstats.icmpv6.typeerr);
#endif
  nsh_output(vtbl, "\n");

  /* Sent packets */

  nsh_output(vtbl, "Sent       ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  %04x", g_netstats.ipv4.sent);
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  %04x", g_netstats.ipv6.sent);
#endif
#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  %04x", g_netstats.tcp.sent);
#endif
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  %04x", g_netstats.udp.sent);
#endif
#ifdef CONFIG_NET_ICMP
  nsh_output(vtbl, "  %04x", g_netstats.icmp.sent);
#endif
#ifdef CONFIG_NET_ICMPv6
  nsh_output(vtbl, "  %04x", g_netstats.icmpv6.sent);
#endif
  nsh_output(vtbl, "\n");

  /* TCP retransmissions */

#ifdef CONFIG_NET_TCP
  nsh_output(vtbl, "  Rexmit   ");
#ifdef CONFIG_NET_IPv4
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_IPv6
  nsh_output(vtbl, "  ----");
#endif
  nsh_output(vtbl, "  %04x", g_netstats.tcp.rexmit);
#ifdef CONFIG_NET_UDP
  nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_ICMP
   nsh_output(vtbl, "  ----");
#endif
#ifdef CONFIG_NET_ICMPv6
   nsh_output(vtbl, "  ----");
#endif
  nsh_output(vtbl, "\n");
#endif /* CONFIG_NET_TCP */
}
#else
# define net_statistics(vtbl)
#endif

/****************************************************************************
 * Name: ifconfig_callback
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_IFUPDOWN) || !defined(CONFIG_NSH_DISABLE_IFCONFIG)
static int ifconfig_callback(FAR struct net_driver_s *dev, void *arg)
{
  struct nsh_vtbl_s *vtbl = (struct nsh_vtbl_s*)arg;
#ifdef CONFIG_NET_IPv4
  struct in_addr addr;
#endif
#ifdef CONFIG_NET_IPv6
  char addrstr[INET6_ADDRSTRLEN];
  uint8_t preflen;
#endif
  uint8_t iff;
  const char *status;
  int ret;

  /* Get the interface status:  RUNNING, UP, or DOWN */

  ret = netlib_getifstatus(dev->d_ifname, &iff);
  if (ret != OK)
    {
      nsh_output(vtbl, "\tGet %s interface flags error: %d\n",
                 dev->d_ifname, ret);
    }

  if (iff & IFF_RUNNING)
    {
      status = "RUNNING";
    }
  else if (iff & IFF_UP)
    {
      status = "UP";
    }
  else
    {
      status = "DOWN";
    }

#if defined(CONFIG_NET_MULTILINK)
  /* If there are multiple link types being supported, then selected the
   * output appropriate for the link type associated with this device.
   */

  switch (dev->d_lltype)
    {
#  if defined(CONFIG_NET_ETHERNET)
      case NET_LL_ETHERNET:
        nsh_output(vtbl, "%s\tLink encap:Ethernet HWaddr %s",
                   dev->d_ifname, ether_ntoa(&dev->d_mac));
        break;
#  endif

#  if defined(CONFIG_NET_SLIP)
      case NET_LL_SLIP:
        nsh_output(vtbl, "%s\tLink encap:SLIP", dev->d_ifname);
        break;
#  endif

#  if defined(CONFIG_NET_PPP)
      case NET_LL_PPP:
        nsh_output(vtbl, "%s\tLink encap:P-t-P", dev->d_ifname);
        break;
#  endif

#  if defined(CONFIG_NET_TUN)
      case NET_LL_TUN:
        nsh_output(vtbl, "%s\tLink encap:TUN", dev->d_ifname);
        break;
#  endif

      default:
        nsh_output(vtbl, "%s\tLink encap:UNSPEC", dev->d_ifname);
    }

  nsh_output(vtbl, " at %s\n", status);

#elif defined(CONFIG_NET_ETHERNET)
  nsh_output(vtbl, "%s\tLink encap:Ethernet HWaddr %s at %s\n",
             dev->d_ifname, ether_ntoa(&dev->d_mac), status);

#elif defined(CONFIG_NET_SLIP)
  nsh_output(vtbl, "%s\tLink encap:SLIP at %s\n", dev->d_ifname, status);

#elif defined(CONFIG_NET_PPP)
  nsh_output(vtbl, "%s\tLink encap:P-t-P at %s\n", dev->d_ifname, status);

#elif defined(CONFIG_NET_TUN)
  nsh_output(vtbl, "%s\tLink encap:TUN at %s\n", dev->d_ifname, status);

#endif

#ifdef CONFIG_NET_IPv4
  /* Show the IPv4 address */

  addr.s_addr = dev->d_ipaddr;
  nsh_output(vtbl, "\tinet addr:%s ", inet_ntoa(addr));

  /* Show the IPv4 default router address */

  addr.s_addr = dev->d_draddr;
  nsh_output(vtbl, "DRaddr:%s ", inet_ntoa(addr));

  /* Show the IPv4 network mask */

  addr.s_addr = dev->d_netmask;
  nsh_output(vtbl, "Mask:%s\n", inet_ntoa(addr));

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
  dns_getserver(&addr);
  nsh_output(vtbl, "\tDNSaddr:%s\n", inet_ntoa(addr));
#endif
#endif

#ifdef CONFIG_NET_IPv6
  /* Convert the 128 network mask to a human friendly prefix length */

  preflen = netlib_ipv6netmask2prefix(dev->d_ipv6netmask);

  /* Show the assigned IPv6 address */

  if (inet_ntop(AF_INET6, dev->d_ipv6addr, addrstr, INET6_ADDRSTRLEN))
    {
      nsh_output(vtbl, "\tinet6 addr:%s/%d\n", addrstr, preflen);
    }

  /* REVISIT: Show the IPv6 default router address */

  if (inet_ntop(AF_INET6, dev->d_ipv6draddr, addrstr, INET6_ADDRSTRLEN))
    {
      nsh_output(vtbl, "\tinet6 DRaddr:%s/%d\n", addrstr, preflen);
    }

#if defined(CONFIG_NSH_DHCPCv6) || defined(CONFIG_NSH_DNS)
#  warning Missing logic
#endif
#endif

  nsh_output(vtbl, "\n");
  return OK;
}
#endif /* !CONFIG_NSH_DISABLE_IFUPDOWN || !CONFIG_NSH_DISABLE_IFCONFIG */

/****************************************************************************
 * Name: tftpc_parseargs
 ****************************************************************************/

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
int tftpc_parseargs(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv,
                    struct tftpc_args_s *args)
{
  FAR const char *fmt = g_fmtarginvalid;
  bool badarg = false;
  int option;

  /* Get the ping options */

  memset(args, 0, sizeof(struct tftpc_args_s));
  while ((option = getopt(argc, argv, ":bnf:h:")) != ERROR)
    {
      switch (option)
        {
          case 'b':
            args->binary = true;
            break;

          case 'n':
            args->binary = false;
            break;

          case 'f':
            args->destpath = optarg;
            break;

          case 'h':
            if (!netlib_ipaddrconv(optarg, (FAR unsigned char*)&args->ipaddr))
              {
                nsh_output(vtbl, g_fmtarginvalid, argv[0]);
                badarg = true;
              }
            break;

          case ':':
            nsh_output(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_output(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered, then return without processing the command */

  if (badarg)
    {
      return ERROR;
    }

  /* There should be exactly one parameter left on the command-line */

  if (optind == argc-1)
    {
      args->srcpath = argv[optind];
    }

  /* optind == argc means that there is nothing left on the command-line */

  else if (optind >= argc)
    {
      fmt = g_fmtargrequired;
      goto errout;
    }

  /* optind < argc-1 means that there are too many arguments on the
   * command-line
   */

  else
    {
      fmt = g_fmttoomanyargs;
      goto errout;
    }

  /* The HOST IP address is also required */

  if (!args->ipaddr)
    {
      fmt = g_fmtargrequired;
      goto errout;
    }

  /* If the destpath was not provided, then we have do a little work. */

  if (!args->destpath)
    {
      char *tmp1;
      char *tmp2;

      /* Copy the srcpath... baseanme might modify it */

      fmt = g_fmtcmdoutofmemory;
      tmp1 = strdup(args->srcpath);
      if (!tmp1)
        {
          goto errout;
        }

      /* Get the basename of the srcpath */

      tmp2 = basename(tmp1);
      if (!tmp2)
        {
          free(tmp1);
          goto errout;
        }

      /* Use that basename as the destpath */

      args->destpath  = strdup(tmp2);
      free(tmp1);
      if (!args->destpath)
        {
          goto errout;
        }

      args->allocated = true;
    }

  return OK;

errout:
  nsh_output(vtbl, fmt, argv[0]);
  return ERROR;
}
#endif

/****************************************************************************
 * Name: wget_callback
 ****************************************************************************/

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#ifndef CONFIG_NSH_DISABLE_WGET
static void wget_callback(FAR char **buffer, int offset, int datend,
                          FAR int *buflen, FAR void *arg)
{
  (void)write((int)arg, &((*buffer)[offset]), datend - offset);
}
#endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cmd_get
 ****************************************************************************/

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#ifndef CONFIG_NSH_DISABLE_GET
int cmd_get(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct tftpc_args_s args;
  char *fullpath;

  /* Parse the input parameter list */

  if (tftpc_parseargs(vtbl, argc, argv, &args) != OK)
    {
      return ERROR;
    }

  /* Get the full path to the local file */

  fullpath = nsh_getfullpath(vtbl, args.destpath);

  /* Then perform the TFTP get operation */

  if (tftpget(args.srcpath, fullpath, args.ipaddr, args.binary) != OK)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "tftpget", NSH_ERRNO);
    }

  /* Release any allocated memory */

  if (args.allocated)
    {
      free(args.destpath);
    }

  free(fullpath);
  return OK;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_ifup
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_IFUPDOWN
int cmd_ifup(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *intf = NULL;
  int ret;

  if (argc != 2)
    {
      nsh_output(vtbl, "Please select nic_name:\n");
      netdev_foreach(ifconfig_callback, vtbl);
      return OK;
    }

  intf = argv[1];
  ret  = netlib_ifup(intf);
  nsh_output(vtbl, "ifup %s...%s\n", intf, (ret == OK) ? "OK" : "Failed");
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_ifdown
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_IFUPDOWN
int cmd_ifdown(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *intf = NULL;
  int ret;

  if (argc != 2)
    {
      nsh_output(vtbl, "Please select nic_name:\n");
      netdev_foreach(ifconfig_callback, vtbl);
      return OK;
    }

  intf = argv[1];
  ret = netlib_ifdown(intf);
  nsh_output(vtbl, "ifdown %s...%s\n", intf, (ret == OK) ? "OK" : "Failed");
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_ifconfig
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_IFCONFIG
int cmd_ifconfig(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
#ifdef CONFIG_NET_IPv4
  struct in_addr addr;
#endif
#ifdef CONFIG_NET_IPv6
  struct in6_addr addr6;
#endif
  in_addr_t gip;
  int i;
  FAR char *intf = NULL;
  FAR char *hostip = NULL;
  FAR char *gwip = NULL;
  FAR char *mask = NULL;
  FAR char *tmp = NULL;
#ifdef CONFIG_NET_ETHERNET
  FAR char *hw = NULL;
#endif
#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
  FAR char *dns = NULL;
#endif
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  bool inet6 = false;
#endif
  bool badarg = false;
  uint8_t mac[IFHWADDRLEN];
#if defined(CONFIG_NSH_DHCPC)
  FAR void *handle;
#endif

  /* With one or no arguments, ifconfig simply shows the status of Ethernet
   * device:
   *
   *   ifconfig
   *   ifconfig [nic_name]
   */

  if (argc <= 2)
    {
      netdev_foreach(ifconfig_callback, vtbl);
      net_statistics(vtbl);
      return OK;
    }

  /* If both the network interface name and an IP address are supplied as
   * arguments, then ifconfig will set the address of the Ethernet device:
   *
   *    ifconfig nic_name ip_address
   */

  if (argc > 2)
    {
      for (i = 0; i < argc; i++)
        {
          if (i == 1)
            {
              intf = argv[i];
            }
          else if (i == 2)
            {
              hostip = argv[i];
            }
          else
            {
              tmp = argv[i];
              if (!strcmp(tmp, "dr") || !strcmp(tmp, "gw") || !strcmp(tmp, "gateway"))
                {
                  if (argc-1 >= i+1)
                    {
                      gwip = argv[i+1];
                      i++;
                    }
                  else
                    {
                      badarg = true;
                    }
                }
              else if (!strcmp(tmp, "netmask"))
                {
                  if (argc-1 >= i+1)
                    {
                      mask = argv[i+1];
                      i++;
                    }
                  else
                    {
                      badarg = true;
                    }
                }
              else if (!strcmp(tmp, "inet"))
                {
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
                  inet6 = false;
#elif !defined(CONFIG_NET_IPv4)
                  badarg = true;
#endif
                }
              else if (!strcmp(tmp, "inet6"))
                {
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
                  inet6 = true;
#elif !defined(CONFIG_NET_IPv6)
                  badarg = true;
#endif
                }

#ifdef CONFIG_NET_ETHERNET
              /* REVISIT: How will we handle Ethernet and SLIP networks together? */

              else if (!strcmp(tmp, "hw"))
                {
                  if (argc-1>=i+1)
                    {
                      hw = argv[i+1];
                      i++;
                      badarg = !netlib_hwmacconv(hw, mac);
                    }
                  else
                    {
                      badarg = true;
                    }
                }
#endif

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
              else if (!strcmp(tmp, "dns"))
                {
                  if (argc-1 >= i+1)
                    {
                      dns = argv[i+1];
                      i++;
                    }
                  else
                    {
                      badarg = true;
                    }
                }
#endif
            }
        }
    }

  if (badarg)
    {
      nsh_output(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

#ifdef CONFIG_NET_ETHERNET
  /* Set Hardware Ethernet MAC address */
  /* REVISIT: How will we handle Ethernet and SLIP networks together? */

  if (hw)
    {
      ndbg("HW MAC: %s\n", hw);
      netlib_setmacaddr(intf, mac);
    }
#endif

   /* Set IP address */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  if (inet6)
#endif
    {
#warning Missing Logic
      UNUSED(addr6);
      UNUSED(gip);
      UNUSED(hostip);
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  else
#endif
    {
#if defined(CONFIG_NSH_DHCPC)
      if (!strcmp(hostip, "dhcp"))
        {
          /* Set DHCP addr */

          ndbg("DHCPC Mode\n");
          gip = addr.s_addr = 0;
        }
      else
#endif
        {
          /* Set host IP address */

          ndbg("Host IP: %s\n", hostip);
          gip = addr.s_addr = inet_addr(hostip);
        }

      netlib_set_ipv4addr(intf, &addr);
    }
#endif /* CONFIG_NET_IPv4 */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  if (inet6)
#endif
    {
#warning Missing Logic
      UNUSED(gwip);
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  else
#endif
    {
      /* Set gateway */

      if (gwip)
        {
          ndbg("Gateway: %s\n", gwip);
          gip = addr.s_addr = inet_addr(gwip);
        }
      else
        {
          if (gip)
            {
              ndbg("Gateway: default\n");
              gip  = NTOHL(gip);
              gip &= ~0x000000ff;
              gip |= 0x00000001;
              gip  = HTONL(gip);
            }

          addr.s_addr = gip;
        }

      netlib_set_dripv4addr(intf, &addr);
    }
#endif /* CONFIG_NET_IPv4 */

  /* Set network mask */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  if (inet6)
#endif
    {
#warning Missing Logic
      UNUSED(mask);
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  else
#endif
    {
      if (mask)
        {
          ndbg("Netmask: %s\n",mask);
          addr.s_addr = inet_addr(mask);
        }
      else
        {
          ndbg("Netmask: Default\n");
          addr.s_addr = inet_addr("255.255.255.0");
        }

      netlib_set_ipv4netmask(intf, &addr);
    }
#endif /* CONFIG_NET_IPv4 */

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  if (inet6)
#endif
    {
#warning Missing Logic
    }
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
  else
#endif
    {
      if (dns)
        {
          ndbg("DNS: %s\n", dns);
          addr.s_addr = inet_addr(dns);
        }
      else
        {
          ndbg("DNS: Default\n");
          addr.s_addr = gip;
        }

      dns_setserver(&addr);
    }
#endif /* CONFIG_NET_IPv4 */
#endif /* CONFIG_NSH_DHCPC || CONFIG_NSH_DNS */

#if defined(CONFIG_NSH_DHCPC)
  /* Get the MAC address of the NIC */

  if (!gip)
    {
      netlib_getmacaddr("eth0", mac);

      /* Set up the DHCPC modules */

      handle = dhcpc_open(&mac, IFHWADDRLEN);

      /* Get an IP address.  Note that there is no logic for renewing the IP
       * address in this example.  The address should be renewed in
       * ds.lease_time/2 seconds.
       */

      if (handle)
        {
          struct dhcpc_state ds;

          (void)dhcpc_request(handle, &ds);
          netlib_set_ipv4addr("eth0", &ds.ipaddr);

          if (ds.netmask.s_addr != 0)
            {
              netlib_set_ipv4netmask("eth0", &ds.netmask);
            }

          if (ds.default_router.s_addr != 0)
            {
              netlib_set_dripv4addr("eth0", &ds.default_router);
            }

          if (ds.dnsaddr.s_addr != 0)
            {
              dns_setserver(&ds.dnsaddr);
            }

          dhcpc_close(handle);
        }
    }
#endif

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_ping
 ****************************************************************************/

#ifdef HAVE_PING
int cmd_ping(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *staddr;
  in_addr_t ipaddr;
  uint32_t start;
  uint32_t next;
  uint32_t dsec = 10;
  uint32_t maxwait;
  uint16_t id;
  int count = 10;
  int seqno;
  int replies = 0;
  int elapsed;
  int ret;
  int tmp;
  int i;

  /* Get the ping options */

  ret = ping_options(vtbl, argc, argv, &count, &dsec, &staddr);
  if (ret < 0)
    {
      return ERROR;
    }

  /* Get the IP address in binary form */

  ret = inet_pton(AF_INET, staddr, &ipaddr);
  if (ret == 0)
    {
      ret = dns_gethostip(staddr, &ipaddr);
      if (ret < 0)
        {
          nsh_output(vtbl, "nsh: %s: unable to resolve hostname '%s'\n", argv[0], staddr);
          return ERROR;
        }
    }

  /* Get the ID to use */

  id = ping_newid();

  /* The maximum wait for a response will be the larger of the inter-ping
   * time and the configured maximum round-trip time.
   */

  maxwait = MAX(dsec, CONFIG_NSH_MAX_ROUNDTRIP);

  /* Loop for the specified count */

  nsh_output(vtbl, "PING %d.%d.%d.%d %d bytes of data\n",
            (ipaddr       ) & 0xff, (ipaddr >> 8  ) & 0xff,
            (ipaddr >> 16 ) & 0xff, (ipaddr >> 24 ) & 0xff,
            DEFAULT_PING_DATALEN);

  start = clock_systimer();
  for (i = 1; i <= count; i++)
    {
      /* Send the ECHO request and wait for the response */

      next  = clock_systimer();
      seqno = icmp_ping(ipaddr, id, i, DEFAULT_PING_DATALEN, maxwait);

      /* Was any response returned? We can tell if a non-negative sequence
       * number was returned.
       */

      if (seqno >= 0 && seqno <= i)
        {
          /* Get the elapsed time from the time that the request was
           * sent until the response was received.  If we got a response
           * to an earlier request, then fudge the elapsed time.
           */

          elapsed = TICK2MSEC(clock_systimer() - next);
          if (seqno < i)
            {
              elapsed += 100 * dsec * (i - seqno);
            }

          /* Report the receipt of the reply */

          nsh_output(vtbl, "%d bytes from %s: icmp_seq=%d time=%d ms\n",
                     DEFAULT_PING_DATALEN, staddr, seqno, elapsed);
          replies++;
        }
      else if (seqno < 0)
        {
          if (seqno == -ETIMEDOUT)
            {
              nsh_output(vtbl, "icmp_seq=%d Request timeout\n", i);
            }
          else if (seqno == -ENETUNREACH)
            {
              nsh_output(vtbl, "icmp_seq=%d Network is unreachable\n", i);
            }
        }

      /* Wait for the remainder of the interval.  If the last seqno<i,
       * then this is a bad idea... we will probably lose the response
       * to the current request!
       */

      elapsed = TICK2DSEC(clock_systimer() - next);
      if (elapsed < dsec)
        {
          usleep(100000 * (dsec - elapsed));
        }
    }

  /* Get the total elapsed time */

  elapsed = TICK2MSEC(clock_systimer() - start);

  /* Calculate the percentage of lost packets */

  tmp = (100*(count - replies) + (count >> 1)) / count;

  nsh_output(vtbl, "%d packets transmitted, %d received, %d%% packet loss, time %d ms\n",
             count, replies, tmp, elapsed);
  return OK;
}
#endif /* HAVE_PING */

/****************************************************************************
 * Name: cmd_ping6
 ****************************************************************************/

#ifdef HAVE_PING6
int cmd_ping6(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *staddr;
  struct in6_addr ipaddr;
  uint32_t start;
  uint32_t next;
  uint32_t dsec = 10;
  uint32_t maxwait;
  uint16_t id;
  int count = 10;
  int seqno;
  int replies = 0;
  int elapsed;
  int ret;
  int tmp;
  int i;

  /* Get the ping options */

  ret = ping_options(vtbl, argc, argv, &count, &dsec, &staddr);
  if (ret < 0)
    {
      return ERROR;
    }

  /* Get the IP address in binary form
   * REVISIT:  DNS hostname look-up not yet supported
   */

#if 0
  ret = dns_gethostip(staddr, &ipaddr);
#else
  ret = inet_pton(AF_INET6, staddr, &ipaddr);
#endif
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

  /* Get the ID to use */

  id = ping_newid();

  /* The maximum wait for a response will be the larger of the inter-ping
   * time and the configured maximum round-trip time.
   */

  maxwait = MAX(dsec, CONFIG_NSH_MAX_ROUNDTRIP);

  /* Loop for the specified count */

  nsh_output(vtbl,
             "PING6 %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x %d bytes of data\n",
             ntohs(ipaddr.s6_addr16[0]), ntohs(ipaddr.s6_addr16[1]),
             ntohs(ipaddr.s6_addr16[2]), ntohs(ipaddr.s6_addr16[3]),
             ntohs(ipaddr.s6_addr16[4]), ntohs(ipaddr.s6_addr16[5]),
             ntohs(ipaddr.s6_addr16[6]), ntohs(ipaddr.s6_addr16[7]),
             DEFAULT_PING_DATALEN);

  start = clock_systimer();
  for (i = 1; i <= count; i++)
    {
      /* Send the ECHO request and wait for the response */

      next  = clock_systimer();
      seqno = icmpv6_ping(ipaddr.s6_addr16, id, i, DEFAULT_PING_DATALEN, maxwait);

      /* Was any response returned? We can tell if a non-negative sequence
       * number was returned.
       */

      if (seqno >= 0 && seqno <= i)
        {
          /* Get the elapsed time from the time that the request was
           * sent until the response was received.  If we got a response
           * to an earlier request, then fudge the elapsed time.
           */

          elapsed = TICK2MSEC(clock_systimer() - next);
          if (seqno < i)
            {
              elapsed += 100 * dsec * (i - seqno);
            }

          /* Report the receipt of the reply */

          nsh_output(vtbl, "%d bytes from %s: icmp_seq=%d time=%d ms\n",
                     DEFAULT_PING_DATALEN, staddr, seqno, elapsed);
          replies++;
        }

      /* Wait for the remainder of the interval.  If the last seqno<i,
       * then this is a bad idea... we will probably lose the response
       * to the current request!
       */

      elapsed = TICK2DSEC(clock_systimer() - next);
      if (elapsed < dsec)
        {
          usleep(100000 * (dsec - elapsed));
        }
    }

  /* Get the total elapsed time */

  elapsed = TICK2MSEC(clock_systimer() - start);

  /* Calculate the percentage of lost packets */

  tmp = (100*(count - replies) + (count >> 1)) / count;

  nsh_output(vtbl, "%d packets transmitted, %d received, %d%% packet loss, time %d ms\n",
             count, replies, tmp, elapsed);
  return OK;
}
#endif /* CONFIG_NET_ICMPv6 && CONFIG_NET_ICMPv6_PING */

/****************************************************************************
 * Name: cmd_put
 ****************************************************************************/

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#ifndef CONFIG_NSH_DISABLE_PUT
int cmd_put(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct tftpc_args_s args;
  char *fullpath;

  /* Parse the input parameter list */

  if (tftpc_parseargs(vtbl, argc, argv, &args) != OK)
    {
      return ERROR;
    }

  /* Get the full path to the local file */

  fullpath = nsh_getfullpath(vtbl, args.srcpath);

  /* Then perform the TFTP put operation */

  if (tftpput(fullpath, args.destpath, args.ipaddr, args.binary) != OK)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "tftpput", NSH_ERRNO);
    }

  /* Release any allocated memory */

  if (args.allocated)
    {
      free(args.destpath);
    }

  free(fullpath);
  return OK;
}
#endif
#endif

/****************************************************************************
 * Name: cmd_wget
 ****************************************************************************/

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#ifndef CONFIG_NSH_DISABLE_WGET
int cmd_wget(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  char *localfile = NULL;
  char *allocfile = NULL;
  char *buffer    = NULL;
  char *fullpath  = NULL;
  char *url;
  const char *fmt;
  bool badarg = false;
  int option;
  int fd = -1;
  int ret;

  /* Get the wget options */

  while ((option = getopt(argc, argv, ":o:")) != ERROR)
    {
      switch (option)
        {
          case 'o':
            localfile = optarg;
            break;

          case ':':
            nsh_output(vtbl, g_fmtargrequired, argv[0]);
            badarg = true;
            break;

          case '?':
          default:
            nsh_output(vtbl, g_fmtarginvalid, argv[0]);
            badarg = true;
            break;
        }
    }

  /* If a bad argument was encountered, then return without processing the
   * command
   */

  if (badarg)
    {
      return ERROR;
    }

  /* There should be exactly on parameter left on the command-line */

  if (optind == argc-1)
    {
      url = argv[optind];
    }
  else if (optind >= argc)
    {
      fmt = g_fmttoomanyargs;
      goto errout;
    }
  else
    {
      fmt = g_fmtargrequired;
      goto errout;
    }

  /* Get the local file name */

  if (!localfile)
    {
      allocfile = strdup(url);
      localfile = basename(allocfile);
    }

  /* Get the full path to the local file */

  fullpath = nsh_getfullpath(vtbl, localfile);

  /* Open the local file for writing */

  fd = open(fullpath, O_WRONLY|O_CREAT|O_TRUNC, 0644);
  if (fd < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "open", NSH_ERRNO);
      ret = ERROR;
      goto exit;
    }

  /* Allocate an I/O buffer */

  buffer = malloc(512);
  if (!buffer)
    {
      fmt = g_fmtcmdoutofmemory;
      goto errout;
    }

  /* And perform the wget */

  ret = wget(url, buffer, 512, wget_callback, (FAR void *)fd);
  if (ret < 0)
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "wget", NSH_ERRNO);
      goto exit;
    }

  /* Free allocated resources */

exit:
  if (fd >= 0)
    {
      close(fd);
    }

  if (allocfile)
    {
      free(allocfile);
    }

  if (fullpath)
    {
      free(fullpath);
    }

  if (buffer)
    {
      free(buffer);
    }

  return ret;

errout:
  nsh_output(vtbl, fmt, argv[0]);
  ret = ERROR;
  goto exit;
}
#endif
#endif

#endif /* CONFIG_NET */
