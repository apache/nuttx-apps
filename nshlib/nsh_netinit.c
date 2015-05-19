/****************************************************************************
 * apps/nshlib/nsh_netinit.c
 *
 *   Copyright (C) 2010-2012, 2014-2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * This is influenced by similar logic from uIP:
 *
 *   Author: Adam Dunkels <adam@sics.se>
 *   Copyright (c) 2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* Is network initialization debug forced on? */

#ifdef CONFIG_NSH_NETINIT_DEBUG
#  undef  CONFIG_DEBUG_VERBOSE
#  define CONFIG_DEBUG_VERBOSE 1
#  undef  CONFIG_DEBUG_NET
#  define CONFIG_DEBUG_NET 1
#endif

#include <sys/ioctl.h>

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <assert.h>
#include <debug.h>

#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/net/mii.h>

#include <apps/netutils/netlib.h>
#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
#  include <apps/netutils/dnsclient.h>
#  include <apps/netutils/dhcpc.h>
#endif

#include "nsh.h"

#ifdef CONFIG_NET

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_NSH_DRIPADDR) && !defined(CONFIG_NSH_DNSIPADDR)
#  define CONFIG_NSH_DNSIPADDR CONFIG_NSH_DRIPADDR
#endif

/* SLIP-specific configuration
 *
 * REVISIT: How will we handle Ethernet and SLIP networks together?  In the
 * future, NSH will need to be extended to handle multiple networks with
 * mixed transports.
 */

#ifdef CONFIG_NET_MULILINK
#  warning REVISIT: CONFIG_NET_MULILINK multilink support incomplete
#endif

/* Select the single network device name supported this this network
 * initialization logci.  If multiple interfaces are present with different
 * link types, the the orider of definition in the following conditional
 * logic will select the one interface that will be used.
 */

#if defined(CONFIG_NET_ETHERNET)
#  define NET_DEVNAME "eth0"
#elif defined(CONFIG_NET_SLIP)
#  define NET_DEVNAME "sl0"
#  ifndef CONFIG_NSH_NOMAC
#    error "CONFIG_NSH_NOMAC must be defined for SLIP"
#  endif
#elif defined(CONFIG_NET_TUN)
#  define NET_DEVNAME "tun0"
#elif defined(CONFIG_NET_LOCAL)
#  define NET_DEVNAME "lo"
#else
#  error ERROR: No link layer protocol defined
#endif

/* We need a valid IP domain (any domain) to create a socket that we can use
 * to comunicate with the network device.
 */

#if defined(CONFIG_NET_IPv4)
#  define AF_INETX AF_INET
#elif defined(CONFIG_NET_IPv6)
#  define AF_INETX AF_INET6
#endif

/* While the network is up, the network monitor really does nothing.  It
 * will wait for a very long time while waiting, it can be awakened by a
 * signal indicating a change in network status.
 */

#define LONG_TIME_SEC    (60*60) /* One hour in seconds */
#define SHORT_TIME_SEC   (2)     /* 2 seconds */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_NSH_NETINIT_MONITOR
static sem_t g_notify_sem;
#endif

#if defined(CONFIG_NET_IPv6) && !defined(CONFIG_NET_ICMPv6_AUTOCONF)
/* Host IPv6 address */

static const uint16_t g_ipv6_hostaddr[8] =
{
  HTONS(CONFIG_NSH_IPv6ADDR_1),
  HTONS(CONFIG_NSH_IPv6ADDR_2),
  HTONS(CONFIG_NSH_IPv6ADDR_3),
  HTONS(CONFIG_NSH_IPv6ADDR_4),
  HTONS(CONFIG_NSH_IPv6ADDR_5),
  HTONS(CONFIG_NSH_IPv6ADDR_6),
  HTONS(CONFIG_NSH_IPv6ADDR_7),
  HTONS(CONFIG_NSH_IPv6ADDR_8),
};

/* Default routine IPv6 address */

