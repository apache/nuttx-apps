/****************************************************************************
 * netutils/netlib/netlib_nodeaddrconv.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "nuttx/wireless/pktradio.h"
#include "netutils/netlib.h"

#if defined(CONFIG_NET_6LOWPAN) || defined(CONFIG_NET_IEEE802154)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int get_nibble(char ch, FAR uint8_t *nibble)
{
  if (ch >= '0' && ch <= '9')
    {
      *nibble = (uint8_t)ch - '0';
    }
  else if (ch >= 'a' && ch <= 'f')
    {
      *nibble = (uint8_t)ch - 'a' + 10;
    }
  else if (ch >= 'A' && ch <= 'F')
    {
      *nibble = (uint8_t)ch - 'A' + 10;
    }
  else
    {
      return -EINVAL;
    }

  return OK;
}

static int get_byte(FAR const char *ptr, FAR uint8_t *byte)
{
  uint8_t accum1;
  uint8_t accum2;
  int ret;

  ret = get_nibble(ptr[0], &accum1);
  if (ret  < 0)
    {
      return ret;
    }

  ret = get_nibble(ptr[1], &accum2);
  if (ret  < 0)
    {
      return ret;
    }

  *byte = accum1 << 4 | accum2;
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_nodeaddrconv
 *
 * Description:
 *   Get the non-IEEE802.15.4 packet radio node address
 *
 * Parameters:
 *   addrstr - A string representing the node address
 *   nodeadd - Location to return the node address
 *
 * Return:
 *   true on success; false on failure.
 *
 ****************************************************************************/

bool netlib_nodeaddrconv(FAR const char *addrstr,
                         FAR struct pktradio_addr_s *nodeaddr)
{
  uint8_t byte;
  int ret;

  DEBUGASSERT(addrstr != NULL && nodeaddr != NULL);
  memset(nodeaddr, 0, sizeof(struct pktradio_addr_s));

  /* FORM: xx:xx:...:xx */

  /* Tolerate leading blanks */

  while (isblank(*addrstr))
    {
      addrstr++;
    }

  for (nodeaddr->pa_addrlen = 0;
       nodeaddr->pa_addrlen < CONFIG_PKTRADIO_ADDRLEN;
       )
    {
      /* Get the next byte in binary form */

      byte = 0;  /* Eliminates a warning */
      ret  = get_byte(addrstr, &byte);
      if (ret < 0)
        {
          wlwarn("get_byte failed: %s\n", addrstr);
          return false;
        }

      /* Save the byte */

      addrstr += 2;
      nodeaddr->pa_addr[nodeaddr->pa_addrlen] = byte;
      nodeaddr->pa_addrlen++;

      /* Tolerate blanks between bytes */

      while (isblank(*addrstr))
        {
          addrstr++;
        }

      /* Continue to the next byte if there is a colon separator.  Return
       * success if we are at the end of the string.  Anything other than
       * {whitespace, :, NUL} is an error.
       */

      if (*addrstr == ':')
        {
          addrstr++;
        }
      else if (*addrstr == '\0')
        {
          return true;
        }
      else
        {
          wlwarn("Unexpected delimiter: %s\n", addrstr);
          break;
        }
    }

  return false;
}

#endif /* CONFIG_NET_6LOWPAN || CONFIG_NET_IEEE802154 */
