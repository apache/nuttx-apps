/****************************************************************************
 * apps/wireless/ieee802154/libutils/ieee802154_addrtostr.c
 *
 *   Copyright (C) 2015 Sebastien Lorquet. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
  uint16_t panid = ((addr->panid & 0xff) << 8) | ((addr->panid >> 8) & 0xff);
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
      uint16_t saddr = ((addr->saddr & 0xff) << 8) | ((addr->saddr >> 8) & 0xff);
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
      return snprintf(buf,len,"<INVAL>");
    }

  return -1;
#endif
  return -1;
}
