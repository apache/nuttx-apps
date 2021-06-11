/****************************************************************************
 * apps/netutils/ntpclient/ntpclient.c
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

#include <stdbool.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <netinet/in.h>

#ifdef CONFIG_LIBC_NETDB
#  include <netdb.h>
#  include <arpa/inet.h>
#endif

#include <nuttx/clock.h>

#include "netutils/ntpclient.h"

#include "ntpv3.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_HAVE_LONG_LONG
#  error "64-bit integer support required for NTP client"
#endif

#if defined(CONFIG_LIBC_NETDB) && !defined(CONFIG_NETUTILS_NTPCLIENT_SERVER)
#  error "CONFIG_NETUTILS_NTPCLIENT_SERVER must be provided"
#endif

#if !defined(CONFIG_LIBC_NETDB) && !defined(CONFIG_NETUTILS_NTPCLIENT_SERVERIP)
#  error "CONFIG_NETUTILS_NTPCLIENT_SERVERIP must be provided"
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES
#  define CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES 5
#elif CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES < 1
#  error "NTP sample number below 1, invalid configuration"
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_SERVER
#  ifdef CONFIG_NETUTILS_NTPCLIENT_SERVERIP
/* Old config support */
#    warning "NTP server hostname not defined, using deprecated server IP address setting"
#    define CONFIG_NETUTILS_NTPCLIENT_SERVER \
          inet_ntoa(CONFIG_NETUTILS_NTPCLIENT_SERVERIP)
#  else
#    error "NTP server hostname not defined"
#  endif
#endif

/* NTP Time is seconds since 1900. Convert to Unix time which is seconds
 * since 1970
 */

#define NTP2UNIX_TRANSLATION 2208988800u

#define NTP_VERSION_V3       3
#define NTP_VERSION_V4       4
#define NTP_VERSION          NTP_VERSION_V4

#define MAX_SERVER_SELECTION_RETRIES 3

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef STR
#  define STR2(x) #x
#  define STR(x) STR2(x)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This enumeration describes the state of the NTP daemon */

enum ntpc_daemon_e
{
  NTP_NOT_RUNNING = 0,
  NTP_STARTED,
  NTP_RUNNING,
  NTP_STOP_REQUESTED,
  NTP_STOPPED
};

/* This type describes the state of the NTP client daemon.  Only one
 * instance of the NTP daemon is permitted in this implementation.
 */

struct ntpc_daemon_s
{
  uint8_t state;       /* See enum ntpc_daemon_e */
  sem_t lock;          /* Used to protect the whole structure */
  sem_t sync;          /* Used to synchronize start and stop events */
  pid_t pid;           /* Task ID of the NTP daemon */
  sq_queue_t kod_list; /* KoD excluded server addresses */
  int family;          /* Allowed address family */
};

union ntp_addr_u
{
  struct sockaddr sa;
#ifdef CONFIG_NET_IPv4
  struct sockaddr_in in4;
#endif
#ifdef CONFIG_NET_IPv6
  struct sockaddr_in6 in6;
#endif
  struct sockaddr_storage ss;
};

/* NTP offset. */

struct ntp_sample_s
{
  int64_t offset;
  int64_t delay;
  union ntp_addr_u srv_addr;
} packet_struct;

/* Server address list. */

struct ntp_servers_s
{
  union ntp_addr_u list[CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES];
  size_t num;
  size_t pos;
  FAR char *hostlist_str;
  FAR char *hostlist_saveptr;
  FAR char *hostnext;
  FAR const char *ntp_servers;
};

/* KoD exclusion list. */

