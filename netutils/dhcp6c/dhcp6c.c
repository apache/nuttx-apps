/****************************************************************************
 * apps/netutils/dhcp6c/dhcp6c.c
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
#include <nuttx/compiler.h>

#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <resolv.h>
#include <string.h>
#include <unistd.h>
#include <debug.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/time.h>
#include <nuttx/clock.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include "netutils/dhcp6c.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DHCPV6_ALL_RELAYS {{{0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02}}}
#define DHCPV6_CLIENT_PORT 546
#define DHCPV6_SERVER_PORT 547
#define DHCPV6_DUID_LLADDR 3
#define DHCPV6_REQ_DELAY 1

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define dhcpv6_for_each_option(_o, start, end, otype, olen, odata)\
    for ((_o) = (FAR uint8_t *)(start); (_o) + 4 <= (FAR uint8_t *)(end) &&\
        ((otype) = (_o)[0] << 8 | (_o)[1]) && ((odata) = (FAR void *)&(_o)[4]) &&\
        ((olen) = (_o)[2] << 8 | (_o)[3]) + (odata) <= (FAR uint8_t *)(end); \
        (_o) += 4 + ((_o)[2] << 8 | (_o)[3]))

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum dhcpv6_opt_e
{
  DHCPV6_OPT_CLIENTID = 1,
  DHCPV6_OPT_SERVERID = 2,
  DHCPV6_OPT_IA_NA = 3,
  DHCPV6_OPT_IA_ADDR = 5,
  DHCPV6_OPT_ORO = 6,
  DHCPV6_OPT_PREF = 7,
  DHCPV6_OPT_ELAPSED = 8,
  DHCPV6_OPT_RELAY_MSG = 9,
  DHCPV6_OPT_AUTH = 11,
  DHCPV6_OPT_STATUS = 13,
  DHCPV6_OPT_RAPID_COMMIT = 14,
  DHCPV6_OPT_RECONF_MESSAGE = 19,
  DHCPV6_OPT_RECONF_ACCEPT = 20,
  DHCPV6_OPT_DNS_SERVERS = 23,
  DHCPV6_OPT_DNS_DOMAIN = 24,
  DHCPV6_OPT_IA_PD = 25,
  DHCPV6_OPT_IA_PREFIX = 26,
  DHCPV6_OPT_INFO_REFRESH = 32,
  DHCPV6_OPT_FQDN = 39,
  DHCPV6_OPT_NTP_SERVER = 56,
  DHCPV6_OPT_SIP_SERVER_D = 21,
  DHCPV6_OPT_SIP_SERVER_A = 22,
};

enum dhcpv6_opt_npt_e
{
  NTP_SRV_ADDR = 1,
  NTP_MC_ADDR = 2,
  NTP_SRV_FQDN = 3
};

enum dhcpv6_msg_e
{
  DHCPV6_MSG_UNKNOWN = 0,
  DHCPV6_MSG_SOLICIT = 1,
  DHCPV6_MSG_ADVERT = 2,
  DHCPV6_MSG_REQUEST = 3,
  DHCPV6_MSG_RENEW = 5,
  DHCPV6_MSG_REBIND = 6,
  DHCPV6_MSG_REPLY = 7,
  DHCPV6_MSG_RELEASE = 8,
  DHCPV6_MSG_DECLINE = 9,
  DHCPV6_MSG_RECONF = 10,
  DHCPV6_MSG_INFO_REQ = 11,
  DHCPV6_MSG_MAX
};

enum dhcpv6_status_e
{
  DHCPV6_NOADDRSAVAIL = 2,
  DHCPV6_NOPREFIXAVAIL = 6
};

enum dhcpv6_mode_e
{
  DHCPV6_UNKNOWN,
  DHCPV6_STATELESS,
  DHCPV6_STATEFUL
};

enum dhcpv6_state_e
{
  STATE_CLIENT_ID,
  STATE_SERVER_ID,
  STATE_SERVER_CAND,
  STATE_ORO,
  STATE_DNS,
  STATE_SEARCH,
  STATE_IA_NA,
  STATE_IA_PD,
  STATE_CUSTOM_OPTS,
  STATE_SNTP_IP,
  STATE_SNTP_FQDN,
  STATE_SIP_IP,
  STATE_SIP_FQDN,
  STATE_MAX
};

enum dhcp6c_ia_mode_e
{
  IA_MODE_NONE,
  IA_MODE_TRY,
  IA_MODE_FORCE,
};

/* DHCPV6 Protocol Headers */

begin_packed_struct struct dhcpv6_header_s
{
  uint8_t msg_type;
  uint8_t tr_id[3];
} end_packed_struct;

begin_packed_struct struct dhcpv6_ia_hdr_s
{
  uint16_t type;
  uint16_t len;
  uint32_t iaid;
  uint32_t t1;
  uint32_t t2;
} end_packed_struct;

begin_packed_struct struct dhcpv6_ia_addr_s
{
  uint16_t type;
  uint16_t len;
  struct in6_addr addr;
  uint32_t preferred;
  uint32_t valid;
} end_packed_struct;

begin_packed_struct struct dhcpv6_ia_prefix_s
{
  uint16_t type;
  uint16_t len;
  uint32_t preferred;
  uint32_t valid;
  uint8_t prefix;
  struct in6_addr addr;
} end_packed_struct;

struct dhcpv6_server_cand_s
{
  bool has_noaddravail;
  bool wants_reconfigure;
  int16_t preference;
  uint8_t duid_len;
  uint8_t duid[130];
};

struct dhcp6c_retx_s
{
  bool delay;
  uint8_t init_timeo;
  uint16_t max_timeo;
  char name[8];
  int(*handler_reply)(FAR void *handle, enum dhcpv6_msg_e orig,
                      FAR const void *opt, FAR const void *end,
                      uint32_t elapsed);
  int(*handler_finish)(FAR void *handle, uint32_t elapsed);
};

struct dhcp6c_state_s
{
  pthread_t thread;
  bool cancel;
  dhcp6c_callback_t callback;
  int sockfd;
  int urandom_fd;
  int ifindex;
  time_t t1;
  time_t t2;
  time_t t3;
  bool request_prefix;
  enum dhcp6c_ia_mode_e ia_mode;
  bool accept_reconfig;
  FAR uint8_t *state_data[STATE_MAX];
  size_t state_len[STATE_MAX];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int dhcp6c_handle_reconfigure(FAR void *handle,
                                     enum dhcpv6_msg_e orig,
                                     FAR const void *opt,
                                     FAR const void *end,
                                     uint32_t elapsed);
static int dhcp6c_handle_advert(FAR void *handle, enum dhcpv6_msg_e orig,
                                FAR const void *opt, FAR const void *end,
                                uint32_t elapsed);
static int dhcp6c_commit_advert(FAR void *handle, uint32_t elapsed);
static int dhcp6c_handle_reply(FAR void *handle, enum dhcpv6_msg_e orig,
                               FAR const void *opt, FAR const void *end,
                               uint32_t elapsed);
static int dhcp6c_handle_rebind_reply(FAR void *handle,
                                      enum dhcpv6_msg_e orig,
                                      FAR const void *opt,
                                      FAR const void *end,
                                      uint32_t elapsed);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* RFC 3315 - 5.5 Timeout and Delay values */

static const struct dhcp6c_retx_s g_dhcp6c_retx[DHCPV6_MSG_MAX] =
{
  {false, 1, 120, "<POLL>", dhcp6c_handle_reconfigure, NULL},
  {true, 1, 120, "SOLICIT", dhcp6c_handle_advert, dhcp6c_commit_advert},
  {0},
  {true, 1, 30, "REQUEST", dhcp6c_handle_reply, NULL},
  {0},
  {false, 10, 600, "RENEW", dhcp6c_handle_reply, NULL},
  {false, 10, 600, "REBIND", dhcp6c_handle_rebind_reply, NULL},
  {0},
  {false, 1, 600, "RELEASE", NULL, NULL},
  {false, 1, 3, "DECLINE", NULL, NULL},
  {0},
  {true, 1, 120, "INFOREQ", dhcp6c_handle_reply, NULL},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint64_t dhcp6c_get_milli_time(void)
{
  struct timespec t;

  clock_gettime(CLOCK_MONOTONIC, &t);

  return t.tv_sec * MSEC_PER_SEC + t.tv_nsec / USEC_PER_SEC;
}

static FAR uint8_t *dhcp6c_resize_state(FAR void *handle,
                                        enum dhcpv6_state_e state,
                                        ssize_t len)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  FAR uint8_t *n;

  if (len == 0)
    {
      return pdhcp6c->state_data[state] + pdhcp6c->state_len[state];
    }

  n = realloc(pdhcp6c->state_data[state], pdhcp6c->state_len[state] + len);
  if (n != NULL || pdhcp6c->state_len[state] + len == 0)
    {
      pdhcp6c->state_data[state] = n;
      n += pdhcp6c->state_len[state];
      pdhcp6c->state_len[state] += len;
    }

  return n;
}

static void dhcp6c_clear_state(FAR void *handle, enum dhcpv6_state_e state)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;

