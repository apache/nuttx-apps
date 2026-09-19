/****************************************************************************
 * apps/netutils/ptpd/ptpd.c
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

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/timex.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <nuttx/debug.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <netutils/ipmsfilter.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <nuttx/clock.h>
#include <nuttx/net/netconfig.h>
#include <netutils/ptpd.h>

#include "netutils/netlib.h"
#include "ptpv2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of consecutive missing hardware TX timestamps after which the
 * driver is assumed not to provide them and software timestamps are used.
 */

#define PTP_HWTS_TX_MAX_FAILURES 3

#if CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS > 0
/* Outlier rejection of the measured phase error: number of recent samples
 * the median is taken over, the least number of samples needed before
 * anything is rejected, and how many samples in a row can be rejected
 * before they are taken as a real change of the phase.
 */

#  define PTP_OUTLIER_HISTORY         5
#  define PTP_OUTLIER_MIN_HISTORY     3
#  define PTP_OUTLIER_MAX_CONSECUTIVE 8
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

#ifdef CONFIG_BUILD_FLAT
/* Carrier structure for querying PTPD status in flat build mode */

struct ptpd_statusreq_s
{
  sem_t done;
  struct ptpd_status_s dest;
};
#endif

/* Main PTPD state storage */

struct ptp_state_s
{
  /* Request for PTPD task to stop or dump status */

  bool stop;
#ifdef CONFIG_BUILD_FLAT
  FAR struct ptpd_statusreq_s *status_req;  /* Set by SIGUSR1 */
#else
  bool dump;                     /* Set by SIGUSR1, checked in main loop */
#endif

  /* Address of network interface we are operating on */

  struct sockaddr_in interface_addr;

  /* Socket bound to interface for transmission */

  int tx_socket;

  /* Hardware TX timestamp retrieval: consecutive failures, and whether it
   * was given up on because the driver does not provide the timestamps.
   */

  unsigned int hwts_tx_failures;
  bool hwts_tx_disabled;

  /* Sockets for PTP event and information ports */

  int event_socket;
  int info_socket;

  /* The ptp device file descriptor */

  clockid_t clockid;

  /* Our own identity as a clock source */

  struct ptp_announce_s own_identity;

  /* Sequence number counters per message type */

  uint16_t announce_seq;
  uint16_t sync_seq;
  uint16_t delay_req_seq;
  uint16_t pdelay_req_seq;

  /* Previous measurement and estimated clock drift rate */

  struct timespec last_delta_timestamp;
  int64_t last_delta_ns;
  int64_t last_adjtime_ns;
  long drift_avg_total_ms;
  long drift_ppb;
  bool has_last_delta;
#if CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS > 0
  int64_t delta_hist[PTP_OUTLIER_HISTORY];
  unsigned int delta_hist_count;
  unsigned int delta_hist_next;
  unsigned int outlier_count;
#endif

  /* Identity of currently selected clock source,
   * from the latest announcement message.
   *
   * The timestamps are used for timeout when a source disappears.
   * They are from the local CLOCK_MONOTONIC.
   */

  bool selected_source_valid;              /* True if operating as client */
  struct ptp_announce_s selected_source;   /* Currently selected server */
  struct timespec last_received_multicast; /* Any multicast packet */
  struct timespec last_received_announce;  /* Announce from any server */
  struct timespec last_received_sync;      /* Sync from selected source */

  /* Last transmitted packet timestamps (CLOCK_MONOTONIC)
   * Used to set transmission interval.
   */

  struct timespec last_transmitted_sync;
  struct timespec last_transmitted_announce;
  struct timespec last_transmitted_delayresp;
  struct timespec last_transmitted_delayreq;
  struct timespec last_transmitted_pdelayreq;

  /* Timestamps related to path delay calculation (CLOCK_REALTIME) */

  bool can_send_delayreq;
  struct timespec delayreq_time;
  int path_delay_avgcount;
  long path_delay_ns;
  long delayreq_interval;
  int64_t sync_diff_ns;
  bool sync_diff_valid;

  /* Timestamps related to P2P peer delay calculation (CLOCK_REALTIME) */

  struct timespec pdelayreq_tx_time;  /* t1 */
  struct timespec pdelayreq_rx_time;  /* t2 */
  struct timespec pdelayresp_rx_time; /* t4 */
  bool pdelay_waiting_followup;

  /* Latest received packet and its timestamp (CLOCK_REALTIME) */

  struct timespec rxtime;
  union
  {
    struct ptp_header_s                header;
    struct ptp_announce_s              announce;
    struct ptp_sync_s                  sync;
    struct ptp_follow_up_s             follow_up;
    struct ptp_delay_req_s             delay_req;
    struct ptp_delay_resp_s            delay_resp;
    struct ptp_pdelay_req_s            pdelay_req;
    struct ptp_pdelay_resp_s           pdelay_resp;
    struct ptp_pdelay_resp_follow_up_s pdelay_resp_fup;
    uint8_t                            raw[128];
  } rxbuf;

  uint8_t rxcmsg[CMSG_LEN(sizeof(struct timespec))];

  /* Buffered sync packet for two-step clock setting where server sends
   * the accurate timestamp in a separate follow-up message.
   */

