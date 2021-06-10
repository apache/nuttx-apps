/****************************************************************************
 * apps/wireless/ieee802154/libmac/ieee802154_setreq.c
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

#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <nuttx/fs/ioctl.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee802154_set_req(int fd, FAR struct ieee802154_set_req_s *req)
{
  int ret;

  ret = ioctl(fd, MAC802154IOC_MLME_SET_REQUEST,
             (unsigned long)((uintptr_t)req));
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "MAC802154IOC_MLME_SET_REQUEST failed: %d\n", ret);
    }

  return ret;
}

/* Wrappers around set_req to make it even easier to set common attributes */

int ieee802154_setchan(int fd, uint8_t chan)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_PHY_CHAN;
  req.attrval.phy.chan = chan;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_seteaddr(int fd, FAR const uint8_t *eaddr)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_EADDR;
  IEEE802154_EADDRCOPY(req.attrval.mac.eaddr, eaddr);

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setsaddr(int fd, FAR const uint8_t *saddr)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_SADDR;
  IEEE802154_SADDRCOPY(req.attrval.mac.saddr, saddr);

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setpanid(int fd, FAR const uint8_t *panid)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_PANID;
  IEEE802154_PANIDCOPY(req.attrval.mac.panid, panid);

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setrxonidle(int fd, bool rxonidle)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_RX_ON_WHEN_IDLE;
  req.attrval.mac.rxonidle = rxonidle;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setpromisc(int fd, bool promisc)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_PROMISCUOUS_MODE;
  req.attrval.mac.promisc_mode = promisc;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setassocpermit(int fd, bool assocpermit)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_ASSOCIATION_PERMIT;
  req.attrval.mac.assocpermit = assocpermit;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_settxpwr(int fd, int32_t txpwr)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_PHY_TX_POWER;
  req.attrval.phy.txpwr = txpwr;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setmaxretries(int fd, uint8_t retries)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_MAC_MAX_FRAME_RETRIES;
  req.attrval.mac.max_retries = retries;

  return ieee802154_set_req(fd, &req);
}

int ieee802154_setfcslen(int fd, uint8_t fcslen)
{
  struct ieee802154_set_req_s req;

  req.attr = IEEE802154_ATTR_PHY_FCS_LEN;
  req.attrval.phy.fcslen = fcslen;

  return ieee802154_set_req(fd, &req);
}