  pdhcp6c->state_len[state] = 0;
}

static void dhcp6c_add_state(FAR void *handle, enum dhcpv6_state_e state,
                             FAR const void *data, size_t len)
{
  FAR uint8_t *n = dhcp6c_resize_state(handle, state, len);

  if (n != NULL)
    {
      memcpy(n, data, len);
    }
}

static size_t dhcp6c_remove_state(FAR void *handle,
                                  enum dhcpv6_state_e state,
                                  size_t offset, size_t len)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  FAR uint8_t *data = pdhcp6c->state_data[state];
  ssize_t len_after = pdhcp6c->state_len[state] - (offset + len);

  if (len_after < 0)
    {
      return pdhcp6c->state_len[state];
    }

  memmove(data + offset, data + offset + len, len_after);

  return pdhcp6c->state_len[state] -= len;
}

static bool dhcp6c_commit_state(FAR void *handle, enum dhcpv6_state_e state,
                                size_t old_len)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  size_t new_len = pdhcp6c->state_len[state] - old_len;
  FAR uint8_t *old_data = pdhcp6c->state_data[state];
  FAR uint8_t *new_data = old_data + old_len;
  bool upd = (new_len != old_len) ||
             (memcmp(old_data, new_data, new_len) != 0);

  memmove(old_data, new_data, new_len);
  dhcp6c_resize_state(handle, state, -old_len);

  return upd;
}

static FAR void *dhcp6c_get_state(FAR void *handle,
                                  enum dhcpv6_state_e state,
                                  FAR size_t *len)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;

  *len = pdhcp6c->state_len[state];
  return pdhcp6c->state_data[state];
}

static void dhcp6c_get_result(FAR void *handle,
                              FAR struct dhcp6c_state *presult)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  size_t s_len;
  FAR uint8_t *s_data;
  uint16_t olen;
  uint16_t otype;
  FAR uint8_t *ite;
  FAR uint8_t *odata;
  FAR struct dhcpv6_ia_addr_s *addr;
  FAR struct dhcpv6_ia_prefix_s *pd;
  FAR struct in6_addr *dns;
  char addr_str[INET6_ADDRSTRLEN];

  if (handle == NULL || presult == NULL)
    {
      return;
    }

  s_data = dhcp6c_get_state(handle, STATE_IA_NA, &s_len);
  dhcpv6_for_each_option(ite, s_data, s_data + s_len, otype, olen, odata)
    {
      addr = (FAR void *)(odata - 4);
      memcpy(&presult->addr, &addr->addr, sizeof(presult->addr));
      inet_ntop(AF_INET6, &presult->addr, addr_str, sizeof(addr_str));
      ninfo("IA_NA %s for iface %i\n", addr_str, pdhcp6c->ifindex);
    }

  s_data = dhcp6c_get_state(handle, STATE_IA_PD, &s_len);
  dhcpv6_for_each_option(ite, s_data, s_data + s_len, otype, olen, odata)
    {
      pd = (FAR void *)(odata - 4);
      memcpy(&presult->pd, &pd->addr, sizeof(presult->pd));
      presult->pl = pd->prefix;
      netlib_prefix2ipv6netmask(presult->pl, &presult->netmask);
      inet_ntop(AF_INET6, &presult->pd, addr_str, sizeof(addr_str));
      ninfo("IA_PD %s for iface %i\n", addr_str, pdhcp6c->ifindex);
      inet_ntop(AF_INET6, &presult->netmask, addr_str, sizeof(addr_str));
      ninfo("netmask %s for iface %i\n", addr_str, pdhcp6c->ifindex);
    }

  dns = dhcp6c_get_state(handle, STATE_DNS, &s_len);
  memcpy(&presult->dns, dns, sizeof(presult->dns));
  inet_ntop(AF_INET6, &presult->dns, addr_str, sizeof(addr_str));
  ninfo("DNS server %s for iface %i\n", addr_str, pdhcp6c->ifindex);

  presult->t1 = pdhcp6c->t1;
  presult->t2 = pdhcp6c->t2;
  ninfo("T1:%d T2:%d for iface %i\n", presult->t1, presult->t2,
        pdhcp6c->ifindex);
}

static void dhcp6c_switch_process(FAR void *handle, FAR const char *name)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  struct dhcp6c_state result;

  ninfo("Process switch to %s\n", name);

  /* Delete lost prefixes and user opts */

  dhcp6c_clear_state(handle, STATE_CUSTOM_OPTS);

  if (pdhcp6c->callback != NULL)
    {
      memset(&result, 0, sizeof(result));
      dhcp6c_get_result(pdhcp6c, &result);
      pdhcp6c->callback(&result);
    }
}

static void dhcp6c_remove_addrs(FAR void *handle)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  size_t ia_na_len;
  FAR uint8_t *ite;
  FAR uint8_t *odata;
  FAR uint8_t *ia_na = dhcp6c_get_state(handle, STATE_IA_NA, &ia_na_len);
  uint16_t otype;
  uint16_t olen;
  FAR struct dhcpv6_ia_addr_s *addr;
  char addr_str[INET6_ADDRSTRLEN];

  dhcpv6_for_each_option(ite, ia_na, ia_na + ia_na_len, otype, olen, odata)
    {
      addr = (FAR void *)(odata - 4);
      inet_ntop(AF_INET6, &addr->addr, addr_str, sizeof(addr_str));
      ninfo("removing address %s/128 for iface %i\n",
            addr_str, pdhcp6c->ifindex);
    }
}

static void dhcp6c_set_iov(FAR struct iovec *piov,
                           FAR void *base, size_t len)
{
  piov->iov_base = base;
  piov->iov_len = len;
}