struct ntp_kod_exclude_s
{
  sq_entry_t node;
  union ntp_addr_u addr;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This type describes the state of the NTP client daemon.  Only one
 * instance of the NTP daemon is permitted in this implementation.  This
 * limitation is due only to this global data structure.
 */

static struct ntpc_daemon_s g_ntpc_daemon =
{
  NTP_NOT_RUNNING,
  SEM_INITIALIZER(1),
  SEM_INITIALIZER(0),
  -1,
  { NULL, NULL },
  AF_UNSPEC,         /* Default is both IPv4 and IPv6 */
};

static struct ntp_sample_s g_last_samples
    [CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES];
unsigned int g_last_nsamples = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sample_cmp
 ****************************************************************************/

static int sample_cmp(FAR const void *_a, FAR const void *_b)
{
  FAR const struct ntp_sample_s *a = _a;
  FAR const struct ntp_sample_s *b = _b;
  int64_t diff = a->offset - b->offset;

  if (diff < 0)
    {
      return -1;
    }
  else if (diff > 0)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

/****************************************************************************
 * Name: int64abs
 ****************************************************************************/

static inline int64_t int64abs(int64_t value)
{
  return value >= 0 ? value : -value;
}

/****************************************************************************
 * Name: ntpc_get_compile_timestamp
 ****************************************************************************/

static time_t ntpc_get_compile_timestamp(void)
{
  struct tm tm;
  int year;
  int day;
  int month;
  bool unknown = true;
  time_t tim;
#ifdef __DATE__
  const char *pmonth;
  const char *pyear;
  const char *pday;

  /* Compile date. Format: "MMM DD YYYY", where MMM is month in three letter
   * format, DD is day of month (left padded with space if less than ten) and
   * YYYY is year. "??? ?? ????" if unknown.
   */

  pmonth = __DATE__;
  pday = pmonth + 4;
  pyear = pmonth + 7;

  year = (pyear[0] - '0') * 1000
         + (pyear[1] - '0') * 100
         + (pyear[2] - '0') * 10
         + (pyear[3] - '0') * 1;

  day = (pday[1] - '0');
  if (pday[0] != ' ')
    {
      day += (pday[0] - '0') * 10;
    }

  unknown = false;
  switch (pmonth[0])
    {
    default:
      unknown = true;
      break;
    case 'J':
      if (pmonth[1] == 'a') /* Jan */
        month = 1;
      else if (pmonth[2] == 'n') /* Jun */
        month = 6;
      else /* Jul */
        month = 7;
      break;
    case 'F': /* Feb */
      month = 2;
      break;
    case 'M':
      if (pmonth[2] == 'r') /* Mar */
        month = 3;
      else /* May */
        month = 5;
      break;
    case 'A':
      if (pmonth[1] == 'p') /* Apr */
        month = 4;
      else /* Aug */
        month = 8;
      break;
    case 'S': /* Sep */
      month = 9;
      break;
    case 'O': /* Oct */
      month = 10;
      break;
    case 'N': /* Nov */
      month = 11;
      break;
    case 'D': /* Dec */
      month = 12;
      break;
    }
#endif

  if (unknown)
    {
      month = 8;
      day = 18;
      year = 2015;
    }

  /* Convert date to timestamp. */

  memset(&tm, 0, sizeof(tm));
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_mday = day;
  tm.tm_mon = month - 1;
  tm.tm_year = year - 1900;

  tim = mktime(&tm);

  /* Reduce by one day to discount timezones. */

  tim -= 24 * 60 * 60;

  return tim;
}

/****************************************************************************
 * Name: ntpc_getuint32
 *
 * Description:
 *   Return the big-endian, 4-byte value in network (big-endian) order.
 *
 ****************************************************************************/

static inline uint32_t ntpc_getuint32(FAR const uint8_t *ptr)
{
  /* Network order is big-endian; host order is irrelevant */

  return (uint32_t)ptr[3] |          /* MS byte appears first in data stream */
         ((uint32_t)ptr[2] << 8) |
         ((uint32_t)ptr[1] << 16) |
         ((uint32_t)ptr[0] << 24);
}

/****************************************************************************
 * Name: ntpc_getuint64
 *
 * Description:
 *   Return the big-endian, 8-byte value in network (big-endian) order.
 *
 ****************************************************************************/

static inline uint64_t ntpc_getuint64(FAR const uint8_t *ptr)
{
  return ((uint64_t)ntpc_getuint32(ptr) << 32) | ntpc_getuint32(&ptr[4]);
}

/****************************************************************************
 * Name: ntpc_setuint32
 *
 * Description:
 *   Write 4-byte value to buffer in network (big-endian) order.
 *
 ****************************************************************************/

static inline void ntpc_setuint32(FAR uint8_t *ptr, uint32_t value)
{
  ptr[3] = (uint8_t)value;
  ptr[2] = (uint8_t)(value >> 8);
  ptr[1] = (uint8_t)(value >> 16);
  ptr[0] = (uint8_t)(value >> 24);
}

/****************************************************************************
 * Name: ntpc_setuint64
 *
 * Description:
 *   Write 8-byte value to buffer in network (big-endian) order.
 *
 ****************************************************************************/

static inline void ntpc_setuint64(FAR uint8_t *ptr, uint64_t value)
{
  ntpc_setuint32(ptr + 0, (uint32_t)(value >> 32));
  ntpc_setuint32(ptr + 4, (uint32_t)value);
}

/****************************************************************************
 * Name: ntp_secpart
 ****************************************************************************/

static uint32_t ntp_secpart(uint64_t time)
{
  /* NTP timestamps are represented as a 64-bit fixed-point number, in
   * seconds relative to 0000 UT on 1 January 1900.  The integer part is
   * in the first 32 bits and the fraction part in the last 32 bits, as
   * shown in the following diagram.
   *
   *    0                   1                   2                   3
   *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                         Integer Part                          |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *   |                         Fraction Part                         |
   *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */

  /* Get seconds part. */

  return time >> 32;
}

/****************************************************************************
 * Name: ntp_nsecpart
 ****************************************************************************/

static uint32_t ntp_nsecpart(uint64_t time)
{
  /* Get fraction part converted to nanoseconds. */

  return ((time & 0xffffffffu) * NSEC_PER_SEC) >> 32;
}

/****************************************************************************
 * Name: timespec2ntp
 *
 * Convert UNIX timespec timestamp to NTP 64-bit fixed point timestamp.
 *
 ****************************************************************************/

static uint64_t timespec2ntp(FAR const struct timespec *ts)
{
  uint64_t ntp_time;

  /* Set fraction part. */

  ntp_time = ((uint64_t)(ts->tv_nsec) << 32) / NSEC_PER_SEC;

  DEBUGASSERT((ntp_time >> 32) == 0);

  /* Set seconds part. */

  ntp_time += (uint64_t)(ts->tv_sec) << 32;

  return ntp_time;
}

/****************************************************************************
 * Name: ntp_gettime
 *
 * Get time in NTP epoch, stored in fixed point uint64_t format (upper 32-bit
 * seconds, lower 32-bit fraction)
 ****************************************************************************/

static uint64_t ntp_localtime(void)
{
  int err = errno;
  struct timespec currts;

  /* Get current clock in NTP epoch. */

  clock_gettime(CLOCK_REALTIME, &currts);
  currts.tv_sec += NTP2UNIX_TRANSLATION;

  /* Restore errno. */

  errno = err;

  return timespec2ntp(&currts);
}

/****************************************************************************
 * Name: ntpc_calculate_offset
 *
 * Description:
 *   Calculate NTP time offset and round-trip delay
 *
 ****************************************************************************/

static void ntpc_calculate_offset(FAR int64_t *offset, FAR int64_t *delay,
                                  uint64_t local_xmittime,
                                  uint64_t local_recvtime,
                                  FAR const uint8_t *remote_recv,
                                  FAR const uint8_t *remote_xmit)
{
  uint64_t remote_recvtime;
  uint64_t remote_xmittime;

  /* Two timestamps from server, when request was received, and response
   * send.
   */

  remote_recvtime = ntpc_getuint64(remote_recv);
  remote_xmittime = ntpc_getuint64(remote_xmit);

  /* Calculate offset of local time compared to remote time.
   * See: https://www.eecis.udel.edu/~mills/time.html
   *      http://nicolas.aimon.fr/2014/12/05/timesync/
   */

  *offset = (int64_t)((remote_recvtime / 2 - local_xmittime / 2) +
                     (remote_xmittime / 2 - local_recvtime / 2));

  /* Calculate roundtrip delay. */

  *delay = (local_recvtime - local_xmittime) -
          (remote_xmittime - remote_recvtime);
}

/****************************************************************************
 * Name: ntpc_settime
 *
 * Description:
 *   Given the NTP time offset, adjust the system time
 *
 ****************************************************************************/

static void ntpc_settime(int64_t offset, FAR struct timespec *start_realtime,
                         FAR struct timespec *start_monotonic)
{
  struct timespec tp;
  struct timespec curr_realtime;
#ifdef CONFIG_CLOCK_MONOTONIC
  struct timespec curr_monotonic;
  int64_t diffms_real;
  int64_t diffms_mono;
  int64_t diff_diff_ms;
#endif

  /* Get the system times */

  clock_gettime(CLOCK_REALTIME, &curr_realtime);

#ifdef CONFIG_CLOCK_MONOTONIC
  clock_gettime(CLOCK_MONOTONIC, &curr_monotonic);

  /* Check differences between monotonic and realtime. */

  diffms_real = curr_realtime.tv_sec - start_realtime->tv_sec;
  diffms_real *= 1000;
  diffms_real += (int64_t)(curr_realtime.tv_nsec -
                           start_realtime->tv_nsec) / (1000 * 1000);

  diffms_mono = curr_monotonic.tv_sec - start_monotonic->tv_sec;
  diffms_mono *= 1000;
  diffms_mono += (int64_t)(curr_monotonic.tv_nsec -
                           start_monotonic->tv_nsec) / (1000 * 1000);

  /* Detect if real-time has been altered by other task. */

  diff_diff_ms = diffms_real - diffms_mono;
  if (diff_diff_ms < 0)
    {
      diff_diff_ms = -diff_diff_ms;
    }

  if (diff_diff_ms >= 1000)
    {
      nwarn("System time altered by other task by %ju msecs, "
            "do not apply offset.\n", (intmax_t)diff_diff_ms);

      return;
    }
#else
  UNUSED(start_monotonic);
#endif

  /* Apply offset */

  tp = curr_realtime;
  tp.tv_sec  += ntp_secpart(offset);
  tp.tv_nsec += ntp_nsecpart(offset);
  while (tp.tv_nsec >= NSEC_PER_SEC)
    {
      tp.tv_nsec -= NSEC_PER_SEC;
      tp.tv_sec++;
    }

  /* Set the system time */

  clock_settime(CLOCK_REALTIME, &tp);

  ninfo("Set time to %ju.%03ld seconds (offset: %s%lu.%03lu).\n",
        (intmax_t)tp.tv_sec, tp.tv_nsec / NSEC_PER_MSEC,
        offset < 0 ? "-" : "",
        (unsigned long)ntp_secpart(int64abs(offset)),
        ntp_nsecpart(int64abs(offset)) / NSEC_PER_MSEC);
}

/****************************************************************************
 * Name: ntp_address_in_kod_list
 *
 * Description: Check if address is in KoD KoD exclusion list.
 *
 ****************************************************************************/

static bool ntp_address_in_kod_list(FAR const union ntp_addr_u *server_addr)
{
  FAR struct ntp_kod_exclude_s *entry;

  entry = (FAR void *)sq_peek(&g_ntpc_daemon.kod_list);
  while (entry)
    {
      if (memcmp(&entry->addr, server_addr, sizeof(*server_addr)) == 0)
        {
          return true;
        }

      entry = (FAR void *)sq_next(&entry->node);
    }

  return false;
}

/****************************************************************************
 * Name: ntp_is_kiss_of_death
 *
 * Description: Check if this is KoD response from the server. If it is,
 * add server to KoD exclusion list and return 'true'.
 *
 ****************************************************************************/

static bool ntp_is_kiss_of_death(FAR const struct ntp_datagram_s *recv,
                                 FAR const union ntp_addr_u *server_addr)
{
  /* KoD only specified for v4. */

  if (GETVN(recv->lvm) != NTP_VERSION_V4)
    {
      if (recv->stratum == 0)
        {
          /* Stratum 0 is unspecified on v3, so ignore packet. */

          return true;
        }
      else
        {
          return false;
        }
    }

  /* KoD message if stratum == 0. */

  if (recv->stratum != 0)
    {
      return false;
    }

  /* KoD message received. */

  /* Check if we need to add server to access exclusion list. */

  if (strncmp((char *)recv->refid, "DENY", 4) == 0 ||
      strncmp((char *)recv->refid, "RSTR", 4) == 0 ||
      strncmp((char *)recv->refid, "RATE", 4) == 0)
    {
      struct ntp_kod_exclude_s *entry;

      entry = calloc(1, sizeof(*entry));
      if (entry)
        {
          entry->addr = *server_addr;
          sq_addlast(&entry->node, &g_ntpc_daemon.kod_list);
        }
    }

  return true;
}

/****************************************************************************
 * Name: ntpc_verify_recvd_ntp_datagram
 ****************************************************************************/

static bool ntpc_verify_recvd_ntp_datagram(
                FAR const struct ntp_datagram_s *xmit,
                FAR const struct ntp_datagram_s *recv,
                size_t nbytes,
                FAR const union ntp_addr_u *xmitaddr,
                FAR const union ntp_addr_u *recvaddr,
                size_t recvaddrlen)
{
  time_t buildtime;
  time_t seconds;

  if (recvaddr->sa.sa_family != xmitaddr->sa.sa_family)
    {
      ninfo("wrong address family\n");

      return false;
    }

#ifdef CONFIG_NET_IPv4
  if (recvaddr->sa.sa_family == AF_INET)
    {
      if (recvaddrlen != sizeof(struct sockaddr_in) ||
          xmitaddr->in4.sin_addr.s_addr != recvaddr->in4.sin_addr.s_addr ||
          xmitaddr->in4.sin_port != recvaddr->in4.sin_port)
        {
          ninfo("response from wrong peer\n");

          return false;
        }
    }
#endif

#ifdef CONFIG_NET_IPv6
  if (recvaddr->sa.sa_family == AF_INET6)
    {
      if (recvaddrlen != sizeof(struct sockaddr_in6) ||
          memcmp(&xmitaddr->in6.sin6_addr,
                 &recvaddr->in6.sin6_addr, sizeof(struct in6_addr)) != 0 ||
          xmitaddr->in6.sin6_port != recvaddr->in6.sin6_port)
        {
          ninfo("response from wrong peer\n");

          return false;
        }
    }
#endif /* CONFIG_NET_IPv6 */

  if (nbytes < NTP_DATAGRAM_MINSIZE)
    {
      /* Too short. */

      ninfo("too short response\n");

      return false;
    }

  if (GETVN(recv->lvm) != NTP_VERSION_V3 &&
      GETVN(recv->lvm) != NTP_VERSION_V4)
    {
      /* Wrong version. */

      ninfo("wrong version: %d\n", GETVN(recv->lvm));

      return false;
    }

  if (GETMODE(recv->lvm) != 4)
    {
      /* Response not in server mode. */

      ninfo("wrong mode: %d\n", GETMODE(recv->lvm));

      return false;
    }

  if (ntp_is_kiss_of_death(recv, xmitaddr))
    {
      /* KoD, Kiss-o'-Death. Ignore response. */

      ninfo("kiss-of-death response\n");

      return false;
    }

  if (GETLI(recv->lvm) == 3)
    {
      /* Clock not synchronized. */

      ninfo("LI: not synchronized\n");

      return false;
    }

  if (memcmp(recv->origtimestamp, xmit->xmittimestamp, 8) != 0)
    {
      /* "The Originate Timestamp in the server reply should match the
       * Transmit Timestamp used in the client request."
       */

      ninfo("xmittimestamp mismatch\n");

      return false;
    }

  if (ntpc_getuint32(recv->reftimestamp) == 0)
    {
      /* Invalid timestamp. */

      ninfo("invalid reftimestamp, 0x%08lx\n",
            (unsigned long)ntpc_getuint32(recv->reftimestamp));

      return false;
    }

  if (ntpc_getuint32(recv->recvtimestamp) == 0)
    {
      /* Invalid timestamp. */

      ninfo("invalid recvtimestamp, 0x%08lx\n",
            (unsigned long)ntpc_getuint32(recv->recvtimestamp));

      return false;
    }

  if (ntpc_getuint32(recv->xmittimestamp) == 0)
    {
      /* Invalid timestamp. */

      ninfo("invalid xmittimestamp, 0x%08lx\n",
            (unsigned long)ntpc_getuint32(recv->xmittimestamp));

      return false;
    }

  if ((int64_t)(ntpc_getuint64(recv->xmittimestamp) -
                ntpc_getuint64(recv->recvtimestamp)) < 0)
    {
      /* Remote received our request after sending response? */

      ninfo("invalid xmittimestamp & recvtimestamp pair\n");

      return false;
    }

  buildtime = ntpc_get_compile_timestamp();
  seconds = ntpc_getuint32(recv->recvtimestamp);
  if (seconds > NTP2UNIX_TRANSLATION)
    {
      seconds -= NTP2UNIX_TRANSLATION;
    }

  if (seconds < buildtime)
    {
      /* Invalid timestamp. */

      ninfo("invalid recvtimestamp, 0x%08lx\n",
            (unsigned long)ntpc_getuint32(recv->recvtimestamp));

      return false;
    }

  return true;
}

/****************************************************************************
 * Name: ntpc_create_dgram_socket
 ****************************************************************************/

static int ntpc_create_dgram_socket(int domain)
{
  struct timeval tv;
  int ret;
  int sd;
  int err;

  /* Create a datagram socket  */

  sd = socket(domain, SOCK_DGRAM, 0);
  if (sd < 0)
    {
      err = errno;
      nerr("ERROR: socket failed: %d\n", err);
      errno = err;
      return sd;
    }

  /* Setup a send timeout on the socket */

  tv.tv_sec  = 5;
  tv.tv_usec = 0;

  ret = setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      err = errno;
      nerr("ERROR: setsockopt(SO_SNDTIMEO) failed: %d\n", errno);
      goto err_close;
    }

  /* Setup a receive timeout on the socket */

  tv.tv_sec  = 5;
  tv.tv_usec = 0;

  ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));
  if (ret < 0)
    {
      err = errno;
      nerr("ERROR: setsockopt(SO_RCVTIMEO) failed: %d\n", errno);
      goto err_close;
    }

  return sd;

