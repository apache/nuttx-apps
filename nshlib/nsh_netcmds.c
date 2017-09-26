/****************************************************************************
 * apps/nshlib/nsh_netcmds.c
 *
 *   Copyright (C) 2007-2012, 2014-2015, 2017 Gregory Nutt. All rights reserved.
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

#include <nuttx/compiler.h>

#include <sys/stat.h>    /* Needed for open */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>       /* Needed for open */
#include <dirent.h>
#include <netdb.h>       /* Needed for gethostbyname */
#include <libgen.h>      /* Needed for basename */
#include <errno.h>
#include <debug.h>

#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT)
#  ifndef CONFIG_NSH_DISABLE_NSLOOKUP
#    include <netdb.h>
#  endif
#endif

#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include <nuttx/irq.h>
#include <nuttx/clock.h>
#include <nuttx/net/net.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/netstats.h>
#include <nuttx/net/ip.h>
#include <nuttx/net/icmp.h>
#include <nuttx/net/icmpv6.h>

#ifdef CONFIG_NET_6LOWPAN
#  include <nuttx/net/sixlowpan.h>
#  ifdef CONFIG_WIRELESS_PKTRADIO
#    include <nuttx/wireless/pktradio.h>
#  endif
#endif

#ifdef CONFIG_NETUTILS_NETLIB
#  include "netutils/netlib.h"
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#  include "netutils/netlib.h"
#  if !defined(CONFIG_NSH_DISABLE_GET) || !defined(CONFIG_NSH_DISABLE_PUT)
#    include "netutils/tftp.h"
#  endif
#endif

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#  ifndef CONFIG_NSH_DISABLE_WGET
#    include "netutils/webclient.h"
#  endif
#endif

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
#  include "netutils/dhcpc.h"
#endif

#include "nsh.h"
#include "nsh_console.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef HAVE_PING
#undef HAVE_PING6
#undef HAVE_HWADDR
#undef HAVE_EADDR
#undef HAVE_RADIOADDR

#if defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING) && \
   !defined(CONFIG_DISABLE_SIGNALS) && !defined(CONFIG_NSH_DISABLE_PING)
#  define HAVE_PING        1
#endif

#if defined(CONFIG_NET_ICMPv6) && defined(CONFIG_NET_ICMPv6_PING) && \
   !defined(CONFIG_DISABLE_SIGNALS) && !defined(CONFIG_NSH_DISABLE_PING6)
#  define HAVE_PING6       1
#endif

#if defined(CONFIG_NET_ETHERNET)
#  define HAVE_HWADDR      1
#elif defined(CONFIG_NET_6LOWPAN)
#  if defined(CONFIG_WIRELESS_IEEE802154)
#    define HAVE_HWADDR    1
#    define HAVE_EADDR     1
#  elif defined(CONFIG_WIRELESS_PKTRADIO)
#    define HAVE_HWADDR    1
#    define HAVE_RADIOADDR 1
#  endif
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

/* Describes the MAC address of the selected device */

#ifdef HAVE_HWADDR
#if defined(CONFIG_NET_ETHERNET)
typedef uint8_t mac_addr_t[IFHWADDRLEN];
#elif defined(HAVE_EADDR)
typedef uint8_t mac_addr_t[8];
#elif defined(HAVE_RADIOADDR)
typedef struct pktradio_addr_s mac_addr_t;
#endif
#endif

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

typedef int (*nsh_netdev_callback_t)(FAR struct nsh_vtbl_s *vtbl,
                                     FAR char *devname);

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
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ping_newid
 ****************************************************************************/