static void dhcp6c_send(FAR void *handle, enum dhcpv6_msg_e type,
                        uint8_t trid[3], uint32_t ecs)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  char fqdn_buf[256];
  struct
  {
    uint16_t type;
    uint16_t len;
    uint8_t flags;
    uint8_t data[256];
  } fqdn;

  size_t fqdn_len;
  size_t cl_id_len;
  FAR void *cl_id;
  size_t srv_id_len;
  FAR void *srv_id;
  size_t ia_pd_len;
  FAR void *ia_pd;
  struct dhcpv6_ia_hdr_s hdr_ia_pd;
  struct dhcpv6_ia_prefix_s pref;
  size_t ia_na_len;
  FAR void *ia_na;
  struct dhcpv6_ia_hdr_s hdr_ia_na;
  struct
  {
    uint16_t type;
    uint16_t length;
  } reconf_accept;

  uint16_t oro_refresh;
  size_t oro_len;
  FAR void *oro;
  struct
  {
    uint8_t type;
    uint8_t trid[3];
    uint16_t elapsed_type;
    uint16_t elapsed_len;
    uint16_t elapsed_value;
    uint16_t oro_type;
    uint16_t oro_len;
  } hdr;

  struct iovec iov[11];
  size_t cnt;
  struct sockaddr_in6 dest =
  {
    AF_INET6, htons(DHCPV6_SERVER_PORT),
    0, DHCPV6_ALL_RELAYS, pdhcp6c->ifindex
  };

  struct msghdr msg;

  /* Build FQDN */

  gethostname(fqdn_buf, sizeof(fqdn_buf));
  fqdn_len = 5 + dn_comp(fqdn_buf, fqdn.data, sizeof(fqdn.data), NULL, NULL);
  fqdn.type = htons(DHCPV6_OPT_FQDN);
  fqdn.len = htons(fqdn_len - 4);
  fqdn.flags = 0;

  /* Build Client ID */

  cl_id = dhcp6c_get_state(handle, STATE_CLIENT_ID, &cl_id_len);

  /* Build Server ID */

  srv_id = dhcp6c_get_state(handle, STATE_SERVER_ID, &srv_id_len);

  /* Build IA_PDs */

  ia_pd = dhcp6c_get_state(handle, STATE_IA_PD, &ia_pd_len);
  hdr_ia_pd.type = htons(DHCPV6_OPT_IA_PD);
  hdr_ia_pd.len = htons(sizeof(hdr_ia_pd) - 4 + ia_pd_len);
  hdr_ia_pd.iaid = 1;
  hdr_ia_pd.t1 = 0;
  hdr_ia_pd.t2 = 0;
  pref.type = htons(DHCPV6_OPT_IA_PREFIX);
  pref.len = htons(25);
  pref.prefix = pdhcp6c->request_prefix;

  if (ia_pd_len == 0 && pdhcp6c->request_prefix &&
      (type == DHCPV6_MSG_SOLICIT || type == DHCPV6_MSG_REQUEST))
    {
      ia_pd = &pref;
      ia_pd_len = sizeof(pref);
    }

  /* Build IA_NAs */

  ia_na = dhcp6c_get_state(handle, STATE_IA_NA, &ia_na_len);
  hdr_ia_na.type = htons(DHCPV6_OPT_IA_NA);
  hdr_ia_na.len = htons(sizeof(hdr_ia_na) - 4 + ia_na_len);
  hdr_ia_na.iaid = 1;
  hdr_ia_na.t1 = 0;
  hdr_ia_na.t2 = 0;

  /* Reconfigure Accept */

  reconf_accept.type = htons(DHCPV6_OPT_RECONF_ACCEPT);
  reconf_accept.length = 0;

  /* Request Information Refresh */

  oro_refresh = htons(DHCPV6_OPT_INFO_REFRESH);

  /* Prepare Header */

  oro = dhcp6c_get_state(handle, STATE_ORO, &oro_len);
  hdr.type = type;
  hdr.trid[0] = trid[0];
  hdr.trid[1] = trid[1];
  hdr.trid[2] = trid[2];
  hdr.elapsed_type = htons(DHCPV6_OPT_ELAPSED);
  hdr.elapsed_len =  htons(2);
  hdr.elapsed_value = htons((ecs > 0xffff) ? 0xffff : ecs);
  hdr.oro_type = htons(DHCPV6_OPT_ORO);
  hdr.oro_len = htons(oro_len);

  /* Prepare iov */

  dhcp6c_set_iov(&iov[0], &hdr, sizeof(hdr));
  dhcp6c_set_iov(&iov[1], oro, oro_len);
  dhcp6c_set_iov(&iov[2], &oro_refresh, 0);
  dhcp6c_set_iov(&iov[3], cl_id, cl_id_len);
  dhcp6c_set_iov(&iov[4], srv_id, srv_id_len);
  dhcp6c_set_iov(&iov[5], &reconf_accept, 0);
  dhcp6c_set_iov(&iov[6], &fqdn, fqdn_len);
  dhcp6c_set_iov(&iov[7], &hdr_ia_na, sizeof(hdr_ia_na));
  dhcp6c_set_iov(&iov[8], ia_na, ia_na_len);
  dhcp6c_set_iov(&iov[9], &hdr_ia_pd, sizeof(hdr_ia_pd));
  dhcp6c_set_iov(&iov[10], ia_pd, ia_pd_len);

  cnt = ARRAY_SIZE(iov);
  if (type == DHCPV6_MSG_INFO_REQ)
    {
      cnt = 5;
      iov[2].iov_len = sizeof(oro_refresh);
      hdr.oro_len = htons(oro_len + sizeof(oro_refresh));
    }
  else if (!pdhcp6c->request_prefix)
    {
      cnt = 9;
    }

  /* Disable IAs if not used */

  if (type == DHCPV6_MSG_SOLICIT)
    {
      iov[5].iov_len = sizeof(reconf_accept);
    }
  else if (type != DHCPV6_MSG_REQUEST)
    {
      if (ia_na_len == 0)
        {
          iov[7].iov_len = 0;
        }

      if (ia_pd_len == 0)
        {
          iov[9].iov_len = 0;
        }
    }

  if (pdhcp6c->ia_mode == IA_MODE_NONE)
    {
      iov[7].iov_len = 0;
    }

  msg.msg_name = &dest;
  msg.msg_namelen = sizeof(dest);
  msg.msg_iov = iov;
  msg.msg_iovlen = cnt;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  sendmsg(pdhcp6c->sockfd, &msg, 0);
}

static int64_t dhcp6c_rand_delay(FAR void *handle, int64_t time)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  int random;

  read(pdhcp6c->urandom_fd, &random, sizeof(random));
  return (time * (random % 1000)) / 10000;
}

static bool dhcp6c_response_is_valid(FAR void *handle, FAR const void *buf,
                                     ssize_t len,
                                     const uint8_t transaction[3],
                                     enum dhcpv6_msg_e type)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  FAR const struct dhcpv6_header_s *rep = buf;
  FAR uint8_t *ite;
  FAR uint8_t *end;
  FAR uint8_t *odata;
  uint16_t otype;
  uint16_t olen;
  bool clientid_ok = false;
  bool serverid_ok = false;
  size_t client_id_len;
  size_t server_id_len;
  FAR void *client_id;
  FAR void *server_id;

  if (len < sizeof(*rep) ||
      memcmp(rep->tr_id, transaction, sizeof(rep->tr_id)) != 0)
    {
      return false;
    }

  if (type == DHCPV6_MSG_SOLICIT)
    {
      if (rep->msg_type != DHCPV6_MSG_ADVERT &&
          rep->msg_type != DHCPV6_MSG_REPLY)
        {
          return false;
        }
    }
  else if (type == DHCPV6_MSG_UNKNOWN)
    {
      if (!pdhcp6c->accept_reconfig ||
          rep->msg_type != DHCPV6_MSG_RECONF)
        {
          return false;
        }
    }
  else if (rep->msg_type != DHCPV6_MSG_REPLY)
    {
      return false;
    }

  end = ((FAR uint8_t *)buf) + len;
  client_id = dhcp6c_get_state(handle, STATE_CLIENT_ID, &client_id_len);
  server_id = dhcp6c_get_state(handle, STATE_SERVER_ID, &server_id_len);

  dhcpv6_for_each_option(ite, &rep[1], end, otype, olen, odata)
    {
      if (otype == DHCPV6_OPT_CLIENTID)
        {
          clientid_ok = (olen + 4u == client_id_len) &&
                        (memcmp((odata - 4), client_id, client_id_len) == 0);
        }
      else if (otype == DHCPV6_OPT_SERVERID)
        {
          serverid_ok = (olen + 4u == server_id_len) &&
                        (memcmp((odata - 4), server_id, server_id_len) == 0);
        }
    }

  return clientid_ok && (serverid_ok || server_id_len == 0);
}

