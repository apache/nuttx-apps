/****************************************************************************
 * apps/netutils/netinit/netinit.c
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

 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* Is network initialization debug forced on? */

#ifdef CONFIG_NETINIT_DEBUG
#  undef  CONFIG_DEBUG_INFO
#  define CONFIG_DEBUG_INFO 1
#  undef  CONFIG_DEBUG_NET
#  define CONFIG_DEBUG_NET 1
#endif

#include <arpa/inet.h>
#include <debug.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/net/mii.h>
#include <sys/boardctl.h>

#include "netutils/netlib.h"

#ifdef CONFIG_NET_6LOWPAN
#  include <nuttx/net/sixlowpan.h>
#endif

#ifdef CONFIG_NET_IEEE802154
#  include <nuttx/net/ieee802154.h>
#endif

#ifdef CONFIG_NETUTILS_NTPCLIENT
#  include "netutils/ntpclient.h"
#endif

#if defined(CONFIG_FSUTILS_IPCFG)
#  include "fsutils/ipcfg.h"
#endif

#include "netutils/netinit.h"

#ifdef CONFIG_NETUTILS_NETINIT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Pick one and at most one supported link layer so that all decisions are
 * made consistently.
 *
 * NOTE: Ethernet should always be selected with IEEE 802.11
 */

#if defined(CONFIG_NET_ETHERNET)
#  undef CONFIG_NET_6LOWPAN
#  undef CONFIG_NET_SLIP
#  undef CONFIG_NET_TUN
#  undef CONFIG_NET_LOCAL
#  undef CONFIG_NET_USRSOCK
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_6LOWPAN)
#  undef CONFIG_NET_SLIP
#  undef CONFIG_NET_TUN
#  undef CONFIG_NET_LOCAL
#  undef CONFIG_NET_USRSOCK
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_SLIP)
#  undef CONFIG_NET_TUN
#  undef CONFIG_NET_LOCAL
#  undef CONFIG_NET_USRSOCK
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_TUN)
#  undef CONFIG_NET_LOCAL
#  undef CONFIG_NET_USRSOCK
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_LOCAL)
#  undef CONFIG_NET_USRSOCK
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_USRSOCK)
#  undef CONFIG_NET_IEEE802154
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_IEEE802154)
#  undef CONFIG_NET_CAN
#  undef CONFIG_NET_LOOPBACK
#elif defined(CONFIG_NET_CAN)
#  undef CONFIG_NET_LOOPBACK
#endif

/* Only Ethernet and 6LoWPAN have MAC layer addresses */

#undef HAVE_MAC
#if defined(CONFIG_NET_ETHERNET) || defined(CONFIG_NET_6LOWPAN)
#  define HAVE_MAC 1
#endif

/* Currently there is only logic in 6LoWPAN configurations to
 * set the IEEE 802.15.4 addresses.
 */

#undef HAVE_EADDR

#if defined(CONFIG_NET_6LOWPAN)
#  if defined(CONFIG_WIRELESS_IEEE802154)
#    define HAVE_EADDR 1
#  elif defined(CONFIG_WIRELESS_PKTRADIO)
#    warning Missing logic
#  endif
#endif

/* Provide a default DNS address */

#if defined(CONFIG_NETINIT_DRIPADDR) && !defined(CONFIG_NETINIT_DNSIPADDR)
#  define CONFIG_NETINIT_DNSIPADDR CONFIG_NETINIT_DRIPADDR
#endif

/* Select the single network device name supported this this network
 * initialization logic.  If multiple interfaces are present with different
 * link types, the order of definition in the following conditional
 * logic will select the one interface that will be used (which might
 * not be the one that you want).
 */