  struct ptp_sync_s twostep_packet;
  struct timespec twostep_rxtime;
  FAR const struct ptpd_config_s *config;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifdef CONFIG_BUILD_FLAT
/* The status request of ptpd_status(). The daemon keeps its address until it
 * answers, which can be after ptpd_status() gave up waiting and returned, so
 * it lives in static memory and never on the stack of the caller. The lock
 * lets only one caller use it at a time.
 */

static struct ptpd_statusreq_s g_statusreq =
{
  SEM_INITIALIZER(0)
};

static pthread_mutex_t g_statusreq_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Convert from timespec to PTP format */

static void timespec_to_ptp_format(FAR const struct timespec *ts,
                                   FAR uint8_t *timestamp)
{
  /* IEEE 1588 uses 48 bits for seconds and 32 bits for nanoseconds,
   * both fields big-endian.
   */

  timestamp[0] = (uint8_t)(ts->tv_sec >> 40);
  timestamp[1] = (uint8_t)(ts->tv_sec >> 32);
  timestamp[2] = (uint8_t)(ts->tv_sec >> 24);
  timestamp[3] = (uint8_t)(ts->tv_sec >> 16);
  timestamp[4] = (uint8_t)(ts->tv_sec >>  8);
  timestamp[5] = (uint8_t)(ts->tv_sec >>  0);

  timestamp[6] = (uint8_t)(ts->tv_nsec >>  24);
  timestamp[7] = (uint8_t)(ts->tv_nsec >>  16);
  timestamp[8] = (uint8_t)(ts->tv_nsec >>   8);
  timestamp[9] = (uint8_t)(ts->tv_nsec >>   0);
}

/* Convert from PTP format to timespec */

static void ptp_format_to_timespec(FAR const uint8_t *timestamp,
                                   FAR struct timespec *ts)
{
  ts->tv_sec =
      (((int64_t)timestamp[0]) << 40)
    | (((int64_t)timestamp[1]) << 32)
    | (((int64_t)timestamp[2]) << 24)
    | (((int64_t)timestamp[3]) << 16)
    | (((int64_t)timestamp[4]) <<  8)
    | (((int64_t)timestamp[5]) <<  0);

  ts->tv_nsec =
      (((long)timestamp[6]) << 24)
    | (((long)timestamp[7]) << 16)
    | (((long)timestamp[8]) <<  8)
    | (((long)timestamp[9]) <<  0);
}

/* Returns true if A is a better clock source than B.
 * Implements Best Master Clock algorithm from IEEE-1588.
 */

static bool is_better_clock(FAR const struct ptp_announce_s *a,
                            FAR const struct ptp_announce_s *b)
{
  /* Main priority field */

  if (a->gm_priority1 < b->gm_priority1)
    {
      return true;
    }

  if (a->gm_priority1 > b->gm_priority1)
    {
      return false;
    }

  /* Clock class */

  if (a->gm_quality[0] < b->gm_quality[0])
    {
      return true;
    }

  if (a->gm_quality[0] > b->gm_quality[0])
    {
      return false;
    }

  /* Clock accuracy */

  if (a->gm_quality[1] < b->gm_quality[1])
    {
      return true;
    }

  if (a->gm_quality[1] > b->gm_quality[1])
    {
      return false;
    }

  /* Clock variance high byte */

  if (a->gm_quality[2] < b->gm_quality[2])
    {
      return true;
    }

  if (a->gm_quality[2] > b->gm_quality[2])
    {
      return false;
    }

  /* Clock variance low byte */

  if (a->gm_quality[3] < b->gm_quality[3])
    {
      return true;
    }

  if (a->gm_quality[3] > b->gm_quality[3])
    {
      return false;
    }

  /* Sub priority field */

  if (a->gm_priority2 < b->gm_priority2)
    {
      return true;
    }

  if (a->gm_priority2 > b->gm_priority2)
    {
      return false;
    }

  return memcmp(a->gm_identity, b->gm_identity, sizeof(a->gm_identity)) < 0;
}

static int64_t timespec_to_ms(FAR const struct timespec *ts)
{
  return ts->tv_sec * MSEC_PER_SEC + (ts->tv_nsec / NSEC_PER_MSEC);
}

/* Add a positive or negative number of nanoseconds to a timespec value. */

static void timespec_add_ns(FAR struct timespec *ts, int64_t ns)
{
  int64_t total = ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec + ns;

  ts->tv_sec  = total / NSEC_PER_SEC;
  ts->tv_nsec = total % NSEC_PER_SEC;

  if (ts->tv_nsec < 0)
    {
      ts->tv_sec--;
      ts->tv_nsec += NSEC_PER_SEC;
    }
}

/* Get positive or negative delta between two timespec values.
 * If value would exceed int64 limit (292 years), return INT64_MAX/MIN.
 */

static int64_t timespec_delta_ns(FAR const struct timespec *ts1,
                                 FAR const struct timespec *ts2)
{
  int64_t delta_s;

  delta_s = ts1->tv_sec - ts2->tv_sec;

  /* Conversion to nanoseconds could overflow if the system time is 64-bit */

  if (delta_s >= INT64_MAX / NSEC_PER_SEC)
    {
      return INT64_MAX;
    }
  else if (delta_s <= INT64_MIN / NSEC_PER_SEC)
    {
      return INT64_MIN;
    }

  return delta_s * NSEC_PER_SEC + (ts1->tv_nsec - ts2->tv_nsec);
}

/* Check if the currently selected source is still valid */

static bool is_selected_source_valid(FAR struct ptp_state_s *state)
{
  struct timespec time_now;
  struct timespec delta;

  if ((state->selected_source.header.messagetype & PTP_MSGTYPE_MASK)
      != PTP_MSGTYPE_ANNOUNCE)
    {
      return false; /* Uninitialized value */
    }

  /* Note: this uses monotonic clock to track the timeout even when
   *       system clock is adjusted.
   */

  clock_gettime(CLOCK_MONOTONIC, &time_now);
  clock_timespec_subtract(&time_now, &state->last_received_sync, &delta);

  if (timespec_to_ms(&delta) > CONFIG_NETUTILS_PTPD_TIMEOUT_MS)
    {
      return false; /* Too long time since received packet */
    }

  return true;
}

/* Increment sequence number for packet type, and copy to header */

static void ptp_increment_sequence(FAR uint16_t *sequence_num,
                                   FAR struct ptp_header_s *hdr)
{
  *sequence_num += 1;
  hdr->sequenceid[0] = (uint8_t)(*sequence_num >> 8);
  hdr->sequenceid[1] = (uint8_t)(*sequence_num);
}

/* Get sequence number from received packet */

static uint16_t ptp_get_sequence(FAR const struct ptp_header_s *hdr)
{
  return ((uint16_t)hdr->sequenceid[0] << 8) | hdr->sequenceid[1];
}

static clockid_t ptp_open(FAR const char *clock)
{
  int fd;

  if (!strcmp(clock, "realtime"))
    {
      return CLOCK_REALTIME;
    }

  fd = open(clock, O_RDWR | O_CLOEXEC);
  if (fd < 0)
    {
      ptperr("Failed to open PTP clock device:%s, %d\n", clock, errno);
      return fd;
    }

  return (fd << CLOCK_SHIFT) | CLOCK_FD;
}

static void ptp_close(clockid_t clockid)
{
  if (clockid > 0 && clockid != CLOCK_REALTIME)
    {
      close(clockid >> CLOCK_SHIFT);
    }
}

static int ptp_gettime(FAR struct ptp_state_s *state,
                       FAR struct timespec *ts)
{
  return clock_gettime(state->clockid, ts);
}

/* Change current system timestamp by jumping */

static int ptp_settime(FAR struct ptp_state_s *state,
                       FAR struct timespec *ts)
{
  return clock_settime(state->clockid, ts);
}

/* Smoothly adjust timestamp. */

static int ptp_adjtime(FAR struct ptp_state_s *state, int64_t delta_ns,
                       int64_t ppb)
{
  if (state->clockid == CLOCK_REALTIME)
    {
      struct timeval delta;

      delta.tv_sec = delta_ns / NSEC_PER_SEC;
      delta_ns -= delta.tv_sec * NSEC_PER_SEC;
      delta.tv_usec = delta_ns / NSEC_PER_USEC;
      return adjtime(&delta, NULL);
    }
  else
    {
      struct timex buf;
      int64_t hw_ppb;
      const int64_t slew_limit_ppb =
        CONFIG_CLOCK_ADJTIME_SLEWLIMIT_PPM * 1000;

      /* delta_ns passed here is adjustment_ns, which already
       * combines frequency drift and current phase error clamped
       * to max_adjust_ns. Converting it to ppb over
       * CONFIG_CLOCK_ADJTIME_PERIOD_MS produces the rate needed to
       * pull the hardware counter into phase lock.
       */

      hw_ppb = delta_ns * MSEC_PER_SEC /
               CONFIG_CLOCK_ADJTIME_PERIOD_MS;

      if (hw_ppb > slew_limit_ppb)
        {
          hw_ppb = slew_limit_ppb;
        }
      else if (hw_ppb < -slew_limit_ppb)
        {
          hw_ppb = -slew_limit_ppb;
        }

      memset(&buf, 0, sizeof(buf));
      buf.freq = hw_ppb * 65536 / 1000;
      buf.modes = ADJ_FREQUENCY;

      return clock_adjtime(state->clockid, &buf);
    }
}

/* Get timestamp of latest received packet */

static int ptp_getrxtime(FAR struct ptp_state_s *state,
                         FAR struct msghdr *rxhdr,
                         FAR struct timespec *ts)
{
  FAR struct cmsghdr *cmsg;

  /* Get hardware or kernel timestamp if available */

  if (!state->config->hardware_ts)
    {
      return ptp_gettime(state, ts);
    }

  for_each_cmsghdr(cmsg, rxhdr)
    {
      if (cmsg->cmsg_level == SOL_SOCKET &&
          cmsg->cmsg_type == SO_TIMESTAMPNS &&
          cmsg->cmsg_len == CMSG_LEN(sizeof(struct timespec)))
        {
          memcpy(ts, CMSG_DATA(cmsg), sizeof(*ts));

          /* Sanity-check the value */

          if (ts->tv_sec > 0 || ts->tv_nsec > 0)
            {
              /* The MAC latches the timestamp later than the frame
               * reaches the wire: compensate the ingress latency.
               */

              timespec_add_ns(ts, -state->config->ingress_latency_ns);
              return OK;
            }
        }
    }

  ptpwarn("CONFIG_NET_TIMESTAMP enabled but did not get packet timestamp\n");
  return ERROR;
}

/* Unsubscribe multicast and destroy sockets */

static int ptp_destroy_state(FAR struct ptp_state_s *state)
{
  struct in_addr mcast_addr;

  ptp_close(state->clockid);

  if (state->config->af == AF_INET)
    {
      mcast_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
      ipmsfilter(&state->interface_addr.sin_addr,
                 &mcast_addr, MCAST_EXCLUDE);

      if (state->config->delay_mechanism == PTP_DELAY_P2P)
        {
          mcast_addr.s_addr = HTONL(PTP_PDELAY_MULTICAST_ADDR);
          ipmsfilter(&state->interface_addr.sin_addr,
                     &mcast_addr, MCAST_EXCLUDE);
        }
    }

  if (state->tx_socket > 0)
    {
      close(state->tx_socket);
      state->tx_socket = -1;
    }

  if (state->event_socket > 0)
    {
      close(state->event_socket);
      state->event_socket = -1;
    }

  if (state->info_socket > 0)
    {
      close(state->info_socket);
      state->info_socket = -1;
    }

  return OK;
}

/* Initialize PTP client/server state and create sockets */

static int ptp_initialize_state(FAR struct ptp_state_s *state)
{
  int ret;
  int arg = 1;
  struct ifreq req;

  state->clockid = ptp_open(state->config->clock);
  if (state->clockid < 0)
    {
      ptperr("Invalid clockid %d for ptp daemon\n", state->clockid);
      return ERROR;
    }

  /* Create sockets */

  if (state->config->af == AF_PACKET)
    {
      struct sockaddr_ll addr;

      state->tx_socket = socket(AF_PACKET, SOCK_RAW, 0);
      if (state->tx_socket < 0)
        {
          ptperr("Failed to create tx socket: %d\n", errno);
          goto errout;
        }

      addr.sll_family = AF_PACKET;
      addr.sll_ifindex = if_nametoindex(state->config->interface);
      addr.sll_protocol = htons(ETHERTYPE_PTP);
      ret = bind(state->tx_socket, (FAR struct sockaddr *)&addr,
                 sizeof(addr));
      if (ret < 0)
        {
          ptperr("ERROR: binding socket failed: %d\n", errno);
          goto errout;
        }

      state->event_socket = dup(state->tx_socket);
      if (state->event_socket < 0)
        {
          ptperr("Failed to dup event socket: %d\n", errno);
          goto errout;
        }

      state->info_socket = -1;
    }
  else if (state->config->af == AF_INET)
    {
      struct sockaddr_in bind_addr;

      state->tx_socket = socket(AF_INET, SOCK_DGRAM, 0);
      if (state->tx_socket < 0)
        {
          ptperr("Failed to create tx socket: %d\n", errno);
          goto errout;
        }

      state->event_socket = socket(AF_INET, SOCK_DGRAM, 0);
      if (state->event_socket < 0)
        {
          ptperr("Failed to create event socket: %d\n", errno);
          goto errout;
        }

      state->info_socket = socket(AF_INET, SOCK_DGRAM, 0);
      if (state->info_socket < 0)
        {
          ptperr("Failed to create info socket: %d\n", errno);
          goto errout;
        }

      /* Bind socket for events to PTP multicast address */

      bind_addr.sin_family = AF_INET;
      bind_addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
      bind_addr.sin_port = HTONS(PTP_UDP_PORT_EVENT);
      ret = bind(state->event_socket, (FAR struct sockaddr *)&bind_addr,
                 sizeof(bind_addr));
      if (ret < 0)
        {
          ptperr("Failed to bind to udp port %d\n", bind_addr.sin_port);
          goto errout;
        }

      /* Bind socket for announcements */

      bind_addr.sin_port = HTONS(PTP_UDP_PORT_INFO);
      ret = bind(state->info_socket, (FAR struct sockaddr *)&bind_addr,
                 sizeof(bind_addr));
      if (ret < 0)
        {
          ptperr("Failed to bind to udp port %d\n", bind_addr.sin_port);
          goto errout;
        }

      /* Bind TX socket to interface address (local addr cannot be
       * multicast)
       */

      bind_addr.sin_addr = state->interface_addr.sin_addr;
      ret = bind(state->tx_socket, (FAR struct sockaddr *)&bind_addr,
                 sizeof(bind_addr));
      if (ret < 0)
        {
          ptperr("Failed to bind tx to port %d\n", bind_addr.sin_port);
          goto errout;
        }
    }

  if (state->config->hardware_ts)
    {
      ret = setsockopt(state->event_socket, SOL_SOCKET, SO_TIMESTAMPNS,
                       &arg, sizeof(arg));

      if (ret < 0)
        {
          ptperr("Failed to enable SO_TIMESTAMPNS: %s\n", strerror(errno));
          goto errout;
        }
    }

  /* Get address information of the specified interface for binding socket
   * Only supports IPv4 currently.
   */

  memset(&req, 0, sizeof(req));
  strlcpy(req.ifr_name, state->config->interface, sizeof(req.ifr_name));

  if (ioctl(state->event_socket, SIOCGIFADDR, (unsigned long)&req) < 0)
    {
      ptperr("Failed to get IP address information for interface %s\n",
             state->config->interface);
      goto errout;
    }

  state->interface_addr = *(FAR struct sockaddr_in *)&req.ifr_ifru.ifru_addr;

  /* Subscribe to PTP multicast address (AF_INET only).
   * Must be done after interface_addr is populated so the IGMP join
   * can locate the correct network device.
   */

  if (state->config->af == AF_INET)
    {
      struct in_addr mcast_addr;

      mcast_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
      ret = ipmsfilter(&state->interface_addr.sin_addr,
                       &mcast_addr, MCAST_INCLUDE);
      if (ret < 0)
        {
          ptperr("Failed to join multicast group: %d\n", errno);
          goto errout;
        }

      if (state->config->delay_mechanism == PTP_DELAY_P2P)
        {
          mcast_addr.s_addr = HTONL(PTP_PDELAY_MULTICAST_ADDR);
          ret = ipmsfilter(&state->interface_addr.sin_addr,
                           &mcast_addr, MCAST_INCLUDE);
          if (ret < 0)
            {
              ptperr("Failed to join peer delay multicast group: %d\n",
                     errno);
              goto errout;
            }
        }
    }

  /* Get hardware address to initialize the identity field in header.
   * Clock identity is EUI-64, which we make from EUI-48.
   */

  if (ioctl(state->event_socket, SIOCGIFHWADDR, (unsigned long)&req) < 0)
    {
      ptperr("Failed to get HW address information for interface %s\n",
             state->config->interface);
      goto errout;
    }

  state->own_identity.header.version = PTP_VERSION_2_0;
  state->own_identity.header.domain = CONFIG_NETUTILS_PTPD_DOMAIN;
  state->own_identity.header.controlfield = 0x05;
  state->own_identity.header.sourceidentity[0] = req.ifr_hwaddr.sa_data[0];
  state->own_identity.header.sourceidentity[1] = req.ifr_hwaddr.sa_data[1];
  state->own_identity.header.sourceidentity[2] = req.ifr_hwaddr.sa_data[2];
  state->own_identity.header.sourceidentity[3] = 0xff;
  state->own_identity.header.sourceidentity[4] = 0xfe;
  state->own_identity.header.sourceidentity[5] = req.ifr_hwaddr.sa_data[3];
  state->own_identity.header.sourceidentity[6] = req.ifr_hwaddr.sa_data[4];
  state->own_identity.header.sourceidentity[7] = req.ifr_hwaddr.sa_data[5];
  state->own_identity.header.sourceportindex[0] = 0;
  state->own_identity.header.sourceportindex[1] = 1;
  state->own_identity.gm_priority1 = CONFIG_NETUTILS_PTPD_PRIORITY1;
  state->own_identity.gm_quality[0] = CONFIG_NETUTILS_PTPD_CLASS;
  state->own_identity.gm_quality[1] = CONFIG_NETUTILS_PTPD_ACCURACY;
  state->own_identity.gm_quality[2] = 0xff; /* No variance estimate */
  state->own_identity.gm_quality[3] = 0xff;
  state->own_identity.gm_priority2 = CONFIG_NETUTILS_PTPD_PRIORITY2;
  memcpy(state->own_identity.gm_identity,
         state->own_identity.header.sourceidentity,
         sizeof(state->own_identity.gm_identity));
  state->own_identity.timesource = CONFIG_NETUTILS_PTPD_CLOCKSOURCE;

  state->delayreq_interval = 1;
  clock_gettime(CLOCK_MONOTONIC, &state->last_received_multicast);

  return OK;

errout:
  ptp_destroy_state(state);
  return ERROR;
}

/* Re-subscribe multicast address.
 * This can become necessary if Ethernet interface gets reset or if external
 * IGMP-compliant Ethernet switch gets plugged in.
 */

static int ptp_check_multicast_status(FAR struct ptp_state_s *state)
{
#if CONFIG_NETUTILS_PTPD_MULTICAST_TIMEOUT_MS > 0
  struct in_addr mcast_addr;
  struct timespec time_now;
  struct timespec delta;
  int ret;

  if (state->config->af != AF_INET)
    {
      return OK;
    }

  clock_gettime(CLOCK_MONOTONIC, &time_now);
  clock_timespec_subtract(&time_now, &state->last_received_multicast,
                          &delta);

  if (timespec_to_ms(&delta) > CONFIG_NETUTILS_PTPD_MULTICAST_TIMEOUT_MS)
    {
      /* Remove and re-add the multicast group */

      state->last_received_multicast = time_now;

      mcast_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
      ipmsfilter(&state->interface_addr.sin_addr,
                 &mcast_addr,
                 MCAST_EXCLUDE);

      ret = ipmsfilter(&state->interface_addr.sin_addr,
                       &mcast_addr,
                       MCAST_INCLUDE);

      if (state->config->delay_mechanism == PTP_DELAY_P2P)
        {
          mcast_addr.s_addr = HTONL(PTP_PDELAY_MULTICAST_ADDR);
          ipmsfilter(&state->interface_addr.sin_addr,
                     &mcast_addr,
                     MCAST_EXCLUDE);

          ret = ipmsfilter(&state->interface_addr.sin_addr,
                           &mcast_addr,
                           MCAST_INCLUDE);
        }

      return ret;
    }

#else
  UNUSED(state);
#endif /* CONFIG_NETUTILS_PTPD_MULTICAST_TIMEOUT_MS */

  return OK;
}

#ifdef CONFIG_NET_TIMESTAMP
/****************************************************************************
 * Name: ptp_get_tx_timestamp
 *
 * Description:
 *   Retrieve the hardware TX timestamp delivered via MSG_ERRQUEUE on the
 *   socket after transmission.
 *
 * Input Parameters:
 *   state - Pointer to PTP daemon state
 *   tx_ts - Location to return the hardware timestamp
 *
 * Returned Value:
 *   OK on success; ERROR on failure or timeout.
 *
 ****************************************************************************/

static int ptp_get_tx_timestamp(FAR struct ptp_state_s *state,
                                FAR struct timespec *tx_ts)
{
  struct pollfd pfd;
  int ret;

  pfd.fd = state->tx_socket;
  pfd.events = POLLPRI;
  pfd.revents = 0;

  ret = poll(&pfd, 1, 500);
  if (ret > 0 && (pfd.revents & (POLLPRI | POLLERR)) != 0)
    {
      char errbuf[128];
      char cmsgbuf[128];
      struct msghdr msg;
      struct iovec iov;
      FAR struct cmsghdr *cmsg;
      ssize_t n;

      memset(&msg, 0, sizeof(msg));
      iov.iov_base = errbuf;
      iov.iov_len = sizeof(errbuf);
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      msg.msg_control = cmsgbuf;
      msg.msg_controllen = sizeof(cmsgbuf);

      n = recvmsg(state->tx_socket, &msg, MSG_ERRQUEUE);
      if (n >= 0)
        {
          for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL;
               cmsg = CMSG_NXTHDR(&msg, cmsg))
            {
              if (cmsg->cmsg_level == SOL_SOCKET &&
                  cmsg->cmsg_type == SO_TIMESTAMPING)
                {
                  FAR struct timespec *ts =
                    (FAR struct timespec *)CMSG_DATA(cmsg);

                  *tx_ts = ts[2];
                  return OK;
                }
            }

          ptpwarn("PTP TX HWTS: recvmsg %zd B without SO_TIMESTAMPING\n",
                  n);
        }
      else
        {
          ptpwarn("PTP TX HWTS: recvmsg MSG_ERRQUEUE failed errno=%d\n",
                  errno);
        }
    }
  else
    {
      ptpwarn("PTP TX HWTS: poll ret=%d revents=0x%04x errno=%d\n",
              ret, pfd.revents, errno);
    }

