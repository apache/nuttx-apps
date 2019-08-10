/****************************************************************************
 * apps/include/wireless/ieee802154.h
 *
 *   Copyright(C) 2015 Sebastien Lorquet. All rights reserved.
 *   Copyright(C) 2017 Verge Inc. All rights reserved.
 *
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *   Author: Anthony Merlino <anthony@vergeaero.com>
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

#include <nuttx/config.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* libmac *******************************************************************/
/* Character driver IOCTL helpers */


int ieee802154_assoc_req(int fd, FAR struct ieee802154_assoc_req_s *req);
int ieee802154_assoc_resp(int fd, FAR struct ieee802154_assoc_resp_s *resp);
int ieee802154_disassoc_req(int fd,
      FAR struct ieee802154_disassoc_req_s *req);
int ieee802154_get_req(int fd, FAR struct ieee802154_get_req_s *req);
int ieee802154_gts_req(int fd, FAR struct ieee802154_gts_req_s *req);
int ieee802154_orphan_resp(int fd,
      FAR struct ieee802154_orphan_resp_s *resp);
int ieee802154_reset_req(int fd, bool resetattr);
int ieee802154_rxenable_req(int fd,
      FAR struct ieee802154_rxenable_req_s *req);
int ieee802154_scan_req(int fd, FAR struct ieee802154_scan_req_s *req);
int ieee802154_set_req(int fd, FAR struct ieee802154_set_req_s *req);
int ieee802154_start_req(int fd, FAR struct ieee802154_start_req_s *req);
int ieee802154_sync_req(int fd, FAR struct ieee802154_sync_req_s *req);
int ieee802154_poll_req(int fd, FAR struct ieee802154_poll_req_s *req);
#if 0
int ieee802154_dps_req(int fd, FAR struct ieee802154_dps_req_s *req);
int ieee802154_sounding_req(int fd,
      FAR struct ieee802154_sounding_req_s *req);
int ieee802154_calibrate_req(int fd,
      FAR struct ieee802154_calibrate_req_s *req);
#endif

/* Get/Set Attribute helpers */

int ieee802154_setchan(int fd, uint8_t chan);
int ieee802154_getchan(int fd, FAR uint8_t *chan);

int ieee802154_setpanid(int fd, FAR const uint8_t *panid);
int ieee802154_getpanid(int fd, FAR uint8_t *panid);

int ieee802154_setsaddr(int fd, FAR const uint8_t *saddr);
int ieee802154_getsaddr(int fd, FAR uint8_t *saddr);

int ieee802154_seteaddr(int fd, FAR const uint8_t *eaddr);
int ieee802154_geteaddr(int fd, FAR uint8_t *eaddr);

int ieee802154_getcoordsaddr(int fd, FAR uint8_t *saddr);

int ieee802154_getcoordeaddr(int fd, FAR uint8_t *eaddr);

int ieee802154_setpromisc(int fd, bool promisc);
int ieee802154_getpromisc(int fd, FAR bool *promisc);

int ieee802154_setrxonidle(int fd, bool rxonidle);
int ieee802154_getrxonidle(int fd, FAR bool *rxonidle);

int ieee802154_settxpwr(int fd, int32_t txpwr);
int ieee802154_gettxpwr(int fd, FAR int32_t *txpwr);

int ieee802154_setcca(int fd, FAR struct ieee802154_cca_s *cca);
int ieee802154_getcca(int fd, FAR struct ieee802154_cca_s *cca);

int ieee802154_setmaxretries(int fd, uint8_t retries);
int ieee802154_getmaxretries(int fd, FAR uint8_t *retries);

int ieee802154_setfcslen(int fd, uint8_t fcslen);
int ieee802154_getfcslen(int fd, FAR uint8_t *fcslen);

int ieee802154_getdevmode(int fd, FAR enum ieee802154_devmode_e *devmode);

int ieee802154_setassocpermit(int fd, bool assocpermit);

#ifdef CONFIG_NET_6LOWPAN
/* Netork driver IOCTL helpers */