err_close:
  close(sd);
  errno = err;
  return ret;
}

/****************************************************************************
 * Name: ntp_gethostip_multi
 ****************************************************************************/

static int ntp_gethostip_multi(FAR const char *hostname,
                               FAR union ntp_addr_u *ipaddr, size_t nipaddr)
{
  struct addrinfo hints;
  FAR struct addrinfo *info;
  FAR struct addrinfo *next;
  int ret;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family   = g_ntpc_daemon.family;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;
  hints.ai_flags    = AI_NUMERICSERV;

  ret = getaddrinfo(hostname, STR(CONFIG_NETUTILS_NTPCLIENT_PORTNO),
                    &hints, &info);
  if (ret != OK)
    {
      nerr("ERROR: getaddrinfo(%s): %d: %s\n", hostname,
           ret, gai_strerror(ret));
      return ERROR;
    }

  ret = 0;
  for (next = info; next != NULL; next = next->ai_next)
    {
      if (ret >= nipaddr)
        {
          break;
        }

      memcpy(ipaddr, next->ai_addr, next->ai_addrlen);
      ret++;
      ipaddr++;
    }

  freeaddrinfo(info);
  return ret;
}

/****************************************************************************
 * Name: ntp_get_next_hostip
 ****************************************************************************/

static int ntp_get_next_hostip(FAR struct ntp_servers_s *srvs,
                               FAR union ntp_addr_u *addr)
{
  int ret;

  if (srvs->pos >= srvs->num)
    {
      FAR char *hostname;

      srvs->pos = 0;
      srvs->num = 0;

      /* Get next hostname. */

      if (srvs->hostnext == NULL)
        {
          if (srvs->hostlist_str)
            {
              free(srvs->hostlist_str);
              srvs->hostlist_str = NULL;
            }

          /* Allocate hostname list buffer */

          srvs->hostlist_str = strdup(srvs->ntp_servers);
          if (!srvs->hostlist_str)
            {
              return ERROR;
            }

          srvs->hostlist_saveptr = NULL;
#ifndef __clang_analyzer__ /* Silence false 'possible memory leak'. */
          srvs->hostnext =
              strtok_r(srvs->hostlist_str, ";", &srvs->hostlist_saveptr);
#endif
        }

      hostname = srvs->hostnext;
      srvs->hostnext = strtok_r(NULL, ";", &srvs->hostlist_saveptr);

      if (!hostname)
        {
          /* Invalid configuration. */

          errno = EINVAL;
          return ERROR;
        }

      /* Refresh DNS for new IP-addresses. */

      ret = ntp_gethostip_multi(hostname, srvs->list,
                                ARRAY_SIZE(srvs->list));
      if (ret <= 0)
        {
          return ERROR;
        }

      srvs->num = ret;
    }

  *addr = srvs->list[srvs->pos++];

  return OK;
}