#if defined(HAVE_PING) || defined(HAVE_PING6)
static uint16_t ping_newid(void)
{
  irqstate_t save = enter_critical_section();
  uint16_t ret = ++g_pingid;
  leave_critical_section(save);
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

  if (optind == argc - 1)
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

#if defined(CONFIG_NET_STATISTICS) && defined(CONFIG_FS_PROCFS) && \
   !defined(CONFIG_FS_PROCFS_EXCLUDE_NET) && \
   !defined(CONFIG_NSH_DISABLE_IFCONFIG)
static inline void net_statistics(FAR struct nsh_vtbl_s *vtbl)
{
  (void)nsh_catfile(vtbl, "ifconfig", CONFIG_NSH_PROC_MOUNTPOINT "/net/stat");
}
#else
# define net_statistics(vtbl)
#endif

/****************************************************************************
 * Name: ifconfig_callback
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_IFUPDOWN) || !defined(CONFIG_NSH_DISABLE_IFCONFIG)
static int ifconfig_callback(FAR struct nsh_vtbl_s *vtbl, FAR char *devname)
{
  char buffer[IFNAMSIZ + 12];

  DEBUGASSERT(vtbl != NULL && devname != NULL);

  /* Construct the full path to the /proc/net entry for this device */

  snprintf(buffer, IFNAMSIZ + 12, CONFIG_NSH_PROC_MOUNTPOINT "/net/%s", devname);
  (void)nsh_catfile(vtbl, "ifconfig", buffer);

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
            if (!netlib_ipv4addrconv(optarg, (FAR unsigned char*)&args->ipaddr))
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

  if (optind == argc - 1)
    {
      args->srcpath = argv[optind];
    }

  /* optind == argc means that there is nothing left on the command-line */

  else if (optind >= argc)
    {
      fmt = g_fmtargrequired;
      goto errout;
    }

  /* optind < argc - 1 means that there are too many arguments on the
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
 * Name: nsh_gethostip
 *
 * Description:
 *   Call gethostbyname() to get the IP address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *   ipv4addr - The location to return the IPv4 address.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

#if defined(HAVE_PING) || defined(HAVE_PING6)
static int nsh_gethostip(FAR char *hostname, FAR union ip_addr_u *ipaddr,
                         int addrtype)
{
#ifdef CONFIG_LIBC_NETDB

  /* Netdb support is enabled */

  FAR struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL)
    {
      nerr("ERROR: gethostbyname failed: %d\n", h_errno);
      return -ENOENT;
    }

#if defined(HAVE_PING) && defined(HAVE_PING6)

  else if (he->h_addrtype != addrtype)
    {
      nerr("ERROR: gethostbyname returned an address of type: %d\n",
           he->h_addrtype);
      return -ENOEXEC;
    }
  else if (addrtype == AF_INET)
    {
      memcpy(&ipaddr->ipv4, he->h_addr, sizeof(in_addr_t));
    }
  else /* if (addrtype == AF_INET6) */
    {
      memcpy(ipaddr->ipv6, he->h_addr, sizeof(net_ipv6addr_t));
    }

#elif defined(HAVE_PING)

  else if (he->h_addrtype != AF_INET)
    {
      nerr("ERROR: gethostbyname returned an address of type: %d\n",
           he->h_addrtype);
      return -ENOEXEC;
    }
  else
    {
      memcpy(&ipaddr->ipv4, he->h_addr, sizeof(in_addr_t));
    }

#else /* if defined(HAVE_PING6) */

  else if (he->h_addrtype != AF_INET6)
    {
      nerr("ERROR: gethostbyname returned an address of type: %d\n",
           he->h_addrtype);
      return -ENOEXEC;
    }
  else
    {
      memcpy(ipaddr->ipv6, he->h_addr, sizeof(net_ipv6addr_t));
    }

#endif

  return OK;

#else /* CONFIG_LIBC_NETDB */

  /* No host name support */

  int ret;

#ifdef HAVE_PING
  /* Convert strings to numeric IPv4 address */

#ifdef HAVE_PING6
  if (addrtype == AF_INET)
#endif
    {
      ret = inet_pton(AF_INET, hostname, &ipaddr->ipv4);
    }
#endif

#ifdef HAVE_PING6
  /* Convert strings to numeric IPv6 address */

#ifdef HAVE_PING
  else
#endif
    {
      ret = inet_pton(AF_INET6, hostname, ipaddr->ipv6);
    }
#endif

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or a
   * valid IPv6 address string, or -1 with errno set to EAFNOSUPPORT if
   * the address family argument is unsupported.
   */

  return (ret > 0) ? OK : ERROR;

#endif /* CONFIG_LIBC_NETDB */
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
  (void)write((int)((intptr_t)arg), &((*buffer)[offset]), datend - offset);
}
#endif
#endif

