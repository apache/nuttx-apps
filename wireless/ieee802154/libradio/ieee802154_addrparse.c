/****************************************************************************
 * apps/wireless/ieee802154/libradio/ieee802154_addrparse.c
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

#include <nuttx/wireless/ieee802154/ieee802154_radio.h>
#include <nuttx/wireless/ieee802154/ieee802154_mac.h>

#include "ieee802154/ieee802154.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int ieee802154_addrparse(FAR struct ieee802154_packet_s *packet,
                         FAR struct ieee802154_addr_s *dest,
                         FAR struct ieee802154_addr_s *src)
{
  uint16_t frame_ctrl;
  int index=3;

  /* Read fc */

  frame_ctrl = packet->data[0];
  frame_ctrl |= packet->data[1] << 8;

  dest->mode = (frame_ctrl & IEEE802154_FRAMECTRL_DADDR)
                >> IEEE802154_FRAMECTRL_SHIFT_DADDR;


  src->mode = (frame_ctrl & IEEE802154_FRAMECTRL_SADDR)
                >> IEEE802154_FRAMECTRL_SHIFT_SADDR;

  /* Decode dest addr */

  switch (dest->mode)
    {
      case IEEE802154_ADDRMODE_SHORT:
        {
          memcpy(&dest->panid, packet->data+index, 2);
          index += 2; /* Skip dest pan id */
          memcpy(&dest->saddr, packet->data+index, 2);
          index += 2; /* Skip dest addr */
        }
        break;

      case IEEE802154_ADDRMODE_EXTENDED:
        {
          memcpy(&dest->panid, packet->data+index, 2);
          index += 2; /* Skip dest pan id */
          memcpy(dest->eaddr, packet->data+index, 8);
          index += 8; /* Skip dest addr */
        }
        break;

      case IEEE802154_ADDRMODE_NONE:
        break;

      default:
        return -EINVAL;
    }

    if ((src->mode == IEEE802154_ADDRMODE_SHORT) ||
        (src->mode == IEEE802154_ADDRMODE_EXTENDED))
      {
        /* If PANID compression, src PAN ID is same as dest */

        if(frame_ctrl & IEEE802154_FRAMECTRL_PANIDCOMP)
          {
            src->panid = dest->panid;
          }
        else
         {
           memcpy(&src->panid, packet->data+index, 2);
           index += 2; /*skip dest pan id*/
         }
      }

  /* Decode source addr */

  switch (src->mode)
    {
      case IEEE802154_ADDRMODE_SHORT:
        {
          memcpy(&src->saddr, packet->data+index, 2);
          index += 2; /* Skip src addr */
        }
        break;

      case IEEE802154_ADDRMODE_EXTENDED:
        {
          memcpy(src->eaddr, packet->data+index, 8);
          index += 8; /* Skip src addr */
        }
        break;

      case IEEE802154_ADDRMODE_NONE:
        break;

      default:
        return -EINVAL;
    }

  return index;
}
