/****************************************************************************
 * apps/netutils/dhcpd/dhcpd.c
 *
 *   Copyright (C) 2007-2009, 2011-2014, 2017, 2020 Gregory Nutt. All rights
 *     reserved.
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

#ifdef CONFIG_NETUTILS_DHCPD_HOST
#  include <stdio.h>

#  define HTONS(a) htons(a)
#  define HTONL(a) htonl(a)

#  define CONFIG_CPP_HAVE_WARNING 1
#  define FAR

#  define nerr(...) printf(__VA_ARGS__)
#  define ninfo(...) printf(__VA_ARGS__)

#  define ERROR (-1)
#  define OK    (0)
#else
#  include <nuttx/config.h>          /* NuttX configuration */
#  include <debug.h>                 /* For nerr, info */
#  include <nuttx/compiler.h>        /* For CONFIG_CPP_HAVE_WARNING */
#  include "netutils/dhcpd.h"        /* Advertised DHCPD APIs */
#endif

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define DHCP_SERVER_PORT         67
#define DHCP_CLIENT_PORT         68

/* Option codes understood in this file
 *                              Code    Data   Description
 *                                      Length
 */
#define DHCP_OPTION_PAD           0  /*  1     Pad                          */
#define DHCP_OPTION_SUBNET_MASK   1  /*  1     Subnet Mask                  */
#define DHCP_OPTION_ROUTER        3  /*  4     Router                       */
#define DHCP_OPTION_DNS_SERVER    6  /*  4N    DNS                          */
#define DHCP_OPTION_REQ_IPADDR   50  /*  4     Requested IP Address         */
#define DHCP_OPTION_LEASE_TIME   51  /*  4     IP address lease time        */
#define DHCP_OPTION_OVERLOAD     52  /*  1     Option overload              */
#define DHCP_OPTION_MSG_TYPE     53  /*  1     DHCP message type            */
#define DHCP_OPTION_SERVER_ID    54  /*  4     Server identifier            */
#define DHCP_OPTION_END         255  /*  0     End                          */

/* Values for the dhcp msg 'op' field */

#define DHCP_REQUEST              1
#define DHCP_REPLY                2

/* DHCP message types understood in this file */

#define DHCPDISCOVER              1  /* Received from client only */
#define DHCPOFFER                 2  /* Sent from server only */
#define DHCPREQUEST               3  /* Received from client only */
#define DHCPDECLINE               4  /* Received from client only */
#define DHCPACK                   5  /* Sent from server only */
#define DHCPNAK                   6  /* Sent from server only */
#define DHCPRELEASE               7  /* Received from client only */
#define DHCPINFORM                8  /* Not used */

/* The form of an option is:
 *   code   - 1 byte
 *   length - 1 byte
 *   data   - variable number of bytes
 */

#define DHCPD_OPTION_CODE         0
#define DHCPD_OPTION_LENGTH       1
#define DHCPD_OPTION_DATA         2

/* Size of options in DHCP message */

#define DHCPD_OPTIONS_SIZE        312

/* Values for htype and hlen field */

#define DHCP_HTYPE_ETHERNET       1
#define DHCP_HLEN_ETHERNET        6

/* Values for flags field */

#define BOOTP_BROADCAST           0x8000

/* Legal values for this option are:
 *
 *   1     the 'file' field is used to hold options
 *   2     the 'sname' field is used to hold options
 *   3     both fields are used to hold options
 */

#define DHCPD_OPTION_FIELD        0
#define DHCPD_FILE_FIELD          1
#define DHCPD_SNAME_FIELD         2

#ifndef CONFIG_NETUTILS_DHCPD_LEASETIME
#  define CONFIG_NETUTILS_DHCPD_LEASETIME (60*60*24*10) /* 10 days */
#  undef CONFIG_NETUTILS_DHCPD_MINLEASETIME
#  undef CONFIG_NETUTILS_DHCPD_MAXLEASETIME
#endif

#ifndef CONFIG_NETUTILS_DHCPD_MINLEASETIME
#  define CONFIG_NETUTILS_DHCPD_MINLEASETIME (60*60*24*1) /* 1 days */
#endif

#ifndef CONFIG_NETUTILS_DHCPD_MAXLEASETIME
#  define CONFIG_NETUTILS_DHCPD_MAXLEASETIME (60*60*24*30) /* 30 days */
#endif

#ifndef CONFIG_NETUTILS_DHCPD_MAXLEASES
#  define CONFIG_NETUTILS_DHCPD_MAXLEASES 16
#endif

#ifndef CONFIG_NETUTILS_DHCPD_STARTIP
#  define CONFIG_NETUTILS_DHCPD_STARTIP (10L<<24|0L<<16|0L<<16|2L)
#endif

#undef CONFIG_NETUTILS_DHCP_OPTION_ENDIP
#define CONFIG_NETUTILS_DHCP_OPTION_ENDIP \
  (CONFIG_NETUTILS_DHCPD_STARTIP + CONFIG_NETUTILS_DHCPD_MAXLEASES - 1)

#ifndef CONFIG_NETUTILS_DHCPD_OFFERTIME
#  define CONFIG_NETUTILS_DHCPD_OFFERTIME (60*60) /* 1 hour */
#endif

#ifndef CONFIG_NETUTILS_DHCPD_DECLINETIME
#  define CONFIG_NETUTILS_DHCPD_DECLINETIME (60*60) /* 1 hour */
#endif

#undef HAVE_ROUTERIP
#if defined(CONFIG_NETUTILS_DHCPD_ROUTERIP) && CONFIG_NETUTILS_DHCPD_ROUTERIP
#  define HAVE_ROUTERIP 1
#endif

#undef HAVE_NETMASK
#if defined(CONFIG_NETUTILS_DHCPD_NETMASK) && CONFIG_NETUTILS_DHCPD_NETMASK
#  define HAVE_NETMASK 1
#endif

#undef HAVE_DNSIP
#if defined(CONFIG_NETUTILS_DHCPD_DNSIP) && CONFIG_NETUTILS_DHCPD_DNSIP
#  define HAVE_DNSIP 1
#endif