static int dhcp6c_command(FAR void *handle, enum dhcpv6_msg_e type)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  const int buf_length = 1536;
  FAR uint8_t *buf = (FAR uint8_t *)malloc(buf_length);
  uint32_t timeout = CONFIG_NETUTILS_DHCP6C_REQUEST_TIMEOUT < 3 ? 3 :
                     CONFIG_NETUTILS_DHCP6C_REQUEST_TIMEOUT;
  FAR const struct dhcp6c_retx_s *retx = &g_dhcp6c_retx[type];
  uint64_t start;
  uint64_t round_start;
  uint64_t round_end;
  uint64_t elapsed;
  uint8_t trid[3];
  ssize_t len = -1;
  int64_t rto = 0;

  if (buf == NULL)
    {
      return -1;
    }

  if (retx->delay)
    {
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = dhcp6c_rand_delay(handle, 10 * DHCPV6_REQ_DELAY);
      nanosleep(&ts, NULL);
    }

  if (type == DHCPV6_MSG_RELEASE || type == DHCPV6_MSG_DECLINE)
    {
      timeout = 3;
    }
  else if (type == DHCPV6_MSG_UNKNOWN)
    {
      timeout = pdhcp6c->t1;
    }
  else if (type == DHCPV6_MSG_RENEW)
    {
      timeout = pdhcp6c->t2 - pdhcp6c->t1;
    }
  else if (type == DHCPV6_MSG_REBIND)
    {
      timeout = pdhcp6c->t3 - pdhcp6c->t2;
    }

  if (timeout == 0)
    {
      len = -1;
      goto end;
    }

  ninfo("Sending %s (timeout %u s)\n", retx->name, timeout);
  start = dhcp6c_get_milli_time();
  round_start = start;

  /* Generate transaction ID */

  read(pdhcp6c->urandom_fd, trid, sizeof(trid));

  do
    {
      rto = (rto == 0) ? (retx->init_timeo * MSEC_PER_SEC +
              dhcp6c_rand_delay(handle, retx->init_timeo * MSEC_PER_SEC)) :
              (2 * rto + dhcp6c_rand_delay(handle, rto));

      if (rto >= retx->max_timeo * MSEC_PER_SEC)
        {
          rto = retx->max_timeo * MSEC_PER_SEC +
                dhcp6c_rand_delay(handle, retx->max_timeo * MSEC_PER_SEC);
        }

      /* Calculate end for this round and elapsed time */

      round_end = round_start + rto;
      elapsed = round_start - start;

      /* Don't wait too long */

      if (round_end - start > timeout * MSEC_PER_SEC)
        {
          round_end = timeout * MSEC_PER_SEC + start;
        }

      /* Built and send package */

      if (type != DHCPV6_MSG_UNKNOWN)
        {
          dhcp6c_send(handle, type, trid, elapsed / 10);
        }

      /* Receive rounds */

      for (; len < 0 && round_start < round_end;
              round_start = dhcp6c_get_milli_time())
        {
          /* Set timeout for receiving */

          uint64_t t = round_end - round_start;
          struct timeval retime =
          {
            t / MSEC_PER_SEC, (t % MSEC_PER_SEC) * MSEC_PER_SEC
          };

          /* check for dhcp6c_close */

          if (pdhcp6c->cancel)
            {
              len = -1;
              goto end;
            }

          setsockopt(pdhcp6c->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                     &retime, sizeof(retime));

          /* Receive cycle */

          len = recv(pdhcp6c->sockfd, buf, buf_length, 0);
          if (type != DHCPV6_MSG_UNKNOWN)
            {
              ninfo("%s[type:%d] recv len[%d]\n", __func__, type, len);
            }

          if (!dhcp6c_response_is_valid(handle, buf, len, trid, type))
            {
              len = -1;
            }

          if (len > 0)
            {
              FAR uint8_t *opt = &buf[4];
              FAR uint8_t *opt_end = opt + len - 4;

              round_start = dhcp6c_get_milli_time();
              elapsed = round_start - start;
              ninfo("Got a valid reply after %ums\n", (unsigned)elapsed);

              if (retx->handler_reply != NULL)
                {
                  len = retx->handler_reply(handle, type, opt,
                                            opt_end, elapsed / MSEC_PER_SEC);
                }
            }
        }

      if (retx->handler_finish != NULL)
        {
          len = retx->handler_finish(handle, elapsed / MSEC_PER_SEC);
        }
    }
  while (len < 0 && elapsed / MSEC_PER_SEC < timeout);

end:
  free(buf);
  return len;
}

static int dhcp6c_poll_reconfigure(FAR void *handle)
{
  int ret = dhcp6c_command(handle, DHCPV6_MSG_UNKNOWN);
  if (ret != -1)
    {
      ret = dhcp6c_command(handle, ret);
    }

  return ret;
}

/* Collect all advertised servers */

static int dhcp6c_handle_advert(FAR void *handle, enum dhcpv6_msg_e orig,
                                FAR const void *opt, FAR const void *end,
                                uint32_t elapsed)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  uint16_t olen;
  uint16_t otype;
  FAR uint8_t *ite0;
  FAR uint8_t *odata;
  struct dhcpv6_server_cand_s cand;

  memset(&cand, 0, sizeof(cand));
  dhcpv6_for_each_option(ite0, opt, end, otype, olen, odata)
    {
      if (otype == DHCPV6_OPT_SERVERID && olen <= 130)
        {
          memcpy(cand.duid, odata, olen);
          cand.duid_len = olen;
        }
      else if (otype == DHCPV6_OPT_STATUS && olen >= 2 && !odata[0]
               && odata[1] == DHCPV6_NOADDRSAVAIL)
        {
          if (pdhcp6c->ia_mode == IA_MODE_FORCE)
            {
              return -1;
            }
          else
            {
              cand.has_noaddravail = true;
              cand.preference -= 1000;
            }
        }
      else if (otype == DHCPV6_OPT_STATUS && olen >= 2 && !odata[0]
               && odata[1] == DHCPV6_NOPREFIXAVAIL)
        {
          cand.preference -= 2000;
        }
      else if (otype == DHCPV6_OPT_PREF && olen >= 1 &&
               cand.preference >= 0)
        {
          cand.preference = odata[1];
        }
      else if (otype == DHCPV6_OPT_RECONF_ACCEPT)
        {
          cand.wants_reconfigure = true;
        }
      else if (otype == DHCPV6_OPT_IA_PD && pdhcp6c->request_prefix)
        {
          FAR struct dhcpv6_ia_hdr_s *h = (FAR void *)odata;
          FAR uint8_t *oend = odata + olen;
          FAR uint8_t *ite1;
          FAR uint8_t *d;

          dhcpv6_for_each_option(ite1, &h[1], oend, otype, olen, d)
            {
              if (otype == DHCPV6_OPT_IA_PREFIX)
                {
                  cand.preference += 2000;
                }
              else if (otype == DHCPV6_OPT_STATUS &&
                       olen >= 2 && d[0] == 0 &&
                       d[1] == DHCPV6_NOPREFIXAVAIL)
                {
                  cand.preference -= 2000;
                }
            }
        }
    }

  if (cand.duid_len > 0)
    {
      dhcp6c_add_state(handle, STATE_SERVER_CAND, &cand, sizeof(cand));
    }

  return 0;
}

