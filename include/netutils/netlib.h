/****************************************************************************
 *  apps/include/netutils/netlib.h
 * Various non-standard APIs to support netutils.  All non-standard and
 * intended only for internal use.
 *
 *   Copyright (C) 2007, 2009, 2011, 2015, 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Some of these APIs derive from uIP.  uIP also has a BSD style license:
 *
 *   Author: Adam Dunkels <adam@sics.se>
 *   Copyright (c) 2002, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_NETUTILS_NETLIB_H
#define __APPS_INCLUDE_NETUTILS_NETLIB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/socket.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include <net/if.h>
#include <netinet/in.h>
#include <nuttx/net/netconfig.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#undef HAVE_ROUTE_PROCFS
#if !defined(CONFIG_DISABLE_MOUNTPOINT) && defined(CONFIG_NET_ROUTE) && \
    defined(CONFIG_FS_PROCFS) && !defined(CONFIG_FS_PROCFS_EXCLUDE_ROUTE)
#  define HAVE_ROUTE_PROCFS
#endif

#ifdef HAVE_ROUTE_PROCFS
#  ifndef CONFIG_NETLIB_PROCFS_MOUNTPT
#    define CONFIG_NETLIB_PROCFS_MOUNTPT "/proc"
#  endif

#  define IPv4_ROUTE_PATH CONFIG_NETLIB_PROCFS_MOUNTPT "/net/route/ipv4"
#  define IPv6_ROUTE_PATH CONFIG_NETLIB_PROCFS_MOUNTPT "/net/route/ipv6"
#endif

/* Using the following definitions, the following socket() arguments should
 * provide a valid socket in all configurations:
 *
 *   ret = socket(NETLIB_SOCK_FAMILY, NETLIB_SOCK_TYPE, NETLIB_SOCK_PROTOCOL);
 */

/* The address family that we used to create the socket really does not
 * matter.  It should, however, be valid in the current configuration.
 */

#if defined(CONFIG_NET_IPv4)
#  define NETLIB_SOCK_FAMILY  AF_INET
#elif defined(CONFIG_NET_IPv6)
#  define NETLIB_SOCK_FAMILY  AF_INET6
#elif defined(CONFIG_NET_LOCAL)
#  define NETLIB_SOCK_FAMILY  AF_LOCAL
#elif defined(CONFIG_NET_PKT)
#  define NETLIB_SOCK_FAMILY  AF_PACKET
#elif defined(CONFIG_NET_IEEE802154)
#  define NETLIB_SOCK_FAMILY  AF_IEEE802154
#elif defined(CONFIG_WIRELESS_PKTRADIO)
#  define NETLIB_SOCK_FAMILY  AF_PKTRADIO
#elif defined(CONFIG_NET_BLUETOOTH)
#  define NETLIB_SOCK_FAMILY  AF_BLUETOOTH
#elif defined(CONFIG_NET_USRSOCK)
#  define NETLIB_SOCK_FAMILY  AF_INET
#else
#  define NETLIB_SOCK_FAMILY  AF_UNSPEC
#endif

/* Socket protocol of zero normally works */

#define NETLIB_SOCK_PROTOCOL  0

/* SOCK_DGRAM is the preferred socket type to use when we just want a
 * socket for performing driver ioctls.  However, we can't use SOCK_DRAM
 * if UDP is disabled.
 *
 * Pick a socket type (and perhaps protocol) compatible with the currently
 * selected address family.
 */

#if NETLIB_SOCK_FAMILY == AF_INET
#  if defined(CONFIG_NET_UDP)
#    define NETLIB_SOCK_TYPE SOCK_DGRAM
#  elif defined(CONFIG_NET_TCP)
#   define NETLIB_SOCK_TYPE SOCK_STREAM
#  elif defined(CONFIG_NET_ICMP_SOCKET)
#   define NETLIB_SOCK_TYPE SOCK_DGRAM
#   undef NETLIB_SOCK_PROTOCOL
#   define NETLIB_SOCK_PROTOCOL IPPROTO_ICMP
#  endif
#elif NETLIB_SOCK_FAMILY == AF_INET6
#  if defined(CONFIG_NET_UDP)
#    define NETLIB_SOCK_TYPE SOCK_DGRAM
#  elif defined(CONFIG_NET_TCP)
#   define NETLIB_SOCK_TYPE SOCK_STREAM
#  elif defined(CONFIG_NET_ICMPv6_SOCKET)
#   define NETLIB_SOCK_TYPE SOCK_DGRAM
#   undef NETLIB_SOCK_PROTOCOL
#   define NETLIB_SOCK_PROTOCOL IPPROTO_ICMP6
#  endif
#elif NETLIB_SOCK_FAMILY == AF_LOCAL
#  if defined(CONFIG_NET_LOCAL_DGRAM)
#    define NETLIB_SOCK_TYPE SOCK_DGRAM
#  elif defined(CONFIG_NET_LOCAL_STREAM)
#     define NETLIB_SOCK_TYPE SOCK_STREAM
#  endif
#elif NETLIB_SOCK_FAMILY == AF_PACKET
#  define NETLIB_SOCK_TYPE SOCK_RAW
#elif NETLIB_SOCK_FAMILY == AF_IEEE802154
#  define NETLIB_SOCK_TYPE SOCK_DGRAM
#elif NETLIB_SOCK_FAMILY == AF_BLUETOOTH
#  define NETLIB_SOCK_TYPE SOCK_RAW
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#ifdef HAVE_ROUTE_PROCFS
#ifdef CONFIG_NET_IPv4
/* Describes one entry from the IPv4 routing table.  All addresses are in
 * host byte order!
 */