static const uint16_t g_ipv6_draddr[8] =
{
  HTONS(CONFIG_NSH_DRIPv6ADDR_1),
  HTONS(CONFIG_NSH_DRIPv6ADDR_2),
  HTONS(CONFIG_NSH_DRIPv6ADDR_3),
  HTONS(CONFIG_NSH_DRIPv6ADDR_4),
  HTONS(CONFIG_NSH_DRIPv6ADDR_5),
  HTONS(CONFIG_NSH_DRIPv6ADDR_6),
  HTONS(CONFIG_NSH_DRIPv6ADDR_7),
  HTONS(CONFIG_NSH_DRIPv6ADDR_8),
};

/* IPv6 netmask */

static const uint16_t g_ipv6_netmask[8] =
{
  HTONS(CONFIG_NSH_IPv6NETMASK_1),
  HTONS(CONFIG_NSH_IPv6NETMASK_2),
  HTONS(CONFIG_NSH_IPv6NETMASK_3),
  HTONS(CONFIG_NSH_IPv6NETMASK_4),
  HTONS(CONFIG_NSH_IPv6NETMASK_5),
  HTONS(CONFIG_NSH_IPv6NETMASK_6),
  HTONS(CONFIG_NSH_IPv6NETMASK_7),
  HTONS(CONFIG_NSH_IPv6NETMASK_8),
};
#endif /* CONFIG_NET_IPv6 && !CONFIG_NET_ICMPv6_AUTOCONF*/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_netinit_configure
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

static void nsh_netinit_configure(void)
{
#ifdef CONFIG_NET_IPv4
  struct in_addr addr;
#endif
#if defined(CONFIG_NSH_DHCPC)
  FAR void *handle;
#endif
#if (defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_NOMAC)) && \
    defined(CONFIG_NET_ETHERNET)
  uint8_t mac[IFHWADDRLEN];
#endif

  nvdbg("Entry\n");

  /* Many embedded network interfaces must have a software assigned MAC */

#if defined(CONFIG_NSH_NOMAC) && defined(CONFIG_NET_ETHERNET)
  /* Use the configured, fixed MAC address */

  mac[0] = (CONFIG_NSH_MACADDR >> (8 * 5)) & 0xff;
  mac[1] = (CONFIG_NSH_MACADDR >> (8 * 4)) & 0xff;
  mac[2] = (CONFIG_NSH_MACADDR >> (8 * 3)) & 0xff;
  mac[3] = (CONFIG_NSH_MACADDR >> (8 * 2)) & 0xff;
  mac[4] = (CONFIG_NSH_MACADDR >> (8 * 1)) & 0xff;
  mac[5] = (CONFIG_NSH_MACADDR >> (8 * 0)) & 0xff;

  /* Set the MAC address */

  netlib_setmacaddr(NET_DEVNAME, mac);

#endif /* CONFIG_NSH_NOMAC && CONFIG_NET_ETHERNET */

#ifdef CONFIG_NET_IPv4
  /* Set up our host address */

#if !defined(CONFIG_NSH_DHCPC)
  addr.s_addr = HTONL(CONFIG_NSH_IPADDR);
#else
  addr.s_addr = 0;
#endif
  netlib_set_ipv4addr(NET_DEVNAME, &addr);

  /* Set up the default router address */

  addr.s_addr = HTONL(CONFIG_NSH_DRIPADDR);
  netlib_set_dripv4addr(NET_DEVNAME, &addr);

  /* Setup the subnet mask */

  addr.s_addr = HTONL(CONFIG_NSH_NETMASK);
  netlib_set_ipv4netmask(NET_DEVNAME, &addr);
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_ICMPv6_AUTOCONF
  /* Perform ICMPv6 auto-configuration */

  netlib_icmpv6_autoconfiguration(NET_DEVNAME);

#else /* CONFIG_NET_ICMPv6_AUTOCONF */

  /* Set up our fixed host address */

  netlib_set_ipv6addr(NET_DEVNAME,
                      (FAR const struct in6_addr *)g_ipv6_hostaddr);

  /* Set up the default router address */

  netlib_set_dripv6addr(NET_DEVNAME,
                        (FAR const struct in6_addr *)g_ipv6_draddr);

  /* Setup the subnet mask */

  netlib_set_ipv6netmask(NET_DEVNAME,
                        (FAR const struct in6_addr *)g_ipv6_netmask);

#endif /* CONFIG_NET_ICMPv6_AUTOCONF */
#endif /* CONFIG_NET_IPv6 */

#if defined(CONFIG_NSH_DHCPC) || defined(CONFIG_NSH_DNS)
  /* Set up the resolver */

  dns_bind();
#if defined(CONFIG_NSH_DNS)
  addr.s_addr = HTONL(CONFIG_NSH_DNSIPADDR);
  dns_setserver(&addr);
#endif
#endif

#if defined(CONFIG_NSH_DHCPC)
  /* Get the MAC address of the NIC */

  netlib_getmacaddr(NET_DEVNAME, mac);

  /* Set up the DHCPC modules */

  handle = dhcpc_open(&mac, IFHWADDRLEN);

  /* Get an IP address.  Note that there is no logic for renewing the IP address in this
   * example.  The address should be renewed in ds.lease_time/2 seconds.
   */

  if (handle)
    {
        struct dhcpc_state ds;
        (void)dhcpc_request(handle, &ds);
        netlib_set_ipv4addr(NET_DEVNAME, &ds.ipaddr);

        if (ds.netmask.s_addr != 0)
          {
            netlib_set_ipv4netmask(NET_DEVNAME, &ds.netmask);
          }

        if (ds.default_router.s_addr != 0)
          {
            netlib_set_dripv4addr(NET_DEVNAME, &ds.default_router);
          }

        if (ds.dnsaddr.s_addr != 0)
          {
            dns_setserver(&ds.dnsaddr);
          }

        dhcpc_close(handle);
    }
#endif

  nvdbg("Exit\n");
}