#undef NETINIT_HAVE_NETDEV
#if defined(CONFIG_DRIVERS_IEEE80211) /* Usually also has CONFIG_NET_ETHERNET */
#  define NET_DEVNAME "wlan0"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_ETHERNET)
#  define NET_DEVNAME "eth0"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_6LOWPAN) || defined(CONFIG_NET_IEEE802154)
#  define NET_DEVNAME "wpan0"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_BLUETOOTH)
#  define NET_DEVNAME "bnep0"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_SLIP)
#  define NET_DEVNAME "sl0"
#  ifndef CONFIG_NETINIT_NOMAC
#    error "CONFIG_NETINIT_NOMAC must be defined for SLIP"
#  endif
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_TUN)
#  define NET_DEVNAME "tun0"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_LOOPBACK)
#  define NET_DEVNAME "lo"
#  define NETINIT_HAVE_NETDEV
#elif defined(CONFIG_NET_CAN)
#  define NET_DEVNAME "can0"
#  define NETINIT_HAVE_NETDEV
#endif

/* If we have no network device (perhaps only USRSOCK, local loopback, or
 * Netlink sockets), then we cannot support the network monitor.
 */

#ifndef NETINIT_HAVE_NETDEV
#  undef CONFIG_NETINIT_MONITOR
#endif

/* While the network is up, the network monitor really does nothing.  It
 * will wait for a very long time while waiting, it can be awakened by a
 * signal indicating a change in network status.
 */

#ifdef CONFIG_SYSTEM_TIME64
#  define LONG_TIME_SEC    (60*60)   /* One hour in seconds */
#else
#  define LONG_TIME_SEC    (5*60)    /* Five minutes in seconds */
#endif

#define SHORT_TIME_SEC     (2)       /* 2 seconds */

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NETINIT_MONITOR
static sem_t g_notify_sem;
#endif

#ifdef CONFIG_NETUTILS_DHCPC
bool g_use_dhcpc;
#endif

#if defined(CONFIG_NET_IPv6) && !defined(CONFIG_NET_ICMPv6_AUTOCONF) && \
   !defined(CONFIG_NET_6LOWPAN)
/* Host IPv6 address */

static const uint16_t g_ipv6_hostaddr[8] =
{
  HTONS(CONFIG_NETINIT_IPv6ADDR_1),
  HTONS(CONFIG_NETINIT_IPv6ADDR_2),
  HTONS(CONFIG_NETINIT_IPv6ADDR_3),
  HTONS(CONFIG_NETINIT_IPv6ADDR_4),
  HTONS(CONFIG_NETINIT_IPv6ADDR_5),
  HTONS(CONFIG_NETINIT_IPv6ADDR_6),
  HTONS(CONFIG_NETINIT_IPv6ADDR_7),
  HTONS(CONFIG_NETINIT_IPv6ADDR_8),
};

/* Default routine IPv6 address */

static const uint16_t g_ipv6_draddr[8] =
{
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_1),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_2),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_3),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_4),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_5),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_6),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_7),
  HTONS(CONFIG_NETINIT_DRIPv6ADDR_8),
};

/* IPv6 netmask */

static const uint16_t g_ipv6_netmask[8] =
{
  HTONS(CONFIG_NETINIT_IPv6NETMASK_1),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_2),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_3),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_4),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_5),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_6),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_7),
  HTONS(CONFIG_NETINIT_IPv6NETMASK_8),
};
#endif /* CONFIG_NET_IPv6 && !CONFIG_NET_ICMPv6_AUTOCONF && !CONFIG_NET_6LOWPAN */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netinit_set_macaddr
 *
 * Description:
 *   Set the hardware MAC address if the hardware is not capable of doing
 *   that for itself.
 *
 ****************************************************************************/

#if defined(NETINIT_HAVE_NETDEV) && defined(CONFIG_NETINIT_NOMAC) && \
    defined(HAVE_MAC)
static void netinit_set_macaddr(void)
{
#if defined(CONFIG_NETINIT_UIDMAC)
  uint8_t uid[CONFIG_BOARDCTL_UNIQUEID_SIZE];
#elif defined(CONFIG_NET_ETHERNET)
  uint8_t mac[IFHWADDRLEN];
#elif defined(HAVE_EADDR)
  uint8_t eaddr[8];
#endif

  /* Many embedded network interfaces must have a software assigned MAC */

#if defined(CONFIG_NETINIT_UIDMAC)
  boardctl(BOARDIOC_UNIQUEID, (uintptr_t)&uid);
  uid[0] = (uid[0] & 0b11110000) | 2; /* Locally Administered MAC */
  netlib_setmacaddr(NET_DEVNAME, uid);

#elif defined(CONFIG_NET_ETHERNET)
  /* Use the configured, fixed MAC address */

  mac[0] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 1)) & 0xff;
  mac[1] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 0)) & 0xff;

  mac[2] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 0)) & 0xff;

  /* Set the MAC address */

  netlib_setmacaddr(NET_DEVNAME, mac);