  return ERROR;
}
#endif

static int ptp_sendmsg(FAR struct ptp_state_s *state, FAR const void *buf,
                       size_t buflen, FAR const void *addr,
                       socklen_t addrlen, FAR struct timespec *sendts)
{
  int ret;
  struct timespec sw_ts;
#ifdef CONFIG_NET_TIMESTAMP
  bool do_hwts = (sendts != NULL && state->config->hardware_ts &&
                  !state->hwts_tx_disabled &&
                  state->config->af == AF_PACKET);
#endif

  if (sendts != NULL)
    {
      ptp_gettime(state, &sw_ts);
    }

  if (state->config->af == AF_PACKET)
    {
      /* IEEE 1588-2008 Annex F multicast MAC addresses */

      const uint8_t ptp_multicast_mac[ETHER_ADDR_LEN] =
        PTP_MULTICAST_MAC;
      const uint8_t ptp_pdelay_multicast_mac[ETHER_ADDR_LEN] =
        PTP_PDELAY_MULTICAST_MAC;
      FAR const struct ptp_header_s *hdr = buf;
      FAR const uint8_t *dst_mac;
      char raw[sizeof(struct ether_header) + sizeof(struct ptp_announce_s)];
      FAR struct ether_header *header;
      struct msghdr msg;
      struct iovec iov;
      uint8_t msgtype;

      DEBUGASSERT(sizeof(struct ptp_announce_s) >= buflen);

      msgtype = hdr->messagetype & PTP_MSGTYPE_MASK;
      if (msgtype == PTP_MSGTYPE_PDELAY_REQ ||
          msgtype == PTP_MSGTYPE_PDELAY_RESP ||
          msgtype == PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP)
        {
          dst_mac = ptp_pdelay_multicast_mac;
        }
      else
        {
          dst_mac = ptp_multicast_mac;
        }

      header = (FAR struct ether_header *)&raw;
      memcpy(header->ether_dhost, dst_mac, ETHER_ADDR_LEN);
      netlib_getmacaddr(state->config->interface, header->ether_shost);
      header->ether_type = htons(ETHERTYPE_PTP);
      memcpy(&raw[sizeof(*header)], buf, buflen);
      buflen += sizeof(*header);

      iov.iov_base = raw;
      iov.iov_len = buflen;

      /* For AF_PACKET SOCK_RAW, msg_name must be NULL as destination
       * is specified in the Ethernet frame header.
       */

      msg.msg_name = NULL;
      msg.msg_namelen = 0;
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      msg.msg_flags = 0;
      msg.msg_control = NULL;
      msg.msg_controllen = 0;

#ifdef CONFIG_NET_TIMESTAMP
      if (do_hwts)
        {
          char drainbuf[128];
          char draincmsg[128];
          struct msghdr drainmsg;
          struct iovec drainiov;
          int val;

          memset(&drainmsg, 0, sizeof(drainmsg));
          drainiov.iov_base = drainbuf;
          drainiov.iov_len = sizeof(drainbuf);
          drainmsg.msg_iov = &drainiov;
          drainmsg.msg_iovlen = 1;
          drainmsg.msg_control = draincmsg;
          drainmsg.msg_controllen = sizeof(draincmsg);

          while (recvmsg(state->tx_socket, &drainmsg,
                         MSG_ERRQUEUE | MSG_DONTWAIT) > 0)
            {
            }

          val = SOF_TIMESTAMPING_TX_HARDWARE |
                SOF_TIMESTAMPING_RAW_HARDWARE;
          setsockopt(state->tx_socket, SOL_SOCKET, SO_TIMESTAMPING,
                     &val, sizeof(val));
        }
#endif

      ret = sendmsg(state->tx_socket, &msg, 0);
      if (ret < 0)
        {
#ifdef CONFIG_NET_TIMESTAMP
          if (do_hwts)
            {
              int val = 0;

              setsockopt(state->tx_socket, SOL_SOCKET, SO_TIMESTAMPING,
                         &val, sizeof(val));
            }

#endif
          return ERROR;
        }
    }
  else
    {
      ret = sendto(state->tx_socket, buf, buflen, 0, addr, addrlen);
    }

  if (sendts != NULL)
    {
#ifdef CONFIG_NET_TIMESTAMP
      if (do_hwts)
        {
          int val = 0;

          if (ptp_get_tx_timestamp(state, sendts) == OK)
            {
              state->hwts_tx_failures = 0;

              /* The frame reaches the wire later than the MAC latches the
               * timestamp: compensate the egress latency.
               */

              timespec_add_ns(sendts, state->config->egress_latency_ns);
            }
          else
            {
              ptpwarn("PTP TX HWTS timeout, fallback to SW ts: "
                      "%jd.%09ld s\n",
                      (intmax_t)sw_ts.tv_sec, sw_ts.tv_nsec);
              *sendts = sw_ts;

              if (++state->hwts_tx_failures >= PTP_HWTS_TX_MAX_FAILURES)
                {
                  state->hwts_tx_disabled = true;
                  ptpwarn("Hardware TX timestamps unavailable, "
                          "using software timestamps\n");
                }
            }

          setsockopt(state->tx_socket, SOL_SOCKET, SO_TIMESTAMPING,
                     &val, sizeof(val));
        }
      else
#endif
        {
          *sendts = sw_ts;
        }
    }

  return ret;
}

