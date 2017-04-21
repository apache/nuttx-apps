/****************************************************************************
 * apps/include/wireless/ieee802154.h
 *
 *   Copyright(C) 2015 Sebastien Lorquet. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_WIRELESS_IEEE802154_H
#define __APPS_INCLUDE_WIRELESS_IEEE802154_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/wireless/ieee802154/ieee802154_radio.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* libradio *****************************************************************/
/* Character driver IOCTL helpers */

int ieee802154_setchan(int fd, uint8_t chan);
int ieee802154_getchan(int fd, FAR uint8_t *chan);

int ieee802154_setpanid(int fd, uint16_t panid);
int ieee802154_getpanid(int fd, FAR uint16_t *panid);

int ieee802154_setsaddr(int fd, uint16_t saddr);
int ieee802154_getsaddr(int fd, FAR uint16_t *saddr);

int ieee802154_seteaddr(int fd, FAR const uint8_t *eaddr);
int ieee802154_geteaddr(int fd, FAR uint8_t *eaddr);

int ieee802154_setpromisc(int fd, bool promisc);
int ieee802154_getpromisc(int fd, FAR bool *promisc);

int ieee802154_setdevmode(int fd, uint8_t devmode);
int ieee802154_getdevmode(int fd, FAR uint8_t *devmode);

int ieee802154_settxpwr(int fd, int32_t txpwr);
int ieee802154_gettxpwr(int fd, FAR int32_t *txpwr);

int ieee802154_setcca(int fd, FAR struct ieee802154_cca_s *cca);
int ieee802154_getcca(int fd, FAR struct ieee802154_cca_s *cca);

int ieee802154_energydetect(int fd, FAR bool *energy);

#ifdef CONFIG_NET_6LOWPAN
/* Netork driver IOCTL helpers */

int sixlowpan_setchan(int sock, FAR const char *ifname, uint8_t chan);
int sixlowpan_getchan(int sock, FAR const char *ifname, FAR uint8_t *chan);

int sixlowpan_setpanid(int sock, FAR const char *ifname, uint16_t panid);
int sixlowpan_getpanid(int sock, FAR const char *ifname,
      FAR uint16_t *panid);

int sixlowpan_setsaddr(int sock, FAR const char *ifname, uint16_t saddr);
int sixlowpan_getsaddr(int sock, FAR const char *ifname,
      FAR uint16_t *saddr);

int sixlowpan_seteaddr(int sock, FAR const char *ifname,
      FAR const uint8_t *eaddr);
int sixlowpan_geteaddr(int sock, FAR const char *ifname,
      FAR uint8_t *eaddr);

int sixlowpan_setpromisc(int sock, FAR const char *ifname, bool promisc);
int sixlowpan_getpromisc(int sock, FAR const char *ifname,
      FAR bool *promisc);

int sixlowpan_setdevmode(int sock, FAR const char *ifname, uint8_t devmode);
int sixlowpan_getdevmode(int sock, FAR const char *ifname,
      FAR uint8_t *devmode);

int sixlowpan_settxpwr(int sock, FAR const char *ifname, int32_t txpwr);
int sixlowpan_gettxpwr(int sock, FAR const char *ifname,
      FAR int32_t *txpwr);

int sixlowpan_setcca(int sock, FAR const char *ifname,
      FAR struct ieee802154_cca_s *cca);
int sixlowpan_getcca(int sock, FAR const char *ifname,
      FAR struct ieee802154_cca_s *cca);

int sixlowpan_energydetect(int sock, FAR const char *ifname,
      FAR bool *energy);
#endif

/* libmac *******************************************************************/

/* libutils *****************************************************************/

int ieee802154_addrparse(FAR struct ieee802154_packet_s *inpacket,
      FAR struct ieee802154_addr_s *dest,
      FAR struct ieee802154_addr_s *src);
int ieee802154_addrstore(FAR struct ieee802154_packet_s *inpacket,
      FAR struct ieee802154_addr_s *dest,
      FAR struct ieee802154_addr_s *src);
int ieee802154_addrtostr(FAR char *buf, int len,
      FAR struct ieee802154_addr_s *addr);

#endif /*__APPS_INCLUDE_WIRELESS_IEEE802154_H */
