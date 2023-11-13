/****************************************************************************
 * apps/netutils/ptpd/ptpd.c
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
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netutils/ipmsfilter.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <nuttx/net/netconfig.h>
#include <netutils/ptpd.h>

#include "ptpv2.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct ptp_state_s
{
  bool stop;

  /* Address of network interface we are operating on */

  struct sockaddr_in interface_addr;

  /* Socket bound to interface for transmission */

  int tx_socket;

  /* Sockets for PTP event and information ports */

  int event_socket;
  int info_socket;

  /* Our own identity as a clock source */

  struct ptp_announce_s own_identity;

  /* Sequence number counters per message type */

  uint16_t announce_seq;
  uint16_t sync_seq;
  uint16_t delay_req_seq;

  /* Identity of currently selected clock source,
   * from the latest announcement message.
   *
   * The timestamp is used for timeout when a source
   * disappears, it is from the local monotonic clock.
   */

  struct ptp_announce_s selected_source;
  struct timespec last_received_sync;
  struct timespec last_received_multicast;

  /* Last transmitted sync & announcement packets */

  struct timespec last_transmitted_sync;
  struct timespec last_transmitted_announce;

  /* Latest received packet and its timestamp */

  struct timespec rxtime;
  union
  {
    struct ptp_header_s     header;
    struct ptp_announce_s   announce;
    struct ptp_sync_s       sync;
    struct ptp_follow_up_s  follow_up;
    uint8_t                 raw[128];
  } rxbuf;

  union
  {
    uint8_t                 raw[64];
  } rxcmsg;

  /* Buffered sync packet for two-step clock setting */

  struct ptp_sync_s twostep_packet;
  struct timespec twostep_rxtime;
};

#ifdef CONFIG_NETUTILS_PTPD_SERVER
#  define PTPD_POLL_INTERVAL CONFIG_NETUTILS_PTPD_SYNC_INTERVAL_MSEC
#else
#  define PTPD_POLL_INTERVAL CONFIG_NETUTILS_PTPD_TIMEOUT_MS
#endif

/* PTP debug messages are enabled by either CONFIG_DEBUG_NET_INFO
 * or separately by CONFIG_NETUTILS_PTPD_DEBUG. This simplifies
 * debugging without having excessive amount of logging from net.
 */

#ifdef CONFIG_NETUTILS_PTPD_DEBUG
#  ifndef CONFIG_DEBUG_NET_INFO
#    define ptpinfo _info
#    define ptpwarn _warn
#    define ptperr  _err
#  else
#    define ptpinfo ninfo
#    define ptpwarn nwarn
#    define ptperr  nerr
#  endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Convert from timespec to PTP format */