/* Send PTP server announcement packet */

static int ptp_send_announce(FAR struct ptp_state_s *state)
{
  struct ptp_announce_s msg;
  struct sockaddr_in addr;
  struct timespec ts;
  int ret;

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_INFO);

  memset(&msg, 0, sizeof(msg));
  msg = state->own_identity;
  msg.header.messagetype = PTP_MSGTYPE_ANNOUNCE;
  msg.header.messagelength[1] = sizeof(msg);

  ptp_increment_sequence(&state->announce_seq, &msg.header);
  ptp_gettime(state, &ts);
  timespec_to_ptp_format(&ts, msg.origintimestamp);

  ret = ptp_sendmsg(state, &msg, sizeof(msg), &addr, sizeof(addr), NULL);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed: %d", errno);
    }
  else
    {
      ptpinfo("Sent announce, seq %ld\n",
              (long)ptp_get_sequence(&msg.header));
    }

  return ret;
}

/* Send PTP server synchronization packet */

static int ptp_send_sync(FAR struct ptp_state_s *state)
{
  struct ptp_sync_s msg;
  struct sockaddr_in addr;
  struct timespec ts;
  int ret;

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_EVENT);

  memset(&msg, 0, sizeof(msg));
  msg.header = state->own_identity.header;
  msg.header.messagetype = PTP_MSGTYPE_SYNC;
  msg.header.messagelength[1] = sizeof(msg);

#ifdef CONFIG_NETUTILS_PTPD_TWOSTEP_SYNC
  msg.header.flags[0] = PTP_FLAGS0_TWOSTEP;
#endif

  /* Timestamp and send the sync message */

  ptp_increment_sequence(&state->sync_seq, &msg.header);
  ptp_gettime(state, &ts);
  timespec_to_ptp_format(&ts, msg.origintimestamp);

  ret = ptp_sendmsg(state, &msg, sizeof(msg), &addr, sizeof(addr), &ts);
  if (ret < 0)
    {
      ptperr("sendmsg for sync message failed: %d\n", errno);
      return ret;
    }

#ifdef CONFIG_NETUTILS_PTPD_TWOSTEP_SYNC

  timespec_to_ptp_format(&ts, msg.origintimestamp);
  msg.header.messagetype = PTP_MSGTYPE_FOLLOW_UP;
  msg.header.flags[0] = 0;
  addr.sin_port = HTONS(PTP_UDP_PORT_INFO);

  ret = ptp_sendmsg(state, &msg, sizeof(msg), &addr, sizeof(addr), NULL);
  if (ret < 0)
    {
      ptperr("ptp sendmsg for follow-up message failed: %d\n", errno);
      return ret;
    }

  ptpinfo("Sent sync + follow-up, seq %ld\n",
          (long)ptp_get_sequence(&msg.header));
#else
  ptpinfo("Sent sync, seq %ld\n",
          (long)ptp_get_sequence(&msg.header));
#endif /* CONFIG_NETUTILS_PTPD_TWOSTEP_SYNC */

  return OK;
}

/* Send delay request packet to selected source */

static int ptp_send_delay_req(FAR struct ptp_state_s *state)
{
  struct ptp_delay_req_s req;
  struct sockaddr_in addr;
  int ret;

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_EVENT);

  memset(&req, 0, sizeof(req));
  req.header = state->own_identity.header;
  req.header.messagetype = PTP_MSGTYPE_DELAY_REQ;
  req.header.messagelength[1] = sizeof(req);
  req.header.logmessageinterval = PTP_LOG_INTERVAL_DELAY_REQ;
  ptp_increment_sequence(&state->delay_req_seq, &req.header);

  ptp_gettime(state, &state->delayreq_time);
  timespec_to_ptp_format(&state->delayreq_time, req.origintimestamp);

  ret = ptp_sendmsg(state, &req, sizeof(req),
                    &addr, sizeof(addr), &state->delayreq_time);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed: %d", errno);
    }
  else
    {
      clock_gettime(CLOCK_MONOTONIC, &state->last_transmitted_delayreq);
      ptpinfo("Sent delay req, seq %ld\n",
              (long)ptp_get_sequence(&req.header));
    }

  return ret;
}

/* Send peer delay request packet (P2P) */

static int ptp_send_pdelay_req(FAR struct ptp_state_s *state)
{
  struct ptp_pdelay_req_s req;
  struct sockaddr_in addr;
  int ret;

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_PDELAY_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_EVENT);

  memset(&req, 0, sizeof(req));
  req.header = state->own_identity.header;
  req.header.messagetype = PTP_MSGTYPE_PDELAY_REQ;
  req.header.version = PTP_VERSION_2_0;
  req.header.messagelength[1] = sizeof(req);
  req.header.controlfield = 0x05;
  req.header.logmessageinterval = PTP_LOG_INTERVAL_DELAY_REQ;
  ptp_increment_sequence(&state->pdelay_req_seq, &req.header);

  /* Starting a new request cycle invalidates any Pdelay_Resp we might
   * still be waiting a Follow_Up for from the previous one (e.g. its
   * Resp was lost and only its Follow_Up shows up later, after this
   * new cycle has already updated pdelay_req_seq). Without this, that
   * orphaned Follow_Up would still pass the sequence check below (it
   * now matches the new cycle) and get paired with pdelayreq_rx_time
   * (t2) captured for the OLD cycle - producing a path delay that is
   * off by roughly one full request interval.
   */

  state->pdelay_waiting_followup = false;

  ptp_gettime(state, &state->pdelayreq_tx_time);
  timespec_to_ptp_format(&state->pdelayreq_tx_time, req.origintimestamp);

  ret = ptp_sendmsg(state, &req, sizeof(req),
                    &addr, sizeof(addr), &state->pdelayreq_tx_time);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed: %d\n", errno);
    }
  else
    {
      clock_gettime(CLOCK_MONOTONIC, &state->last_transmitted_pdelayreq);
      ptpinfo("Sent Pdelay_Req, seq %d\n",
              ptp_get_sequence(&req.header));
    }

  return ret;
}

/* Check if we need to send packets */

static int ptp_periodic_send(FAR struct ptp_state_s *state)
{
  /* If there is no better master clock on the network,
   * act as the reference source and send server packets.
   */

  if (!state->config->client_only && !state->selected_source_valid)
    {
      struct timespec time_now;
      struct timespec delta;

      clock_gettime(CLOCK_MONOTONIC, &time_now);
      clock_timespec_subtract(&time_now,
        &state->last_transmitted_announce, &delta);
      if (state->config->bmca && timespec_to_ms(&delta)
          > CONFIG_NETUTILS_PTPD_ANNOUNCE_INTERVAL_MSEC)
        {
          state->last_transmitted_announce = time_now;
          ptp_send_announce(state);
        }

      clock_timespec_subtract(&time_now,
        &state->last_transmitted_sync, &delta);
      if (timespec_to_ms(&delta) > CONFIG_NETUTILS_PTPD_SYNC_INTERVAL_MSEC)
        {
          state->last_transmitted_sync = time_now;
          ptp_send_sync(state);
        }
    }

  if (state->config->delay_mechanism == PTP_DELAY_E2E &&
      state->selected_source_valid && state->can_send_delayreq)
    {
      struct timespec time_now;
      struct timespec delta;
      long interval_s;

      clock_gettime(CLOCK_MONOTONIC, &time_now);
      clock_timespec_subtract(&time_now,
                              &state->last_transmitted_delayreq, &delta);

      interval_s = (state->delayreq_interval > 0) ?
                    state->delayreq_interval : 1;

      if (timespec_to_ms(&delta) >= interval_s * MSEC_PER_SEC)
        {
          ptp_send_delay_req(state);
        }
    }

  if (state->config->delay_mechanism == PTP_DELAY_P2P)
    {
      struct timespec time_now;
      struct timespec delta;
      long interval_s;

      clock_gettime(CLOCK_MONOTONIC, &time_now);
      clock_timespec_subtract(&time_now,
                              &state->last_transmitted_pdelayreq, &delta);

      interval_s = (state->delayreq_interval > 0) ?
                    state->delayreq_interval : 1;

      if (timespec_to_ms(&delta) >= interval_s * MSEC_PER_SEC)
        {
          ptp_send_pdelay_req(state);
        }
    }

  return OK;
}