#elif defined(HAVE_EADDR)
  /* Use the configured, fixed extended address */

  eaddr[0] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 3)) & 0xff;
  eaddr[1] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 2)) & 0xff;
  eaddr[2] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 1)) & 0xff;
  eaddr[3] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 0)) & 0xff;

  eaddr[4] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 3)) & 0xff;
  eaddr[5] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 2)) & 0xff;
  eaddr[6] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 1)) & 0xff;
  eaddr[7] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 0)) & 0xff;

  /* Set the 6LoWPAN extended address */

  netlib_seteaddr(NET_DEVNAME, eaddr);
#endif /* CONFIG_NET_ETHERNET or HAVE_EADDR */
}
#else
#  define netinit_set_macaddr()
#endif

#if defined(CONFIG_NETINIT_THREAD) && CONFIG_NETINIT_RETRY_MOUNTPATH > 0
static inline void netinit_checkpath(void)
{
  int retries = CONFIG_NETINIT_RETRY_MOUNTPATH;
  while (retries > 0)
    {
      DIR * dir = opendir(CONFIG_IPCFG_PATH);
      if (dir)
        {
          /* Directory exists. */

          closedir(dir);
          break;
        }
      else
        {
        usleep(100000);
        }

      retries--;
    }
}
#endif

/****************************************************************************
 * Name: netinit_set_ipv4addrs
 *
 * Description:
 *   Setup IPv4 addresses.
 *
 ****************************************************************************/

#if defined(NETINIT_HAVE_NETDEV) && !defined(CONFIG_NET_6LOWPAN) && ! \
    defined(CONFIG_NET_IEEE802154) && defined(CONFIG_NET_IPv4)