static void timespec_to_ptp_format(struct timespec *ts, uint8_t *timestamp)
{
  /* IEEE 1588 uses 48 bits for seconds and 32 bits for nanoseconds,
   * both fields big-endian.
   */

#ifdef CONFIG_SYSTEM_TIME64
  timestamp[0] = (uint8_t)(ts->tv_sec >> 40);
  timestamp[1] = (uint8_t)(ts->tv_sec >> 32);
#else
  timestamp[0] = 0;
  timestamp[1] = 0;
#endif
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

static void ptp_format_to_timespec(uint8_t *timestamp, struct timespec *ts)
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

static bool is_better_clock(struct ptp_announce_s *a,
                            struct ptp_announce_s *b)
{
  if  (a->gm_priority1 < b->gm_priority1     /* Main priority field */
    || a->gm_quality[0] < b->gm_quality[0]   /* Clock class */
    || a->gm_quality[1] < b->gm_quality[1]   /* Clock accuracy */
    || a->gm_quality[2] < b->gm_quality[2]   /* Clock variance high byte */
    || a->gm_quality[3] < b->gm_quality[3]   /* Clock variance low byte */
    || a->gm_priority2 < b->gm_priority2     /* Sub priority field */
    || memcmp(a->gm_identity, b->gm_identity, sizeof(a->gm_identity)) < 0)
    {
      return true;
    }
    else
    {
      return false;
    }
}

static int64_t timespec_to_ms(struct timespec *ts)
{
  return ts->tv_sec * MSEC_PER_SEC + (ts->tv_nsec / NSEC_PER_MSEC);
}

/* Check if the currently selected source is still valid */

static bool is_selected_source_valid(struct ptp_state_s *state)
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

static void ptp_increment_sequence(uint16_t *sequence_num,
                                   struct ptp_header_s *hdr)
{
  *sequence_num += 1;
  hdr->sequenceid[0] = (uint8_t)(*sequence_num >> 8);
  hdr->sequenceid[1] = (uint8_t)(*sequence_num);
}

/* Get sequence number from received packet */

static uint16_t ptp_get_sequence(struct ptp_header_s *hdr)
{
  return ((uint16_t)hdr->sequenceid[0] << 8) | hdr->sequenceid[1];
}

/* Get current system timestamp as a timespec
 * TODO: Possibly add support for selecting different clock or using
 *       architecture-specific interface for clock access.
 */

static int ptp_gettime(struct ptp_state_s *state, struct timespec *ts)
{
  UNUSED(state);
  return clock_gettime(CLOCK_REALTIME, ts);
}

/* Change current system timestamp by jumping */

static int ptp_settime(struct ptp_state_s *state, struct timespec *ts)
{
  UNUSED(state);
  return clock_settime(CLOCK_REALTIME, ts);
}

/* Smoothly adjust timestamp.
 * TODO: adjtime() limits to microsecond resolution.
 */

static int ptp_adjtime(struct ptp_state_s *state, struct timespec *ts)
{
  struct timeval delta;
  UNUSED(state);
  TIMESPEC_TO_TIMEVAL(&delta, ts);
  return adjtime(&delta, NULL);
}

/* Get timestamp of latest received packet */

static int ptp_getrxtime(struct ptp_state_s *state, struct timespec *ts)
{
  UNUSED(state);
  *ts = state->rxtime;

  /* TODO: Implement SO_TIMINGS in NuttX core, and then fetch the
   *       timestamp from state->cmsg.
   */

  return OK;
}

/* Initialize PTP client/server state and create sockets */

static int ptp_initialize_state(struct ptp_state_s *state,
                                const char *interface)
{
  int ret;
  struct ifreq req;
  struct sockaddr_in bind_addr;

  /* Create sockets */

  state->tx_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (state->tx_socket < 0)
    {
      ptperr("Failed to create tx socket: %d\n", errno);
      return ERROR;
    }

  state->event_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (state->event_socket < 0)
    {
      ptperr("Failed to create event socket: %d\n", errno);
      return ERROR;
    }

  state->info_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (state->info_socket < 0)
    {
      ptperr("Failed to create info socket: %d\n", errno);
      return ERROR;
    }

  /* Get address information of the specified interface for binding socket
   * Only supports IPv4 currently.
   */

  memset(&req, 0, sizeof(req));
  strncpy(req.ifr_name, interface, sizeof(req.ifr_name));

  if (ioctl(state->event_socket, SIOCGIFADDR, (unsigned long)&req) < 0)
    {
      ptperr("Failed to get IP address information for interface %s\n",
             interface);
      return ERROR;
    }

  state->interface_addr = *(struct sockaddr_in *)&req.ifr_ifru.ifru_addr;

  /* Get hardware address to initialize the identity field in header.
   * Clock identity is EUI-64, which we make from EUI-48.
   */

  if (ioctl(state->event_socket, SIOCGIFHWADDR, (unsigned long)&req) < 0)
    {
      ptperr("Failed to get HW address information for interface %s\n",
             interface);
      return ERROR;
    }

  state->own_identity.header.version = 2;
  state->own_identity.header.domain = CONFIG_NETUTILS_PTPD_DOMAIN;
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

  /* Subscribe to PTP multicast address */

  bind_addr.sin_family = AF_INET;
  bind_addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);

  clock_gettime(CLOCK_MONOTONIC, &state->last_received_multicast);

  ret = ipmsfilter(&state->interface_addr.sin_addr,
                   &bind_addr.sin_addr,
                   MCAST_INCLUDE);
  if (ret < 0)
    {
      ptperr("Failed to bind multicast address: %d\n", errno);
      return ERROR;
    }

  /* Bind socket for events */

  bind_addr.sin_port = HTONS(PTP_UDP_PORT_EVENT);
  ret = bind(state->event_socket, (struct sockaddr *)&bind_addr,
             sizeof(bind_addr));
  if (ret < 0)
    {
      ptperr("Failed to bind to udp port %d\n", bind_addr.sin_port);
      return ERROR;
    }

  /* Bind socket for announcements */

  bind_addr.sin_port = HTONS(PTP_UDP_PORT_INFO);
  ret = bind(state->info_socket, (struct sockaddr *)&bind_addr,
             sizeof(bind_addr));
  if (ret < 0)
    {
      ptperr("Failed to bind to udp port %d\n", bind_addr.sin_port);
      return ERROR;
    }

  /* Bind TX socket to interface address (local addr cannot be multicast) */

  bind_addr.sin_addr = state->interface_addr.sin_addr;
  ret = bind(state->tx_socket, (struct sockaddr *)&bind_addr,
             sizeof(bind_addr));
  if (ret < 0)
    {
      ptperr("Failed to bind tx to port %d\n", bind_addr.sin_port);
      return ERROR;
    }

  return OK;
}