/****************************************************************************
 * Name: nsh_foreach_netdev
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_IFUPDOWN) || !defined(CONFIG_NSH_DISABLE_IFCONFIG)
static int nsh_foreach_netdev(nsh_netdev_callback_t callback,
                              FAR struct nsh_vtbl_s *vtbl,
                              FAR char *cmd)
{
  FAR struct dirent *entry;
  FAR DIR *dir;
  int ret = OK;

  /* Open the /proc/net directory */

  dir = opendir(CONFIG_NSH_PROC_MOUNTPOINT "/net");
  if (dir == NULL)
    {
      nsh_output(vtbl, g_fmtcmdfailed, cmd, "opendir", NSH_ERRNO);
      return ERROR;
    }

  /* Look for device configuration "regular" files */

  while ((entry = readdir(dir)) != NULL)
    {
      /* Skip anything that is not a regular file and skip the file
       * /proc/dev/stat which does not correspond to a network driver.
       */

      if (entry->d_type == DTYPE_FILE &&
          strcmp(entry->d_name, "stat") != 0)
        {
          /* Performt he callback.  It returns any non-zero value, then
           * terminate the serach.
           */

          ret = callback(vtbl, entry->d_name);
          if (ret != OK)
            {
              break;
            }
        }
    }

  /* Close the directory */

  (void)closedir(dir);
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_addrconv
 ****************************************************************************/

#ifdef HAVE_HWADDR
static inline bool nsh_addrconv(FAR const char *hwstr, FAR mac_addr_t *macaddr)
{
  /* REVISIT: How will we handle Ethernet and SLIP networks together? */

#if defined(CONFIG_NET_ETHERNET)
  return !netlib_ethaddrconv(hwstr, *macaddr);
#elif defined(HAVE_EADDR)
  return !netlib_eaddrconv(hwstr, *macaddr);
#elif defined(HAVE_RADIOADDR)
  return !netlib_nodeaddrconv(hwstr, macaddr);
#else
  return -ENOSUPP;
#endif
}
#endif

/****************************************************************************
 * Name: nsh_sethwaddr
 ****************************************************************************/

#ifdef HAVE_HWADDR
static inline void nsh_sethwaddr(FAR const char *ifname, FAR mac_addr_t *macaddr)
{
#if defined(CONFIG_NET_ETHERNET)
  netlib_setmacaddr(ifname, *macaddr);
#elif defined(HAVE_EADDR)
  netlib_seteaddr(ifname, *macaddr);
#elif defined(HAVE_RADIOADDR)
  netlib_setnodeaddr(ifname, macaddr);
#endif
}
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
  FAR char *ifname = NULL;
  int ret;

  if (argc != 2)
    {
      nsh_output(vtbl, "Please select ifname:\n");
      return nsh_foreach_netdev(ifconfig_callback, vtbl, "ifup");
    }

  ifname = argv[1];
  ret  = netlib_ifup(ifname);
  nsh_output(vtbl, "ifup %s...%s\n", ifname, (ret == OK) ? "OK" : "Failed");
  return ret;
}
#endif

