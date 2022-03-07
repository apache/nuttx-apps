/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak.h
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

#ifndef __APPS_WIRELESS_IEEE802154_I8SAK_I8SAK_H
#define __APPS_WIRELESS_IEEE802154_I8SAK_I8SAK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "i8sak_events.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#if !defined(CONFIG_IEEE802154_I8SAK_DEFAULT_EP_EADDR)
#define CONFIG_IEEE802154_I8SAK_DEFAULT_EP_EADDR 0xAABBCCAABBCC000A
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_DEFAULT_EP_SADDR)
#define CONFIG_IEEE802154_I8SAK_DEFAULT_EP_SADDR 0x000A
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_DEFAULT_EP_PANID)
#define CONFIG_IEEE802154_I8SAK_DEFAULT_EP_PANID 0xABCD
#endif

#ifdef CONFIG_NET_6LOWPAN
#if !defined(CONFIG_IEEE802154_I8SAK_DEFAULT_PORT)
#define CONFIG_IEEE802154_I8SAK_DEFAULT_PORT 61616
#endif
#endif

#define I8SAK_MAX_IFNAME            12

#define I8SAK_DAEMONNAME_FMT        "i8sak_%s"
#define I8SAK_DAEMONNAME_PREFIX_LEN 6
#define I8SAK_MAX_DAEMONNAME        I8SAK_DAEMONNAME_PREFIX_LEN + I8SAK_MAX_IFNAME

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum i8sak_mode_e
{
  I8SAK_MODE_CHAR = 0,
  I8SAK_MODE_NETIF,
};

enum i8sak_cmd_e
{
  I8_CMD_NONE = 0x00,
  I8_CMD_SNIFFER,
  I8_CMD_STARTPAN,
  I8_CMD_ACCEPTASSOC,
  I8_CMD_ASSOC,
  I8_CMD_TX,
  I8_CMD_BLASTER,
  I8_CMD_POLL,
};

struct i8sak_s
{
  /* Support singly linked list */

  FAR struct i8sak_s *flink;

  enum i8sak_mode_e mode;

  bool initialized      : 1;
  bool daemon_shutdown  : 1;
  bool verbose          : 1;
  bool acceptall        : 1;
  bool assoc            : 1;

  pid_t daemon_pid;
  FAR char ifname[I8SAK_MAX_IFNAME];
  int result;
  int fd;            /* File/Socket descriptor. Only to be used by operations
                      * occurring within the Daemon TCB
                      */

  sem_t exclsem;     /* For synchronizing access to the signaling semaphore */
  sem_t sigsem;      /* For signaling various tasks */
  sem_t updatesem;   /* For signaling the daemon that it's settings have changed */
  sem_t daemonlock;  /* For synchronizing access to managing the daemon */

  int daemonusers;

  /* Fields related to event listener */

  bool eventlistener_run;
  pthread_t eventlistener_threadid;

  sem_t eventsem;  /* For synchronizing access to the event receiver list */
  sq_queue_t eventreceivers;
  sq_queue_t eventreceivers_free;
  struct i8sak_eventreceiver_s
  eventreceiver_pool[CONFIG_I8SAK_NEVENTRECEIVERS];

  /* MAC related fields */

  uint8_t msdu_handle;

  /* Physical Layer Settings */

  uint8_t chan;
  uint8_t chpage;

  enum ieee802154_addrmode_e addrmode;

  /* Support a single endpoint. This is used for all interactions. It must
   * be updated either inline with the command or manually before performing
   * an operation.
   */

  struct ieee802154_addr_s ep_addr;
#ifdef CONFIG_NET_6LOWPAN
  struct sockaddr_in6 ep_in6addr;
#endif

  /* For the Coordinator, we keep a Short Address that will be handed out
   * next. After assigning the address to a device requesting association,
   * this field is simply incremented.  This means short addresses will be
   * assigned to associating devices in order.
   */

  uint8_t next_saddr[IEEE802154_SADDRSIZE];

  /* General payload field used for blaster/tx commands */

