/****************************************************************************
 * apps/netutils/netlib/netlib_nodeaddrconv.c
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
