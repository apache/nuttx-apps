/****************************************************************************
 * apps/wireless/ieee802154/i8sak/i8sak.h
 * IEEE 802.15.4 Swiss Army Knife
 *
 *   Copyright (C) 2014-2015, 2017 Gregory Nutt. All rights reserved.
 *   Copyright (C) 2014-2015 Sebastien Lorquet. All rights reserved.
 *   Copyright (C) 2017 Verge Inc. All rights reserved.
 *
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *   Author: Anthony Merlino <anthony@vergeaero.com>
 *   Author: Gregory Nuttx <gnutt@nuttx.org>
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

#ifndef __APPS_EXAMPLES_WIRELESS_IEEE802154_I8SAK_H
#define __APPS_EXAMPLES_WIRELESS_IEEE802154_I8SAK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "wpanlistener.h"

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#if !defined(CONFIG_IEEE802154_I8SAK_PANCOORD_SADDR)
#define CONFIG_IEEE802154_I8SAK_PANCOORD_SADDR 0x000A
#endif
#if !defined(CONFIG_IEEE802154_I8SAK_PANCOORD_EADDR)
#define CONFIG_IEEE802154_I8SAK_PANCOORD_EADDR 0xDEADBEEF00FADE0A
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_COORD_SADDR)
#define CONFIG_IEEE802154_I8SAK_COORD_SADDR 0x000B
#endif
#if !defined(CONFIG_IEEE802154_I8SAK_COORD_EADDR)
#define CONFIG_IEEE802154_I8SAK_COORD_EADDR 0xDEADBEEF00FADE0B
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_DEV_SADDR)
#define CONFIG_IEEE802154_I8SAK_DEV_SADDR 0x000C
#endif
#if !defined(CONFIG_IEEE802154_I8SAK_DEV_EADDR)
#define CONFIG_IEEE802154_I8SAK_DEV_EADDR 0xDEADBEEF00FADE0C
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_PANID)
#define CONFIG_IEEE802154_I8SAK_PANID 0xFADE
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_BLATER_PERIOD)
#define CONFIG_IEEE802154_I8SAK_BLATER_PERIOD 1000
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_CHNUM)
#define CONFIG_IEEE802154_I8SAK_CHNUM 11
#endif

#if !defined(CONFIG_IEEE802154_I8SAK_CHPAGE)
#define CONFIG_IEEE802154_I8SAK_CHPAGE 0
#endif

#define I8SAK_DAEMONNAME_FMT    "i8sak_%s"
#define I8SAK_MAX_DEVNAME 12
/* /dev/ is 5 characters */
#define I8SAK_DAEMONNAME_FMTLEN (6 + (I8SAK_MAX_DEVNAME-5) + 1)

/* Helper Macros *************************************************************/

#define PRINT_COORDEADDR(eaddr) \
  printf("    Coordinator EADDR: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", \
    eaddr[0], eaddr[1], eaddr[2], eaddr[3], eaddr[4], eaddr[5], eaddr[6], eaddr[7])

#define PRINT_COORDSADDR(saddr) \
  printf("    Coordinator SADDR: %02X:%02X\n", saddr[0], saddr[1])

/****************************************************************************
 * Public Types
 ****************************************************************************/

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

  bool initialized      : 1;
  bool daemon_started   : 1;
  bool daemon_shutdown  : 1;
  bool addrset          : 1;
  bool blasterenabled   : 1;
  bool verbose          : 1;
  bool indirect         : 1;
  bool acceptall        : 1;
  bool assoc            : 1;

  pid_t daemon_pid;
  FAR char devname[I8SAK_MAX_DEVNAME];
  struct wpanlistener_s wpanlistener;
  int result;
  int fd;

  sem_t exclsem;   /* For synchronizing access to the signaling semaphore */
  sem_t sigsem;    /* For signaling various tasks */
  sem_t updatesem; /* For signaling the daemon that it's settings have changed */

  uint8_t msdu_handle;

  /* Settings */

  uint8_t chan;
  uint8_t chpage;
  struct ieee802154_addr_s addr;
  struct ieee802154_addr_s ep;
  uint8_t next_saddr[IEEE802154_SADDRSIZE];
  uint8_t payload[IEEE802154_MAX_MAC_PAYLOAD_SIZE];
  uint16_t payload_len;
  int blasterperiod;

  struct ieee802154_pandesc_s pandescs[CONFIG_MAC802154_NPANDESC];
  uint8_t npandesc;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int i8sak_tx(FAR struct i8sak_s *i8sak, int fd);
long i8sak_str2long(FAR const char *str);
uint8_t i8sak_str2luint8(FAR const char *str);
uint16_t i8sak_str2luint16(FAR const char *str);
uint8_t i8sak_char2nibble(char ch);

int i8sak_str2payload(FAR const char *str, FAR uint8_t *buf);
void i8sak_str2eaddr(FAR const char *str, FAR uint8_t *eaddr);
void i8sak_str2saddr(FAR const char *str, FAR uint8_t *saddr);
void i8sak_str2panid(FAR const char *str, FAR uint8_t *panid);
bool i8sak_str2bool(FAR const char *str);

void i8sak_startpan_cmd    (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_acceptassoc_cmd (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_assoc_cmd       (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_scan_cmd        (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_tx_cmd          (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_poll_cmd        (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_sniffer_cmd     (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_blaster_cmd     (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_chan_cmd        (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_coordinfo_cmd   (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_reset_cmd       (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);
void i8sak_regdump_cmd     (FAR struct i8sak_s *i8sak, int argc, FAR char *argv[]);

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline void i8sak_cmd_error(FAR struct i8sak_s *i8sak)
{
  sem_post(&i8sak->exclsem);
  exit(EXIT_FAILURE);
}

#endif /* __APPS_EXAMPLES_WIRELESS_IEEE802154_I8SAK_H */
