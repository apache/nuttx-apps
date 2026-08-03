/****************************************************************************
 * apps/wireless/lora_pkt_fwd/lora_pkt_fwd.c
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

/* Semtech UDP packet forwarder, protocol version 2.
 *
 * Two loops share the concentrator character device:
 *
 *   uplink   - polls /dev/loraN, serialises the packets into a "rxpk" JSON
 *              object and sends it as PUSH_DATA.  Also pushes the periodic
 *              "stat" object.
 *   downlink - sends PULL_DATA keepalives so that the server knows where to
 *              reach us, and turns each PULL_RESP into a write() on the
 *              concentrator followed by a TX_ACK.
 *
 * No floating point is used: frequencies are printed from their integer Hz
 * value and the signal levels come from the driver already scaled by ten.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <nuttx/compiler.h>
#include <nuttx/wireless/lpwan/lora_gw.h>

#include "netutils/base64.h"
#include "netutils/netlib.h"

#include "lora_pkt_fwd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LORA_FWD_PROTOCOL   2

#define LORA_FWD_JSONSIZE   4096     /* Largest JSON object we build */
#define LORA_FWD_RXBUFSIZE  1024
#define LORA_FWD_NPKT       4        /* Packets fetched per read() */
#define LORA_FWD_POLL_MS    100      /* Receive polling interval */
#define LORA_FWD_RECV_MS    500      /* Downlink socket timeout */
#define LORA_FWD_RESOLVE_TRIES 10    /* Attempts to resolve the server */
#define LORA_FWD_RESOLVE_DELAY 5     /* Seconds between those attempts */

/* A socket call can block for a while when the stack is resolving the
 * hardware address of the server, so give both loops room to notice a stop
 * request before giving up on them.
 */

#define LORA_FWD_STOP_TIMEOUT  20

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct lora_fwd_config_s g_config =
{
  .server        = CONFIG_LORA_PKT_FWD_SERVER,
  .port_up       = CONFIG_LORA_PKT_FWD_PORT_UP,
  .port_down     = CONFIG_LORA_PKT_FWD_PORT_DOWN,
  .keepalive_sec = CONFIG_LORA_PKT_FWD_KEEPALIVE,
  .stat_sec      = CONFIG_LORA_PKT_FWD_STATINTERVAL
};

static struct lora_fwd_stats_s g_stats;

static volatile bool g_running;
static volatile bool g_stopreq;
static time_t g_starttime;

static int g_devfd = -1;
static int g_sock_up = -1;
static int g_sock_down = -1;

static struct sockaddr_in g_addr_up;
static struct sockaddr_in g_addr_down;

static pthread_t g_downthread;

/* Last error reported by each socket, so that an unreachable server is
 * logged once instead of on every datagram.
 */

static int g_lasterr_up;
static int g_lasterr_down;

/* Datagram of the uplink path, with the JSON built in place right after the
 * twelve byte header.  It lives in static storage because two buffers of
 * this size do not fit on the stack of the daemon.  Only the uplink loop
 * touches it.
 */

static uint8_t g_pushbuf[12 + LORA_FWD_JSONSIZE];

#define LORA_FWD_JSON ((FAR char *)&g_pushbuf[12])

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lora_fwd_token
 *
 * Description:
 *   Two random bytes identifying a request, echoed back in the acknowledge.
 *
 ****************************************************************************/

static void lora_fwd_token(FAR uint8_t *buffer)
{
  uint32_t value = arc4random();

  buffer[0] = value & 0xff;
  buffer[1] = (value >> 8) & 0xff;
}

/****************************************************************************
 * Name: lora_fwd_resolve
 *
 * Description:
 *   Resolve the configured server, which may be a host name or a literal
 *   address, and fill in the uplink and downlink socket addresses.
 *
 ****************************************************************************/