struct netlib_ipv4_route_s
{
  in_addr_t prefix;               /* Routing prefix */
  in_addr_t netmask;              /* Routing netmask */
  in_addr_t router;               /* Router IPv4 address */
};
#endif

#ifdef CONFIG_NET_IPv6
/* Describes one entry from the IPv6 routing table.  All addresses are in
 * host byte order!
 */

struct netlib_ipv6_route_s
{
  uint16_t prefix[8];             /* Routing prefix */
  uint16_t netmask[8];            /* Routing netmask */
  uint16_t router[8];             /* Router IPv6 address */
};
#endif
#endif /* HAVE_ROUTE_PROCFS */

#ifdef CONFIG_NETLINK_ROUTE
/* Describes one device returned by netlib_get_devices() */

struct netlib_device_s
{
#ifdef CONFIG_NETDEV_IFINDEX
  uint8_t ifindex;                /* Interface index */
#endif
  char ifname[IFNAMSIZ];          /* Interface name */
};
#endif /* CONFIG_NETLINK_ROUTE*/

#ifdef CONFIG_NETUTILS_NETLIB_GENERICURLPARSER
struct url_s
{
  FAR char *scheme;
  int       schemelen;
  FAR char *user;
  int       userlen;
  FAR char *password;
  int       passwordlen;
  FAR char *host;
  int       hostlen;
  int       port;
  FAR char *path;
  int       pathlen;
  FAR char *parameters;
  int       parameterslen;
  FAR char *bookmark;
  int       bookmarklen;
};
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NETLINK_ROUTE
/* Return a list of all devices */

ssize_t netlib_get_devices(FAR struct netlib_device_s *devlist,
                           unsigned int nentries, sa_family_t family);
#endif

/* Convert a textual representation of an IP address to a numerical representation.
 *
 * This function takes a textual representation of an IP address in
 * the form a.b.c.d and converts it into a 4-byte array that can be
 * used by other uIP functions.
 *
 * addrstr A pointer to a string containing the IP address in
 * textual form.
 *
 * addr A pointer to a 4-byte array that will be filled in with
 * the numerical representation of the address.
 *
 * Return: 0 If the IP address could not be parsed.
 * Return: Non-zero If the IP address was parsed.
 */

bool netlib_ipv4addrconv(FAR const char *addrstr, FAR uint8_t *addr);
bool netlib_ethaddrconv(FAR const char *hwstr, FAR uint8_t *hw);

#ifdef CONFIG_NET_ETHERNET
/* Get and set IP/MAC addresses (Ethernet L2 only) */

int netlib_setmacaddr(FAR const char *ifname, FAR const uint8_t *macaddr);
int netlib_getmacaddr(FAR const char *ifname, FAR uint8_t *macaddr);
#endif

#ifdef CONFIG_WIRELESS_IEEE802154
/* IEEE 802.15.4 MAC IOCTL commands. */

int netlib_seteaddr(FAR const char *ifname, FAR const uint8_t *eaddr);
int netlib_getpanid(FAR const char *ifname, FAR uint8_t *panid);
bool netlib_saddrconv(FAR const char *hwstr, FAR uint8_t *hw);
bool netlib_eaddrconv(FAR const char *hwstr, FAR uint8_t *hw);
#endif

#ifdef CONFIG_WIRELESS_PKTRADIO
/* IEEE 802.15.4 MAC IOCTL commands. */

struct pktradio_properties_s; /* Forward reference */
struct pktradio_addr_s;       /* Forward reference */

int netlib_getproperties(FAR const char *ifname,
                         FAR struct pktradio_properties_s *properties);
int netlib_setnodeaddr(FAR const char *ifname,
                       FAR const struct pktradio_addr_s *nodeaddr);
int netlib_getnodnodeaddr(FAR const char *ifname,
                          FAR struct pktradio_addr_s *nodeaddr);
bool netlib_nodeaddrconv(FAR const char *addrstr,
                         FAR struct pktradio_addr_s *nodeaddr);
#endif

/* IP address support */