/* Process received PTP announcement */

static int ptp_process_announce(FAR struct ptp_state_s *state,
                                FAR struct ptp_announce_s *msg)
{
  clock_gettime(CLOCK_MONOTONIC, &state->last_received_announce);

  if (state->config->bmca && is_better_clock(msg, &state->own_identity))
    {
      if (!state->selected_source_valid ||
          is_better_clock(msg, &state->selected_source))
        {
          ptpinfo("Switching to better PTP time source\n");

          state->selected_source = *msg;
          state->last_received_sync = state->last_received_announce;
          if (state->config->delay_mechanism == PTP_DELAY_E2E)
            {
              state->path_delay_avgcount = 0;
              state->path_delay_ns = 0;
              state->delayreq_time.tv_sec = 0;
            }
        }
    }

  return OK;
}

#if CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS > 0
/* Tell whether a phase error measurement is an outlier, i.e. it differs from
 * the median of the latest accepted ones by more than the threshold. A
 * measurement that is disturbed on its own (a late receive timestamp, for
 * example) would otherwise move the frequency and phase corrections.
 *
 * A change that lasts is not an outlier: after a few rejections in a row
 * the measurement is accepted and the history starts over.
 */

static bool ptp_is_outlier(FAR struct ptp_state_s *state, int64_t delta_ns)
{
  int64_t sorted[PTP_OUTLIER_HISTORY];
  int64_t deviation;
  unsigned int count = state->delta_hist_count;
  unsigned int i;
  unsigned int j;

  if (count >= PTP_OUTLIER_MIN_HISTORY)
    {
      for (i = 0; i < count; i++)
        {
          int64_t value = state->delta_hist[i];

          for (j = i; j > 0 && sorted[j - 1] > value; j--)
            {
              sorted[j] = sorted[j - 1];
            }

          sorted[j] = value;
        }

      deviation = delta_ns - sorted[count / 2];
      if (deviation < 0)
        {
          deviation = -deviation;
        }

      if (deviation > CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS)
        {
          if (++state->outlier_count < PTP_OUTLIER_MAX_CONSECUTIVE)
            {
              return true;
            }

          state->delta_hist_count = 0;
          state->delta_hist_next  = 0;
        }
    }

  state->outlier_count = 0;
  state->delta_hist[state->delta_hist_next] = delta_ns;
  state->delta_hist_next = (state->delta_hist_next + 1) %
                           PTP_OUTLIER_HISTORY;
  if (state->delta_hist_count < PTP_OUTLIER_HISTORY)
    {
      state->delta_hist_count++;
    }

  return false;
}
#endif

/* Update local clock either by smooth adjustment or by jumping.
 * Remote time was remote_timestamp at local_timestamp.
 */

static int ptp_update_local_clock(FAR struct ptp_state_s *state,
                                  FAR struct timespec *remote_timestamp,
                                  FAR struct timespec *local_timestamp)
{
  int ret;
  int64_t delta_ns;
  int64_t absdelta_ns;
  const int64_t adj_limit_ns = CONFIG_NETUTILS_PTPD_SETTIME_THRESHOLD_MS
                               * (int64_t)NSEC_PER_MSEC;

  ptpinfo("Local time: %jd.%09ld, remote time %jd.%09ld\n",
          (intmax_t)local_timestamp->tv_sec,
          local_timestamp->tv_nsec,
          (intmax_t)remote_timestamp->tv_sec,
          remote_timestamp->tv_nsec);

  delta_ns = timespec_delta_ns(remote_timestamp, local_timestamp);
  delta_ns += state->path_delay_ns;
  absdelta_ns = (delta_ns < 0) ? -delta_ns : delta_ns;

  if (absdelta_ns > adj_limit_ns)
    {
      /* Large difference, move by jumping.
       * Account for delay since packet was received.
       */

      struct timespec new_time;

      ptp_gettime(state, &new_time);
      clock_timespec_subtract(&new_time, local_timestamp, &new_time);
      clock_timespec_add(&new_time, remote_timestamp, &new_time);
      ret = ptp_settime(state, &new_time);

      /* Reinitialize drift adjustment parameters */

      state->last_delta_timestamp = new_time;
      state->last_delta_ns = 0;
      state->last_adjtime_ns = 0;
      state->drift_avg_total_ms = 0;
      state->drift_ppb = 0;
      state->has_last_delta = false;
#if CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS > 0
      state->delta_hist_count = 0;
      state->delta_hist_next  = 0;
      state->outlier_count    = 0;
#endif

      if (ret == OK)
        {
          ptpinfo("Jumped to timestamp %jd.%09ld s\n",
                  (intmax_t)new_time.tv_sec, new_time.tv_nsec);
        }
      else
        {
          ptperr("ptp_settime() failed: %d\n", errno);
        }
    }
  else
    {
      /* Track drift rate based on two consecutive measurements and
       * the adjustment that was made previously.
       */

      int64_t drift_ppb = 0;
      struct timespec interval;
      int interval_ms = 0;
      int max_avg_period_ms;
      int64_t adjustment_ns;
      const int64_t max_adjust_ns =
        (int64_t)CONFIG_CLOCK_ADJTIME_SLEWLIMIT_PPM *
        CONFIG_CLOCK_ADJTIME_PERIOD_MS;

#if CONFIG_NETUTILS_PTPD_OUTLIER_THRESHOLD_NS > 0
      if (ptp_is_outlier(state, delta_ns))
        {
          ptpwarn("Discarding outlier sample: delta %" PRId64 " ns\n",
                  delta_ns);
          return OK;
        }
#endif

      if (!state->has_last_delta)
        {
          /* First measurement after jump or startup: no previous
           * delta available to compute frequency drift rate.
           */

          adjustment_ns = delta_ns;
        }
      else
        {
          clock_timespec_subtract(local_timestamp,
                                  &state->last_delta_timestamp,
                                  &interval);
          interval_ms = timespec_to_ms(&interval);

          if (interval_ms > 0 &&
              interval_ms < CONFIG_NETUTILS_PTPD_TIMEOUT_MS)
            {
              /* Natural change in delta over the interval, accounting for
               * the adjustment applied during that same interval.
               */

              drift_ppb = (delta_ns - state->last_delta_ns +
                           state->last_adjtime_ns) * MSEC_PER_SEC
                          / interval_ms;
            }
          else
            {
              ptpwarn("Measurement interval out of range: %d ms\n",
                      interval_ms);
              drift_ppb = state->drift_ppb;
              interval_ms = 1;
            }

          if (drift_ppb > CONFIG_NETUTILS_PTPD_MAX_DRIFT_PPB ||
              drift_ppb < -CONFIG_NETUTILS_PTPD_MAX_DRIFT_PPB)
            {
              /* Physically implausible for a real crystal oscillator -
               * almost always the result of an abnormally short interval
               * between samples (e.g. a burst of packets right after a
               * clock source outage/reconnect) rather than actual drift.
               * Discard it instead of letting it corrupt the long-term
               * average; CLOCK_ADJTIME_SLEWLIMIT_PPM is a much looser
               * hardware safety bound and would let this through
               * unchanged.
               */

              ptpwarn("Drift estimate out of range: %lld\n",
                      (long long)drift_ppb);
              drift_ppb = state->drift_ppb;
            }

          /* Update the exponential sliding average */

          state->drift_avg_total_ms += interval_ms;
          max_avg_period_ms = CONFIG_NETUTILS_PTPD_DRIFT_AVERAGE_S
                              * MSEC_PER_SEC;
          if (state->drift_avg_total_ms > max_avg_period_ms)
            {
              state->drift_avg_total_ms = max_avg_period_ms;
            }

          state->drift_ppb += (drift_ppb - state->drift_ppb) * interval_ms
                            / state->drift_avg_total_ms;

          /* Compute the adjustment to compensate frequency drift plus
           * current phase error.
           */

          adjustment_ns = state->drift_ppb * CONFIG_CLOCK_ADJTIME_PERIOD_MS
                          / MSEC_PER_SEC;
          adjustment_ns += delta_ns;
        }

      /* Clamp adjustment to the hardware slew limit so that last_adjtime_ns
       * accurately reflects what adjtime() will actually perform.
       */

      if (adjustment_ns > max_adjust_ns)
        {
          adjustment_ns = max_adjust_ns;
        }
      else if (adjustment_ns < -max_adjust_ns)
        {
          adjustment_ns = -max_adjust_ns;
        }

      /* Apply adjustment and store information for next time */

      state->last_delta_ns = delta_ns;
      state->last_delta_timestamp = *local_timestamp;
      state->last_adjtime_ns = adjustment_ns;
      state->has_last_delta = true;

      ptpinfo("Delta: %+lld ns, adjustment %+lld ns, drift rate %+lld ppb\n",
              (long long)delta_ns,
              (long long)state->last_adjtime_ns,
              (long long)state->drift_ppb);

      ret = ptp_adjtime(state, adjustment_ns,
                         absdelta_ns >
                         CONFIG_NETUTILS_PTPD_ADJTIME_THRESHOLD_NS ?
                         drift_ppb : state->drift_ppb);

      if (ret != OK)
        {
          ptperr("ptp_adjtime() failed: %d\n", errno);
        }

      /* Clock is tracking the master, allow sending delay requests */

      state->can_send_delayreq = true;
    }

  return ret;
}