  uint8_t payload[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
  uint16_t payload_len;

  /* Fields to hold scan results in */

  struct ieee802154_pandesc_s pandescs[CONFIG_MAC802154_NPANDESC];
  uint8_t npandesc;

  /* Blaster command parameters */

  bool startblaster;
  bool blasterenabled;
  pthread_t blaster_threadid;
  int blasterperiod;

  /* Sniffer command parameters */

  bool startsniffer;
  bool snifferenabled;
  pthread_t sniffer_threadid;
#ifdef CONFIG_NET_6LOWPAN
  uint16_t snifferport;
#endif
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int i8sak_requestdaemon(FAR struct i8sak_s *i8sak);
int i8sak_releasedaemon(FAR struct i8sak_s *i8sak);

uint8_t   i8sak_char2nibble (char ch);
long      i8sak_str2long    (FAR const char *str);
uint8_t   i8sak_str2luint8  (FAR const char *str);
uint16_t  i8sak_str2luint16 (FAR const char *str);
bool      i8sak_str2bool    (FAR const char *str);

int       i8sak_str2payload (FAR const char *str, FAR uint8_t *buf);
void      i8sak_str2eaddr   (FAR const char *str, FAR uint8_t *eaddr);
void      i8sak_str2saddr   (FAR const char *str, FAR uint8_t *saddr);
void      i8sak_str2panid   (FAR const char *str, FAR uint8_t *panid);

/* Command Functions. Alphabetical Order */

void i8sak_acceptassoc_cmd (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_assoc_cmd       (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_blaster_cmd     (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_get_cmd         (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_poll_cmd        (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_regdump_cmd     (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_reset_cmd       (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_scan_cmd        (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_set_cmd         (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_sniffer_cmd     (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_startpan_cmd    (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);
void i8sak_tx_cmd          (FAR struct i8sak_s *i8sak,
                            int argc, FAR char *argv[]);

/* Command background threads */

pthread_addr_t i8sak_blaster_thread(pthread_addr_t arg);
pthread_addr_t i8sak_sniffer_thread(pthread_addr_t arg);

#define PRINTF_FORMAT_EADDR(eaddr) \
  "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", \
  eaddr[0], eaddr[1], eaddr[2], eaddr[3], \
  eaddr[4], eaddr[5], eaddr[6], eaddr[7]

#define PRINTF_FORMAT_SADDR(saddr) \
  "%02X:%02X\n", saddr[0], saddr[1]

#define PRINTF_FORMAT_PANID(panid) \
  "%02X:%02X\n", panid[0], panid[1]

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline void i8sak_cmd_error(FAR struct i8sak_s *i8sak)
{
  sem_post(&i8sak->exclsem);
  exit(EXIT_FAILURE);
}

#ifdef CONFIG_NET_6LOWPAN
static inline void i8sak_update_ep_ip(FAR struct i8sak_s *i8sak)
{
  i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[0] = HTONS(0xfe80);
  i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[1] = 0;
  i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[2] = 0;
  i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[3] = 0;

  if (i8sak->ep_addr.mode == IEEE802154_ADDRMODE_EXTENDED)
    {
     i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[4] =
        HTONS(((uint16_t)i8sak->ep_addr.eaddr[0] << 8) |
              (uint16_t)i8sak->ep_addr.eaddr[1]);
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[5] =
        HTONS(((uint16_t)i8sak->ep_addr.eaddr[2] << 8) |
              (uint16_t)i8sak->ep_addr.eaddr[3]);
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[6] =
        HTONS(((uint16_t)i8sak->ep_addr.eaddr[4] << 8) |
              (uint16_t)i8sak->ep_addr.eaddr[5]);
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[7] =
        HTONS(((uint16_t)i8sak->ep_addr.eaddr[6] << 8) |
              (uint16_t)i8sak->ep_addr.eaddr[7]);

      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[4] ^= HTONS(0x0200);
    }
  else
    {
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[4] = 0;
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[5] = HTONS(0x00ff);
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[6] = HTONS(0xfe00);
      i8sak->ep_in6addr.sin6_addr.in6_u.u6_addr16[7] =
        ((uint16_t)i8sak->ep_addr.saddr[0] << 8 |
         (uint16_t)i8sak->ep_addr.saddr[1]);
    }
}
#endif

#endif /* __APPS_WIRELESS_IEEE802154_I8SAK_I8SAK_H */