#if 0
int sixlowpan_mcps_register(int fd, FAR const char *ifname,
      FAR const struct ieee802154_mcps_register_s *info);
int sixlowpan_mlme_register(int fd, FAR const char *ifname,
      FAR const struct ieee802154_mlme_register_s *info);
#endif
int sixlowpan_assoc_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_assoc_req_s *req);
int sixlowpan_assoc_resp(int sock, FAR const char *ifname,
      FAR struct ieee802154_assoc_resp_s *resp);
int sixlowpan_disassoc_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_disassoc_req_s *req);
int sixlowpan_get_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_get_req_s *req);
int sixlowpan_gts_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_gts_req_s *req);
int sixlowpan_orphan_resp(int sock, FAR const char *ifname,
      FAR struct ieee802154_orphan_resp_s *resp);
int sixlowpan_reset_req(int sock, FAR const char *ifname, bool resetattr);
int sixlowpan_rxenable_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_rxenable_req_s *req);
int sixlowpan_scan_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_scan_req_s *req);
int sixlowpan_set_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_set_req_s *req);
int sixlowpan_start_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_start_req_s *req);
int sixlowpan_sync_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_sync_req_s *req);
int sixlowpan_poll_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_poll_req_s *req);
#if 0
int sixlowpan_dps_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_dps_req_s *req);
int sixlowpan_sounding_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_sounding_req_s *req);
int sixlowpan_calibrate_req(int sock, FAR const char *ifname,
      FAR const struct ieee802154_calibrate_req_s *req);
#endif

int sixlowpan_setchan(int sock, FAR const char *ifname, uint8_t chan);
int sixlowpan_getchan(int sock, FAR const char *ifname, FAR uint8_t *chan);

int sixlowpan_setpanid(int sock, FAR const char *ifname,
      FAR const uint8_t *panid);
int sixlowpan_getpanid(int sock, FAR const char *ifname,
      FAR uint8_t *panid);

int sixlowpan_setsaddr(int sock, FAR const char *ifname,
      FAR const uint8_t *saddr);
int sixlowpan_getsaddr(int sock, FAR const char *ifname,
      FAR uint8_t *saddr);

int sixlowpan_seteaddr(int sock, FAR const char *ifname,
      FAR const uint8_t *eaddr);
int sixlowpan_geteaddr(int sock, FAR const char *ifname,
      FAR uint8_t *eaddr);

int sixlowpan_getcoordsaddr(int fd, FAR const char *ifname, FAR uint8_t *saddr);

int sixlowpan_getcoordeaddr(int fd, FAR const char *ifname, FAR uint8_t *eaddr);

int sixlowpan_setpromisc(int sock, FAR const char *ifname, bool promisc);
int sixlowpan_getpromisc(int sock, FAR const char *ifname,
      FAR bool *promisc);

int sixlowpan_setrxonidle(int sock, FAR const char *ifname, bool rxonidle);
int sixlowpan_getrxonidle(int sock, FAR const char *ifname,
      FAR bool *rxonidle);

int sixlowpan_settxpwr(int sock, FAR const char *ifname, int32_t txpwr);
int sixlowpan_gettxpwr(int sock, FAR const char *ifname,
      FAR int32_t *txpwr);

int sixlowpan_setmaxretries(int sock, FAR const char *ifname, uint8_t retries);
int sixlowpan_getmaxretries(int sock, FAR const char *ifname,
      FAR uint8_t *retries);

int sixlowpan_setfcslen(int sock, FAR const char *ifname, uint8_t fcslen);
int sixlowpan_getfcslen(int sock, FAR const char *ifname, FAR uint8_t *fcslen);

int sixlowpan_getdevmode(int fd, FAR const char *ifname,
                         FAR enum ieee802154_devmode_e *devmode);

int sixlowpan_setassocpermit(int sock, FAR const char *ifname, bool assocpermit);

#endif /* CONFIG_NET_6LOWPAN*/

/* libutils *****************************************************************/

int ieee802154_addrtostr(FAR char *buf, int len,
      FAR struct ieee802154_addr_s *addr);

#endif /*__APPS_INCLUDE_WIRELESS_IEEE802154_H */