static void ptp_add_correction_time(FAR const uint8_t *correction,
                                    FAR struct timespec *ts)
{
  uint64_t correction_time = (((uint64_t)correction[0]) << 40)
                           | (((uint64_t)correction[1]) << 32)
                           | (((uint64_t)correction[2]) << 24)
                           | (((uint64_t)correction[3]) << 16)
                           | (((uint64_t)correction[4]) <<  8)
                           | (((uint64_t)correction[5]) <<  0);

  ptpinfo("correction before: %jd.%09ld\n", (intmax_t)ts->tv_sec,
          ts->tv_nsec);

  ts->tv_sec  += correction_time / NSEC_PER_SEC;
  ts->tv_nsec += correction_time % NSEC_PER_SEC;
  if (ts->tv_nsec >= NSEC_PER_SEC)
    {
      ts->tv_nsec -= NSEC_PER_SEC;
      ts->tv_sec  += 1;
    }

  ptpinfo("correction after: %jd.%09ld\n", (intmax_t)ts->tv_sec,
          ts->tv_nsec);
}

/* Process received PTP sync packet */

static int ptp_process_sync(FAR struct ptp_state_s *state,
                            FAR struct ptp_sync_s *msg)
{
  struct timespec remote_time;

  if (state->config->bmca &&
      memcmp(msg->header.sourceidentity,
             state->selected_source.header.sourceidentity,
             sizeof(msg->header.sourceidentity)) != 0)
    {
      /* This packet wasn't from the currently selected source */

      return OK;
    }

  /* Update timeout tracking */

  clock_gettime(CLOCK_MONOTONIC, &state->last_received_sync);

  if (msg->header.flags[0] & PTP_FLAGS0_TWOSTEP)
    {
      /* We need to wait for a follow-up packet before setting the clock. */

      state->twostep_rxtime = state->rxtime;
      state->twostep_packet = *msg;
      ptpinfo("Waiting for follow-up\n");
      return OK;
    }

  /* Update local clock */

  ptp_format_to_timespec(msg->origintimestamp, &remote_time);
  ptp_add_correction_time(msg->header.correction, &remote_time);
  state->sync_diff_ns = timespec_delta_ns(&state->rxtime, &remote_time);
  state->sync_diff_valid = true;
  return ptp_update_local_clock(state, &remote_time, &state->rxtime);
}

static int ptp_process_followup(FAR struct ptp_state_s *state,
                                FAR struct ptp_follow_up_s *msg)
{
  struct timespec remote_time;

  if (state->config->bmca &&
      memcmp(msg->header.sourceidentity,
             state->twostep_packet.header.sourceidentity,
             sizeof(msg->header.sourceidentity)) != 0)
    {
      return OK; /* This packet wasn't from the currently selected source */
    }

  if (ptp_get_sequence(&msg->header)
      != ptp_get_sequence(&state->twostep_packet.header))
    {
      ptpwarn("PTP follow-up packet sequence %ld does not match initial "
              "sync packet sequence %ld, ignoring\n",
              (long)ptp_get_sequence(&msg->header),
              (long)ptp_get_sequence(&state->twostep_packet.header));
      return OK;
    }

  /* Update local clock based on the remote timestamp we received now
   * and the local timestamp of when the sync packet was received.
   */

  ptp_format_to_timespec(msg->origintimestamp, &remote_time);

  /* add correction time */

  ptp_add_correction_time(msg->header.correction, &remote_time);

  /* Store (t2 - t1) for canonical IEEE 1588-2008 §11.3 path delay */

  state->sync_diff_ns = timespec_delta_ns(&state->twostep_rxtime,
                                          &remote_time);
  state->sync_diff_valid = true;

  /* done */

  return ptp_update_local_clock(state, &remote_time, &state->twostep_rxtime);
}

static int ptp_process_delay_req(FAR struct ptp_state_s *state,
                                 FAR struct ptp_delay_req_s *msg)
{
  struct ptp_delay_resp_s resp;
  struct sockaddr_in addr;
  int ret;

  if (state->selected_source_valid)
    {
      /* We are operating as a client, ignore delay requests */

      return OK;
    }

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_INFO);

  memset(&resp, 0, sizeof(resp));
  resp.header = state->own_identity.header;
  resp.header.messagetype = PTP_MSGTYPE_DELAY_RESP;
  resp.header.messagelength[1] = sizeof(resp);
  timespec_to_ptp_format(&state->rxtime, resp.receivetimestamp);
  memcpy(resp.reqidentity, msg->header.sourceidentity,
         sizeof(resp.reqidentity));
  memcpy(resp.reqportindex, msg->header.sourceportindex,
         sizeof(resp.reqportindex));
  memcpy(resp.header.sequenceid, msg->header.sequenceid,
         sizeof(resp.header.sequenceid));
  resp.header.logmessageinterval = CONFIG_NETUTILS_PTPD_DELAYRESP_INTERVAL;

  ret = ptp_sendmsg(state, &resp, sizeof(resp), &addr, sizeof(addr), NULL);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed: %d", errno);
    }
  else
    {
      clock_gettime(CLOCK_MONOTONIC, &state->last_transmitted_delayresp);
      ptpinfo("Sent delay resp, seq %ld\n",
              (long)ptp_get_sequence(&msg->header));
    }

  return ret;
}

/* Record and filter measured path delay (used by both E2E and P2P) */

static void ptp_record_path_delay(FAR struct ptp_state_s *state,
                                  int64_t path_delay)
{
  int64_t max_path_delay;

  max_path_delay = CONFIG_NETUTILS_PTPD_MAX_PATH_DELAY_NS;

  if (max_path_delay < 10 * NSEC_PER_MSEC)
    {
      /* Software TX latency on delay measurement transmission can add up
       * to several milliseconds. Allow up to 10 ms until hardware TX
       * timestamping is available.
       */

      max_path_delay = 10 * NSEC_PER_MSEC;
    }

  if (path_delay >= -100000 && path_delay < max_path_delay)
    {
      if (path_delay < 0)
        {
          path_delay = 0;
        }

      if (state->path_delay_avgcount <
          CONFIG_NETUTILS_PTPD_DELAYREQ_AVGCOUNT)
        {
          state->path_delay_avgcount++;
        }

      state->path_delay_ns += (path_delay - state->path_delay_ns)
                              / state->path_delay_avgcount;

      ptpinfo("Path delay: %" PRId64 " ns (avg: %ld ns)\n",
              path_delay, state->path_delay_ns);
    }
  else
    {
      ptpwarn("Path delay out of range: %" PRId64 " ns\n", path_delay);
    }
}

static int ptp_process_delay_resp(FAR struct ptp_state_s *state,
                                  FAR struct ptp_delay_resp_s *msg)
{
  int64_t path_delay;
  struct timespec remote_rxtime;
  uint16_t sequence;
  int interval;
  bool source_match;
  bool request_match;

  source_match = memcmp(msg->header.sourceidentity,
                        state->selected_source.header.sourceidentity,
                        sizeof(msg->header.sourceidentity)) == 0;
  request_match = memcmp(msg->reqidentity,
                         state->own_identity.header.sourceidentity,
                         sizeof(msg->reqidentity)) == 0;

  if (!state->selected_source_valid || !state->sync_diff_valid ||
      !source_match || !request_match)
    {
      ptpwarn("Delay_Resp ignored: valid=%d, sync_valid=%d, src_match=%d, "
              "req_match=%d\n",
              state->selected_source_valid, state->sync_diff_valid,
              source_match, request_match);
      return OK; /* This packet wasn't for us */
    }

  sequence = ptp_get_sequence(&msg->header);

  if (sequence != state->delay_req_seq)
    {
      ptpwarn("Ignoring out-of-sequence delay resp (%d vs. expected %d)\n",
              (int)sequence, (int)state->delay_req_seq);
      return OK;
    }

  /* Path delay is calculated as the average between delta for sync
   * message (t2 - t1) and delta for delay req message (t4 - t3).
   * (IEEE-1588 section 11.3: Delay request-response mechanism)
   */

  ptp_format_to_timespec(msg->receivetimestamp, &remote_rxtime);
  path_delay = timespec_delta_ns(&remote_rxtime, &state->delayreq_time);
  path_delay = (state->sync_diff_ns + path_delay) / 2;

  ptp_record_path_delay(state, path_delay);

  /* Calculate interval until next packet */

  if (msg->header.logmessageinterval <= 12)
    {
      interval = (1 << msg->header.logmessageinterval);
    }
  else
    {
      interval = 4096; /* Refuse to obey excessively long intervals */
    }

  /* Randomize up to 2x nominal delay) */

  state->delayreq_interval = interval + (random() % interval);

  return OK;
}

/* Process received peer delay request (responder role) */

static int ptp_process_pdelay_req(FAR struct ptp_state_s *state,
                                  FAR struct ptp_pdelay_req_s *msg)
{
  struct ptp_pdelay_resp_s resp;
  struct ptp_pdelay_resp_follow_up_s fup;
  struct sockaddr_in addr;
  struct timespec t3;
  int ret;

  if (state->config->delay_mechanism != PTP_DELAY_P2P)
    {
      return OK;
    }

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_PDELAY_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_EVENT);

  memset(&resp, 0, sizeof(resp));
  resp.header = state->own_identity.header;
  resp.header.messagetype = PTP_MSGTYPE_PDELAY_RESP;
  resp.header.version = PTP_VERSION_2_0;
  resp.header.messagelength[1] = sizeof(resp);
  resp.header.flags[0] = PTP_FLAGS0_TWOSTEP;
  resp.header.controlfield = 0x05;
  memcpy(resp.header.sequenceid, msg->header.sequenceid,
         sizeof(resp.header.sequenceid));
  resp.header.logmessageinterval = 0x7f;

  timespec_to_ptp_format(&state->rxtime, resp.requestreceipttimestamp);
  memcpy(resp.reqidentity, msg->header.sourceidentity,
         sizeof(resp.reqidentity));
  memcpy(resp.reqportindex, msg->header.sourceportindex,
         sizeof(resp.reqportindex));

  ret = ptp_sendmsg(state, &resp, sizeof(resp), &addr, sizeof(addr), &t3);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed for Pdelay_Resp: %d\n", errno);
      return ret;
    }

  clock_gettime(CLOCK_MONOTONIC, &state->last_transmitted_delayresp);
  ptpinfo("Sent Pdelay_Resp, seq %d\n",
          ptp_get_sequence(&resp.header));

  /* Send Pdelay_Resp_Follow_Up with transmit timestamp t3 */

  addr.sin_port = HTONS(PTP_UDP_PORT_INFO);

  memset(&fup, 0, sizeof(fup));
  fup.header = state->own_identity.header;
  fup.header.messagetype = PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP;
  fup.header.version = PTP_VERSION_2_0;
  fup.header.messagelength[1] = sizeof(fup);
  fup.header.controlfield = 0x05;
  memcpy(fup.header.sequenceid, msg->header.sequenceid,
         sizeof(fup.header.sequenceid));
  fup.header.logmessageinterval = 0x7f;

  timespec_to_ptp_format(&t3, fup.responseorigintimestamp);
  memcpy(fup.reqidentity, msg->header.sourceidentity,
         sizeof(fup.reqidentity));
  memcpy(fup.reqportindex, msg->header.sourceportindex,
         sizeof(fup.reqportindex));

  ret = ptp_sendmsg(state, &fup, sizeof(fup), &addr, sizeof(addr), NULL);
  if (ret < 0)
    {
      ptperr("ptp sendmsg failed for Pdelay_Resp_Follow_Up: %d\n", errno);
      return ret;
    }

  ptpinfo("Sent Pdelay_Resp_Follow_Up, seq %d\n",
          ptp_get_sequence(&fup.header));

  return OK;
}