static int dhcp6c_commit_advert(FAR void *handle, uint32_t elapsed)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  size_t cand_len;
  FAR struct dhcpv6_server_cand_s *c = NULL;
  FAR struct dhcpv6_server_cand_s *cand = dhcp6c_get_state(handle,
                                                           STATE_SERVER_CAND,
                                                           &cand_len);
  bool retry = false;

  for (size_t i = 0; i < cand_len / sizeof(*c); ++i)
    {
      if (cand[i].has_noaddravail)
        {
          retry = true;
        }

      if (c == NULL || c->preference < cand[i].preference)
        {
          c = &cand[i];
        }
    }

  if (retry && pdhcp6c->ia_mode == IA_MODE_TRY)
    {
      /* We give it a second try without the IA_NA */

      pdhcp6c->ia_mode = IA_MODE_NONE;
      return dhcp6c_command(handle, DHCPV6_MSG_SOLICIT);
    }

  if (c != NULL)
    {
      uint16_t hdr[2] =
      {
        htons(DHCPV6_OPT_SERVERID), htons(c->duid_len)
      };

      dhcp6c_add_state(handle, STATE_SERVER_ID, hdr, sizeof(hdr));
      dhcp6c_add_state(handle, STATE_SERVER_ID, c->duid, c->duid_len);
      pdhcp6c->accept_reconfig = c->wants_reconfigure;
    }

  dhcp6c_clear_state(handle, STATE_SERVER_CAND);

  if (c == NULL)
    {
      return -1;
    }
  else if (pdhcp6c->request_prefix || pdhcp6c->ia_mode != IA_MODE_NONE)
    {
      return DHCPV6_STATEFUL;
    }
  else
    {
      return DHCPV6_STATELESS;
    }
}

static time_t dhcp6c_parse_ia(FAR void *handle, FAR void *opt, FAR void *end)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  uint32_t timeout = UINT32_MAX;
  uint16_t otype;
  uint16_t olen;
  uint16_t stype;
  uint16_t slen;
  FAR uint8_t *ite0;
  FAR uint8_t *odata;
  FAR uint8_t *sdata;

  /* Update address IA */

  dhcpv6_for_each_option(ite0, opt, end, otype, olen, odata)
    {
      if (otype == DHCPV6_OPT_IA_PREFIX)
        {
          FAR struct dhcpv6_ia_prefix_s *prefix = (FAR void *)(odata - 4);
          FAR struct dhcpv6_ia_prefix_s *local = NULL;
          uint32_t valid;
          uint32_t pref;
          size_t pd_len;
          FAR uint8_t *pd;
          FAR uint8_t *ite1;

          if (olen + 4u < sizeof(*prefix))
            {
              continue;
            }

          olen = sizeof(*prefix);
          valid = ntohl(prefix->valid);
          pref = ntohl(prefix->preferred);

          if (pref > valid)
            {
              continue;
            }

          /* Search matching IA */

          pd = dhcp6c_get_state(handle, STATE_IA_PD, &pd_len);
          dhcpv6_for_each_option(ite1, pd, pd + pd_len,
                                 stype, slen, sdata)
            {
              if (memcmp(sdata + 8, odata + 8,
                         sizeof(local->addr) + 1) == 0)
                {
                  local = (FAR void *)(sdata - 4);
                }
            }

          if (local != NULL)
            {
              local->preferred = prefix->preferred;
              local->valid = prefix->valid;
            }
          else
            {
              dhcp6c_add_state(handle, STATE_IA_PD, prefix, olen);
            }

          if (timeout > valid)
            {
              timeout = valid;
            }
        }
      else if (otype == DHCPV6_OPT_IA_ADDR)
        {
          FAR struct dhcpv6_ia_addr_s *addr = (FAR void *)(odata - 4);
          FAR struct dhcpv6_ia_addr_s *local = NULL;
          uint32_t pref;
          uint32_t valid;
          size_t na_len;
          FAR uint8_t *na;
          FAR uint8_t *ite1;

          if (olen + 4u < sizeof(*addr))
            {
              continue;
            }

          olen = sizeof(*addr);
          pref = ntohl(addr->preferred);
          valid = ntohl(addr->valid);

          if (pref > valid)
            {
              continue;
            }

          /* Search matching IA */

          na = dhcp6c_get_state(handle, STATE_IA_NA, &na_len);
          dhcpv6_for_each_option(ite1, na, na + na_len,
                                 stype, slen, sdata)
            {
              if (memcmp(sdata, odata, sizeof(local->addr)) == 0)
                {
                  local = (FAR void *)(sdata - 4);
                }
            }

          if (local != NULL)
            {
              local->preferred = addr->preferred;
              local->valid = addr->valid;
            }
          else
            {
              dhcp6c_add_state(handle, STATE_IA_NA, addr, olen);
            }

          if (timeout > valid)
            {
              timeout = valid;
            }
        }
    }

  return timeout;
}

