/****************************************************************************
 * ieee802154/common/ieee802154_addrparse.c
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
#include <string.h>
#include <errno.h>
#include <nuttx/ieee802154/ieee802154.h>
#include <apps/ieee802154/ieee802154.h>

int ieee802154_addrparse(FAR struct ieee802154_packet_s *packet,
                         FAR struct ieee802154_addr_s *dest,
                         FAR struct ieee802154_addr_s *src)
{
  uint8_t fc1, fc2, daddr, saddr;
  int index=3;

  /* read fc */
  fc1 = packet->data[0];
  fc2 = packet->data[1];
  daddr = fc2 & IEEE802154_FC2_DADDR;
  saddr = fc2 & IEEE802154_FC2_SADDR;

  /* decode dest addr */

  if(daddr == IEEE802154_DADDR_SHORT)
    {
      memcpy(&dest->ia_panid, packet->data+index, 2);
      index += 2; /* skip dest pan id */
      memcpy(&dest->ia_saddr, packet->data+index, 2);
      index += 2; /* skip dest addr */
      dest->ia_len = 2;
    }
  else if(daddr == IEEE802154_DADDR_EXT)
    {
      memcpy(&dest->ia_panid, packet->data+index, 2);
      index += 2; /* skip dest pan id */
      memcpy(dest->ia_eaddr, packet->data+index, 8);
      index += 8; /* skip dest addr */
      dest->ia_len = 8;
    }
  else if(daddr == IEEE802154_DADDR_NONE)
    {
      dest->ia_len = 0;
    }
  else
    {
      return -EINVAL;
    }

  /* decode source pan id according to compression */
  if( (saddr == IEEE802154_SADDR_SHORT) || (saddr == IEEE802154_SADDR_EXT) )
    {
      if(fc1 & IEEE802154_FC1_INTRA)
        {
          src->ia_panid = dest->ia_panid;
        }
      else
        {
          memcpy(&src->ia_panid, packet->data+index, 2);
          index += 2; /*skip dest pan id*/
        }
    }

  /* decode source addr */

  if(saddr == IEEE802154_SADDR_SHORT)
    {
      memcpy(&src->ia_saddr, packet->data+index, 2);
      index += 2; /* skip dest addr */
      src->ia_len = 2;
    }
  if(saddr == IEEE802154_SADDR_EXT)
    {
      memcpy(src->ia_eaddr, packet->data+index, 8);
      index += 8; /* skip dest addr */
      src->ia_len = 8;
    }
  else if(saddr == IEEE802154_SADDR_NONE)
    {
      src->ia_len = 0;
    }
  else
    {
      return -EINVAL;
    }

  return index;
}