/****************************************************************************
 * Name: nsh_netinit_signal
 *
 * Description:
 *   This signal handler responds to changes in PHY status.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_NETINIT_MONITOR
static void nsh_netinit_signal(int signo, FAR siginfo_t *siginfo,
                               FAR void * context)
{
  int semcount;
  int ret;

  /* What is the count on the semaphore?  Don't over-post */

  ret = sem_getvalue(&g_notify_sem, &semcount);
  nlldbg("Entry: semcount=%d\n", semcount);

  if (ret == OK && semcount <= 0)
    {
      sem_post(&g_notify_sem);
    }

  nllvdbg("Exit\n");
}
#endif

/****************************************************************************
 * Name: nsh_netinit_monitor
 *
 * Description:
 *   Monitor link status, gracefully taking the link up and down as the
 *   link becomes available or as the link is lost.
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_NETINIT_MONITOR
static int nsh_netinit_monitor(void)
{
  struct timespec abstime;
  struct timespec reltime;
  struct ifreq ifr;
  struct sigaction act;
  bool devup;
  int ret;
  int sd;

  nvdbg("Entry\n");

  /* Initialize the notification semaphore */

  DEBUGVERIFY(sem_init(&g_notify_sem, 0, 0));

  /* Get a socket descriptor that we can use to communicate with the network
   * interface driver.
   */

  sd = socket(AF_INETX, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);

      ndbg("ERROR: Failed to create a socket: %d\n", ret);
      goto errout;
    }

  /* Attach a signal handler so that we do not lose PHY events */

  act.sa_sigaction = nsh_netinit_signal;
  act.sa_flags = SA_SIGINFO;

  ret = sigaction(CONFIG_NSH_NETINIT_SIGNO, &act, NULL);
  if (ret < 0)
    {
      ret = -errno;
      DEBUGASSERT(ret < 0);

      ndbg("ERROR: sigaction() failed: %d\n", ret);
      goto errout_with_socket;
    }

  /* Now loop, waiting for changes in link status */

  for (;;)
    {
      /* Configure to receive a signal on changes in link status */

      strncpy(ifr.ifr_name, NET_DEVNAME, IFNAMSIZ);

      ifr.ifr_mii_notify_pid   = 0; /* PID=0 means this task */
      ifr.ifr_mii_notify_signo = CONFIG_NSH_NETINIT_SIGNO;
      ifr.ifr_mii_notify_arg   = NULL;

      ret = ioctl(sd, SIOCMIINOTIFY, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          ndbg("ERROR: ioctl(SIOCMIINOTIFY) failed: %d\n", ret);
          goto errout_with_sigaction;
        }

      /* Does the driver think that the link is up or down? */

      ret = ioctl(sd, SIOCGIFFLAGS, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          ndbg("ERROR: ioctl(SIOCGIFFLAGS) failed: %d\n", ret);
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

          ndbg("ERROR: ioctl(SIOCGMIIPHY) failed: %d\n", ret);
          goto errout_with_notification;
        }

      /* Read the PHY status register */

      ifr.ifr_mii_reg_num = MII_MSR;

      ret = ioctl(sd, SIOCGMIIREG, (unsigned long)&ifr);
      if (ret < 0)
        {
          ret = -errno;
          DEBUGASSERT(ret < 0);

          ndbg("ERROR: ioctl(SIOCGMIIREG) failed: %d\n", ret);
          goto errout_with_notification;
        }

      nvdbg("%s: devup=%d PHY address=%02x MSR=%04x\n",
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

              nvdbg("Bringing the link up\n");

              ifr.ifr_flags = IFF_UP;
              ret = ioctl(sd, SIOCSIFFLAGS, (unsigned long)&ifr);
              if (ret < 0)
                {
                  ret = -errno;
                  DEBUGASSERT(ret < 0);

                  ndbg("ERROR: ioctl(SIOCSIFFLAGS) failed: %d\n", ret);
                  goto errout_with_notification;
                }

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

              nvdbg("Taking the link down\n");

              ifr.ifr_flags = IFF_DOWN;
              ret = ioctl(sd, SIOCSIFFLAGS, (unsigned long)&ifr);
              if (ret < 0)
                {
                  ret = -errno;
                  DEBUGASSERT(ret < 0);

                  ndbg("ERROR: ioctl(SIOCSIFFLAGS) failed: %d\n", ret);
                  goto errout_with_notification;
                }
            }

          /* In either case, wait for the short, configurable delay */

          reltime.tv_sec  = CONFIG_NSH_NETINIT_RETRYMSEC / 1000;
          reltime.tv_nsec = (CONFIG_NSH_NETINIT_RETRYMSEC % 1000) * 1000000;
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

      (void)sem_timedwait(&g_notify_sem, &abstime);
      sched_unlock();
    }

  /* TODO: Stop the PHY notifications and remove the signal handler. */