static int dhcp6c_handle_reply(FAR void *handle, enum dhcpv6_msg_e orig,
                               FAR const void *opt, FAR const void *end,
                               uint32_t elapsed)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  uint16_t otype;
  uint16_t olen;
  FAR uint8_t *ite0;
  FAR uint8_t *odata;
  bool have_update = false;
  size_t ia_na_len;
  size_t ia_pd_len;
  size_t dns_len;
  size_t search_len;
  size_t sntp_ip_len;
  size_t sntp_dns_len;
  size_t sip_ip_len;
  size_t sip_fqdn_len;
  FAR uint8_t *ia_na = dhcp6c_get_state(handle, STATE_IA_NA, &ia_na_len);
  FAR uint8_t *ia_pd = dhcp6c_get_state(handle, STATE_IA_PD, &ia_pd_len);
  FAR uint8_t *ia_end;
  pdhcp6c->t1 = UINT32_MAX;
  pdhcp6c->t2 = UINT32_MAX;
  pdhcp6c->t3 = UINT32_MAX;

  dhcp6c_get_state(handle, STATE_DNS, &dns_len);
  dhcp6c_get_state(handle, STATE_SEARCH, &search_len);
  dhcp6c_get_state(handle, STATE_SNTP_IP, &sntp_ip_len);
  dhcp6c_get_state(handle, STATE_SNTP_FQDN, &sntp_dns_len);
  dhcp6c_get_state(handle, STATE_SIP_IP, &sip_ip_len);
  dhcp6c_get_state(handle, STATE_SIP_FQDN, &sip_fqdn_len);

  /* Decrease valid and preferred lifetime of prefixes */

  dhcpv6_for_each_option(ite0, ia_pd, ia_pd + ia_pd_len, otype, olen, odata)
    {
      FAR struct dhcpv6_ia_prefix_s *p = (FAR void *)(odata - 4);
      uint32_t valid = ntohl(p->valid);
      uint32_t pref = ntohl(p->preferred);

      if (valid != UINT32_MAX)
        {
          p->valid = (valid < elapsed) ? 0 : htonl(valid - elapsed);
        }

      if (pref != UINT32_MAX)
        {
          p->preferred = (pref < elapsed) ? 0 : htonl(pref - elapsed);
        }
    }

  /* Decrease valid and preferred lifetime of addresses */

  dhcpv6_for_each_option(ite0, ia_na, ia_na + ia_na_len, otype, olen, odata)
    {
      FAR struct dhcpv6_ia_addr_s *p = (FAR void *)(odata - 4);
      uint32_t valid = ntohl(p->valid);
      uint32_t pref = ntohl(p->preferred);

      if (valid != UINT32_MAX)
        {
          p->valid = (valid < elapsed) ? 0 : htonl(valid - elapsed);
        }

      if (pref != UINT32_MAX)
        {
          p->preferred = (pref < elapsed) ? 0 : htonl(pref - elapsed);
        }
    }

  /* Parse and find all matching IAs */

  dhcpv6_for_each_option(ite0, opt, end, otype, olen, odata)
    {
      if ((otype == DHCPV6_OPT_IA_PD || otype == DHCPV6_OPT_IA_NA)
           && olen > sizeof(struct dhcpv6_ia_hdr_s))
        {
          FAR struct dhcpv6_ia_hdr_s *ia_hdr = (FAR void *)(odata - 4);
          time_t l_t1 = ntohl(ia_hdr->t1);
          time_t l_t2 = ntohl(ia_hdr->t2);
          uint16_t stype;
          uint16_t slen;
          FAR uint8_t *ite1;
          FAR uint8_t *sdata;
          time_t n;

          /* Test ID and T1-T2 validity */

          if (ia_hdr->iaid != 1 || l_t2 < l_t1)
            {
              continue;
            }

          /* Test status and bail if error */

          dhcpv6_for_each_option(ite1, &ia_hdr[1], odata + olen,
                                 stype, slen, sdata)
            {
              if (stype == DHCPV6_OPT_STATUS && slen >= 2 &&
                  (sdata[0] || sdata[1]))
                {
                  continue;
                }
            }

          /* Update times */

          if (l_t1 > 0 && pdhcp6c->t1 > l_t1)
            {
              pdhcp6c->t1 = l_t1;
            }

          if (l_t2 > 0 && pdhcp6c->t2 > l_t2)
            {
              pdhcp6c->t2 = l_t2;
            }

          /* Always report update in case we have IA_PDs so that
           * the state-script is called with updated times
           */

          if (otype == DHCPV6_OPT_IA_PD && pdhcp6c->request_prefix)
            {
              have_update = true;
            }

          n = dhcp6c_parse_ia(handle, &ia_hdr[1], odata + olen);
          if (n < pdhcp6c->t1)
            {
              pdhcp6c->t1 = n;
            }

          if (n < pdhcp6c->t2)
            {
              pdhcp6c->t2 = n;
            }

          if (n < pdhcp6c->t3)
            {
              pdhcp6c->t3 = n;
            }
        }
      else if (otype == DHCPV6_OPT_DNS_SERVERS)
        {
          if (olen % 16 == 0)
            {
              dhcp6c_add_state(handle, STATE_DNS, odata, olen);
            }
        }
      else if (otype == DHCPV6_OPT_DNS_DOMAIN)
        {
          dhcp6c_add_state(handle, STATE_SEARCH, odata, olen);
        }
      else if (otype == DHCPV6_OPT_NTP_SERVER)
        {
          uint16_t stype;
          uint16_t slen;
          FAR uint8_t *sdata;
          FAR uint8_t *ite1;

          /* Test status and bail if error */

          dhcpv6_for_each_option(ite1, odata, odata + olen,
                                 stype, slen, sdata)
            {
              if (slen == 16 &&
                  (stype == NTP_MC_ADDR || stype == NTP_SRV_ADDR))
                {
                  dhcp6c_add_state(handle, STATE_SNTP_IP, sdata, slen);
                }
              else if (slen > 0 && stype == NTP_SRV_FQDN)
                {
                  dhcp6c_add_state(handle, STATE_SNTP_FQDN, sdata, slen);
                }
            }
        }
      else if (otype == DHCPV6_OPT_SIP_SERVER_A)
        {
          if (olen == 16)
            {
              dhcp6c_add_state(handle, STATE_SIP_IP, odata, olen);
            }
        }
      else if (otype == DHCPV6_OPT_SIP_SERVER_D)
        {
          dhcp6c_add_state(handle, STATE_SIP_FQDN, odata, olen);
        }
      else if (otype == DHCPV6_OPT_INFO_REFRESH && olen >= 4)
        {
          uint32_t refresh = ntohl(*((FAR uint32_t *)odata));
          if (refresh < (uint32_t)pdhcp6c->t1)
            {
              pdhcp6c->t1 = refresh;
            }
        }
      else if (otype != DHCPV6_OPT_CLIENTID && otype != DHCPV6_OPT_SERVERID)
        {
          dhcp6c_add_state(handle, STATE_CUSTOM_OPTS, (odata - 4), olen + 4);
        }
    }

  if (opt != NULL)
    {
      size_t new_ia_pd_len;
      size_t new_ia_na_len;
      have_update |= dhcp6c_commit_state(handle, STATE_DNS, dns_len);
      have_update |= dhcp6c_commit_state(handle, STATE_SEARCH, search_len);
      have_update |= dhcp6c_commit_state(handle, STATE_SNTP_IP,
                                         sntp_ip_len);
      have_update |= dhcp6c_commit_state(handle, STATE_SNTP_FQDN,
                                         sntp_dns_len);
      have_update |= dhcp6c_commit_state(handle, STATE_SIP_IP, sip_ip_len);
      have_update |= dhcp6c_commit_state(handle, STATE_SIP_FQDN,
                                         sip_fqdn_len);
      dhcp6c_get_state(handle, STATE_IA_PD, &new_ia_pd_len);
      dhcp6c_get_state(handle, STATE_IA_NA, &new_ia_na_len);
      have_update |= (new_ia_pd_len != ia_pd_len) ||
                     (new_ia_na_len != ia_na_len);
    }

  /* Delete prefixes with 0 valid-time */

  ia_pd = dhcp6c_get_state(handle, STATE_IA_PD, &ia_pd_len);
  ia_end = ia_pd + ia_pd_len;
  dhcpv6_for_each_option(ite0, ia_pd, ia_end, otype, olen, odata)
    {
      FAR struct dhcpv6_ia_prefix_s *p = (FAR void *)(odata - 4);
      while (!p->valid)
        {
          ia_end = ia_pd + dhcp6c_remove_state(handle, STATE_IA_PD,
                   (FAR uint8_t *)p - ia_pd, olen + 4);
          have_update = true;
        }
    }

  /* Delete addresses with 0 valid-time */

  ia_na = dhcp6c_get_state(handle, STATE_IA_NA, &ia_na_len);
  ia_end = ia_na + ia_na_len;
  dhcpv6_for_each_option(ite0, ia_na, ia_end, otype, olen, odata)
    {
      FAR struct dhcpv6_ia_addr_s *p = (FAR void *)(odata - 4);
      while (!p->valid)
        {
          ia_end = ia_na + dhcp6c_remove_state(handle, STATE_IA_NA,
                   (FAR uint8_t *)p - ia_na, olen + 4);
          have_update = true;
        }
    }

  return have_update;
}