static inline void netinit_set_ipv4addrs(void)
{
  struct in_addr addr;
#ifdef CONFIG_FSUTILS_IPCFG
  struct ipv4cfg_s ipv4cfg;
  int ret;

  /* Attempt to obtain IPv4 address configuration from the IP configuration
   * file.
   */

#if defined(CONFIG_NETINIT_THREAD) && CONFIG_NETINIT_RETRY_MOUNTPATH > 0
  netinit_checkpath();
#endif

  ret = ipcfg_read(NET_DEVNAME, (FAR struct ipcfg_s *)&ipv4cfg, AF_INET);
#ifdef CONFIG_NETUTILS_DHCPC
  if (ret >= 0 && ipv4cfg.proto != IPv4PROTO_NONE)
#else
  if (ret >= 0 && IPCFG_HAVE_STATIC(ipv4cfg.proto))
#endif
    {
      /* Check if we are using DHCPC */

#ifdef CONFIG_NETUTILS_DHCPC
      if (IPCFG_USE_DHCP(ipv4cfg.proto))
        {
          g_use_dhcpc = true;
          addr.s_addr = 0;
        }
      else
#endif
        {
          /* We are not using DHCPC.  We need an IP address */

#ifdef CONFIG_NETINIT_IPADDR
          /* Check if we have a static IP address in the configuration file */

          if (IPCFG_HAVE_STATIC(ipv4cfg.proto))
            {
              addr.s_addr = ipv4cfg.ipaddr;
            }
          else
            {
              /* This is not a good option, but in this case what else can
               * we do?
               */

              addr.s_addr = HTONL(CONFIG_NETINIT_IPADDR);
            }
#else
          /* Use whatever was provided in the file (might be zero) */

          addr.s_addr = ipv4cfg.ipaddr;
#endif
        }

      netlib_set_ipv4addr(NET_DEVNAME, &addr);

      /* Set up the remaining addresses */

      if (IPCFG_HAVE_STATIC(ipv4cfg.proto))
        {
          /* Set up the default router address */

          addr.s_addr = ipv4cfg.router;
          netlib_set_dripv4addr(NET_DEVNAME, &addr);

          /* Setup the subnet mask */

          addr.s_addr = ipv4cfg.netmask;
          netlib_set_ipv4netmask(NET_DEVNAME, &addr);
        }

#ifdef CONFIG_NETUTILS_DHCPC
      /* No static addresses?  That is fine if we are have addresses
       * provided by the configuration, or if we are using DHCP.
       */

      else if (g_use_dhcpc)
        {
          /* Set up the default router address and sub-net mask */

          addr.s_addr = 0;
          netlib_set_dripv4addr(NET_DEVNAME, &addr);
          netlib_set_ipv4netmask(NET_DEVNAME, &addr);
        }
#endif
      else
        {
          /* Otherwise, set up the configured default router address */

          addr.s_addr = HTONL(CONFIG_NETINIT_DRIPADDR);
          netlib_set_dripv4addr(NET_DEVNAME, &addr);

          /* Setup the subnet mask */

          addr.s_addr = HTONL(CONFIG_NETINIT_NETMASK);
          netlib_set_ipv4netmask(NET_DEVNAME, &addr);
        }

#ifdef CONFIG_NETINIT_DNS
      /* Set up the DNS address.  Was one provided in the configuration? */

      if (ipv4cfg.dnsaddr == 0)
        {
          /* No, use the configured default */

          addr.s_addr = HTONL(CONFIG_NETINIT_DNSIPADDR);
        }
      else
        {
          addr.s_addr = ipv4cfg.dnsaddr;
        }

      netlib_set_ipv4dnsaddr(&addr);
#endif
    }
  else
#endif
    {
      /* Set up our host address */

#ifdef CONFIG_NETINIT_DHCPC
      g_use_dhcpc = true;
      addr.s_addr = 0;
#else
      addr.s_addr = HTONL(CONFIG_NETINIT_IPADDR);
#endif
      netlib_set_ipv4addr(NET_DEVNAME, &addr);

      /* Set up the default router address */

      addr.s_addr = HTONL(CONFIG_NETINIT_DRIPADDR);
      netlib_set_dripv4addr(NET_DEVNAME, &addr);

      /* Setup the subnet mask */

      addr.s_addr = HTONL(CONFIG_NETINIT_NETMASK);
      netlib_set_ipv4netmask(NET_DEVNAME, &addr);

#ifdef CONFIG_NETINIT_DNS
      addr.s_addr = HTONL(CONFIG_NETINIT_DNSIPADDR);
      netlib_set_ipv4dnsaddr(&addr);
#endif
    }
}
#endif

/****************************************************************************
 * Name: netinit_set_ipv6addrs
 *
 * Description:
 *   Setup IPv6 addresses.
 *
 ****************************************************************************/

#if defined(NETINIT_HAVE_NETDEV) && !defined(CONFIG_NET_6LOWPAN) && ! \
    defined(CONFIG_NET_IEEE802154) && defined(CONFIG_NET_IPv6)
static inline void netinit_set_ipv6addrs(void)
{
#ifndef CONFIG_NET_ICMPv6_AUTOCONF
#ifdef CONFIG_FSUTILS_IPCFG
  struct ipv6cfg_s ipv6cfg;
  int ret;
#endif

#ifdef CONFIG_FSUTILS_IPCFG
  /* Attempt to obtain IPv6 address configuration from the IP configuration
   * file.
   */

#if defined(CONFIG_NETINIT_THREAD) && CONFIG_NETINIT_RETRY_MOUNTPATH > 0
  netinit_checkpath();
#endif

  ret = ipcfg_read(NET_DEVNAME, (FAR struct ipcfg_s *)&ipv6cfg, AF_INET6);
  if (ret >= 0 && IPCFG_HAVE_STATIC(ipv6cfg.proto))
    {
      /* Set up our fixed host address */

      netlib_set_ipv6addr(NET_DEVNAME, &ipv6cfg.ipaddr);

      /* Set up the default router address */

      netlib_set_dripv6addr(NET_DEVNAME, &ipv6cfg.router);

      /* Setup the subnet mask */

      netlib_set_ipv6netmask(NET_DEVNAME, &ipv6cfg.netmask);
    }
  else
#endif
    {
      /* Set up our fixed host address */

      netlib_set_ipv6addr(NET_DEVNAME,
                          (FAR const struct in6_addr *)g_ipv6_hostaddr);

      /* Set up the default router address */

      netlib_set_dripv6addr(NET_DEVNAME,
                            (FAR const struct in6_addr *)g_ipv6_draddr);

      /* Setup the subnet mask */

      netlib_set_ipv6netmask(NET_DEVNAME,
                            (FAR const struct in6_addr *)g_ipv6_netmask);
    }
#endif /* CONFIG_NET_ICMPv6_AUTOCONF */
}
#endif