/* Unsubscribe multicast and destroy sockets */

static int ptp_destroy_state(struct ptp_state_s *state)
{
  struct in_addr mcast_addr;

  mcast_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  ipmsfilter(&state->interface_addr.sin_addr,
              &mcast_addr,
              MCAST_EXCLUDE);

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

/* Re-subscribe multicast address.
 * This can become necessary if Ethernet interface gets reset or if external
 * IGMP-compliant Ethernet switch gets plugged in.
 */

static int ptp_check_multicast_status(struct ptp_state_s *state)
{
#if CONFIG_NETUTILS_PTPD_MULTICAST_TIMEOUT_MS > 0
  struct in_addr mcast_addr;
  struct timespec time_now;
  struct timespec delta;

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

      return ipmsfilter(&state->interface_addr.sin_addr,
                        &mcast_addr,
                        MCAST_INCLUDE);
    }

#else
  UNUSED(state);
#endif /* CONFIG_NETUTILS_PTPD_MULTICAST_TIMEOUT_MS */

  return OK;
}

/* Send PTP server announcement packet */

static int ptp_send_announce(struct ptp_state_s *state)
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

  ptp_increment_sequence(&state->sync_seq, &msg.header);
  ptp_gettime(state, &ts);
  timespec_to_ptp_format(&ts, msg.origintimestamp);

  ret = sendto(state->tx_socket, &msg, sizeof(msg), 0,
    (struct sockaddr *)&addr, sizeof(addr));

  if (ret < 0)
    {
      ptperr("sendto failed: %d", errno);
    }
  else
    {
      ptpinfo("Sent announce, seq %ld\n",
              (long)ptp_get_sequence(&msg.header));
    }

  return ret;
}

/* Send PTP server synchronization packet */