#ifdef CONFIG_NET_IPv4
int netlib_get_ipv4addr(FAR const char *ifname, FAR struct in_addr *addr);
int netlib_set_ipv4addr(FAR const char *ifname, FAR const struct in_addr *addr);
int netlib_set_dripv4addr(FAR const char *ifname, FAR const struct in_addr *addr);
int netlib_get_dripv4addr(FAR const char *ifname, FAR struct in_addr *addr);
int netlib_set_ipv4netmask(FAR const char *ifname, FAR const struct in_addr *addr);
int netlib_get_ipv4netmask(FAR const char *ifname, FAR struct in_addr *addr);
int netlib_ipv4adaptor(in_addr_t destipaddr, FAR in_addr_t *srcipaddr);
#endif

#ifdef CONFIG_NET_IPv6
int netlib_get_ipv6addr(FAR const char *ifname, FAR struct in6_addr *addr);
int netlib_set_ipv6addr(FAR const char *ifname, FAR const struct in6_addr *addr);
int netlib_set_dripv6addr(FAR const char *ifname, FAR const struct in6_addr *addr);
int netlib_set_ipv6netmask(FAR const char *ifname, FAR const struct in6_addr *addr);
int netlib_ipv6adaptor(FAR const struct in6_addr *destipaddr,
                       FAR struct in6_addr *srcipaddr);

uint8_t netlib_ipv6netmask2prefix(FAR const uint16_t *mask);
void netlib_prefix2ipv6netmask(uint8_t preflen, FAR struct in6_addr *netmask);
#ifdef CONFIG_NETLINK_ROUTE
struct neighbor_entry_s;
ssize_t netlib_get_nbtable(FAR struct neighbor_entry_s *nbtab,
                           unsigned int nentries);
#endif
#endif /* CONFIG_NET_IPv6 */

#ifdef CONFIG_NETDEV_WIRELESS_IOCTL
int netlib_getessid(FAR const char *ifname, FAR char *essid, size_t idlen);
int netlib_setessid(FAR const char *ifname, FAR const char *essid);
#endif

#ifdef CONFIG_NET_ARP
/* ARP Table Support */

int netlib_del_arpmapping(FAR const struct sockaddr_in *inaddr);
int netlib_get_arpmapping(FAR const struct sockaddr_in *inaddr,
                          FAR uint8_t *macaddr);
int netlib_set_arpmapping(FAR const struct sockaddr_in *inaddr,
                          FAR const uint8_t *macaddr);
#ifdef CONFIG_NETLINK_ROUTE
struct arp_entry_s;
ssize_t netlib_get_arptable(FAR struct arp_entry_s *arptab,
                            unsigned int nentries);
#endif
#endif

#ifdef HAVE_ROUTE_PROCFS
#  ifdef CONFIG_NET_IPv4
#    define netlib_open_ipv4route()        fopen(IPv4_ROUTE_PATH, "r")
#    define netlib_close_ipv4route(stream) fclose(stream)
ssize_t netlib_read_ipv4route(FILE *stream,
                              FAR struct netlib_ipv4_route_s *route);
int netlib_ipv4router(FAR const struct in_addr *destipaddr,
                      FAR struct in_addr *router);
#  endif
#  ifdef CONFIG_NET_IPv6
#    define netlib_open_ipv6route()        fopen(IPv6_ROUTE_PATH, "r")
#    define netlib_close_ipv6route(stream) fclose(stream)
ssize_t netlib_read_ipv6route(FILE *stream,
                              FAR struct netlib_ipv6_route_s *route);
int netlib_ipv6router(FAR const struct in6_addr *destipaddr,
                      FAR struct in6_addr *router);
#  endif
#endif

#if defined(CONFIG_NETLINK_ROUTE) && defined(CONFIG_NET_ROUTE)
struct rtentry;  /* Forward reference */
ssize_t netlib_get_route(FAR struct rtentry *rtelist,
                         unsigned int nentries, sa_family_t family);
#endif

#ifdef CONFIG_NET_ICMPv6_AUTOCONF
/* ICMPv6 Autoconfiguration */

int netlib_icmpv6_autoconfiguration(FAR const char *ifname);
#endif

/* HTTP support */

int  netlib_parsehttpurl(FAR const char *url, uint16_t *port,
                      FAR char *hostname, int hostlen,
                      FAR char *filename, int namelen);

#ifdef CONFIG_NETUTILS_NETLIB_GENERICURLPARSER
int netlib_parseurl(FAR const char *str, FAR struct url_s *url);
#endif

/* Generic server logic */

int netlib_listenon(uint16_t portno);
void netlib_server(uint16_t portno, pthread_startroutine_t handler,
                int stacksize);

int netlib_getifstatus(FAR const char *ifname, FAR uint8_t *flags);
int netlib_ifup(FAR const char *ifname);
int netlib_ifdown(FAR const char *ifname);

/* DNS server addressing */

#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NETDB_DNSCLIENT)
int netlib_set_ipv4dnsaddr(FAR const struct in_addr *inaddr);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_NETLIB_H */