/****************************************************************************
 * Name: netinit_set_ipaddrs
 *
 * Description:
 *   Setup IP addresses.
 *
 *   For 6LoWPAN, the IP address derives from the MAC address.  Setting it
 *   to any user provided value is asking for trouble.
 *
 ****************************************************************************/

#if defined(NETINIT_HAVE_NETDEV) && !defined(CONFIG_NET_6LOWPAN) && ! \
    defined(CONFIG_NET_IEEE802154)
static void netinit_set_ipaddrs(void)
{
#ifdef CONFIG_NET_IPv4
  netinit_set_ipv4addrs();
#endif

#ifdef CONFIG_NET_IPv6
  netinit_set_ipv6addrs();
#endif
}
#else
#  define netinit_set_ipaddrs()
#endif

/****************************************************************************
 * Name: netinit_net_bringup()
 *
 * Description:
 *   Bring up the configured network
 *
 ****************************************************************************/

#if defined(NETINIT_HAVE_NETDEV) && !defined(CONFIG_NETINIT_NETLOCAL)
static void netinit_net_bringup(void)
{
  /* Bring the network up. */

  if (netlib_ifup(NET_DEVNAME) < 0)
    {
      return;
    }

#if defined(CONFIG_WIRELESS_WAPI) && defined(CONFIG_DRIVERS_IEEE80211)
  /* Associate the wlan with an access point. */

  if (netinit_associate(NET_DEVNAME) < 0)
    {
      return;
    }
#endif

#ifdef CONFIG_NET_ICMPv6_AUTOCONF
  /* Perform ICMPv6 auto-configuration */

  netlib_icmpv6_autoconfiguration(NET_DEVNAME);
#endif

#ifdef CONFIG_NETUTILS_DHCPC
  if (g_use_dhcpc)
    {
      if (netlib_obtain_ipv4addr(NET_DEVNAME) < 0)
        {
          return;
        }
    }
#endif

#ifdef CONFIG_NETUTILS_NTPCLIENT
  /* Start the NTP client */

  ntpc_start();
#endif
}
#else
#  define netinit_net_bringup()
#endif

/****************************************************************************
 * Name: netinit_configure
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

static void netinit_configure(void)
{
#ifdef NETINIT_HAVE_NETDEV
  /* Many embedded network interfaces must have a software assigned MAC */

  netinit_set_macaddr();

  /* Set up IP addresses */

  netinit_set_ipaddrs();

  /* That completes the 'local' initialization of the network device. */

#ifndef CONFIG_NETINIT_NETLOCAL
  /* Bring the network up. */

  netinit_net_bringup();
#endif
#endif /* NETINIT_HAVE_NETDEV */
}

/****************************************************************************
 * Name: netinit_signal
 *
 * Description:
 *   This signal handler responds to changes in PHY status.
 *
 ****************************************************************************/

#ifdef CONFIG_NETINIT_MONITOR
static void netinit_signal(int signo, FAR siginfo_t *siginfo,
                           FAR void *context)
{
  int semcount;
  int ret;

  /* What is the count on the semaphore?  Don't over-post */

  ret = sem_getvalue(&g_notify_sem, &semcount);
  ninfo("Entry: semcount=%d\n", semcount);

  if (ret == OK && semcount <= 0)
    {
      sem_post(&g_notify_sem);
    }

  ninfo("Exit\n");
}
#endif