errout_with_notification:
#  warning Missing logic
errout_with_sigaction:
#  warning Missing logic
errout_with_socket:
  close(sd);
errout:
  ndbg("Aborting\n");
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_netinit_thread
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

#ifdef CONFIG_NSH_NETINIT_THREAD
static pthread_addr_t nsh_netinit_thread(pthread_addr_t arg)
{
  nvdbg("Entry\n");

  /* Configure the network */

  nsh_netinit_configure();

#ifdef CONFIG_NSH_NETINIT_MONITOR
  /* Monitor the network status */

  nsh_netinit_monitor();
#endif

  nvdbg("Exit\n");
  return NULL;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_netinit
 *
 * Description:
 *   Initialize the network per the selected NuttX configuration
 *
 ****************************************************************************/

int nsh_netinit(void)
{
#ifdef CONFIG_NSH_NETINIT_THREAD
  struct sched_param  sparam;
  pthread_attr_t      attr;
  pthread_t           tid;
  int                 ret;

  /* Start the network initialization thread to perform the network bring-up
   * asynchronously.
   */

  pthread_attr_init(&attr);
  sparam.sched_priority = CONFIG_NSH_NETINIT_THREAD_PRIORITY;
  (void)pthread_attr_setschedparam(&attr, &sparam);
  (void)pthread_attr_setstacksize(&attr, CONFIG_NSH_NETINIT_THREAD_STACKSIZE);

  nvdbg("Starting netinit thread\n");
  ret = pthread_create(&tid, &attr, nsh_netinit_thread, NULL);
  if (ret != OK)
    {
      ndbg("ERROR: Failed to create netinit thread: %d\n", ret);
      (void)nsh_netinit_thread(NULL);
    }
  else
    {
      /* Detach the thread because we will not be joining to it */

      (void)pthread_detach(tid);

      /* Name the thread */

      pthread_setname_np(tid, "netinit");
    }

  return OK;

#else
  /* Perform network initialization sequentially */

  nsh_netinit_configure();
  return OK;
#endif
}

#endif /* CONFIG_NET */