#undef HAVE_LEASE_TIME
#if defined(CONFIG_NETUTILS_DHCPD_HOST) || !defined(CONFIG_DISABLE_POSIX_TIMERS)
#  define HAVE_LEASE_TIME 1
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This structure describes one element in the lease table. There is one
 * slot in the lease table for each assign-able IP address (hence, the IP
 * address itself does not have to be in the table.
 */

struct lease_s
{
  uint8_t  mac[DHCP_HLEN_ETHERNET]; /* MAC address (network order) -- could be larger! */
  bool     allocated;               /* true: IP address is allocated */
#ifdef HAVE_LEASE_TIME
  time_t   expiry;                  /* Lease expiration time (seconds past Epoch) */
#endif
};

struct dhcpmsg_s
{
  uint8_t  op;
  uint8_t  htype;
  uint8_t  hlen;
  uint8_t  hops;
  uint8_t  xid[4];
  uint16_t secs;
  uint16_t flags;
  uint8_t  ciaddr[4];
  uint8_t  yiaddr[4];
  uint8_t  siaddr[4];
  uint8_t  giaddr[4];
  uint8_t  chaddr[16];
#ifndef CONFIG_NET_DHCP_LIGHT
  uint8_t  sname[64];
  uint8_t  file[128];
#endif
  uint8_t  options[312];
};

/* This enumeration describes the state of the DHCPD daemon */

enum dhcpd_daemon_e
{
  DHCPD_NOT_RUNNING = 0,
  DHCPD_STARTED,
  DHCPD_RUNNING,
  DHCPD_STOP_REQUESTED,
  DHCPD_STOPPED
};

struct dhcpd_state_s
{
  /* Server configuration */

  in_addr_t        ds_serverip;     /* The server IP address */

  /* Message buffers */

  struct dhcpmsg_s ds_inpacket;     /* Holds the incoming DHCP client message */
  struct dhcpmsg_s ds_outpacket;    /* Holds the outgoing DHCP server message */

  /* Parsed options from the incoming DHCP client message */

  uint8_t          ds_optmsgtype;   /* Incoming DHCP message type */
  in_addr_t        ds_optreqip;     /* Requested IP address (host order) */
  in_addr_t        ds_optserverip;  /* Serverip IP address (host order) */
  time_t           ds_optleasetime; /* Requested lease time (host order) */

  /* End option pointer for outgoing DHCP server message */

  uint8_t         *ds_optend;

  /* Leases */

  struct lease_s   ds_leases[CONFIG_NETUTILS_DHCPD_MAXLEASES];
};

/* This type describes the state of the DHCPD client daemon.  Only one
 * instance of the DHCPD daemon is permitted in this implementation.
 */

struct dhcpd_daemon_s
{
  uint8_t          ds_state;        /* See enum dhcpd_daemon_e */
  sem_t            ds_lock;         /* Used to protect the whole structure */
  sem_t            ds_sync;         /* Used to synchronize start and stop events */
  pid_t            ds_pid;          /* Task ID of the DHCPD daemon */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t        g_magiccookie[4] =
{
  99, 130, 83, 99
};

static struct dhcpd_state_s g_state;

/* This type describes the state of the DHCPD client daemon.  Only one
 * instance of the DHCPD daemon is permitted in this implementation.  This
 * limitation is due only to this global data structure.
 */

static struct dhcpd_daemon_s g_dhcpd_daemon =
{
  DHCPD_NOT_RUNNING,
  SEM_INITIALIZER(1),
  SEM_INITIALIZER(0),
  -1
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpd_arpupdate
 ****************************************************************************/

#ifndef CONFIG_NETUTILS_DHCPD_HOST
static inline void dhcpd_arpupdate(FAR uint8_t *ipaddr, FAR uint8_t *hwaddr)
{
  struct sockaddr_in inaddr;

  /* Put the protocol address in a standard form. ipaddr is assumed to be in
   * network order by the memcpy.
   */

  inaddr.sin_family = AF_INET;
  inaddr.sin_port = 0;
  memcpy(&inaddr.sin_addr.s_addr, ipaddr, sizeof(in_addr_t));

  /* Update the ARP table */

  netlib_set_arpmapping(&inaddr, hwaddr);
}
#else
#  define dhcpd_arpupdate(ipaddr,hwaddr)
#endif

/****************************************************************************
 * Name: dhcpd_time
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_DHCPD_HOST
# define dhcpd_time() time(0)
#elif defined(HAVE_LEASE_TIME)
static time_t dhcpd_time(void)
{
  struct timespec ts;
  time_t ret = 0;

  if (clock_gettime(CLOCK_REALTIME, &ts) == OK)
    {
      ret = ts.tv_sec;
    }

  return ret;
}
#else
# define dhcpd_time() (0)
#endif

/****************************************************************************
 * Name: dhcpd_leaseexpired
 ****************************************************************************/

#ifdef HAVE_LEASE_TIME
static inline bool dhcpd_leaseexpired(struct lease_s *lease)
{
  if (lease->expiry > dhcpd_time())
    {
      return false;
    }
  else
    {
      memset(lease, 0, sizeof(struct lease_s));
      return true;
    }
}
#else
# define dhcpd_leaseexpired(lease) (false)
#endif

/****************************************************************************
 * Name: dhcpd_setlease
 ****************************************************************************/

struct lease_s *dhcpd_setlease(const uint8_t *mac,
                               in_addr_t ipaddr, time_t expiry)
{
  /* Calculate the offset from the first IP address managed by DHCPD.
   * ipaddr must be in host order!
   */

  int ndx = ipaddr - CONFIG_NETUTILS_DHCPD_STARTIP;
  struct lease_s *ret = NULL;

  ninfo("ipaddr: %08" PRIx32 " ipaddr: %08" PRIx32 " ndx: %d MAX: %d\n",
        (uint32_t)ipaddr, (uint32_t)CONFIG_NETUTILS_DHCPD_STARTIP, ndx,
        CONFIG_NETUTILS_DHCPD_MAXLEASES);

  /* Verify that the address offset is within the supported range */

  if (ndx >= 0 && ndx < CONFIG_NETUTILS_DHCPD_MAXLEASES)
    {
       ret = &g_state.ds_leases[ndx];
       memcpy(ret->mac, mac, DHCP_HLEN_ETHERNET);
       ret->allocated = true;
#ifdef HAVE_LEASE_TIME
       ret->expiry = dhcpd_time() + expiry;
#endif
    }

  return ret;
}

/****************************************************************************
 * Name: dhcp_leaseipaddr
 ****************************************************************************/

static inline in_addr_t dhcp_leaseipaddr(FAR struct lease_s *lease)
{
  /* Return IP address in host order */

  return (in_addr_t)(lease - g_state.ds_leases) +
         CONFIG_NETUTILS_DHCPD_STARTIP;
}

/****************************************************************************
 * Name: dhcpd_findbymac
 ****************************************************************************/

static FAR struct lease_s *dhcpd_findbymac(FAR const uint8_t *mac)
{
  int i;

  for (i = 0; i < CONFIG_NETUTILS_DHCPD_MAXLEASES; i++)
    {
      if (memcmp(g_state.ds_leases[i].mac, mac, DHCP_HLEN_ETHERNET) == 0)
        {
          return &(g_state.ds_leases[i]);
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: dhcpd_findbyipaddr
 ****************************************************************************/

static FAR struct lease_s *dhcpd_findbyipaddr(in_addr_t ipaddr)
{
  if (ipaddr >= CONFIG_NETUTILS_DHCPD_STARTIP &&
      ipaddr <= CONFIG_NETUTILS_DHCP_OPTION_ENDIP)
    {
      FAR struct lease_s *lease =
        &g_state.ds_leases[ipaddr - CONFIG_NETUTILS_DHCPD_STARTIP];
      if (lease->allocated > 0)
        {
          return lease;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: dhcpd_allocipaddr
 ****************************************************************************/

static in_addr_t dhcpd_allocipaddr(void)
{
  struct lease_s *lease = NULL;
  in_addr_t ipaddr, startaddr;

  ipaddr = startaddr = CONFIG_NETUTILS_DHCPD_STARTIP;
  for (; ipaddr <= CONFIG_NETUTILS_DHCP_OPTION_ENDIP; ipaddr++)
    {
      /* Skip over address ending in 0 or 255 */

      if ((ipaddr & 0xff) == 0 || (ipaddr & 0xff) == 0xff)
        {
          continue;
        }

      /* Is there already a lease on this address?  If so, has it expired? */

      lease = dhcpd_findbyipaddr(ipaddr);
      if ((!lease || dhcpd_leaseexpired(lease)))
        {
#ifdef CONFIG_CPP_HAVE_WARNING
#  warning "FIXME: Should check if anything responds to an ARP request or ping"
#  warning "       to verify that there is no other user of this IP address"
#endif
          memset(g_state.ds_leases[ipaddr - startaddr].mac,
          0, DHCP_HLEN_ETHERNET);
          g_state.ds_leases[ipaddr - startaddr].allocated = true;
#ifdef HAVE_LEASE_TIME
          g_state.ds_leases[ipaddr - startaddr].expiry =
            dhcpd_time() + CONFIG_NETUTILS_DHCPD_OFFERTIME;
#endif
          /* Return the address in host order */

          return ipaddr;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: dhcpd_parseoptions
 ****************************************************************************/

static inline bool dhcpd_parseoptions(void)
{
  uint32_t tmp;
  uint8_t *ptr;
#ifndef CONFIG_NET_DHCP_LIGHT
  uint8_t overloaded;
  uint8_t currfield;
#endif
  int optlen = 0;
  int remaining;

  /* Verify that the option field starts with a valid magic number */

  ptr = g_state.ds_inpacket.options;
  if (memcmp(ptr, g_magiccookie, 4) != 0)
    {
      /* Bad magic number... skip g_state.ds_outpacket */

      nerr("ERROR: Bad magic: %d,%d,%d,%d\n",
           ptr[0], ptr[1], ptr[2], ptr[3]);
      return false;
    }

  /* Set up to parse the options */

  ptr       += 4;
  remaining  = DHCPD_OPTIONS_SIZE - 4;
#ifndef CONFIG_NET_DHCP_LIGHT
  overloaded = DHCPD_OPTION_FIELD;
  currfield  = DHCPD_OPTION_FIELD;
#endif

  /* Set all options to the default value */

  g_state.ds_optmsgtype   = 0;    /* Incoming DHCP message type */
  g_state.ds_optreqip     = 0;    /* Requested IP address (host order) */
  g_state.ds_optserverip  = 0;    /* Serverip IP address (host order) */
  g_state.ds_optleasetime = 0;    /* Requested lease time (host order) */
  g_state.ds_optend       = NULL;

  do
    {
      /* The form of an option is:
       *   code   - 1 byte
       *   length - 1 byte
       *   data   - variable number of bytes
       */

      switch (ptr[DHCPD_OPTION_CODE])
        {
          /* Skip over any padding bytes */

          case DHCP_OPTION_PAD:
            optlen = 1;
            break;

          /* the Overload option is used to indicate that the DHCP 'sname'
           * or 'file' fields are being overloaded by using them to carry
           * DHCP options. A DHCP server inserts this option if the
           * returned parameters will exceed the usual space allotted for
           * options.
           *
           * If this option is present, the client interprets the specified
           * additional fields after it concludes interpretation of the
           * standard option fields.
           *
           * Legal values for this option are:
           *
           *   1     the 'file' field is used to hold options
           *   2     the 'sname' field is used to hold options
           *   3     both fields are used to hold options
           */

#ifndef CONFIG_NET_DHCP_LIGHT
          case DHCP_OPTION_OVERLOAD:
            optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            if (optlen >= 3 && optlen < remaining)
              {
                overloaded = ptr[DHCPD_OPTION_DATA];
              }
            break;
#endif

          case DHCP_OPTION_END:
#ifndef CONFIG_NET_DHCP_LIGHT
            if (currfield == DHCPD_OPTION_FIELD &&
                (overloaded & DHCPD_FILE_FIELD) != 0)
              {
                ptr       = g_state.ds_inpacket.file;
                remaining = sizeof(g_state.ds_inpacket.file);
                currfield = DHCPD_FILE_FIELD;
              }
            else if (currfield == DHCPD_FILE_FIELD &&
                     (overloaded & DHCPD_SNAME_FIELD) != 0)
              {
                ptr       = g_state.ds_inpacket.sname;
                remaining = sizeof(g_state.ds_inpacket.sname);
                currfield = DHCPD_SNAME_FIELD;
              }
            else
              {
                return true;
              }
            break;
#else
            return true;
#endif

          case DHCP_OPTION_REQ_IPADDR: /* Requested IP Address */
            optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            if (optlen >= 6 && optlen < remaining)
              {
                memcpy(&tmp, &ptr[DHCPD_OPTION_DATA], 4);
                g_state.ds_optreqip = (in_addr_t)ntohl(tmp);
              }
            break;

          case DHCP_OPTION_LEASE_TIME: /* IP address lease time */
             optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            if (optlen >= 6 && optlen < remaining)
              {
                memcpy(&tmp, &ptr[DHCPD_OPTION_DATA], 4);
                g_state.ds_optleasetime = (time_t)ntohl(tmp);
              }
            break;

         case DHCP_OPTION_MSG_TYPE: /* DHCP message type */
            optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            if (optlen >= 3 && optlen < remaining)
              {
                g_state.ds_optmsgtype = ptr[DHCPD_OPTION_DATA];
              }
            break;

          case DHCP_OPTION_SERVER_ID: /* Server identifier */
            optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            if (optlen >= 6 && optlen < remaining)
              {
                memcpy(&tmp, &ptr[DHCPD_OPTION_DATA], 4);
                g_state.ds_optserverip = (in_addr_t)ntohl(tmp);
              }
            break;

          default:

            /* Skip over unsupported options */

            optlen = ptr[DHCPD_OPTION_LENGTH] + 2;
            break;
        }

      /* Advance to the next option */

      ptr       += optlen;
      remaining -= optlen;
    }
  while (remaining > 0);

  return false;
}

/****************************************************************************
 * Name: dhcpd_verifyreqip
 ****************************************************************************/

static inline bool dhcpd_verifyreqip(void)
{
  struct lease_s *lease;

  /* Verify that the requested IP address is within the supported lease
   * range
   */

  if (g_state.ds_optreqip >= CONFIG_NETUTILS_DHCPD_STARTIP &&
      g_state.ds_optreqip <= CONFIG_NETUTILS_DHCP_OPTION_ENDIP)
    {
      /* And verify that the lease has not already been taken or offered
       * (unless the lease/offer is expired, then the address is free game).
       */

      lease = dhcpd_findbyipaddr(g_state.ds_optreqip);
      if (!lease || dhcpd_leaseexpired(lease))
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: dhcpd_verifyreqleasetime
 ****************************************************************************/

static inline bool dhcpd_verifyreqleasetime(uint32_t *leasetime)
{
  uint32_t tmp = g_state.ds_optleasetime;

  /* Did the client request a specific lease time? */

  if (tmp != 0)
    {
      /* Yes..  Verify that the requested lease time is within a
       * valid range
       */

      if (tmp > CONFIG_NETUTILS_DHCPD_MAXLEASETIME)
        {
          tmp = CONFIG_NETUTILS_DHCPD_MAXLEASETIME;
        }
      else if (tmp < CONFIG_NETUTILS_DHCPD_MINLEASETIME)
        {
          tmp = CONFIG_NETUTILS_DHCPD_MINLEASETIME;
        }

      /* Return the clipped lease time */

      *leasetime = tmp;
      return true;
    }

  return false;
}

/****************************************************************************
 * Name: dhcpd_addoption
 ****************************************************************************/

static int dhcpd_addoption(uint8_t *option)
{
  int offset;
  int len = 4;

  if (g_state.ds_optend)
    {
      offset = g_state.ds_optend - g_state.ds_outpacket.options;
      len    = option[DHCPD_OPTION_LENGTH] + 2;

      /* Check if the option will fit into the options array */

      if (offset + len + 1 < DHCPD_OPTIONS_SIZE)
        {
          /* Copy the option into the option array */

          memcpy(g_state.ds_optend, option, len);
          g_state.ds_optend += len;
         *g_state.ds_optend  = DHCP_OPTION_END;
        }
    }

  return len;
}

/****************************************************************************
 * Name: dhcpd_addoption8
 ****************************************************************************/

static int dhcpd_addoption8(uint8_t code, uint8_t value)
{
  uint8_t option[3];

  /* Construct the option sequence */

  option[DHCPD_OPTION_CODE]   = code;
  option[DHCPD_OPTION_LENGTH] = 1;
  option[DHCPD_OPTION_DATA]   = value;

  /* Add the option sequence to the response */

  return dhcpd_addoption(option);
}

/****************************************************************************
 * Name: dhcpd_addoption32
 ****************************************************************************/

static int dhcpd_addoption32(uint8_t code, uint32_t value)
{
  uint8_t option[6];

  /* Construct the option sequence */

  option[DHCPD_OPTION_CODE]   = code;
  option[DHCPD_OPTION_LENGTH] = 4;
  memcpy(&option[DHCPD_OPTION_DATA], &value, 4);

  /* Add the option sequence to the response */

  return dhcpd_addoption(option);
}

/****************************************************************************
 * Name: dhcp_addoption32p
 ****************************************************************************/

#ifdef HAVE_DNSIP
static int dhcp_addoption32p(uint8_t code, FAR uint8_t *value)
{
  uint8_t option[6];

  /* Construct the option sequence */

  option[DHCPD_OPTION_CODE]   = code;
  option[DHCPD_OPTION_LENGTH] = 4;
  memcpy(&option[DHCPD_OPTION_DATA], value, 4);

  /* Add the option sequence to the response */

  return dhcpd_addoption(option);
}
#endif

/****************************************************************************
 * Name: dhcpd_socket
 ****************************************************************************/

static inline int dhcpd_socket(void)
{
  int sockfd;
#if defined(HAVE_SO_REUSEADDR) || defined(HAVE_SO_BROADCAST)
  int optval;
  int ret;
#endif

  /* Create a socket to listen for requests from DHCP clients */

  sockfd = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Configure the socket */

#ifdef HAVE_SO_REUSEADDR
  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (FAR void *)&optval,
                   sizeof(int));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SO_REUSEADDR failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }
#endif

#ifdef HAVE_SO_BROADCAST
  optval = 1;
  ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (FAR void *)&optval,
                   sizeof(int));
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SO_BROADCAST failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }
#endif

  return sockfd;
}

/****************************************************************************
 * Name: dhcpd_openresponder
 ****************************************************************************/

static inline int dhcpd_openresponder(void)
{
  struct sockaddr_in addr;
  int sockfd;
  int ret;

  ninfo("Responder: %08" PRIx32 "\n", ntohl(g_state.ds_serverip));

  /* Create a socket to listen for requests from DHCP clients */

  sockfd = dhcpd_socket();
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Bind the socket to a local port. */

  addr.sin_family      = AF_INET;
  addr.sin_port        = 0;
  addr.sin_addr.s_addr = g_state.ds_serverip;

  ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      nerr("ERROR: bind failed, port=%d addr=%08lx: %d\n",
           addr.sin_port, (long)addr.sin_addr.s_addr, errno);
      close(sockfd);
      return ERROR;
    }

  return sockfd;
}

/****************************************************************************
 * Name: dhcpd_initpacket
 ****************************************************************************/

static void dhcpd_initpacket(uint8_t mtype)
{
  uint32_t nulladdr = 0;

  /* Set up the generic parts of the DHCP server message */

  memset(&g_state.ds_outpacket, 0, sizeof(struct dhcpmsg_s));

  g_state.ds_outpacket.op         = DHCP_REPLY;
  g_state.ds_outpacket.htype      = g_state.ds_inpacket.htype;
  g_state.ds_outpacket.hlen       = g_state.ds_inpacket.hlen;

  memcpy(&g_state.ds_outpacket.xid, &g_state.ds_inpacket.xid, 4);
  memcpy(g_state.ds_outpacket.chaddr, g_state.ds_inpacket.chaddr, 16);

  if (memcmp(g_state.ds_outpacket.giaddr, &nulladdr, 4) != 0)
    {
      g_state.ds_outpacket.flags  = g_state.ds_inpacket.flags;
    }

  /* Add the generic options */

  memcpy(g_state.ds_outpacket.options, g_magiccookie, 4);
  g_state.ds_optend = &g_state.ds_outpacket.options[4];
 *g_state.ds_optend = DHCP_OPTION_END;
  dhcpd_addoption8(DHCP_OPTION_MSG_TYPE, mtype);
  dhcpd_addoption32(DHCP_OPTION_SERVER_ID, g_state.ds_serverip);
}

/****************************************************************************
 * Name: dhcpd_sendpacket
 ****************************************************************************/

static int dhcpd_sendpacket(int bbroadcast)
{
  struct sockaddr_in addr;
  in_addr_t ipaddr;
  int sockfd;
  int len;
  int ret = ERROR;

#ifdef CONFIG_NETUTILS_DHCPD_IGNOREBROADCAST
  /* This is a hack.  I've had problems with Windows machines responding
   * to unicast.  I think this is associated with a Windows registry key in
   * HKEY_LOCAL_MACHINE\
   * SYSTEM\CurrentControlSet\Services\DHCPServer\Parameters:
   * The IgnoreBroadcastFlag value controls this behavior:  A value of 1 will
   * cause the server to ignore the client broadcast flag and always respond
   * with multicast; the value 0 to allows clients to request unicast.
   */

  ipaddr = INADDR_BROADCAST;
#else
  const uint8_t anyipaddr[4] =
  {
  };

  /* Determine which address to respond to (or if we need to broadcast the
   * response)
   *
   * (1) If he caller know that it needs to multicast the response, it will
   *     set broadcast.
   * (2) Otherwise, if the client already has and address (ciaddr), then use
   *     that for unicast
   * (3) Broadcast if the client says it can't handle uni-cast
   *     (BOOTP_BROADCAST set)
   * (4) Otherwise, the client claims it can handle the uni-casst response
   *     and we will uni-cast to the offered address (yiaddr).
   *
   * NOTE: We really should also check the giaddr field.  If no zero, the
   * server should send any return messages to the 'DHCP server' port on the
   *  BOOTP relay agent whose address appears in 'giaddr'.
   */

  if (bbroadcast)
    {
      ipaddr = INADDR_BROADCAST;
    }
  else if (memcmp(g_state.ds_outpacket.ciaddr, anyipaddr, 4) != 0)
    {
      dhcpd_arpupdate(g_state.ds_outpacket.ciaddr,
                      g_state.ds_outpacket.chaddr);
      memcpy(&ipaddr, g_state.ds_outpacket.ciaddr, 4);
    }
  else if (g_state.ds_outpacket.flags & HTONS(BOOTP_BROADCAST))
    {
      ipaddr = INADDR_BROADCAST;
    }
  else
    {
      dhcpd_arpupdate(g_state.ds_outpacket.yiaddr,
                      g_state.ds_outpacket.chaddr);
      memcpy(&ipaddr, g_state.ds_outpacket.yiaddr, 4);
    }
#endif

  /* Create a socket to respond with a packet to the client.  We
   * cannot re-use the listener socket because it is not bound correctly
   */

  sockfd = dhcpd_openresponder();
  if (sockfd >= 0)
    {
      /* Then send the response to the DHCP client port at that address */

      memset(&addr, 0, sizeof(struct sockaddr_in));
      addr.sin_family      = AF_INET;
      addr.sin_port        = HTONS(DHCP_CLIENT_PORT);
      addr.sin_addr.s_addr = ipaddr;

      /* Send the minimum sized packet that includes the END option */

      len = (g_state.ds_optend - (FAR uint8_t *)&g_state.ds_outpacket) + 1;
      ninfo("sendto %08lx:%04x len=%d\n",
            (long)ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port), len);

      ret = sendto(sockfd, &g_state.ds_outpacket, len, 0,
                   (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
      close(sockfd);
    }

  return ret;
}

/****************************************************************************
 * Name: dhcpd_sendoffer
 ****************************************************************************/

static inline int dhcpd_sendoffer(in_addr_t ipaddr, uint32_t leasetime)
{
  in_addr_t netaddr;
#ifdef HAVE_DNSIP
  uint32_t dnsaddr;
  dnsaddr = htonl(CONFIG_NETUTILS_DHCPD_DNSIP);
#endif
  /* IP address is in host order */

  ninfo("Sending offer: %08lx\n", (long)ipaddr);

  /* Initialize the outgoing packet */

  dhcpd_initpacket(DHCPOFFER);

  /* Add the address offered to the client (converting to network order) */

  netaddr = htonl(ipaddr);
  memcpy(g_state.ds_outpacket.yiaddr, &netaddr, 4);

  /* Add the leasetime to the response options */

  dhcpd_addoption32(DHCP_OPTION_LEASE_TIME, htonl(leasetime));
#ifdef HAVE_NETMASK
  dhcpd_addoption32(DHCP_OPTION_SUBNET_MASK,
                    htonl(CONFIG_NETUTILS_DHCPD_NETMASK));
#endif
#ifdef HAVE_ROUTERIP
  dhcpd_addoption32(DHCP_OPTION_ROUTER,
                    htonl(CONFIG_NETUTILS_DHCPD_ROUTERIP));
#endif
#ifdef HAVE_DNSIP
  dhcp_addoption32p(DHCP_OPTION_DNS_SERVER, (FAR uint8_t *)&dnsaddr);
#endif

  /* Send the offer response */

#ifdef CONFIG_NETUTILS_DHCPD_IGNOREBROADCAST
  return dhcpd_sendpacket(true);
#else
  return dhcpd_sendpacket(false);
#endif
}

/****************************************************************************
 * Name: dhcpd_sendnak
 ****************************************************************************/

static int dhcpd_sendnak(void)
{
  /* Initialize and send the NAK response */

  dhcpd_initpacket(DHCPNAK);
  memcpy(g_state.ds_outpacket.ciaddr, g_state.ds_inpacket.ciaddr, 4);
  return dhcpd_sendpacket(true);
}

/****************************************************************************
 * Name: dhcpd_sendack
 ****************************************************************************/

int dhcpd_sendack(in_addr_t ipaddr)
{
  uint32_t leasetime = CONFIG_NETUTILS_DHCPD_LEASETIME;
  in_addr_t netaddr;
#ifdef HAVE_DNSIP
  uint32_t dnsaddr;
  dnsaddr = htonl(CONFIG_NETUTILS_DHCPD_DNSIP);
#endif

  /* Initialize the ACK response */

  dhcpd_initpacket(DHCPACK);
  memcpy(g_state.ds_outpacket.ciaddr, g_state.ds_inpacket.ciaddr, 4);

  /* Add the IP address assigned to the client */

  netaddr = htonl(ipaddr);
  memcpy(g_state.ds_outpacket.yiaddr, &netaddr, 4);

  /* Did the client request a specific lease time? */

  dhcpd_verifyreqleasetime(&leasetime);

  /* Add the lease time to the response */

  dhcpd_addoption32(DHCP_OPTION_LEASE_TIME, htonl(leasetime));
#ifdef HAVE_NETMASK
  dhcpd_addoption32(DHCP_OPTION_SUBNET_MASK,
                    htonl(CONFIG_NETUTILS_DHCPD_NETMASK));
#endif
#ifdef HAVE_ROUTERIP
  dhcpd_addoption32(DHCP_OPTION_ROUTER,
                    htonl(CONFIG_NETUTILS_DHCPD_ROUTERIP));
#endif
#ifdef HAVE_DNSIP
  dhcp_addoption32p(DHCP_OPTION_DNS_SERVER, (FAR uint8_t *)&dnsaddr);
#endif

#ifdef CONFIG_NETUTILS_DHCPD_IGNOREBROADCAST
  if (dhcpd_sendpacket(true) < 0)
#else
  if (dhcpd_sendpacket(false) < 0)
#endif
    {
      return ERROR;
    }

  dhcpd_setlease(g_state.ds_inpacket.chaddr, ipaddr, leasetime);
  return OK;
}

/****************************************************************************
 * Name: dhcpd_discover
 ****************************************************************************/

static inline int dhcpd_discover(void)
{
  struct lease_s *lease;
  in_addr_t ipaddr;
  uint32_t leasetime = CONFIG_NETUTILS_DHCPD_LEASETIME;

  /* Check if the client is already in the lease table */

  lease = dhcpd_findbymac(g_state.ds_inpacket.chaddr);
  if (lease)
    {
      /* Yes... get the remaining time on the lease */

#ifdef HAVE_LEASE_TIME
      if (!dhcpd_leaseexpired(lease))
        {
          leasetime = lease->expiry - dhcpd_time();
          if (leasetime < CONFIG_NETUTILS_DHCPD_MINLEASETIME)
            {
              leasetime = CONFIG_NETUTILS_DHCPD_MINLEASETIME;
            }
        }
#endif

      /* Get the IP address associated with the lease (host order) */

      ipaddr = dhcp_leaseipaddr(lease);
      ninfo("Already have lease for IP %08lx\n", (long)ipaddr);
    }

  /* Check if the client has requested a specific IP address */

  else if (dhcpd_verifyreqip())
    {
      /* Use the requested IP address (host order) */

      ipaddr = g_state.ds_optreqip;
      ninfo("User requested IP %08lx\n", (long)ipaddr);
    }
  else
    {
      /* No... allocate a new IP address (host order) */

      ipaddr = dhcpd_allocipaddr();
      ninfo("Allocated IP %08lx\n", (long)ipaddr);
    }

  /* Did we get any IP address? */

  if (!ipaddr)
    {
      /* Nope... return failure */

      nerr("ERROR: Failed to get IP address\n");
      return ERROR;
    }

  /* Reserve the leased IP for a shorter time for the offer */

  if (!dhcpd_setlease(g_state.ds_inpacket.chaddr, ipaddr,
                      CONFIG_NETUTILS_DHCPD_OFFERTIME))
    {
      nerr("ERROR: Failed to set lease\n");
      return ERROR;
    }

  /* Check if the client has requested a specific lease time */

  dhcpd_verifyreqleasetime(&leasetime);

  /* Send the offer response */

  return dhcpd_sendoffer(ipaddr, leasetime);
}

/****************************************************************************
 * Name: dhcpd_request
 ****************************************************************************/

static inline int dhcpd_request(void)
{
  struct lease_s *lease;
  in_addr_t ipaddr = 0;
  uint8_t response = 0;

  /* Check if this client already holds a lease. This can happen when the
   * client (1) the IP is reserved for the client from a previous offer,
   * or (2) the client is re-initializing or rebooting while the lease is
   * still valid.
   */

  lease = dhcpd_findbymac(g_state.ds_inpacket.chaddr);
  if (lease)
    {
      /* Yes.. the client already holds a lease.  Verify that the request is
       * consistent* with the existing lease (host order).
       */

      ipaddr = dhcp_leaseipaddr(lease);
      ninfo("Lease ipaddr: %08" PRIx32 " Server IP: %08" PRIx32
            " Requested IP: %08" PRIx32 "\n",
            (uint32_t)ipaddr,
            (uint32_t)g_state.ds_optserverip,
            (uint32_t)g_state.ds_optreqip);

      if (g_state.ds_optserverip)
        {
          /* ACK if the serverip is correct and the requested IP address is
           * the one already offered to the client.
           */

          if (g_state.ds_optserverip == ntohl(g_state.ds_serverip) &&
             (g_state.ds_optreqip != 0 || g_state.ds_optreqip == ipaddr))
            {
              response = DHCPACK;
            }
          else
            {
              response = DHCPNAK;
            }
        }

      /* We have the lease and no server IP was requested. Was a specific IP
       * address requested? (host order)
       */

      else if (g_state.ds_optreqip)
        {
          /* Yes..ACK if the requested IP address is the one already leased.
           * Both addresses are in host order.
           */

          if (ipaddr == g_state.ds_optreqip)
            {
              response = DHCPACK;
            }
          else
            {
              response = DHCPNAK;
            }
        }

      /* The client has specified neither a server IP nor requested
       * IP address
       */

      else
        {
          /* ACK if the IP used by the client is the one already assigned
           * to it. NOTE ipaddr is in host order; ciaddr is network order!
           */

          uint32_t tmp = htonl(ipaddr);
          if (memcmp(&tmp, g_state.ds_inpacket.ciaddr, 4) == 0)
            {
              response = DHCPACK;
            }
          else
            {
              response = DHCPNAK;
            }
        }
    }

  /* The client does not hold a lease (referenced by its MAC address) and is
   * requesting a specific IP address that was, apparently, never offered to
   * to the client.  Perform some sanity checks before sending the NAK.
   */

  else if (g_state.ds_optreqip && !g_state.ds_optserverip)
    {
      ninfo("Server IP: %08" PRIx32 " Requested IP: %08" PRIx32 "\n",
            (uint32_t)g_state.ds_optserverip,
            (uint32_t)g_state.ds_optreqip);

      /* Is this IP address already assigned? */

      lease = dhcpd_findbyipaddr(g_state.ds_optreqip);
      if (lease)
        {
          /* Yes.. Send NAK unless the lease has expired */

          if (!dhcpd_leaseexpired(lease))
            {
              response = DHCPNAK;
            }
        }

      /* DHCPREQUEST without DHCPDISCOVER:
       * The request IP address is in the range but has not been leased,
       * maybe requested before the last shutdown, lease again.
       */

      else if (g_state.ds_optreqip >= CONFIG_NETUTILS_DHCPD_STARTIP &&
               g_state.ds_optreqip <= CONFIG_NETUTILS_DHCP_OPTION_ENDIP)
        {
          ipaddr = g_state.ds_optreqip;
          response = DHCPACK;
        }

      /* The requested IP address out of range, negative */

      else
        {
          response = DHCPNAK;
        }
    }

  /* Otherwise, the client does not hold a lease and is not requesting any
   * specific IP address.
   */

  /* Finally, either (1) send the ACK, (2) send a NAK, or (3) remain silent
   * based on the checks above.
   */

  if (response == DHCPACK)
    {
      ninfo("ACK IP %08lx\n", (long)ipaddr);
      dhcpd_sendack(ipaddr);
    }
  else if (response == DHCPNAK)
    {
      ninfo("NAK IP %08lx\n", (long)ipaddr);
      dhcpd_sendnak();
    }
  else
    {
      ninfo("Remaining silent IP %08lx\n", (long)ipaddr);
    }

  return OK;
}

/****************************************************************************
 * Name: dhcpd_decline
 ****************************************************************************/

static inline int dhcpd_decline(void)
{
  struct lease_s *lease;

  /* Find the lease associated with this hardware address */

  lease = dhcpd_findbymac(g_state.ds_inpacket.chaddr);
  if (lease)
    {
      /* Disassociate the IP from the MAC, but prevent re-used of this
       * address for a period of time.
       */

      memset(lease->mac, 0, DHCP_HLEN_ETHERNET);
#ifdef HAVE_LEASE_TIME
      lease->expiry = dhcpd_time() + CONFIG_NETUTILS_DHCPD_DECLINETIME;
#endif
    }

  return OK;
}

static inline int dhcpd_release(void)
{
  struct lease_s *lease;

  /* Find the lease associated with this hardware address */

  lease = dhcpd_findbymac(g_state.ds_inpacket.chaddr);
  if (lease)
    {
      /* Release the IP address now */

      memset(lease, 0, sizeof(struct lease_s));
    }

  return OK;
}

/****************************************************************************
 * Name: dhcpd_openlistener
 ****************************************************************************/

static inline int dhcpd_openlistener(FAR const char *interface)
{
  struct sockaddr_in addr;
  struct ifreq req;
  int sockfd;
  int ret;

  /* Create a socket to listen for requests from DHCP clients */

  sockfd = dhcpd_socket();
  if (sockfd < 0)
    {
      nerr("ERROR: socket failed: %d\n", errno);
      return ERROR;
    }

  /* Get the IP address of the selected device */

  strncpy(req.ifr_name, interface, IFNAMSIZ);
  ret = ioctl(sockfd, SIOCGIFADDR, (unsigned long)&req);
  if (ret < 0)
    {
      nerr("ERROR: setsockopt SIOCGIFADDR failed: %d\n", errno);
      close(sockfd);
      return ERROR;
    }

  g_state.ds_serverip = ((FAR struct sockaddr_in *)
    &req.ifr_addr)->sin_addr.s_addr;

  ninfo("serverip: %08" PRIx32 "\n", ntohl(g_state.ds_serverip));

  /* Bind the socket to a local port. We have to bind to INADDRY_ANY to
   * receive broadcast messages.
   */

  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(DHCP_SERVER_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (ret < 0)
    {
      nerr("ERROR: bind failed, port=%d addr=%08lx: %d\n",
           addr.sin_port, (long)addr.sin_addr.s_addr, errno);
      close(sockfd);
      return ERROR;
    }

  return sockfd;
}

/****************************************************************************
 * Name: dhcpd_task_run
 ****************************************************************************/

static int dhcpd_task_run(int argc, char **argv)
{
  return dhcpd_run(argv[1]);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dhcpd_run
 ****************************************************************************/

int dhcpd_run(FAR const char *interface)
{
  int sockfd;
  int nbytes;

  ninfo("Started\n");

  /* Exit if the dhcpd is already executed */

  if (g_dhcpd_daemon.ds_state == DHCPD_RUNNING)
    {
      return OK;
    }

  /* Update the pid if running in daemon mode */

  g_dhcpd_daemon.ds_pid = getpid();

  /* Indicate that we have started */

  g_dhcpd_daemon.ds_state = DHCPD_RUNNING;

  sem_post(&g_dhcpd_daemon.ds_sync);

  /* Initialize everything to zero */

  memset(&g_state, 0, sizeof(struct dhcpd_state_s));

  /* Now loop indefinitely, reading packets from the DHCP server socket */

  sockfd = -1;
  while (g_dhcpd_daemon.ds_state != DHCPD_STOP_REQUESTED)
    {
      /* Create a socket to listen for requests from DHCP clients */

      if (sockfd < 0)
        {
          sockfd = dhcpd_openlistener(interface);
          if (sockfd < 0)
            {
                nerr("ERROR: Failed to create socket\n");
                break;
            }
        }

      /* Read the next g_state.ds_outpacket */

      nbytes = recv(sockfd, &g_state.ds_inpacket,
                    sizeof(struct dhcpmsg_s), 0);
      if (nbytes < 0)
        {
          /* On errors (other EINTR), close the socket and try again */

          nerr("ERROR: recv failed: %d\n", errno);
          if (errno != EINTR && errno != EAGAIN)
            {
              close(sockfd);
              sockfd = -1;
            }

          continue;
        }

      /* Parse the incoming message options */

      if (!dhcpd_parseoptions())
        {
          /* Failed to parse the message options */

          nerr("ERROR: No msg type\n");
          continue;
        }

#ifdef CONFIG_NETUTILS_DHCPD_HOST
      /* Get the poor little uC a change to get its recvfrom in place */

      usleep(500 * 1000);
#endif

      /* Now process the incoming DHCP message by its message type */

      switch (g_state.ds_optmsgtype)
        {
          case DHCPDISCOVER:
            ninfo("DHCPDISCOVER\n");
            dhcpd_discover();
            break;

          case DHCPREQUEST:
            ninfo("DHCPREQUEST\n");
            dhcpd_request();
            break;

          case DHCPDECLINE:
            ninfo("DHCPDECLINE\n");
            dhcpd_decline();
            break;

          case DHCPRELEASE:
            ninfo("DHCPRELEASE\n");
            dhcpd_release();
            break;

          case DHCPINFORM: /* Not supported */
          default:
            nerr("ERROR: Unsupported message type: %d\n",
                 g_state.ds_optmsgtype);
            break;
        }
    }

  g_dhcpd_daemon.ds_pid   = -1;
  g_dhcpd_daemon.ds_state = DHCPD_STOPPED;
  sem_post(&g_dhcpd_daemon.ds_sync);

  return OK;
}

/****************************************************************************
 * Name: dhcpd_start
 *
 * Description:
 *   Start the DHCPD daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the DHCPDC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int dhcpd_start(FAR const char *interface)
{
  FAR char *argv[2];
  int pid;

  argv[0] = (char *)interface;
  argv[1] = NULL;

  /* Is the DHCPD in a non-running state? */

  sem_wait(&g_dhcpd_daemon.ds_lock);
  if (g_dhcpd_daemon.ds_state == DHCPD_NOT_RUNNING ||
      g_dhcpd_daemon.ds_state == DHCPD_STOPPED)
    {
      /* Start the DHCPD daemon */

      g_dhcpd_daemon.ds_state = DHCPD_STARTED;
      pid =
        task_create("DHCPD daemon", CONFIG_NETUTILS_DHCPD_PRIORITY,
                    CONFIG_NETUTILS_DHCPD_STACKSIZE, dhcpd_task_run,
                    argv);

      /* Handle failures to start the DHCPD daemon */

      if (pid < 0)
        {
          int errval = errno;
          DEBUGASSERT(errval > 0);

          g_dhcpd_daemon.ds_state = DHCPD_STOPPED;
          nerr("ERROR: Failed to start the DHCPD daemon: %d\n", errval);
          sem_post(&g_dhcpd_daemon.ds_lock);
          return -errval;
        }

      /* Wait for any daemon state change */

      do
        {
          sem_wait(&g_dhcpd_daemon.ds_sync);
        }
      while (g_dhcpd_daemon.ds_state == DHCPD_STARTED);
    }

  sem_post(&g_dhcpd_daemon.ds_lock);
  return OK;
}

/****************************************************************************
 * Name: dhcpd_stop
 *
 * Description:
 *   Stop the DHCPD daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.  The current
 *   implementation only returns success.
 *
 ****************************************************************************/

int dhcpd_stop(void)
{
  int ret;

  /* Is the DHCPD in a running state? */

  sem_wait(&g_dhcpd_daemon.ds_lock);
  if (g_dhcpd_daemon.ds_state == DHCPD_STARTED ||
      g_dhcpd_daemon.ds_state == DHCPD_RUNNING)
    {
      /* Yes.. request that the daemon stop. */

      g_dhcpd_daemon.ds_state = DHCPD_STOP_REQUESTED;

      /* Wait for any daemon state change */

      do
        {
          /* Signal the DHCPD client */

          ret = kill(g_dhcpd_daemon.ds_pid,
                     CONFIG_NETUTILS_DHCPD_SIGWAKEUP);

          if (ret < 0)
            {
              nerr("ERROR: kill pid %d failed: %d\n",
                   g_dhcpd_daemon.ds_pid, errno);
              break;
            }

          /* Wait for the DHCPD client to respond to the stop request */

          sem_wait(&g_dhcpd_daemon.ds_sync);
        }
      while (g_dhcpd_daemon.ds_state == DHCPD_STOP_REQUESTED);
    }

  sem_post(&g_dhcpd_daemon.ds_lock);
  return OK;
}