/****************************************************************************
 * Name: netinit_monitor
 *
 * Description:
 *   Monitor link status, gracefully taking the link up and down as the
 *   link becomes available or as the link is lost.
 *
 ****************************************************************************/

#ifdef CONFIG_NETINIT_MONITOR
static int netinit_monitor(void)
{
  struct timespec abstime;
  struct timespec reltime;
  struct ifreq ifr;
  struct sigaction act;
  struct sigaction oact;
  bool devup;
  int ret;
  int sd;

  ninfo("Entry\n");

  /* Initialize the notification semaphore */

  DEBUGVERIFY(sem_init(&g_notify_sem, 0, 0));

  /* Get a socket descriptor that we can use to communicate with the network
   * interface driver.
   */

  sd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sd < 0)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);

      nerr("ERROR: Failed to create a socket: %d\n", ret);
      goto errout;
    }

  /* Attach a signal handler so that we do not lose PHY events */

  act.sa_sigaction = netinit_signal;
  act.sa_flags = SA_SIGINFO;

  ret = sigaction(CONFIG_NETINIT_SIGNO, &act, &oact);
  if (ret < 0)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);

      nerr("ERROR: sigaction() failed: %d\n", ret);
      goto errout_with_socket;
    }

  /* Now loop, waiting for changes in link status */

  for (; ; )
    {
      /* Configure to receive a signal on changes in link status */

      memset(&ifr, 0, sizeof(struct ifreq));
      strlcpy(ifr.ifr_name, NET_DEVNAME, IFNAMSIZ);

      ifr.ifr_mii_notify_event.sigev_notify = SIGEV_SIGNAL;
      ifr.ifr_mii_notify_event.sigev_signo  = CONFIG_NETINIT_SIGNO;

      ret = ioctl(sd, SIOCMIINOTIFY, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          nerr("ERROR: ioctl(SIOCMIINOTIFY) failed: %d\n", ret);
          goto errout_with_sigaction;
        }

      /* Does the driver think that the link is up or down? */

      ret = ioctl(sd, SIOCGIFFLAGS, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          nerr("ERROR: ioctl(SIOCGIFFLAGS) failed: %d\n", ret);
          goto errout_with_notification;
        }

      devup = ((ifr.ifr_flags & IFF_UP) != 0);

      /* Get the current PHY address in use.  This probably does not change,
       * but just in case...
       *
       * NOTE: We are assuming that the network device name is preserved in
       * the ifr structure.
       */

      ret = ioctl(sd, SIOCGMIIPHY, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          nerr("ERROR: ioctl(SIOCGMIIPHY) failed: %d\n", ret);
          goto errout_with_notification;
        }

      /* Read the PHY status register */

      ifr.ifr_mii_reg_num = MII_MSR;

      ret = ioctl(sd, SIOCGMIIREG, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          nerr("ERROR: ioctl(SIOCGMIIREG) failed: %d\n", ret);
          goto errout_with_notification;
        }

      ninfo("%s: devup=%d PHY address=%02x MSR=%04x\n",
            ifr.ifr_name, devup, ifr.ifr_mii_phy_id, ifr.ifr_mii_val_out);

      /* Check for link up or down */

      if ((ifr.ifr_mii_val_out & MII_MSR_LINKSTATUS) != 0)
        {
          /* Link up... does the drive think that the link is up? */

          if (!devup)
            {
              /* No... We just transitioned from link down to link up.
               * Bring the link up.
               */

              ninfo("Bringing the link up\n");

              ifr.ifr_flags = IFF_UP;
              ret = ioctl(sd, SIOCSIFFLAGS, (unsigned long)&ifr);
              if (ret < 0)
                {
                  ret = -errno;
                  DEBUGASSERT(ret < 0);

                  nerr("ERROR: ioctl(SIOCSIFFLAGS) failed: %d\n", ret);
                  goto errout_with_notification;
                }

#ifdef CONFIG_NET_ICMPv6_AUTOCONF
              /* Perform ICMPv6 auto-configuration */

              netlib_icmpv6_autoconfiguration(ifr.ifr_name);
#endif
              /* And wait for a short delay.  We will want to recheck the
               * link status again soon.
               */

              reltime.tv_sec  = SHORT_TIME_SEC;
              reltime.tv_nsec = 0;
            }
          else
            {
              /* The link is still up.  Take a long, well-deserved rest */

              reltime.tv_sec  = LONG_TIME_SEC;
              reltime.tv_nsec = 0;
            }
        }
      else
        {
          /* Link down... Was the driver link state already down? */

          if (devup)
            {
              /* No... we just transitioned from link up to link down.  Take
               * the link down.
               */

              ninfo("Taking the link down\n");

              ifr.ifr_flags = 0;
              ret = ioctl(sd, SIOCSIFFLAGS, (unsigned long)&ifr);
              if (ret < 0)
                {
                  ret = -errno;
                  DEBUGASSERT(ret < 0);

                  nerr("ERROR: ioctl(SIOCSIFFLAGS) failed: %d\n", ret);
                  goto errout_with_notification;
                }
            }

          /* In either case, wait for the short, configurable delay */

          reltime.tv_sec  = CONFIG_NETINIT_RETRYMSEC / 1000;
          reltime.tv_nsec = (CONFIG_NETINIT_RETRYMSEC % 1000) * 1000000;
        }

      /* Now wait for either the semaphore to be posted for a timed-out to
       * occur.
       */

      sched_lock();
      DEBUGVERIFY(clock_gettime(CLOCK_REALTIME, &abstime));

      abstime.tv_sec  += reltime.tv_sec;
      abstime.tv_nsec += reltime.tv_nsec;
      if (abstime.tv_nsec >= 1000000000)
        {
          abstime.tv_sec++;
          abstime.tv_nsec -= 1000000000;
        }

      sem_timedwait(&g_notify_sem, &abstime);
      sched_unlock();
    }

  /* TODO: Stop the PHY notifications and remove the signal handler. */