static int dhcp6c_handle_reconfigure(FAR void *handle,
                                     enum dhcpv6_msg_e orig,
                                     FAR const void *opt,
                                     FAR const void *end,
                                     uint32_t elapsed)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  uint16_t otype;
  uint16_t olen;
  FAR uint8_t *odata;
  FAR uint8_t *ite;
  uint8_t msg = DHCPV6_MSG_RENEW;

  /* TODO: should verify the reconfigure message */

  dhcpv6_for_each_option(ite, opt, end, otype, olen, odata)
    {
      if (otype == DHCPV6_OPT_RECONF_MESSAGE && olen == 1 &&
          (odata[0] == DHCPV6_MSG_RENEW ||
           odata[0] == DHCPV6_MSG_INFO_REQ))
        {
          msg = odata[0];
        }
    }

  pdhcp6c->t1 -= elapsed;
  pdhcp6c->t2 -= elapsed;
  pdhcp6c->t3 -= elapsed;

  if (pdhcp6c->t1 < 0)
    {
      pdhcp6c->t1 = 0;
    }

  if (pdhcp6c->t2 < 0)
    {
      pdhcp6c->t2 = 0;
    }

  if (pdhcp6c->t3 < 0)
    {
      pdhcp6c->t3 = 0;
    }

  dhcp6c_handle_reply(handle, DHCPV6_MSG_UNKNOWN, NULL, NULL, elapsed);

  return msg;
}

static int dhcp6c_handle_rebind_reply(FAR void *handle,
                                      enum dhcpv6_msg_e orig,
                                      FAR const void *opt,
                                      FAR const void *end,
                                      uint32_t elapsed)
{
  dhcp6c_handle_advert(handle, orig, opt, end, elapsed);
  if (dhcp6c_commit_advert(handle, elapsed) < 0)
    {
      return -1;
    }

  return dhcp6c_handle_reply(handle, orig, opt, end, elapsed);
}

static int dhcp6c_single_request(FAR void *args)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)args;
  FAR const char *process = NULL;
  enum dhcpv6_mode_e mode;
  enum dhcpv6_msg_e type;
  int ret = -1;

  dhcp6c_clear_state(pdhcp6c, STATE_SERVER_ID);
  dhcp6c_clear_state(pdhcp6c, STATE_SERVER_CAND);
  dhcp6c_clear_state(pdhcp6c, STATE_IA_PD);
  dhcp6c_clear_state(pdhcp6c, STATE_SNTP_IP);
  dhcp6c_clear_state(pdhcp6c, STATE_SNTP_FQDN);
  dhcp6c_clear_state(pdhcp6c, STATE_SIP_IP);
  dhcp6c_clear_state(pdhcp6c, STATE_SIP_FQDN);
  dhcp6c_clear_state(pdhcp6c, STATE_CUSTOM_OPTS);
  ret = dhcp6c_command(pdhcp6c, DHCPV6_MSG_SOLICIT);
  if (ret < 0)
    {
      return -1;
    }
  else if (ret == DHCPV6_STATELESS)
    {
      mode = DHCPV6_STATELESS;
      type = DHCPV6_MSG_INFO_REQ;
      process = "informed";
    }
  else
    {
      mode = DHCPV6_STATEFUL;
      type = DHCPV6_MSG_REQUEST;
      process = "bound";
    }

  ret = dhcp6c_command(pdhcp6c, type);
  if (ret >= 0)
    {
      ret = mode;
      dhcp6c_switch_process(pdhcp6c, process);
    }

  return ret;
}

static int dhcp6c_lease(FAR void *args, uint8_t type)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)args;
  enum dhcpv6_mode_e mode = (enum dhcpv6_mode_e)type;
  size_t ia_pd_len;
  size_t ia_na_len;
  size_t ia_pd_new;
  size_t ia_na_new;
  size_t server_id_len;
  int ret = -1;

  if (mode == DHCPV6_STATELESS)
    {
      /* Stateless mode */

      while (!pdhcp6c->cancel)
        {
          /* Wait for T1 to expire or until we get a reconfigure */

          ret = dhcp6c_poll_reconfigure(pdhcp6c);
          if (ret >= 0)
            {
              dhcp6c_switch_process(pdhcp6c, "informed");
            }

          if (pdhcp6c->cancel)
            {
              break;
            }

          /* Information-Request */

          ret = dhcp6c_command(pdhcp6c, DHCPV6_MSG_INFO_REQ);
          if (ret < 0)
            {
              nerr("DHCPV6_MSG_INFO_REQ error\n");
              break;
            }
          else
            {
              dhcp6c_switch_process(pdhcp6c, "informed");
            }
        }
    }
  else
    {
      /* Stateful mode */

      while (!pdhcp6c->cancel)
        {
          /* Renew Cycle
           * Wait for T1 to expire or until we get a reconfigure
           */

          ret = dhcp6c_poll_reconfigure(pdhcp6c);
          if (ret >= 0)
            {
              dhcp6c_switch_process(pdhcp6c, "updated");
            }

          if (pdhcp6c->cancel)
            {
              break;
            }

          dhcp6c_get_state(pdhcp6c, STATE_IA_PD, &ia_pd_len);
          dhcp6c_get_state(pdhcp6c, STATE_IA_NA, &ia_na_len);

          /* If we have any IAs, send renew, otherwise request */

          if (ia_pd_len == 0 && ia_na_len == 0)
            ret = dhcp6c_command(pdhcp6c, DHCPV6_MSG_REQUEST);
          else
            ret = dhcp6c_command(pdhcp6c, DHCPV6_MSG_RENEW);

          if (pdhcp6c->cancel)
            {
              break;
            }

          if (ret >= 0)
            {
              /* Publish updates */

              dhcp6c_switch_process(pdhcp6c, "updated");
            }
          else
            {
              /* Remove binding */

              dhcp6c_clear_state(pdhcp6c, STATE_SERVER_ID);

              /* If we have IAs, try rebind otherwise restart */

              ret = dhcp6c_command(pdhcp6c, DHCPV6_MSG_REBIND);
              dhcp6c_get_state(pdhcp6c, STATE_IA_PD, &ia_pd_new);
              dhcp6c_get_state(pdhcp6c, STATE_IA_NA, &ia_na_new);

              /* We lost all our IAs, restart */

              if (ret < 0 || (ia_pd_new == 0 && ia_pd_len) ||
                  (ia_na_new == 0 && ia_na_len))
                {
                  break;
                }
              else if (ret >= 0)
                {
                  dhcp6c_switch_process(pdhcp6c, "rebound");
                }
            }
        }
    }

  dhcp6c_get_state(pdhcp6c, STATE_IA_PD, &ia_pd_len);
  dhcp6c_get_state(pdhcp6c, STATE_IA_NA, &ia_na_len);
  dhcp6c_get_state(pdhcp6c, STATE_SERVER_ID, &server_id_len);

  /* Add all prefixes to lost prefixes */

  dhcp6c_clear_state(pdhcp6c, STATE_IA_PD);
  dhcp6c_switch_process(pdhcp6c, "unbound");

  /* Remove assigned addresses */

  if (ia_na_len > 0)
    {
      dhcp6c_remove_addrs(pdhcp6c);
    }

  if (server_id_len > 0 && (ia_pd_len > 0 || ia_na_len > 0))
    {
      dhcp6c_command(pdhcp6c, DHCPV6_MSG_RELEASE);
    }

  return ret;
}

static FAR void *dhcp6c_run(FAR void *args)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)args;
  int ret;

  while (!pdhcp6c->cancel)
    {
      ret = dhcp6c_single_request(pdhcp6c);
      if (ret > 0)
        {
          dhcp6c_lease(pdhcp6c, ret);
        }
    }

  return 0;
}

