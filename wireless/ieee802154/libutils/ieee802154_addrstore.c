/****************************************************************************
 * apps/wireless/ieee802154/libutils/ieee802154_addrparse.c
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

int ieee802154_addrstore(FAR struct ieee802154_packet_s *packet,
                         FAR struct ieee802154_addr_s *dest,
                         FAR struct ieee802154_addr_s *src)
{
  uint16_t frame_ctrl;
  int index = 3;       /* Skip fc and seq */

  /* Get the frame control word so we can manipulate it */

  frame_ctrl = packet->data[0];
  frame_ctrl |= packet->data[1] << 8;

  /* Clear the destination address mode */

  frame_ctrl &= ~IEEE802154_FRAMECTRL_DADDR;

  /* Encode dest addr */

  if(dest == NULL || dest->mode == IEEE802154_ADDRMODE_NONE)
    {
      /* Set the destination address mode to none */

      frame_ctrl |= IEEE802154_ADDRMODE_NONE << IEEE802154_FRAMECTRL_SHIFT_DADDR;
    }
  else
    {
      memcpy(packet->data+index, &dest->panid, 2);
      index += 2;   /* Skip dest pan id */

      /* Set the dest address mode field */

      frame_ctrl |= dest->mode << IEEE802154_FRAMECTRL_SHIFT_DADDR;

      if(dest->mode == IEEE802154_ADDRMODE_SHORT)
        {
          memcpy(packet->data+index, &dest->saddr, 2);
          index += 2;  /* Skip dest addr */
        }
      else if(dest->mode == IEEE802154_ADDRMODE_EXTENDED)
        {
          memcpy(packet->data+index, &dest->panid, 2);
          index += 2;  /* Skip dest pan id */
          memcpy(packet->data+index, dest->eaddr, 8);
          index += 8;  /* Skip dest addr */
        }
      else
        {
          return -EINVAL;
        }
    }

  /* Clear the PAN ID Compression Field */

  frame_ctrl &= ~IEEE802154_FRAMECTRL_PANIDCOMP;

  if( (dest != NULL && dest->mode != IEEE802154_ADDRMODE_NONE) &&
      (src != NULL && src->mode != IEEE802154_ADDRMODE_NONE) )
    {
      /* We have both adresses, encode source pan id according to compression */

      if( dest->panid == src->panid)
        {
          frame_ctrl |= IEEE802154_FRAMECTRL_PANIDCOMP;
        }
    }

  /* Clear the source address mode */

  frame_ctrl &= ~IEEE802154_FRAMECTRL_SADDR;

  /* Encode source addr */

  if(src == NULL || src->mode == IEEE802154_ADDRMODE_NONE)
    {
      /* Set the source address mode to none */

      frame_ctrl |= IEEE802154_ADDRMODE_NONE << IEEE802154_FRAMECTRL_SHIFT_SADDR;
    }
  else
    {
      /* Add src pan id if it was not compressed before */

      if(!(frame_ctrl & IEEE802154_FRAMECTRL_PANIDCOMP))
        {
          memcpy(packet->data+index, &src->panid, 2);
          index += 2; /*skip src pan id*/
        }

      /* Set the source address mode field */

      frame_ctrl |= src->mode << IEEE802154_FRAMECTRL_SHIFT_SADDR;

      if(src->mode == IEEE802154_ADDRMODE_SHORT)
        {
          memcpy(packet->data+index, &src->saddr, 2);
          index += 2;  /* Skip src addr */
        }
      else if(src->mode == IEEE802154_ADDRMODE_EXTENDED)
        {
          memcpy(packet->data+index, src->eaddr, 8);
          index += 8;  /* Skip src addr */
        }
      else
        {
          return -EINVAL;
        }
    }

  /* Copy the frame control word back to the buffer */

  memcpy(packet->data, &frame_ctrl, 2);

  return index;
}