errout_with_notification:
  ifr.ifr_mii_notify_event.sigev_notify = SIGEV_NONE;
  ioctl(sd, SIOCMIINOTIFY, (unsigned long)&ifr);
errout_with_sigaction:
  sigaction(CONFIG_NETINIT_SIGNO, &oact, NULL);
errout_with_socket:
  close(sd);
errout:
  nerr("ERROR: Aborting\n");
  return ret;
}
#endif

/****************************************************************************
 * Name: netinit_thread
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

#ifdef CONFIG_NETINIT_THREAD
static pthread_addr_t netinit_thread(pthread_addr_t arg)
{
  ninfo("Entry\n");

  /* Configure the network */

  netinit_configure();

#ifdef CONFIG_NETINIT_MONITOR
  /* Monitor the network status */

  netinit_monitor();
#endif

  ninfo("Exit\n");
  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netinit_bringup
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

int netinit_bringup(void)
{
#ifdef CONFIG_NETINIT_THREAD
  struct sched_param sparam;
  pthread_attr_t     attr;
  pthread_t          tid;
  int                ret;

  /* Start the network initialization thread to perform the network bring-up
   * asynchronously.
   */

  pthread_attr_init(&attr);
  sparam.sched_priority = CONFIG_NETINIT_THREAD_PRIORITY;
  pthread_attr_setschedparam(&attr, &sparam);
  pthread_attr_setstacksize(&attr, CONFIG_NETINIT_THREAD_STACKSIZE);

  ninfo("Starting netinit thread\n");
  ret = pthread_create(&tid, &attr, netinit_thread, NULL);
  if (ret != OK)
    {
      nerr("ERROR: Failed to create netinit thread: %d\n", ret);
      netinit_thread(NULL);
    }
  else
    {
      /* Detach the thread because we will not be joining to it */

      pthread_detach(tid);

      /* Name the thread */

      pthread_setname_np(tid, "netinit");
    }

  return OK;

#else
  /* Perform network initialization sequentially */

  netinit_configure();
  return OK;
#endif
}

#endif /* CONFIG_NETUTILS_NETINIT */