/****************************************************************************
 * Name: cmd_ifdown
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_IFUPDOWN
int cmd_ifdown(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR char *ifname = NULL;
  int ret;

  if (argc != 2)
    {
      nsh_output(vtbl, "Please select ifname:\n");
      return nsh_foreach_netdev(ifconfig_callback, vtbl, "ifdown");
    }

  ifname = argv[1];
  ret = netlib_ifdown(ifname);
  nsh_output(vtbl, "ifdown %s...%s\n", ifname, (ret == OK) ? "OK" : "Failed");
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
  FAR char *ifname = NULL;
  FAR char *hostip = NULL;
  FAR char *gwip = NULL;
  FAR char *mask = NULL;
  FAR char *tmp = NULL;
#ifdef HAVE_HWADDR
  FAR char *hw = NULL;
#endif
#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
  FAR char *dns = NULL;
#endif
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
  bool inet6 = false;
#endif
  bool missingarg = true;
  bool badarg = false;
#ifdef HAVE_HWADDR
  mac_addr_t macaddr;
#endif
#if defined(CONFIG_NSH_DHCPC)
  FAR void *handle;
#endif
  int ret;

  /* With one or no arguments, ifconfig simply shows the status of the
   * network device:
   *
   *   ifconfig
   *   ifconfig [interface]
   */

  if (argc <= 2)
    {
      ret = nsh_foreach_netdev(ifconfig_callback, vtbl, "ifconfig");
      if (ret < 0)
        {
          return ERROR;
        }

      net_statistics(vtbl);
      return OK;
    }

  /* If both the network interface name and an IP address are supplied as
   * arguments, then ifconfig will set the address of the Ethernet device:
   *
   *    ifconfig ifname [ip_address] [named options]
   */

  if (argc > 2)
    {
      for (i = 1; i < argc; i++)
        {
          if (i == 1)
            {
              ifname = argv[i];
              missingarg = false;
            }
          else
            {
              tmp = argv[i];

              if (!strcmp(tmp, "dr") || !strcmp(tmp, "gw") || !strcmp(tmp, "gateway"))
                {
                  if (argc - 1 >= i + 1)
                    {
                      gwip = argv[i + 1];
                      i++;
                    }
                  else
                    {
                      badarg = true;
                    }
                }
              else if (!strcmp(tmp, "netmask"))
                {
                  if (argc - 1 >= i + 1)
                    {
                      mask = argv[i + 1];
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

#ifdef HAVE_HWADDR
              /* REVISIT: How will we handle Ethernet and SLIP networks together? */

              else if (!strcmp(tmp, "hw"))
                {
                  if (argc - 1 >= i + 1)
                    {
                      hw = argv[i + 1];
                      i++;

                      badarg = nsh_addrconv(hw, &macaddr);
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
                  if (argc - 1 >= i + 1)
                    {
                      dns = argv[i + 1];
                      i++;
                    }
                  else
                    {
                      badarg = true;
                    }
                }
#endif
              else if (i == 2)
                {
                  hostip = tmp;
                }
              else
                {
                  badarg = true;
                }
            }
        }
    }

  if (missingarg)
    {
      nsh_output(vtbl, g_fmtargrequired, argv[0]);
      return ERROR;
    }

  if (badarg)
    {
      nsh_output(vtbl, g_fmtarginvalid, argv[0]);
      return ERROR;
    }

#ifdef HAVE_HWADDR
  /* Set Hardware Ethernet MAC address */
  /* REVISIT: How will we handle Ethernet and SLIP networks together? */

  if (hw != NULL)
    {
      ninfo("HW MAC: %s\n", hw);
      nsh_sethwaddr(ifname, &macaddr);
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
      if (hostip != NULL)
        {
#if defined(CONFIG_NSH_DHCPC)
          if (!strcmp(hostip, "dhcp"))
            {
              /* Set DHCP addr */

              ninfo("DHCPC Mode\n");
              gip = addr.s_addr = 0;
            }
          else
#endif
            {
              /* Set host IP address */

              ninfo("Host IP: %s\n", hostip);
              gip = addr.s_addr = inet_addr(hostip);
            }
        }

      netlib_set_ipv4addr(ifname, &addr);
    }
#endif /* CONFIG_NET_IPv4 */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
  if (inet6 != NULL)
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
          ninfo("Gateway: %s\n", gwip);
          gip = addr.s_addr = inet_addr(gwip);
        }
      else
        {
          if (gip)
            {
              ninfo("Gateway: default\n");
              gip  = NTOHL(gip);
              gip &= ~0x000000ff;
              gip |= 0x00000001;
              gip  = HTONL(gip);
            }

          addr.s_addr = gip;
        }

      netlib_set_dripv4addr(ifname, &addr);
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
          ninfo("Netmask: %s\n",mask);
          addr.s_addr = inet_addr(mask);
        }
      else
        {
          ninfo("Netmask: Default\n");
          addr.s_addr = inet_addr("255.255.255.0");
        }

      netlib_set_ipv4netmask(ifname, &addr);
    }
#endif /* CONFIG_NET_IPv4 */

  UNUSED(ifname); /* Not used in all configurations */

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
          ninfo("DNS: %s\n", dns);
          addr.s_addr = inet_addr(dns);
        }
      else
        {
          ninfo("DNS: Default\n");
          addr.s_addr = gip;
        }

      netlib_set_ipv4dnsaddr(&addr);
    }
#endif /* CONFIG_NET_IPv4 */
#endif /* CONFIG_NSH_DHCPC || CONFIG_NSH_DNS */

#if defined(CONFIG_NSH_DHCPC)
  /* Get the MAC address of the NIC */

  if (!gip)
    {
      netlib_getmacaddr("eth0", macaddr);

      /* Set up the DHCPC modules */

      handle = dhcpc_open("eth0", &macaddr, IFHWADDRLEN);

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
              netlib_set_ipv4dnsaddr(&ds.dnsaddr);
            }

          dhcpc_close(handle);
        }
    }