/****************************************************************************
 * Name: ntpc_get_ntp_sample
 ****************************************************************************/

static int ntpc_get_ntp_sample(FAR struct ntp_servers_s *srvs,
                               FAR struct ntp_sample_s *samples,
                               int curr_idx)
{
  FAR struct ntp_sample_s *sample = &samples[curr_idx];
  uint64_t xmit_time, recv_time;
  union ntp_addr_u server;
  union ntp_addr_u recvaddr;
  struct ntp_datagram_s xmit;
  struct ntp_datagram_s recv;
  socklen_t socklen;
  ssize_t nbytes;
  int errval;
  int retry = 0;
  int nsamples = curr_idx;
  bool addr_ok;
  int ret;
  int sd = -1;
  int i;

  /* Setup an ntp_addr_u with information about the server we are
   * going to ask the time from.
   */

  memset(&server, 0, sizeof(server));

  do
    {
      addr_ok = true;

      ret = ntp_get_next_hostip(srvs, &server);
      if (ret < 0)
        {
          errval = errno;

          nerr("ERROR: ntp_get_next_hostip() failed: %d\n", errval);

          goto sock_error;
        }

      /* Make sure that server not in exclusion list. */

      if (ntp_address_in_kod_list(&server))
        {
          if (retry < MAX_SERVER_SELECTION_RETRIES)
            {
              ninfo("on KoD list. retry DNS.\n");

              retry++;
              addr_ok = false;
              continue;
            }
          else
            {
              errval = -EALREADY;
              goto sock_error;
            }
        }

      /* Make sure that this sample is from new server. */

      for (i = 0; i < nsamples; i++)
        {
          if (memcmp(&server, &samples[i].srv_addr, sizeof(server)) == 0)
            {
              /* Already have sample from this server, retry DNS. */

              ninfo("retry DNS\n");

              if (retry < MAX_SERVER_SELECTION_RETRIES)
                {
                  retry++;
                  addr_ok = false;
                  break;
                }
              else
                {
                  /* Accept same server if cannot get DNS for other server. */

                  break;
                }
            }
        }
    }
  while (!addr_ok);

  /* Open socket. */

  sd = ntpc_create_dgram_socket(server.sa.sa_family);
  if (sd < 0)
    {
      errval = errno;

      nerr("ERROR: ntpc_create_dgram_socket() failed: %d\n", errval);

      goto sock_error;
    }

  /* Format the transmit datagram */

  memset(&xmit, 0, sizeof(xmit));
  xmit.lvm = MKLVM(0, NTP_VERSION, 3);

  ninfo("Sending a NTPv%d packet\n", NTP_VERSION);

  xmit_time = ntp_localtime();
  ntpc_setuint64(xmit.xmittimestamp, xmit_time);

  socklen = (server.sa.sa_family == AF_INET) ? sizeof(struct sockaddr_in)
                                             : sizeof(struct sockaddr_in6);
  ret = sendto(sd, &xmit, sizeof(struct ntp_datagram_s),
               0, &server.sa, socklen);
  if (ret < 0)
    {
      errval = errno;

      nerr("ERROR: sendto() failed: %d\n", errval);

      goto sock_error;
    }

  /* Attempt to receive a packet (with a timeout that was set up via
   * setsockopt() above)
   */

  nbytes = recvfrom(sd, (void *)&recv, sizeof(struct ntp_datagram_s),
                    0, &recvaddr.sa, &socklen);
  recv_time = ntp_localtime();

  /* Check if the received message was long enough to be a valid NTP
   * datagram.
   */

  if (nbytes > 0 && ntpc_verify_recvd_ntp_datagram(
                          &xmit, &recv, nbytes, &server, &recvaddr, socklen))
    {
      close(sd);
      sd = -1;

      ninfo("Calculate offset\n");

      memset(sample, 0, sizeof(struct ntp_sample_s));

      sample->srv_addr = server;

      ntpc_calculate_offset(&sample->offset, &sample->delay,
                            xmit_time, recv_time, recv.recvtimestamp,
                            recv.xmittimestamp);

      return OK;
    }
  else
    {
      /* Check for errors.  Short datagrams are handled as error. */

      errval = errno;

      if (nbytes >= 0)
        {
          errval = EMSGSIZE;
        }
      else
        {
          nerr("ERROR: recvfrom() failed: %d\n", errval);
        }

      goto sock_error;
    }

sock_error:
  if (sd >= 0)
    {
      close(sd);
      sd = -1;
    }

  errno = errval;
  return ERROR;
}