/* Process received peer delay response (requester role) */

static int ptp_process_pdelay_resp(FAR struct ptp_state_s *state,
                                   FAR struct ptp_pdelay_resp_s *msg)
{
  uint16_t sequence;

  if (state->config->delay_mechanism != PTP_DELAY_P2P)
    {
      return OK;
    }

  if (memcmp(msg->reqidentity, state->own_identity.header.sourceidentity,
             sizeof(msg->reqidentity)) != 0)
    {
      return OK; /* Not for us */
    }

  sequence = ptp_get_sequence(&msg->header);
  if (sequence != state->pdelay_req_seq)
    {
      ptpwarn("Ignoring out-of-sequence Pdelay_Resp (%d vs. expected %d)\n",
              sequence, state->pdelay_req_seq);
      return OK;
    }

  /* Store t4 (local receive timestamp) and t2 (receipt timestamp
   * from peer).
   */

  state->pdelayresp_rx_time = state->rxtime;
  ptp_format_to_timespec(msg->requestreceipttimestamp,
                         &state->pdelayreq_rx_time);
  ptp_add_correction_time(msg->header.correction,
                          &state->pdelayreq_rx_time);

  if (msg->header.flags[0] & PTP_FLAGS0_TWOSTEP)
    {
      state->pdelay_waiting_followup = true;
      ptpinfo("Waiting for Pdelay_Resp_Follow_Up, seq %d\n",
              sequence);
    }
  else
    {
      /* One-step: turnaround time (t3 - t2) is carried in correctionField */

      int64_t t4_t1_ns;
      int64_t t3_t2_ns;
      int64_t path_delay;
      uint64_t correction_time;

      correction_time = (((uint64_t)msg->header.correction[0]) << 40)
                      | (((uint64_t)msg->header.correction[1]) << 32)
                      | (((uint64_t)msg->header.correction[2]) << 24)
                      | (((uint64_t)msg->header.correction[3]) << 16)
                      | (((uint64_t)msg->header.correction[4]) <<  8)
                      | msg->header.correction[5];

      t4_t1_ns = timespec_delta_ns(&state->pdelayresp_rx_time,
                                   &state->pdelayreq_tx_time);
      t3_t2_ns = correction_time;
      path_delay = (t4_t1_ns - t3_t2_ns) / 2;

      ptp_record_path_delay(state, path_delay);
    }

  return OK;
}

/* Process received peer delay response follow-up (requester role) */

static int ptp_process_pdelay_resp_followup(
             FAR struct ptp_state_s *state,
             FAR struct ptp_pdelay_resp_follow_up_s *msg)
{
  struct timespec t3;
  int64_t t4_t1_ns;
  int64_t t3_t2_ns;
  int64_t path_delay;
  uint16_t sequence;

  if (state->config->delay_mechanism != PTP_DELAY_P2P ||
      !state->pdelay_waiting_followup)
    {
      return OK;
    }

  if (memcmp(msg->reqidentity, state->own_identity.header.sourceidentity,
             sizeof(msg->reqidentity)) != 0)
    {
      return OK;
    }

  sequence = ptp_get_sequence(&msg->header);
  if (sequence != state->pdelay_req_seq)
    {
      ptpwarn("Ignoring out-of-sequence Pdelay_Resp_Follow_Up "
              "(%d vs. expected %d)\n",
              sequence, state->pdelay_req_seq);
      return OK;
    }

  state->pdelay_waiting_followup = false;

  ptp_format_to_timespec(msg->responseorigintimestamp, &t3);
  ptp_add_correction_time(msg->header.correction, &t3);

  /* IEEE 1588-2008 §11.4.3: meanPathDelay = ((t4 - t1) - (t3 - t2)) / 2 */

  t4_t1_ns = timespec_delta_ns(&state->pdelayresp_rx_time,
                               &state->pdelayreq_tx_time);
  t3_t2_ns = timespec_delta_ns(&t3, &state->pdelayreq_rx_time);
  path_delay = (t4_t1_ns - t3_t2_ns) / 2;

  ptp_record_path_delay(state, path_delay);

  return OK;
}

/* Determine received packet type and process it */

