/****************************************************************************
 * apps/wireless/ieee802154/libutils/ieee802154_addrtostr.c
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
#include <stdint.h>
#include <stdio.h>

#include <nuttx/wireless/ieee802154/ieee802154_mac.h>
#include "wireless/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee802154_addrtostr(FAR char *buf, int len,
                         FAR struct ieee802154_addr_s *addr)
{
#if 0
#ifndef CONFIG_BIG_ENDIAN
  uint16_t panid = ((addr->panid & 0xff) << 8) |
                   ((addr->panid >> 8) & 0xff);
#else
  uint16_t panid = addr->panid;
#endif

  if (addr->mode == IEEE802154_ADDRMODE_NONE)
    {
      return snprintf(buf, len, "none");
    }
  else if (addr->mode == IEEE802154_ADDRMODE_SHORT)
    {
#ifndef CONFIG_BIG_ENDIAN
      uint16_t saddr = ((addr->saddr & 0xff) << 8) |
                       ((addr->saddr >> 8) & 0xff);
#else
      uint16_t saddr = addr->saddr;
#endif
      return snprintf(buf, len, "%04X/%04X", panid, saddr);
    }
  else if (addr->mode == IEEE802154_ADDRMODE_EXTENDED)
    {
      int i;
      int off = snprintf(buf, len, "%04X/", panid);

      for (i = 0; i < 8; i++)
        {
          off += snprintf(buf + off, len  -off, "%02X", addr->eaddr[i]);
        }

      return off;
    }
  else
    {
      return snprintf(buf, len, "<INVAL>");
    }

  return -1;
#endif
  return -1;
}