#endif

#if !defined(CONFIG_NET_IPv4) && !defined(CONFIG_NET_IPv6)
  UNUSED(hostip);
  UNUSED(mask);
  UNUSED(gwip);
  UNUSED(gip);
#endif

  return OK;
}
#endif


/****************************************************************************
 * Name: cmd_nslookup
 ****************************************************************************/

#if defined(CONFIG_LIBC_NETDB) && defined(CONFIG_NETDB_DNSCLIENT) && \
   !defined(CONFIG_NSH_DISABLE_NSLOOKUP)
int cmd_nslookup(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  FAR struct hostent *host;
  FAR const char *addrtype;
  char buffer[48];

  /* We should be guaranteed this by the command line parser */

  DEBUGASSERT(argc == 2);

  /* Get the matching address + any aliases */

  host = gethostbyname(argv[1]);
  if (!host)
    {
      /* REVISIT: gethostbyname() does not set errno, but h_errno */

       nsh_output(vtbl, g_fmtcmdfailed, argv[0], "gethostbyname", NSH_ERRNO);
       return ERROR;
    }

  /* Convert the address to a string */
  /* Handle IPv4 addresses */

  if (host->h_addrtype == AF_INET)
    {
      if (inet_ntop(AF_INET, host->h_addr, buffer, 48) == NULL)
        {
          nsh_output(vtbl, g_fmtcmdfailed, argv[0], "inet_ntop", NSH_ERRNO);
          return ERROR;
        }

      addrtype = "IPv4";
    }

  /* Handle IPv6 addresses */

  else /* if (host->h_addrtype == AF_INET6) */
    {
      DEBUGASSERT(host->h_addrtype == AF_INET6);

      if (inet_ntop(AF_INET6, host->h_addr, buffer, 48) == NULL)
        {
          nsh_output(vtbl, g_fmtcmdfailed, argv[0], "inet_ntop", NSH_ERRNO);
          return ERROR;
        }

      addrtype = "IPv6";
    }

  /* Print the host name / address mapping */

  nsh_output(vtbl, "Host: %s  %s Addr: %s\n", host->h_name, addrtype, buffer);

  /* Print any host name aliases */

  if (host->h_aliases != NULL && *host->h_aliases != NULL)
    {
      FAR char **alias;

      nsh_output(vtbl, "Aliases:");
      for (alias = host->h_aliases; *alias != NULL; alias++)
        {
          nsh_output(vtbl, " %s", *alias);
        }

      nsh_output(vtbl, "\n");
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: cmd_arp
 ****************************************************************************/

#if defined(CONFIG_NET_ARP) && !defined(CONFIG_NSH_DISABLE_ARP)
int cmd_arp(FAR struct nsh_vtbl_s *vtbl, int argc, char **argv)
{
  struct sockaddr_in inaddr;
  struct ether_addr mac;
  int ret;

  /* Forms:
   *
   * aap -a <ipaddr>
   * arp -d <ipdaddr>
   * arp -s <ipaddr> <hwaddr>
   */

  if (strcmp(argv[1], "-a") == 0)
    {
      if (argc != 3)
        {
          goto errout_toomany;
        }

      /* Show the corresponding hardware address */

      inaddr.sin_family      = AF_INET;
      inaddr.sin_port        = 0;
      inaddr.sin_addr.s_addr = inet_addr(argv[2]);

      ret = netlib_get_arpmapping(&inaddr, mac.ether_addr_octet);
      if (ret < 0)
        {
          goto errout_cmdfaild;
        }

      nsh_output(vtbl, "HWaddr: %s\n",  ether_ntoa(&mac));
    }
  else if (strcmp(argv[1], "-d") == 0)
    {
      if (argc != 3)
        {
          goto errout_toomany;
        }

      /* Delete the corresponding address mapping from the arp table */

      inaddr.sin_family      = AF_INET;
      inaddr.sin_port        = 0;
      inaddr.sin_addr.s_addr = inet_addr(argv[2]);

      ret = netlib_del_arpmapping(&inaddr);
      if (ret < 0)
        {
          goto errout_cmdfaild;
        }
    }
  else if (strcmp(argv[1], "-s") == 0)
    {
      if (argc != 4)
        {
          goto errout_missing;
        }

      /* Convert the MAC address string to a binary */

      if (!netlib_ethaddrconv(argv[3], mac.ether_addr_octet))
        {
          goto errout_invalid;
        }

      /* Add the address mapping to the arp table */

      inaddr.sin_family      = AF_INET;
      inaddr.sin_port        = 0;
      inaddr.sin_addr.s_addr = inet_addr(argv[2]);

      ret = netlib_set_arpmapping(&inaddr, mac.ether_addr_octet);
      if (ret < 0)
        {
          goto errout_cmdfaild;
        }
    }
  else
    {
      goto errout_invalid;
    }

  return OK;

/* Error exits */

errout_cmdfaild:
  if (ret == -ENOENT)
    {
      nsh_output(vtbl, g_fmtnosuch, argv[0], "ARP entry", argv[2]);
    }
  else
    {
      nsh_output(vtbl, g_fmtcmdfailed, argv[0], "ioctl", NSH_ERRNO);
    }

  return ERROR;

errout_missing:
  nsh_output(vtbl, g_fmttoomanyargs, argv[0]);
  return ERROR;

errout_toomany:
  nsh_output(vtbl, g_fmtargrequired, argv[0]);
  return ERROR;

errout_invalid:
  nsh_output(vtbl, g_fmtarginvalid, argv[0]);
  return ERROR;
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
  systime_t start;
  systime_t next;
  int32_t elapsed;
  uint32_t dsec = 10;
  uint32_t maxwait;
  uint16_t id;
  int count = 10;
  int seqno;
  int replies = 0;
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

  ret = nsh_gethostip(staddr, (FAR union ip_addr_u *)&ipaddr, AF_INET);
  if (ret < 0)
    {
      nsh_output(vtbl, "nsh: %s: unable to resolve hostname '%s'\n", argv[0], staddr);
      return ERROR;
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

          elapsed = (int32_t)TICK2MSEC(clock_systimer() - next);
          if (seqno < i)
            {
              elapsed += 100 * dsec * (i - seqno);
            }

          /* Report the receipt of the reply */

          nsh_output(vtbl, "%d bytes from %s: icmp_seq=%d time=%ld ms\n",
                     DEFAULT_PING_DATALEN, staddr, seqno, (long)elapsed);
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

      elapsed = (int32_t)TICK2DSEC(clock_systimer() - next);
      if (elapsed < dsec)
        {
          usleep(100000 * (dsec - elapsed));
        }
    }

  /* Get the total elapsed time */

  elapsed = (int32_t)TICK2MSEC(clock_systimer() - start);

  /* Calculate the percentage of lost packets */

  tmp = (100*(count - replies) + (count >> 1)) / count;

  nsh_output(vtbl, "%d packets transmitted, %d received, %d%% packet loss, time %ld ms\n",
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
  systime_t start;
  systime_t next;
  int32_t elapsed;
  uint32_t dsec = 10;
  uint32_t maxwait;
  uint16_t id;
  int count = 10;
  int seqno;
  int replies = 0;
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

  ret = nsh_gethostip(staddr, (FAR union ip_addr_u *)&ipaddr, AF_INET6);
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

          elapsed = (int32_t)TICK2MSEC(clock_systimer() - next);
          if (seqno < i)
            {
              elapsed += 100 * dsec * (i - seqno);
            }

          /* Report the receipt of the reply */

          nsh_output(vtbl, "%d bytes from %s: icmp_seq=%d time=%ld ms\n",
                     DEFAULT_PING_DATALEN, staddr, seqno, (long)elapsed);
          replies++;
        }

      /* Wait for the remainder of the interval.  If the last seqno<i,
       * then this is a bad idea... we will probably lose the response
       * to the current request!
       */

      elapsed = (int32_t)TICK2DSEC(clock_systimer() - next);
      if (elapsed < dsec)
        {
          usleep(100000 * (dsec - elapsed));
        }
    }

  /* Get the total elapsed time */

  elapsed = (int32_t)TICK2MSEC(clock_systimer() - start);

  /* Calculate the percentage of lost packets */

  tmp = (100*(count - replies) + (count >> 1)) / count;

  nsh_output(vtbl, "%d packets transmitted, %d received, %d%% packet loss, time %ld ms\n",
             count, replies, tmp, (long)elapsed);
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

  if (optind == argc - 1)
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

  ret = wget(url, buffer, 512, wget_callback, (FAR void *)((intptr_t)fd));
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