static int ptp_process_rx_packet(FAR struct ptp_state_s *state,
                                 ssize_t length)
{
  if (state->config->af == AF_PACKET)
    {
      /* Remove the header of ether message */

      FAR struct ethhdr *header = (FAR struct ethhdr *)state->rxbuf.raw;

      if (htons(header->h_proto) != ETHERTYPE_PTP)
        {
          ptpwarn("RX dropped: non-PTP proto 0x%04x (expected 0x%04x)\n",
                  ntohs(header->h_proto), ETHERTYPE_PTP);
          return -EINVAL;
        }

      length -= sizeof(*header);
      memmove(&state->rxbuf.raw, header + 1, length);
    }

  if (length < sizeof(struct ptp_header_s))
    {
      ptpwarn("Ignoring invalid PTP packet, length only %d bytes\n",
              (int)length);
      return OK;
    }

  ptpinfo("RX PTP: type=0x%02x (masked: 0x%02x), ver=0x%02x, domain=%d, "
          "seq=%d, len=%zd\n",
          state->rxbuf.header.messagetype,
          state->rxbuf.header.messagetype & PTP_MSGTYPE_MASK,
          state->rxbuf.header.version,
          state->rxbuf.header.domain,
          ptp_get_sequence(&state->rxbuf.header),
          length);

  if (state->rxbuf.header.domain != CONFIG_NETUTILS_PTPD_DOMAIN)
    {
      ptpwarn("RX dropped: domain mismatch %d != %d\n",
              state->rxbuf.header.domain, CONFIG_NETUTILS_PTPD_DOMAIN);

      /* Part of different clock domain, ignore */

      return OK;
    }

  clock_gettime(CLOCK_MONOTONIC, &state->last_received_multicast);

  switch (state->rxbuf.header.messagetype & PTP_MSGTYPE_MASK)
    {
      case PTP_MSGTYPE_ANNOUNCE:
        ptpinfo("Got announce packet, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_announce(state, &state->rxbuf.announce);

      case PTP_MSGTYPE_SYNC:
        ptpinfo("Got sync packet, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_sync(state, &state->rxbuf.sync);

      case PTP_MSGTYPE_FOLLOW_UP:
        ptpinfo("Got follow-up packet, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_followup(state, &state->rxbuf.follow_up);

      case PTP_MSGTYPE_DELAY_RESP:
        ptpinfo("Got delay-resp, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_delay_resp(state, &state->rxbuf.delay_resp);

      case PTP_MSGTYPE_DELAY_REQ:
        ptpinfo("Got delay req, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_delay_req(state, &state->rxbuf.delay_req);

      case PTP_MSGTYPE_PDELAY_REQ:
        ptpinfo("Got pdelay req, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_pdelay_req(state, &state->rxbuf.pdelay_req);

      case PTP_MSGTYPE_PDELAY_RESP:
        ptpinfo("Got pdelay resp, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_pdelay_resp(state, &state->rxbuf.pdelay_resp);

      case PTP_MSGTYPE_PDELAY_RESP_FOLLOW_UP:
        ptpinfo("Got pdelay resp follow-up, seq %d\n",
                ptp_get_sequence(&state->rxbuf.header));
        return ptp_process_pdelay_resp_followup(
                 state, &state->rxbuf.pdelay_resp_fup);

      default:
        ptpwarn("Ignoring unknown PTP packet type: 0x%02x "
                "(masked: 0x%02x)\n",
                state->rxbuf.header.messagetype,
                state->rxbuf.header.messagetype & PTP_MSGTYPE_MASK);
        return OK;
    }
}

/* Signal handler for status / stop requests */

static void ptp_signal_handler(int signo, FAR siginfo_t *siginfo,
                               FAR void *context)
{
  FAR struct ptp_state_s *state = (FAR struct ptp_state_s *)siginfo->si_user;

  if (signo == SIGHUP)
    {
      state->stop = true;
    }
  else if (signo == SIGUSR1)
    {
#ifdef CONFIG_BUILD_FLAT
      state->status_req = siginfo->si_value.sival_ptr;
#else
      state->dump = true;
#endif
    }
}

static void ptp_setup_sighandlers(FAR struct ptp_state_s *state)
{
  struct sigaction act;

  act.sa_sigaction = ptp_signal_handler;
  sigfillset(&act.sa_mask);
  act.sa_flags = SA_SIGINFO;
  act.sa_user = state;

  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGUSR1, &act, NULL);
}

/* Populate status information structure from current state */

static void ptp_populate_status(FAR struct ptp_state_s *state,
                                FAR struct ptpd_status_s *status)
{
  memset(status, 0, sizeof(*status));
  status->clock_source_valid = state->selected_source_valid;

  if (status->clock_source_valid)
    {
      FAR struct ptp_announce_s *s = &state->selected_source;

      memcpy(status->clock_source_info.id,
             s->header.sourceidentity,
             sizeof(status->clock_source_info.id));

      status->clock_source_info.utcoffset =
          (int16_t)(((uint16_t)s->utcoffset[0] << 8) | s->utcoffset[1]);
      status->clock_source_info.priority1 = s->gm_priority1;
      status->clock_source_info.clockclass = s->gm_quality[0];
      status->clock_source_info.accuracy = s->gm_quality[1];
      status->clock_source_info.priority2 = s->gm_priority2;
      status->clock_source_info.variance =
          ((uint16_t)s->gm_quality[2] << 8) | s->gm_quality[3];

      memcpy(status->clock_source_info.gm_id,
             s->gm_identity,
             sizeof(status->clock_source_info.gm_id));

      status->clock_source_info.stepsremoved =
          ((uint16_t)s->stepsremoved[0] << 8) | s->stepsremoved[1];
      status->clock_source_info.timesource = s->timesource;
    }

  status->last_clock_update = state->last_delta_timestamp;
  status->last_delta_ns     = state->last_delta_ns;
  status->last_adjtime_ns   = state->last_adjtime_ns;
  status->drift_ppb         = state->drift_ppb;
  status->path_delay_ns     = state->path_delay_ns;

  status->last_received_multicast    = state->last_received_multicast;
  status->last_received_announce     = state->last_received_announce;
  status->last_received_sync         = state->last_received_sync;
  status->last_transmitted_sync      = state->last_transmitted_sync;
  status->last_transmitted_announce  = state->last_transmitted_announce;
  status->last_transmitted_delayresp = state->last_transmitted_delayresp;
  status->last_transmitted_delayreq  = state->last_transmitted_delayreq;
  status->last_transmitted_pdelayreq = state->last_transmitted_pdelayreq;
}

#ifdef CONFIG_BUILD_FLAT
/* Process status information request in flat build mode */

static void ptp_process_statusreq(FAR struct ptp_state_s *state)
{
  FAR struct ptpd_statusreq_s *req = state->status_req;

  if (req == NULL)
    {
      return; /* No active request */
    }

  state->status_req = NULL;
  ptp_populate_status(state, &req->dest);

  /* Post semaphore to inform that we are done. The request belongs to the
   * caller of ptpd_status() and must not be touched after this.
   */

  sem_post(&req->done);
}
#else

/* Dump status to file when requested via signal.
 * Write atomically: temp file + rename.
 */

static void ptp_dump_status_file(FAR struct ptp_state_s *state)
{
  struct ptpd_status_s status;
  char tmppath[64];
  int fd;
  int ret;

  if (!state->dump)
    {
      return;
    }

  state->dump = false;

  ptp_populate_status(state, &status);

  snprintf(tmppath, sizeof(tmppath), "%s.tmp",
           CONFIG_NETUTILS_PTPD_STATUSFILE);

  fd = open(tmppath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
  if (fd < 0)
    {
      return;
    }

  ret = write(fd, &status, sizeof(status));
  close(fd);

  if (ret == sizeof(status))
    {
      rename(tmppath, CONFIG_NETUTILS_PTPD_STATUSFILE);
    }
  else
    {
      unlink(tmppath);
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ptpd_start
 *
 * Description:
 *   Start the PTP daemon and bind it to specified interface.
 *
 * Input Parameters:
 *   interface - Name of the network interface to bind to, e.g. "eth0"
 *
 * Returned Value:
 *   On success, the non-negative task ID of the PTP daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ptpd_start(FAR const struct ptpd_config_s *config)
{
  FAR struct ptp_state_s *state;
  struct pollfd pollfds[2];
  struct msghdr rxhdr;
  struct iovec rxiov;
  int timeout;
  int idx = 1;
  int status = OK;
  int ret;

  memset(&rxhdr, 0, sizeof(rxhdr));
  memset(&rxiov, 0, sizeof(rxiov));

  state = calloc(1, sizeof(struct ptp_state_s));
  if (state == NULL)
    {
      return -ENOMEM;
    }

  state->config = config;
  status = ptp_initialize_state(state);
  if (status != OK)
    {
      ptperr("Failed to initialize PTP state, exiting\n");
      goto errout;
    }

  if (config->client_only)
    {
      timeout = CONFIG_NETUTILS_PTPD_TIMEOUT_MS;
    }
  else
    {
      timeout = CONFIG_NETUTILS_PTPD_SYNC_INTERVAL_MSEC;
    }

  ptp_setup_sighandlers(state);

  pollfds[0].events = POLLIN;
  pollfds[0].fd = state->event_socket;
  if (state->info_socket > 0)
    {
      pollfds[1].events = POLLIN;
      pollfds[1].fd = state->info_socket;
      idx++;
    }

  while (!state->stop)
    {
      state->can_send_delayreq = false;

      rxhdr.msg_name = NULL;
      rxhdr.msg_namelen = 0;
      rxhdr.msg_iov = &rxiov;
      rxhdr.msg_iovlen = 1;
      rxhdr.msg_control = &state->rxcmsg;
      rxhdr.msg_controllen = sizeof(state->rxcmsg);
      rxhdr.msg_flags = 0;
      rxiov.iov_base = &state->rxbuf;
      rxiov.iov_len = sizeof(state->rxbuf);

      pollfds[0].revents = 0;
      pollfds[1].revents = 0;
      ret = poll(pollfds, idx, timeout);

      if (pollfds[0].revents)
        {
#ifdef CONFIG_NET_TIMESTAMP
          if ((pollfds[0].revents & POLLERR) != 0)
            {
              char errbuf[128];
              char cmsgbuf[128];
              struct msghdr errhdr;
              struct iovec erriov;

              memset(&errhdr, 0, sizeof(errhdr));
              erriov.iov_base = errbuf;
              erriov.iov_len = sizeof(errbuf);
              errhdr.msg_iov = &erriov;
              errhdr.msg_iovlen = 1;
              errhdr.msg_control = cmsgbuf;
              errhdr.msg_controllen = sizeof(cmsgbuf);

              while (recvmsg(state->event_socket, &errhdr,
                             MSG_ERRQUEUE | MSG_DONTWAIT) > 0)
                {
                }
            }
#endif

          /* Receive time-critical packet if POLLIN or POLLRDNORM
           * is signaled.
           */

          if ((pollfds[0].revents & (POLLIN | POLLRDNORM)) != 0)
            {
              while ((ret = recvmsg(state->event_socket, &rxhdr,
                                    MSG_DONTWAIT)) > 0)
                {
                  ptp_getrxtime(state, &rxhdr, &state->rxtime);
                  ptp_process_rx_packet(state, ret);

                  rxhdr.msg_namelen    = 0;
                  rxhdr.msg_iovlen     = 1;
                  rxhdr.msg_controllen = sizeof(state->rxcmsg);
                  rxhdr.msg_flags      = 0;
                  rxiov.iov_len        = sizeof(state->rxbuf);
                }
            }
        }

      if (pollfds[1].revents)
        {
          /* Receive non-time-critical packet. */

          ret = recv(state->info_socket, &state->rxbuf, sizeof(state->rxbuf),
                     MSG_DONTWAIT);
          if (ret > 0)
            {
              ptp_process_rx_packet(state, ret);
            }
        }

      if (pollfds[0].revents == 0 && pollfds[1].revents == 0)
        {
          /* No packets received, check for multicast timeout */

          ptp_check_multicast_status(state);
        }

      ptp_periodic_send(state);

      state->selected_source_valid = is_selected_source_valid(state);
#ifdef CONFIG_BUILD_FLAT
      ptp_process_statusreq(state);
#else
      ptp_dump_status_file(state);
#endif
    }

errout:
  ptp_destroy_state(state);
  free(state);

  return status;
}

/****************************************************************************
 * Name: ptpd_status
 *
 * Description:
 *   Query status from a running PTP daemon.
 *
 * Input Parameters:
 *   pid     - Process ID previously returned by ptpd_start()
 *   status  - Pointer to storage for status information.
 *
 * Returned Value:
 *   On success, returns OK.
 *   On failure, a negated errno value is returned.
 *
 * Assumptions/Limitations:
 *   Multiple threads with priority less than CONFIG_NETUTILS_PTPD_SERVERPRIO
 *   can request status simultaneously. If higher priority threads request
 *   status simultaneously, some of the requests may timeout.
 *
 ****************************************************************************/

int ptpd_status(int pid, FAR struct ptpd_status_s *status)
{
#ifdef CONFIG_BUILD_FLAT
  int ret = OK;
  union sigval val;
  struct timespec timeout;

  memset(status, 0, sizeof(struct ptpd_status_s));

  pthread_mutex_lock(&g_statusreq_lock);

  /* Drop the late answer to a request that timed out earlier */

  while (sem_trywait(&g_statusreq.done) == 0)
    {
    }

  /* Send the status request */

  val.sival_ptr = &g_statusreq;

  if (sigqueue(pid, SIGUSR1, val) != OK)
    {
      ret = -errno;
      goto errout;
    }

  /* Wait for status request to be handled */

  clock_gettime(CLOCK_MONOTONIC, &timeout);
  timeout.tv_sec += 1;
  if (sem_clockwait(&g_statusreq.done, CLOCK_MONOTONIC, &timeout) != 0)
    {
      ret = -errno;
    }
  else
    {
      memcpy(status, &g_statusreq.dest, sizeof(struct ptpd_status_s));
    }

errout:
  pthread_mutex_unlock(&g_statusreq_lock);
  return ret;
#else
  int fd;
  int ret;
  int elapsed;

  memset(status, 0, sizeof(struct ptpd_status_s));

  /* Signal daemon to dump fresh status */

  unlink(CONFIG_NETUTILS_PTPD_STATUSFILE);

  if (kill(pid, SIGUSR1) != OK)
    {
      return -errno;
    }

  /* Wait for status file to appear (up to 3s) */

  for (elapsed = 0; elapsed < 30; elapsed++)
    {
      usleep(100000);
      if (access(CONFIG_NETUTILS_PTPD_STATUSFILE, F_OK) == 0)
        {
          break;
        }
    }

  if (elapsed >= 30)
    {
      return -ETIMEDOUT;
    }

  fd = open(CONFIG_NETUTILS_PTPD_STATUSFILE, O_RDONLY | O_CLOEXEC);
  if (fd < 0)
    {
      return -errno;
    }

  ret = read(fd, status, sizeof(*status));
  close(fd);

  if (ret != sizeof(*status))
    {
      return ret < 0 ? -errno : -EIO;
    }

  return OK;
#endif
}

/****************************************************************************
 * Name: ptpd_stop
 *
 * Description:
 *   Stop PTP daemon
 *
 * Input Parameters:
 *   pid     - Process ID previously returned by ptpd_start()
 *
 * Returned Value:
 *   On success, returns OK.
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ptpd_stop(int pid)
{
  if (kill(pid, SIGHUP) == OK)
    {
      return OK;
    }
  else
    {
      return -errno;
    }
}