static int ptp_send_sync(struct ptp_state_s *state)
{
  struct msghdr txhdr;
  struct iovec txiov;
  struct ptp_sync_s msg;
  struct sockaddr_in addr;
  struct timespec ts;
  uint8_t controlbuf[64];
  int ret;

  memset(&txhdr, 0, sizeof(txhdr));
  memset(&txiov, 0, sizeof(txiov));

  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = HTONL(PTP_MULTICAST_ADDR);
  addr.sin_port        = HTONS(PTP_UDP_PORT_EVENT);

  memset(&msg, 0, sizeof(msg));
  msg.header = state->own_identity.header;
  msg.header.messagetype = PTP_MSGTYPE_SYNC;
  msg.header.messagelength[1] = sizeof(msg);
  msg.header.flags[0] = PTP_FLAGS0_TWOSTEP;

  txhdr.msg_name = &addr;
  txhdr.msg_namelen = sizeof(addr);
  txhdr.msg_iov = &txiov;
  txhdr.msg_iovlen = 1;
  txhdr.msg_control = controlbuf;
  txhdr.msg_controllen = sizeof(controlbuf);
  txiov.iov_base = &msg;
  txiov.iov_len = sizeof(msg);

  /* Timestamp and send the sync message */

  ptp_increment_sequence(&state->sync_seq, &msg.header);
  ptp_gettime(state, &ts);
  timespec_to_ptp_format(&ts, msg.origintimestamp);

  ret = sendmsg(state->tx_socket, &txhdr, 0);
  if (ret < 0)
    {
      ptperr("sendmsg for sync message failed: %d\n", errno);
      return ret;
    }

  /* Get timestamp after send completes and send follow-up message
   *
   * TODO: Implement SO_TIMINGS and use the actual tx timestamp here.
   */

  ptp_gettime(state, &ts);
  timespec_to_ptp_format(&ts, msg.origintimestamp);
  msg.header.messagetype = PTP_MSGTYPE_FOLLOW_UP;
  msg.header.flags[0] = 0;
  addr.sin_port = HTONS(PTP_UDP_PORT_INFO);

  ret = sendto(state->tx_socket, &msg, sizeof(msg), 0,
               (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      ptperr("sendto for follow-up message failed: %d\n", errno);
      return ret;
    }

  ptpinfo("Sent sync + follow-up, seq %ld\n",
          (long)ptp_get_sequence(&msg.header));

  return OK;
}

/* Check if we need to send packets */

static int ptp_periodic_send(struct ptp_state_s *state)
{
#ifdef CONFIG_NETUTILS_PTPD_SERVER
  /* If there is no better master clock on the network,
   * act as the reference source and send server packets.
   */

  if (!is_selected_source_valid(state))
    {
      struct timespec time_now;
      struct timespec delta;

      clock_gettime(CLOCK_MONOTONIC, &time_now);
      clock_timespec_subtract(&time_now,
        &state->last_transmitted_announce, &delta);
      if (timespec_to_ms(&delta)
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
#endif

  return OK;
}

/* Process received PTP announcement */

static int ptp_process_announce(struct ptp_state_s *state,
                                struct ptp_announce_s *msg)
{
  if (is_better_clock(msg, &state->own_identity))
    {
      if (!is_selected_source_valid(state) ||
          is_better_clock(msg, &state->selected_source))
        {
          ptpinfo("Switching to better PTP time source\n");

          state->selected_source = *msg;
          clock_gettime(CLOCK_MONOTONIC, &state->last_received_sync);
        }
    }

  return OK;
}

/* Update local clock by delta, either by smooth adjustment or by jumping. */

static int ptp_update_local_clock(struct ptp_state_s *state,
                                  struct timespec *delta)
{
  int ret;
  struct timespec local_time;

  if (timespec_to_ms(delta) > CONFIG_NETUTILS_PTPD_SETTIME_THRESHOLD_MS)
    {
      /* Add delta to current local time in order to account for any latency
       * between packet reception and clock setting.
       */

      ptp_gettime(state, &local_time);
      clock_timespec_add(&local_time, delta, &local_time);
      ret = ptp_settime(state, &local_time);

      if (ret == OK)
        {
          ptpinfo("Jumped to timestamp %ld.%09ld s\n",
            (long)local_time.tv_sec, (long)local_time.tv_nsec);
        }
      else
        {
          ptperr("ptp_settime() failed: %d\n", errno);
        }
    }
  else
    {
      ret = ptp_adjtime(state, delta);

      if (ret == OK)
        {
          ptpinfo("Adjusting clock by %ld.%09ld s\n", (long)delta->tv_sec,
                  (long)delta->tv_nsec);
        }
      else
        {
          ptperr("ptp_adjtime() failed: %d\n", errno);
        }
    }

  return ret;
}

/* Process received PTP sync packet */

static int ptp_process_sync(struct ptp_state_s *state,
                            struct ptp_sync_s *msg)
{
  struct timespec remote_time;
  struct timespec local_time;
  struct timespec delta;

  if (memcmp(msg->header.sourceidentity,
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

      ptp_getrxtime(state, &state->twostep_rxtime);
      state->twostep_packet = *msg;
      ptpinfo("Waiting for follow-up\n");
      return OK;
    }

  /* Calculate delta between local and remote time */

  ptp_format_to_timespec(msg->origintimestamp, &remote_time);
  ptp_getrxtime(state, &local_time);
  clock_timespec_subtract(&remote_time, &local_time, &delta);

  return ptp_update_local_clock(state, &delta);
}

static int ptp_process_followup(struct ptp_state_s *state,
                                struct ptp_follow_up_s *msg)
{
  struct timespec remote_time;
  struct timespec delta;

  if (memcmp(msg->header.sourceidentity,
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

  ptp_format_to_timespec(msg->origintimestamp, &remote_time);
  clock_timespec_subtract(&remote_time, &state->twostep_rxtime, &delta);

  return ptp_update_local_clock(state, &delta);
}

/* Determine received packet type and process it */

static int ptp_process_rx_packet(struct ptp_state_s *state, ssize_t length)
{
  ptpwarn("Got packet: %d bytes\n", length);

  if (length < sizeof(struct ptp_header_s))
    {
      ptpwarn("Ignoring invalid PTP packet, length only %d bytes\n",
              (int)length);
      return OK;
    }

  if (state->rxbuf.header.domain != CONFIG_NETUTILS_PTPD_DOMAIN)
    {
      /* Part of different clock domain, ignore */

      return OK;
    }

  clock_gettime(CLOCK_MONOTONIC, &state->last_received_multicast);

  switch (state->rxbuf.header.messagetype & PTP_MSGTYPE_MASK)
  {
#ifdef CONFIG_NETUTILS_PTPD_CLIENT
    case PTP_MSGTYPE_ANNOUNCE:
      ptpinfo("Got announce packet, seq %ld\n",
              (long)ptp_get_sequence(&state->rxbuf.header));
      return ptp_process_announce(state, &state->rxbuf.announce);

    case PTP_MSGTYPE_SYNC:
      ptpinfo("Got sync packet, seq %ld\n",
              (long)ptp_get_sequence(&state->rxbuf.header));
      return ptp_process_sync(state, &state->rxbuf.sync);

    case PTP_MSGTYPE_FOLLOW_UP:
      ptpinfo("Got follow-up packet, seq %ld\n",
              (long)ptp_get_sequence(&state->rxbuf.header));
      return ptp_process_followup(state, &state->rxbuf.follow_up);
#endif

    default:
      ptpinfo("Ignoring unknown PTP packet type: 0x%02x\n",
              state->rxbuf.header.messagetype);
      return OK;
  }
}

static int ptp_daemon(int argc, FAR char** argv)
{
  const char *interface = "eth0";
  struct ptp_state_s *state;
  struct pollfd pollfds[2];
  struct msghdr rxhdr;
  struct iovec rxiov;
  int ret;

  memset(&rxhdr, 0, sizeof(rxhdr));
  memset(&rxiov, 0, sizeof(rxiov));

  state = calloc(1, sizeof(struct ptp_state_s));

  if (argc > 1)
    {
      interface = argv[1];
    }

  if (ptp_initialize_state(state, interface) != OK)
    {
      ptperr("Failed to initialize PTP state, exiting\n");
      return ERROR;
    }

  pollfds[0].events = POLLIN;
  pollfds[0].fd = state->event_socket;
  pollfds[1].events = POLLIN;
  pollfds[1].fd = state->info_socket;

  while (!state->stop)
    {
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
      ret = poll(pollfds, 2, PTPD_POLL_INTERVAL);
      ptp_gettime(state, &state->rxtime);

      if (pollfds[0].revents)
        {
          /* Receive time-critical packet, potentially with cmsg
           * indicating the timestamp.
           */

          ret = recvmsg(state->event_socket, &rxhdr, MSG_DONTWAIT);
          if (ret > 0)
            {
              ptp_process_rx_packet(state, ret);
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
    }

  ptp_destroy_state(state);
  free(state);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ptpd_start
 *
 * Description:
 *   Start the PTP daemon and bind it to specified interface.
 *
 * Returned Value:
 *   On success, the non-negative task ID of the PTP daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ptpd_start(const char *interface)
{
  int pid;
  FAR char *task_argv[] = {
    (FAR char *)interface,
    NULL
  };

  pid = task_create("PTPD", CONFIG_NETUTILS_PTPD_SERVERPRIO,
    CONFIG_NETUTILS_PTPD_STACKSIZE, ptp_daemon, task_argv);

  /* Use kill with signal 0 to check if the process is still alive
   * after initialization.
   */

  usleep(USEC_PER_TICK);
  if (kill(pid, 0) != OK)
    {
      return -1;
    }
  else
    {
      return pid;
    }
}

/* TODO: Implement status and stop interfaces */