static int lora_fwd_resolve(void)
{
  FAR struct addrinfo *res = NULL;
  struct addrinfo hints;
  int ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  ret = getaddrinfo(g_config.server, NULL, &hints, &res);
  if (ret != 0 || res == NULL)
    {
      fprintf(stderr, "lora: cannot resolve %s\n", g_config.server);
      return -EHOSTUNREACH;
    }

  memcpy(&g_addr_up, res->ai_addr, sizeof(struct sockaddr_in));
  freeaddrinfo(res);

  g_addr_up.sin_family = AF_INET;
  g_addr_up.sin_port   = htons(g_config.port_up);

  g_addr_down          = g_addr_up;
  g_addr_down.sin_port = htons(g_config.port_down);

  return OK;
}

/****************************************************************************
 * Name: lora_fwd_opensockets
 ****************************************************************************/

static int lora_fwd_opensockets(void)
{
  struct timeval tv;
  int ret;

  ret = lora_fwd_resolve();
  if (ret < 0)
    {
      return ret;
    }

  g_sock_up = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (g_sock_up < 0)
    {
      return -errno;
    }

  g_sock_down = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (g_sock_down < 0)
    {
      ret = -errno;
      close(g_sock_up);
      g_sock_up = -1;
      return ret;
    }

  /* Both sockets time out so that neither loop can wedge */

  tv.tv_sec  = 0;
  tv.tv_usec = LORA_FWD_RECV_MS * 1000;
  setsockopt(g_sock_down, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(g_sock_up, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  printf("lora: forwarding to %s (up %u, down %u)\n",
         g_config.server, g_config.port_up, g_config.port_down);
  return OK;
}

/****************************************************************************
 * Name: lora_fwd_closesockets
 ****************************************************************************/

static void lora_fwd_closesockets(void)
{
  if (g_sock_up >= 0)
    {
      close(g_sock_up);
      g_sock_up = -1;
    }

  if (g_sock_down >= 0)
    {
      close(g_sock_down);
      g_sock_down = -1;
    }
}

/****************************************************************************
 * Name: lora_fwd_push
 *
 * Description:
 *   Send one PUSH_DATA carrying a JSON object.
 *
 ****************************************************************************/

static int lora_fwd_push(size_t jsonlen)
{
  int ret;

  if (jsonlen == 0 || jsonlen > LORA_FWD_JSONSIZE)
    {
      return -E2BIG;
    }

  g_pushbuf[0] = LORA_FWD_PROTOCOL;
  lora_fwd_token(&g_pushbuf[1]);
  g_pushbuf[3] = LORA_PKT_PUSH_DATA;
  memcpy(&g_pushbuf[4], g_config.eui, LORA_FWD_EUILEN);

  ret = sendto(g_sock_up, g_pushbuf, 12 + jsonlen, 0,
               (FAR struct sockaddr *)&g_addr_up, sizeof(g_addr_up));
  if (ret < 0)
    {
      ret = -errno;
      if (ret != g_lasterr_up)
        {
          g_lasterr_up = ret;
          fprintf(stderr, "lora: cannot reach %s: %d\n", g_config.server,
                  -ret);
        }

      return ret;
    }

  g_lasterr_up = 0;

  g_stats.push_sent++;
  return OK;
}

/****************************************************************************
 * Name: lora_fwd_pull
 *
 * Description:
 *   Send one PULL_DATA so that the server learns our address and can push
 *   downlinks back through the same NAT binding.
 *
 ****************************************************************************/

static int lora_fwd_pull(void)
{
  uint8_t buffer[12];
  int ret;

  buffer[0] = LORA_FWD_PROTOCOL;
  lora_fwd_token(&buffer[1]);
  buffer[3] = LORA_PKT_PULL_DATA;
  memcpy(&buffer[4], g_config.eui, LORA_FWD_EUILEN);

  ret = sendto(g_sock_down, buffer, sizeof(buffer), 0,
               (FAR struct sockaddr *)&g_addr_down, sizeof(g_addr_down));
  if (ret < 0)
    {
      ret = -errno;
      if (ret != g_lasterr_down)
        {
          g_lasterr_down = ret;
          fprintf(stderr, "lora: keepalive to %s failed: %d\n",
                  g_config.server, -ret);
        }

      return ret;
    }

  g_lasterr_down = 0;

  g_stats.pull_sent++;
  return OK;
}

/****************************************************************************
 * Name: lora_fwd_bwstr
 ****************************************************************************/

static FAR const char *lora_fwd_bwstr(uint8_t bandwidth)
{
  switch (bandwidth)
    {
      case LORA_GW_BW_250K:
        return "BW250";

      case LORA_GW_BW_500K:
        return "BW500";

      default:
        return "BW125";
    }
}

/****************************************************************************
 * Name: lora_fwd_crstr
 ****************************************************************************/

static FAR const char *lora_fwd_crstr(uint8_t coderate)
{
  switch (coderate)
    {
      case WLIOC_LORA_CR_4_6:
        return "4/6";

      case WLIOC_LORA_CR_4_7:
        return "4/7";

      case WLIOC_LORA_CR_4_8:
        return "4/8";

      default:
        return "4/5";
    }
}

/****************************************************************************
 * Name: lora_fwd_buildrxpk
 *
 * Description:
 *   Serialise received packets into the "rxpk" object of the Semtech
 *   protocol.  Frequencies are printed straight from their integer Hz value
 *   so that no floating point support is needed.
 *
 ****************************************************************************/

static int lora_fwd_buildrxpk(FAR const struct lora_gw_rxpkt_s *pkts,
                              int npkt, FAR char *json, size_t jsonlen)
{
  char payload[512];
  size_t b64len;
  size_t pos = 0;
  int i;
  int ret;

  ret = snprintf(json, jsonlen, "{\"rxpk\":[");
  if (ret < 0)
    {
      return ret;
    }

  pos = ret;

  for (i = 0; i < npkt; i++)
    {
      FAR const struct lora_gw_rxpkt_s *pkt = &pkts[i];

      b64len = sizeof(payload);
      if (base64_encode(pkt->payload, pkt->size, payload, &b64len) == NULL)
        {
          continue;
        }

      payload[b64len] = '\0';

      ret = snprintf(json + pos, jsonlen - pos,
                     "%s{\"tmst\":%" PRIu32 ","
                     "\"chan\":%u,"
                     "\"rfch\":%u,"
                     "\"freq\":%" PRIu32 ".%06" PRIu32 ","
                     "\"stat\":%d,"
                     "\"modu\":\"LORA\","
                     "\"datr\":\"SF%u%s\","
                     "\"codr\":\"%s\","
                     "\"lsnr\":%d.%d,"
                     "\"rssi\":%d,"
                     "\"size\":%u,"
                     "\"data\":\"%s\"}",
                     i > 0 ? "," : "",
                     pkt->count_us,
                     pkt->if_chain,
                     pkt->rf_chain,
                     pkt->freq_hz / 1000000, pkt->freq_hz % 1000000,
                     pkt->status == LORA_GW_STAT_CRC_OK ? 1 :
                     (pkt->status == LORA_GW_STAT_NO_CRC ? 0 : -1),
                     pkt->datarate, lora_fwd_bwstr(pkt->bandwidth),
                     lora_fwd_crstr(pkt->coderate),
                     pkt->snr_db10 / 10, abs(pkt->snr_db10 % 10),
                     pkt->rssi_dbm10 / 10,
                     pkt->size, payload);

      if (ret < 0 || (size_t)ret >= jsonlen - pos)
        {
          break;
        }

      pos += ret;
    }

  ret = snprintf(json + pos, jsonlen - pos, "]}");
  if (ret < 0)
    {
      return ret;
    }

  return pos + ret;
}

/****************************************************************************
 * Name: lora_fwd_buildstat
 ****************************************************************************/

static int lora_fwd_buildstat(FAR char *json, size_t jsonlen)
{
  struct lora_gw_status_s status;
  char timestr[32];
  struct tm tm;
  time_t now;
  unsigned int ackr_int = 0;
  unsigned int ackr_frac = 0;

  memset(&status, 0, sizeof(status));
  if (g_devfd >= 0)
    {
      ioctl(g_devfd, WLIOC_GW_GETSTATUS, (unsigned long)&status);
    }

  now = time(NULL);
  gmtime_r(&now, &tm);
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tm);

  /* Acknowledge ratio in percent, one decimal, computed with integers */

  if (g_stats.push_sent > 0)
    {
      uint32_t permille = (uint32_t)((uint64_t)g_stats.push_ack * 1000 /
                                     g_stats.push_sent);
      ackr_int  = permille / 10;
      ackr_frac = permille % 10;
    }

  return snprintf(json, jsonlen,
                  "{\"stat\":{"
                  "\"time\":\"%s GMT\","
                  "\"rxnb\":%" PRIu32 ","
                  "\"rxok\":%" PRIu32 ","
                  "\"rxfw\":%" PRIu32 ","
                  "\"ackr\":%u.%u,"
                  "\"dwnb\":%" PRIu32 ","
                  "\"txnb\":%" PRIu32
                  "}}",
                  timestr,
                  status.rx_ok + status.rx_nocrc,
                  status.rx_ok,
                  g_stats.rx_fwd,
                  ackr_int, ackr_frac,
                  g_stats.tx_req,
                  g_stats.tx_ok);
}

/****************************************************************************
 * Name: lora_fwd_jsonfind
 *
 * Description:
 *   Locate the value of a key in a flat JSON object.
 *
 ****************************************************************************/

static FAR const char *lora_fwd_jsonfind(FAR const char *json,
                                         FAR const char *key)
{
  char pattern[24];
  FAR const char *p;

  snprintf(pattern, sizeof(pattern), "\"%s\"", key);

  p = strstr(json, pattern);
  if (p == NULL)
    {
      return NULL;
    }

  p = strchr(p + strlen(pattern), ':');
  if (p == NULL)
    {
      return NULL;
    }

  p++;
  while (*p == ' ')
    {
      p++;
    }

  return p;
}

/****************************************************************************
 * Name: lora_fwd_jsonbool
 ****************************************************************************/

static bool lora_fwd_jsonbool(FAR const char *json, FAR const char *key,
                              bool dflt)
{
  FAR const char *value = lora_fwd_jsonfind(json, key);

  return value != NULL ? (*value == 't') : dflt;
}

/****************************************************************************
 * Name: lora_fwd_jsonuint
 ****************************************************************************/

static uint32_t lora_fwd_jsonuint(FAR const char *json, FAR const char *key,
                                  uint32_t dflt)
{
  FAR const char *value = lora_fwd_jsonfind(json, key);

  return value != NULL ? strtoul(value, NULL, 10) : dflt;
}

/****************************************************************************
 * Name: lora_fwd_jsonint
 ****************************************************************************/

static int32_t lora_fwd_jsonint(FAR const char *json, FAR const char *key,
                                int32_t dflt)
{
  FAR const char *value = lora_fwd_jsonfind(json, key);

  return value != NULL ? strtol(value, NULL, 10) : dflt;
}

/****************************************************************************
 * Name: lora_fwd_jsonstr
 ****************************************************************************/

static int lora_fwd_jsonstr(FAR const char *json, FAR const char *key,
                            FAR char *out, size_t outlen)
{
  FAR const char *value = lora_fwd_jsonfind(json, key);
  size_t i = 0;

  if (value == NULL || *value != '"')
    {
      return -ENOENT;
    }

  value++;
  while (*value != '\0' && *value != '"' && i < outlen - 1)
    {
      out[i++] = *value++;
    }

  out[i] = '\0';
  return i;
}

/****************************************************************************
 * Name: lora_fwd_jsonfreq
 *
 * Description:
 *   Read a frequency given in MHz with a fractional part and return it in
 *   Hz, without going through a double.  "917.2" and "917.200000" both give
 *   917200000.
 *
 ****************************************************************************/

static uint32_t lora_fwd_jsonfreq(FAR const char *json,
                                  FAR const char *key)
{
  FAR const char *value = lora_fwd_jsonfind(json, key);
  uint32_t mhz = 0;
  uint32_t frac = 0;
  int digits = 0;

  if (value == NULL)
    {
      return 0;
    }

  while (*value >= '0' && *value <= '9')
    {
      mhz = mhz * 10 + (*value++ - '0');
    }

  if (*value == '.')
    {
      value++;
      while (*value >= '0' && *value <= '9' && digits < 6)
        {
          frac = frac * 10 + (*value++ - '0');
          digits++;
        }
    }

  while (digits++ < 6)
    {
      frac *= 10;
    }

  return mhz * 1000000 + frac;
}

/****************************************************************************
 * Name: lora_fwd_txack
 ****************************************************************************/

static void lora_fwd_txack(FAR const uint8_t *token,
                           FAR const struct sockaddr_in *peer,
                           FAR const char *error)
{
  uint8_t buffer[128];
  int len;

  buffer[0] = LORA_FWD_PROTOCOL;
  buffer[1] = token[0];
  buffer[2] = token[1];
  buffer[3] = LORA_PKT_TX_ACK;

  len = snprintf((FAR char *)&buffer[4], sizeof(buffer) - 4,
                 "{\"txpk_ack\":{\"error\":\"%s\"}}", error);

  sendto(g_sock_down, buffer, 4 + len, 0,
         (FAR const struct sockaddr *)peer, sizeof(*peer));
}

/****************************************************************************
 * Name: lora_fwd_pullresp
 *
 * Description:
 *   Turn a PULL_RESP into a transmission.
 *
 ****************************************************************************/

static void lora_fwd_pullresp(FAR const uint8_t *raw, size_t rawlen,
                              FAR const uint8_t *token,
                              FAR const struct sockaddr_in *peer)
{
  struct lora_gw_txpkt_s txpkt;
  char json[LORA_FWD_RXBUFSIZE];
  char datr[16];
  char codr[8];
  char data[512];
  size_t decoded;
  size_t len;
  int sf;
  int bw;
  int ret;

  g_stats.tx_req++;

  len = rawlen < sizeof(json) - 1 ? rawlen : sizeof(json) - 1;
  memcpy(json, raw, len);
  json[len] = '\0';

  if (strstr(json, "txpk") == NULL)
    {
      fprintf(stderr, "lora: PULL_RESP without txpk\n");
      lora_fwd_txack(token, peer, "INVALID_JSON");
      return;
    }

  memset(&txpkt, 0, sizeof(txpkt));

  if (lora_fwd_jsonbool(json, "imme", false))
    {
      txpkt.tx_mode = LORA_GW_TX_IMMEDIATE;
    }
  else
    {
      txpkt.tx_mode  = LORA_GW_TX_TIMESTAMPED;
      txpkt.count_us = lora_fwd_jsonuint(json, "tmst", 0);
    }

  txpkt.freq_hz    = lora_fwd_jsonfreq(json, "freq");
  txpkt.rf_chain   = lora_fwd_jsonuint(json, "rfch", 0);
  txpkt.rf_power   = lora_fwd_jsonint(json, "powe", 14);
  txpkt.modulation = LORA_GW_MOD_LORA;
  txpkt.invert_pol = lora_fwd_jsonbool(json, "ipol", true);
  txpkt.preamble   = lora_fwd_jsonuint(json, "prea", 8);
  txpkt.no_crc     = lora_fwd_jsonbool(json, "ncrc", false);
  txpkt.size       = lora_fwd_jsonuint(json, "size", 0);

  /* Data rate, given as "SF7BW125" */

  sf = 7;
  bw = 125;
  memset(datr, 0, sizeof(datr));
  if (lora_fwd_jsonstr(json, "datr", datr, sizeof(datr)) > 0)
    {
      sscanf(datr, "SF%dBW%d", &sf, &bw);
    }

  txpkt.datarate = sf;
  if (bw == 500)
    {
      txpkt.bandwidth = LORA_GW_BW_500K;
    }
  else if (bw == 250)
    {
      txpkt.bandwidth = LORA_GW_BW_250K;
    }
  else
    {
      txpkt.bandwidth = LORA_GW_BW_125K;
    }

  /* Coding rate */

  memset(codr, 0, sizeof(codr));
  lora_fwd_jsonstr(json, "codr", codr, sizeof(codr));
  if (strcmp(codr, "4/6") == 0 || strcmp(codr, "2/3") == 0)
    {
      txpkt.coderate = WLIOC_LORA_CR_4_6;
    }
  else if (strcmp(codr, "4/7") == 0)
    {
      txpkt.coderate = WLIOC_LORA_CR_4_7;
    }
  else if (strcmp(codr, "4/8") == 0 || strcmp(codr, "1/2") == 0)
    {
      txpkt.coderate = WLIOC_LORA_CR_4_8;
    }
  else
    {
      txpkt.coderate = WLIOC_LORA_CR_4_5;
    }

  /* Payload */

  memset(data, 0, sizeof(data));
  ret = lora_fwd_jsonstr(json, "data", data, sizeof(data));
  if (ret > 0)
    {
      decoded = sizeof(txpkt.payload);
      if (base64_decode(data, ret, txpkt.payload, &decoded) == NULL)
        {
          fprintf(stderr, "lora: cannot decode the downlink payload\n");
          lora_fwd_txack(token, peer, "INVALID_JSON");
          return;
        }

      if (txpkt.size == 0 || txpkt.size > decoded)
        {
          txpkt.size = decoded;
        }
    }

  if (txpkt.size == 0 || txpkt.freq_hz == 0)
    {
      lora_fwd_txack(token, peer, "INVALID_JSON");
      return;
    }

  ret = write(g_devfd, &txpkt, sizeof(txpkt));
  if (ret == sizeof(txpkt))
    {
      g_stats.tx_ok++;
      lora_fwd_txack(token, peer, "NONE");
      printf("lora: downlink %" PRIu32 ".%06" PRIu32 " MHz SF%u %u bytes\n",
             txpkt.freq_hz / 1000000, txpkt.freq_hz % 1000000,
             txpkt.datarate, txpkt.size);
    }
  else
    {
      fprintf(stderr, "lora: transmit failed: %d\n", errno);
      lora_fwd_txack(token, peer, "TX_FAIL");
    }
}

/****************************************************************************
 * Name: lora_fwd_downloop
 *
 * Description:
 *   Keepalive and downlink handling.
 *
 ****************************************************************************/

static FAR void *lora_fwd_downloop(FAR void *arg)
{
  uint8_t buffer[LORA_FWD_RXBUFSIZE];
  struct sockaddr_in peer;
  socklen_t peerlen;
  time_t lastpull = 0;
  ssize_t nread;

  UNUSED(arg);

  /* Announce ourselves before any uplink is forwarded */

  lora_fwd_pull();
  lastpull = time(NULL);

  while (!g_stopreq)
    {
      peerlen = sizeof(peer);
      nread = recvfrom(g_sock_down, buffer, sizeof(buffer), 0,
                       (FAR struct sockaddr *)&peer, &peerlen);

      if (nread >= 4)
        {
          switch (buffer[3])
            {
              case LORA_PKT_PULL_ACK:
                g_stats.pull_ack++;
                break;

              case LORA_PKT_PULL_RESP:
                lora_fwd_pullresp(&buffer[4], nread - 4, &buffer[1], &peer);
                break;

              default:
                fprintf(stderr, "lora: unexpected downlink type 0x%02x\n",
                        buffer[3]);
                break;
            }
        }

      if (time(NULL) - lastpull >= g_config.keepalive_sec)
        {
          lora_fwd_pull();
          lastpull = time(NULL);
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: lora_fwd_uploop
 *
 * Description:
 *   Forward received packets and push the periodic status.
 *
 ****************************************************************************/

static void lora_fwd_uploop(void)
{
  struct lora_gw_rxpkt_s pkts[LORA_FWD_NPKT];
  uint8_t ack[4];
  time_t laststat;
  ssize_t nread;
  int npkt;
  int len;

  laststat = time(NULL);

  while (!g_stopreq)
    {
      nread = read(g_devfd, pkts, sizeof(pkts));
      if (nread > 0)
        {
          npkt = nread / sizeof(struct lora_gw_rxpkt_s);

          len = lora_fwd_buildrxpk(pkts, npkt, LORA_FWD_JSON,
                                   LORA_FWD_JSONSIZE);
          if (len > 0 && lora_fwd_push(len) == OK)
            {
              g_stats.rx_fwd += npkt;
              printf("lora: forwarded %d packet(s)\n", npkt);
            }
          else
            {
              g_stats.rx_drop += npkt;
            }
        }
      else if (nread < 0 && errno != EAGAIN)
        {
          fprintf(stderr, "lora: read error %d\n", errno);
          break;
        }

      if (time(NULL) - laststat >= g_config.stat_sec)
        {
          len = lora_fwd_buildstat(LORA_FWD_JSON, LORA_FWD_JSONSIZE);
          if (len > 0)
            {
              lora_fwd_push(len);
            }

          laststat = time(NULL);
        }

      /* Collect the acknowledges of what we sent */

      while (recv(g_sock_up, ack, sizeof(ack), MSG_DONTWAIT) > 0)
        {
          g_stats.push_ack++;
        }

      usleep(LORA_FWD_POLL_MS * 1000);
    }
}

/****************************************************************************
 * Name: lora_fwd_daemon
 ****************************************************************************/

static int lora_fwd_daemon(int argc, FAR char *argv[])
{
  pthread_attr_t attr;
  int retries;
  int ret;

  UNUSED(argc);
  UNUSED(argv);

  g_devfd = open(CONFIG_LORA_PKT_FWD_DEVPATH,
                 O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (g_devfd < 0)
    {
      fprintf(stderr, "lora: cannot open %s: %d\n",
              CONFIG_LORA_PKT_FWD_DEVPATH, errno);
      g_running = false;
      return EXIT_FAILURE;
    }

  ret = ioctl(g_devfd, WLIOC_GW_START, 0);
  if (ret < 0)
    {
      fprintf(stderr, "lora: cannot start the concentrator: %d\n", errno);
      goto errout_dev;
    }

  /* The name of the server may not resolve on the first try, for instance
   * when the link has just come up.  Keep trying rather than tearing the
   * concentrator down.
   */

  for (retries = 0; retries < LORA_FWD_RESOLVE_TRIES; retries++)
    {
      ret = lora_fwd_opensockets();
      if (ret >= 0 || g_stopreq)
        {
          break;
        }

      sleep(LORA_FWD_RESOLVE_DELAY);
    }

  if (ret < 0)
    {
      goto errout_stop;
    }

  memset(&g_stats, 0, sizeof(g_stats));
  g_starttime = time(NULL);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, CONFIG_LORA_PKT_FWD_STACKSIZE);

  ret = pthread_create(&g_downthread, &attr, lora_fwd_downloop, NULL);
  pthread_attr_destroy(&attr);

  if (ret != 0)
    {
      fprintf(stderr, "lora: cannot start the downlink thread: %d\n", ret);
      ret = -ret;
      goto errout_sockets;
    }

  pthread_setname_np(g_downthread, "lora_down");

  printf("lora: packet forwarder running\n");

  lora_fwd_uploop();

  pthread_join(g_downthread, NULL);

errout_sockets:
  lora_fwd_closesockets();

errout_stop:
  ioctl(g_devfd, WLIOC_GW_STOP, 0);

errout_dev:
  close(g_devfd);
  g_devfd = -1;
  g_running = false;
  g_stopreq = false;

  printf("lora: packet forwarder stopped\n");
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lora_fwd_init
 *
 * Description:
 *   Resolve the compiled in defaults once.  Safe to call repeatedly.
 *
 ****************************************************************************/

void lora_fwd_init(void)
{
  static bool initialised;
  uint8_t mac[IFHWADDRLEN];

  if (initialised)
    {
      return;
    }

  initialised = true;

  if (lora_fwd_parse_eui(CONFIG_LORA_PKT_FWD_EUI, g_config.eui) < 0)
    {
      fprintf(stderr, "lora: bad gateway identifier '%s'\n",
              CONFIG_LORA_PKT_FWD_EUI);
    }

#ifdef CONFIG_LORA_PKT_FWD_EUI_FROM_MAC
  /* Build an EUI-64 out of the Ethernet address, the usual way: the three
   * bytes of the vendor identifier, then FFFE, then the rest.
   */

  if (netlib_getmacaddr(LORA_FWD_IFNAME, mac) == OK)
    {
      g_config.eui[0] = mac[0];
      g_config.eui[1] = mac[1];
      g_config.eui[2] = mac[2];
      g_config.eui[3] = 0xff;
      g_config.eui[4] = 0xfe;
      g_config.eui[5] = mac[3];
      g_config.eui[6] = mac[4];
      g_config.eui[7] = mac[5];
    }
#else
  UNUSED(mac);
#endif
}

/****************************************************************************
 * Name: lora_fwd_start
 ****************************************************************************/

int lora_fwd_start(void)
{
  int pid;

  lora_fwd_init();

  if (g_running)
    {
      return -EALREADY;
    }

  g_stopreq = false;
  g_running = true;

  pid = task_create("lora_fwd", CONFIG_LORA_PKT_FWD_PRIORITY,
                    CONFIG_LORA_PKT_FWD_STACKSIZE, lora_fwd_daemon, NULL);
  if (pid < 0)
    {
      g_running = false;
      return -errno;
    }

  return OK;
}

/****************************************************************************
 * Name: lora_fwd_stop
 ****************************************************************************/

int lora_fwd_stop(void)
{
  int i;

  if (!g_running)
    {
      return OK;
    }

  g_stopreq = true;

  /* Give both loops time to notice: a socket call may be blocked resolving
   * the address of an unreachable server.
   */

  for (i = 0; i < LORA_FWD_STOP_TIMEOUT * 10 && g_running; i++)
    {
      usleep(100000);
    }

  return g_running ? -ETIMEDOUT : OK;
}

/****************************************************************************
 * Name: lora_fwd_isrunning
 ****************************************************************************/

bool lora_fwd_isrunning(void)
{
  return g_running;
}

/****************************************************************************
 * Name: lora_fwd_config
 ****************************************************************************/

FAR struct lora_fwd_config_s *lora_fwd_config(void)
{
  return &g_config;
}

/****************************************************************************
 * Name: lora_fwd_getstats
 ****************************************************************************/

void lora_fwd_getstats(FAR struct lora_fwd_stats_s *stats)
{
  memcpy(stats, &g_stats, sizeof(*stats));

  stats->uptime_sec = g_running ? (uint32_t)(time(NULL) - g_starttime) : 0;
}

/****************************************************************************
 * Name: lora_fwd_setserver
 *
 * Description:
 *   Change the network server.  Takes effect the next time the forwarder is
 *   started.
 *
 ****************************************************************************/

int lora_fwd_setserver(FAR const char *host, int port_up, int port_down)
{
  if (host != NULL)
    {
      strlcpy(g_config.server, host, sizeof(g_config.server));
    }

  if (port_up > 0 && port_up <= UINT16_MAX)
    {
      g_config.port_up = port_up;
    }

  if (port_down > 0 && port_down <= UINT16_MAX)
    {
      g_config.port_down = port_down;
    }

  return OK;
}

/****************************************************************************
 * Name: lora_fwd_parse_eui
 *
 * Description:
 *   Convert a sixteen digit hexadecimal string into the eight byte gateway
 *   identifier.
 *
 ****************************************************************************/

int lora_fwd_parse_eui(FAR const char *str, FAR uint8_t *eui)
{
  char byte[3];
  int i;

  if (str == NULL || strlen(str) < 2 * LORA_FWD_EUILEN)
    {
      return -EINVAL;
    }

  byte[2] = '\0';

  for (i = 0; i < LORA_FWD_EUILEN; i++)
    {
      byte[0] = str[2 * i];
      byte[1] = str[2 * i + 1];

      if (!isxdigit(byte[0]) || !isxdigit(byte[1]))
        {
          return -EINVAL;
        }

      eui[i] = strtoul(byte, NULL, 16);
    }

  return OK;
}