/****************************************************************************
 * Name: ntpc_daemon
 *
 * Description:
 *   This the the NTP client daemon.  This is a *very* minimal
 *   implementation! An NTP request is and the system clock is set when the
 *   response is received
 *
 ****************************************************************************/

static int ntpc_daemon(int argc, FAR char **argv)
{
  struct ntp_sample_s samples[CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES];
  struct ntp_servers_s srvs;
  int exitcode = EXIT_SUCCESS;
  int retries = 0;
  int nsamples;
  int ret;

  /* We must always be invoked with a list of servers. */

  DEBUGASSERT(argc > 1 && argv[1] != NULL && *argv[1] != '\0');

  memset(&srvs, 0, sizeof(srvs));

  /* Indicate that we have started */

  g_ntpc_daemon.state = NTP_RUNNING;
  sem_post(&g_ntpc_daemon.sync);

  /* Here we do the communication with the NTP server. We collect set of
   * NTP samples (hopefully from different servers when using DNS) and
   * select median time-offset of samples. This is to filter out
   * misconfigured server giving wrong timestamps.
   *
   * NOTE that the scheduler is locked whenever this loop runs.  That
   * assures both:  (1) that there are no asynchronous stop requests and
   * (2) that we are not suspended while in critical moments when we about
   * to set the new time.  This sounds harsh, but this function is suspended
   * most of the time either: (1) sending a datagram, (2) receiving a
   * datagram, or (3) waiting for the next poll cycle.
   *
   * TODO: The first datagram that is sent is usually lost.  That is because
   * the MAC address of the NTP server is not in the ARP table.  This is
   * particularly bad here because the request will not be sent again until
   * the long delay expires leaving the system with bad time for a long time
   * initially.  Solutions:
   *
   * 1. Fix send logic so that it assures that the ARP request has been
   *    sent and the entry is in the ARP table before sending the packet
   *    (best).
   * 2. Add some ad hoc logic here so that there is no delay until at least
   *    one good time is received.
   */

  sched_lock();
  while (g_ntpc_daemon.state != NTP_STOP_REQUESTED)
    {
      struct timespec start_realtime, start_monotonic;
      int errval = 0;
      int i;

      free(srvs.hostlist_str);
      memset(&srvs, 0, sizeof(srvs));
      srvs.ntp_servers = argv[1];

      memset(samples, 0, sizeof(samples));

      clock_gettime(CLOCK_REALTIME, &start_realtime);
#ifdef CONFIG_CLOCK_MONOTONIC
      clock_gettime(CLOCK_MONOTONIC, &start_monotonic);
#endif

      /* Collect samples. */

      nsamples = 0;
      for (i = 0; i < CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES; i++)
        {
          /* Get next sample. */

          ret = ntpc_get_ntp_sample(&srvs, samples, nsamples);
          if (ret < 0)
            {
              errval = errno;
            }
          else
            {
              ++nsamples;
            }
        }

      /* Analyse samples. */

      if (nsamples > 0)
        {
          int64_t offset;

          /* Select median offset of samples. */

          qsort(samples, nsamples, sizeof(*samples), sample_cmp);

          for (i = 0; i < nsamples; i++)
            {
              ninfo("NTP sample[%d]: offset: %s%lu.%03lu sec, "
                    "round-trip delay: %s%lu.%03lu sec\n",
                    i,
                    samples[i].offset < 0 ? "-" : "",
                    (unsigned long)ntp_secpart(int64abs(samples[i].offset)),
                    ntp_nsecpart(int64abs(samples[i].offset))
                      / NSEC_PER_MSEC,
                    samples[i].delay < 0 ? "-" : "",
                    (unsigned long)ntp_secpart(int64abs(samples[i].delay)),
                    ntp_nsecpart(int64abs(samples[i].delay))
                      / NSEC_PER_MSEC);
            }

          if ((nsamples % 2) == 1)
            {
              offset = samples[nsamples / 2].offset;
            }
          else
            {
              int64_t offset1 = samples[nsamples / 2].offset;
              int64_t offset2 = samples[nsamples / 2 - 1].offset;

              /* Average of two middle offsets. */

              if (offset1 > 0 && offset2 > 0)
                {
                  offset = ((uint64_t)offset1 + (uint64_t)offset2) / 2;
                }
              else if (offset1 < 0 && offset2 < 0)
                {
                  offset1 = -offset1;
                  offset2 = -offset2;

                  offset = ((uint64_t)offset1 + (uint64_t)offset2) / 2;

                  offset = -offset;
                }
              else
                {
                  offset = (offset1 + offset2) / 2;
                }
            }

          /* Adjust system time. */

          ntpc_settime(offset, &start_realtime, &start_monotonic);

          /* Save samples for ntpc_status() */

          sem_wait(&g_ntpc_daemon.lock);
          g_last_nsamples = nsamples;
          memcpy(&g_last_samples, samples, nsamples * sizeof(*samples));
          sem_post(&g_ntpc_daemon.lock);

#ifndef CONFIG_NETUTILS_NTPCLIENT_STAY_ON
          /* Configured to exit at success. */

          exitcode = EXIT_SUCCESS;
          break;

#else

          /* A full implementation of an NTP client would require much
           * more.  I think we can skip most of that here.
           */

          if (g_ntpc_daemon.state == NTP_RUNNING)
            {
              ninfo("Waiting for %d seconds\n",
                    CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);

              sleep(CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC);
              retries = 0;
            }

          continue;
#endif
        }

      /* Exceeded maximum retries? */

      if (retries++ >= CONFIG_NETUTILS_NTPCLIENT_RETRIES)
        {
          nerr("ERROR: too many retries: %d\n", retries - 1);
          exitcode = EXIT_FAILURE;
          break;
        }

      /* Is this error a signal? If not, sleep before retry. */

      if (errval != EINTR)
        {
          sleep(1);
        }

      /* Keep retrying. */
    }

  /* The NTP client is terminating */

  sched_unlock();

  g_ntpc_daemon.state = NTP_STOPPED;
  sem_post(&g_ntpc_daemon.sync);
  free(srvs.hostlist_str);
  return exitcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ntpc_dualstack_family()
 *
 * Description:
 *   Set the protocol family used (AF_INET, AF_INET6 or AF_UNSPEC)
 *
 ****************************************************************************/

void ntpc_dualstack_family(int family)
{
  sem_wait(&g_ntpc_daemon.lock);
  g_ntpc_daemon.family = family;
  sem_post(&g_ntpc_daemon.lock);
}

/****************************************************************************
 * Name: ntpc_start_with_list
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start_with_list(FAR const char *ntp_server_list)
{
  FAR char *task_argv[] =
    {
      (FAR char *)ntp_server_list,
      NULL
    };

  /* Is the NTP in a non-running state? */

  sem_wait(&g_ntpc_daemon.lock);
  if (g_ntpc_daemon.state == NTP_NOT_RUNNING ||
      g_ntpc_daemon.state == NTP_STOPPED)
    {
      /* Start the NTP daemon */

      g_ntpc_daemon.state = NTP_STARTED;
      g_ntpc_daemon.pid =
        task_create("NTP daemon", CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO,
                    CONFIG_NETUTILS_NTPCLIENT_STACKSIZE, ntpc_daemon,
                    task_argv);

      /* Handle failures to start the NTP daemon */

      if (g_ntpc_daemon.pid < 0)
        {
          int errval = errno;
          DEBUGASSERT(errval > 0);

          g_ntpc_daemon.state = NTP_STOPPED;
          nerr("ERROR: Failed to start the NTP daemon: %d\n", errval);
          sem_post(&g_ntpc_daemon.lock);
          return -errval;
        }

      /* Wait for any daemon state change */

      do
        {
          sem_wait(&g_ntpc_daemon.sync);
        }
      while (g_ntpc_daemon.state == NTP_STARTED);
    }

  sem_post(&g_ntpc_daemon.lock);
  return g_ntpc_daemon.pid;
}