static FAR void *dhcp6c_precise_open(FAR const char *ifname,
                                     enum dhcp6c_ia_mode_e ia_mode,
                                     bool request_pd,
                                     uint16_t opt[], int cnt)
{
  FAR struct dhcp6c_state_s *pdhcp6c;
  struct ifreq ifr;
  size_t client_id_len;
  int val = 1;
  uint16_t oro[] =
  {
    htons(DHCPV6_OPT_DNS_SERVERS),
    htons(DHCPV6_OPT_DNS_DOMAIN),
    htons(DHCPV6_OPT_NTP_SERVER),
    htons(DHCPV6_OPT_SIP_SERVER_A),
    htons(DHCPV6_OPT_SIP_SERVER_D)
  };

  struct sockaddr_in6 client_addr =
  {
    AF_INET6,
    htons(DHCPV6_CLIENT_PORT),
    0,
    {0},
    0
  };

  pdhcp6c = malloc(sizeof(struct dhcp6c_state_s));
  if (pdhcp6c == NULL)
    {
      return NULL;
    }

  memset(pdhcp6c, 0, sizeof(*pdhcp6c));
  pdhcp6c->urandom_fd = open("/dev/urandom", O_CLOEXEC | O_RDONLY);
  if (pdhcp6c->urandom_fd < 0)
    {
      free(pdhcp6c);
      return NULL;
    }

  pdhcp6c->sockfd = socket(AF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
  if (pdhcp6c->sockfd < 0)
    {
      close(pdhcp6c->urandom_fd);
      free(pdhcp6c);
      return NULL;
    }

  /* Detect interface */

  strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
  if (ioctl(pdhcp6c->sockfd, SIOCGIFINDEX, &ifr))
    {
      close(pdhcp6c->urandom_fd);
      close(pdhcp6c->sockfd);
      free(pdhcp6c);
      return NULL;
    }

  pdhcp6c->ifindex = ifr.ifr_ifindex;

  /* Create client DUID */

  dhcp6c_get_state(pdhcp6c, STATE_CLIENT_ID, &client_id_len);
  if (client_id_len == 0)
    {
      uint8_t duid[14] =
      {
        0, DHCPV6_OPT_CLIENTID, 0, 10, 0,
        DHCPV6_DUID_LLADDR, 0, 1
      };

      uint8_t zero[ETHER_ADDR_LEN];
      struct ifreq ifs[100];
      FAR struct ifreq *ifp;
      FAR struct ifreq *ifend;
      struct ifconf ifc;

      ioctl(pdhcp6c->sockfd, SIOCGIFHWADDR, &ifr);
      memcpy(&duid[8], ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
      memset(zero, 0, sizeof(zero));
      ifc.ifc_req = ifs;
      ifc.ifc_len = sizeof(ifs);

      if (memcmp(&duid[8], zero, ETHER_ADDR_LEN) == 0 &&
          ioctl(pdhcp6c->sockfd, SIOCGIFCONF, &ifc) >= 0)
        {
          /* If our interface doesn't have an address... */

          ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
          for (ifp = ifc.ifc_req; ifp < ifend &&
               memcmp(&duid[8], zero, 6) == 0; ifp++)
            {
              memcpy(ifr.ifr_name, ifp->ifr_name,
                     sizeof(ifr.ifr_name));
              ioctl(pdhcp6c->sockfd, SIOCGIFHWADDR, &ifr);
              memcpy(&duid[8], ifr.ifr_hwaddr.sa_data,
                     ETHER_ADDR_LEN);
            }
        }

      dhcp6c_add_state(pdhcp6c, STATE_CLIENT_ID, duid, sizeof(duid));
    }

  /* Create ORO */

  dhcp6c_add_state(pdhcp6c, STATE_ORO, oro, sizeof(oro));

  /* Configure IPv6-options */

  setsockopt(pdhcp6c->sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &val, sizeof(val));
  setsockopt(pdhcp6c->sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  setsockopt(pdhcp6c->sockfd, SOL_SOCKET, UDP_BINDTODEVICE, ifname,
             strlen(ifname));
  if (bind(pdhcp6c->sockfd, (struct sockaddr *)&client_addr,
           sizeof(client_addr)) != 0)
    {
      close(pdhcp6c->urandom_fd);
      close(pdhcp6c->sockfd);
      free(pdhcp6c);
      return NULL;
    }

  pdhcp6c->thread = 0;
  pdhcp6c->cancel = false;
  pdhcp6c->t1 = 0;
  pdhcp6c->t2 = 0;
  pdhcp6c->t3 = 0;
  pdhcp6c->request_prefix = request_pd;
  switch (ia_mode)
    {
      case IA_MODE_NONE:
      case IA_MODE_TRY:
      case IA_MODE_FORCE:
        break;
      default:
        ia_mode = IA_MODE_TRY;
        break;
    }

  pdhcp6c->ia_mode = ia_mode;
  pdhcp6c->accept_reconfig = false;
  if (opt != NULL && cnt > 0)
    {
      uint16_t opttype;
      for (int i = 0; i < cnt; i++)
        {
          opttype = htons(opt[i]);
          dhcp6c_add_state(pdhcp6c, STATE_ORO, &opttype, 2);
        }
    }

  return pdhcp6c;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR void *dhcp6c_open(FAR const char *interface)
{
  return dhcp6c_precise_open(interface, IA_MODE_TRY, true, NULL, 0);
}

int dhcp6c_request(FAR void *handle, FAR struct dhcp6c_state *presult)
{
  int ret;

  if (handle == NULL)
    {
      return ERROR;
    }

  ret = dhcp6c_single_request(handle);
  if (ret >= 0)
    {
      dhcp6c_get_result(handle, presult);
      return OK;
    }
  else
    {
      return ERROR;
    }
}

int dhcp6c_request_async(FAR void *handle, dhcp6c_callback_t callback)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  int ret;

  if (handle == NULL)
    {
      return ERROR;
    }

  if (pdhcp6c->thread)
    {
      nerr("DHCP6C thread already running\n");
      return ERROR;
    }

  pdhcp6c->callback = callback;
  ret = pthread_create(&pdhcp6c->thread, NULL, dhcp6c_run, pdhcp6c);
  if (ret != 0)
    {
      nerr("Failed to start the DHCP6C thread\n");
      return ERROR;
    }

  return OK;
}

void dhcp6c_cancel(FAR void *handle)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;
  sighandler_t old;
  int ret;

  if (pdhcp6c != NULL)
    {
      pdhcp6c->cancel = true;
      if (pdhcp6c->thread)
        {
          old = signal(SIGQUIT, SIG_IGN);

          /* Signal the dhcp6c_run */

          ret = pthread_kill(pdhcp6c->thread, SIGQUIT);
          if (ret != 0)
            {
              nerr("pthread_kill DHCP6C thread\n");
            }

          /* Wait for the end of dhcp6c_run */

          ret = pthread_join(pdhcp6c->thread, NULL);
          if (ret != 0)
            {
              nerr("pthread_join DHCP6C thread\n");
            }

          pdhcp6c->thread = 0;
          signal(SIGQUIT, old);
        }
    }
}

void dhcp6c_close(FAR void *handle)
{
  FAR struct dhcp6c_state_s *pdhcp6c = (FAR struct dhcp6c_state_s *)handle;

  if (pdhcp6c != NULL)
    {
      dhcp6c_cancel(pdhcp6c);
      if (pdhcp6c->urandom_fd > 0)
        {
          close(pdhcp6c->urandom_fd);
        }

      if (pdhcp6c->sockfd > 0)
        {
          close(pdhcp6c->sockfd);
        }

      for (int i = 0; i < STATE_MAX; i++)
        {
          if (pdhcp6c->state_data[i] != NULL)
            {
              free(pdhcp6c->state_data[i]);
            }
        }

      free(pdhcp6c);
    }
}