/****************************************************************************
 * Name: ntpc_start
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start(void)
{
  return ntpc_start_with_list(CONFIG_NETUTILS_NTPCLIENT_SERVER);
}

/****************************************************************************
 * Name: ntpc_stop
 *
 * Description:
 *   Stop the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.  The current
 *   implementation only returns success.
 *
 ****************************************************************************/

int ntpc_stop(void)
{
  int ret;

  /* Is the NTP in a running state? */

  sem_wait(&g_ntpc_daemon.lock);
  if (g_ntpc_daemon.state == NTP_STARTED ||
      g_ntpc_daemon.state == NTP_RUNNING)
    {
      /* Yes.. request that the daemon stop. */

      g_ntpc_daemon.state = NTP_STOP_REQUESTED;

      /* Wait for any daemon state change */

      do
        {
          /* Signal the NTP client */

          ret = kill(g_ntpc_daemon.pid,
                     CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP);

          if (ret < 0)
            {
              nerr("ERROR: kill pid %d failed: %d\n",
                   g_ntpc_daemon.pid, errno);
              break;
            }

          /* Wait for the NTP client to respond to the stop request */

          sem_wait(&g_ntpc_daemon.sync);
        }
      while (g_ntpc_daemon.state == NTP_STOP_REQUESTED);
    }

  sem_post(&g_ntpc_daemon.lock);
  return OK;
}

int ntpc_status(struct ntpc_status_s *statusp)
{
  unsigned int i;

  sem_wait(&g_ntpc_daemon.lock);
  statusp->nsamples = g_last_nsamples;
  for (i = 0; i < g_last_nsamples; i++)
    {
      statusp->samples[i].offset = g_last_samples[i].offset;
      statusp->samples[i].delay = g_last_samples[i].delay;
      statusp->samples[i]._srv_addr_store = g_last_samples[i].srv_addr.ss;
      statusp->samples[i].srv_addr = (FAR const struct sockaddr *)
                                     &statusp->samples[i]._srv_addr_store;
    }

  sem_post(&g_ntpc_daemon.lock);
  return OK;
}
