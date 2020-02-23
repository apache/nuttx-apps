/****************************************************************************
 * apps/netutils/pppd/ahdlc.c
 * Ahdlc receive and transmit processor for PPP engine.
 *
 *   Version: 0.1 Original Version Jan 11, 1998
 *   Copyright (C) 1998, Mycal Labs www.mycal.com
 *   Copyright (c) 2003, Mike Johnson, Mycal Labs, www.mycal.net
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Mike Johnson/Mycal Labs
 *    www.mycal.net.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ppp_conf.h"
#include "ppp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if PPP_DEBUG
#  define DEBUG1(x) debug_printf x
#  define PACKET_TX_DEBUG 1
#else
#  define DEBUG1(x)
#  define PACKET_TX_DEBUG 0
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Simple and fast CRC16 routine for embedded processors.
 *
 *    Just slightly slower than the table lookup method but consumes
 *    almost no space.  Much faster and smaller than the loop and
 *    shift method that is widely used in the embedded space.
 *    Can be optimized even more in .ASM
 *
 *    data = (crcvalue ^ inputchar) & 0xff;
 *    data = (data ^ (data << 4)) & 0xff;
 *    crc = (crc >> 8) ^ ((data << 8) ^ (data <<3) ^ (data >> 4))
 *
 ****************************************************************************/

static uint16_t crcadd(uint16_t crcvalue, uint8_t c)
{
  uint16_t b;

  b = (crcvalue ^ c) & 0xFF;
  b = (b ^ (b << 4)) & 0xFF;
  b = (b << 8) ^ (b << 3) ^ (b >> 4);

  return ((crcvalue >> 8) ^ b);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * ahdlc_init(buffer, buffersize) - this initializes the ahdlc engine to
 *    allow for rx frames.
 *
 ****************************************************************************/

void ahdlc_init(struct ppp_context_s *ctx)
{
  ctx->ahdlc_flags = PPP_RX_ASYNC_MAP;
  ctx->ahdlc_rx_count = 0;
  ctx->ahdlc_tx_offline = 0;

#ifdef PPP_STATISTICS
  ctx->ahdlc_crc_error = 0;
  ctx->ahdlc_rx_tobig_error = 0;
#endif
}

/****************************************************************************
 * ahdlc_rx_ready() - resets the ahdlc engine to the beginning of frame
 *    state.
 *
 ****************************************************************************/

void ahdlc_rx_ready(struct ppp_context_s *ctx)
{
  ctx->ahdlc_rx_count = 0;
  ctx->ahdlc_rx_crc = 0xffff;
  ctx->ahdlc_flags |= PPP_RX_READY;
}

/****************************************************************************
 * ahdlc receive function - This routine processes incoming bytes and tries
 *    to build a PPP frame.
 *
 *    Two possible reasons that ahdlc_rx will not process characters:
 *        o Buffer is locked - in this case ahdlc_rx returns 1, char
 *            sending routing should retry.
 *
 ****************************************************************************/

uint8_t ahdlc_rx(FAR struct ppp_context_s *ctx, uint8_t c)
{
  /* Check to see if PPP packet is useable, we should have hardware flow
   * control set, but if host ignores it and sends us a char when the PPP
   * Receive packet is in use, discard the character.
   */

  if ((ctx->ahdlc_flags & PPP_RX_READY) != 0)
    {
      /* Check to see if character is less than 0x20 hex we really should set
       * AHDLC_RX_ASYNC_MAP on by default and only turn it off when it is
       * negotiated off to handle some buggy stacks.
       */

      if ((c < 0x20) && ((ctx->ahdlc_flags & PPP_RX_ASYNC_MAP) == 0))
        {
          /* Discard character */

          DEBUG1(("Discard because char is < 0x20 hex and asysnc map is 0\n"));
          return 0;
        }

      /* Are we in escaped mode? */

      if ((ctx->ahdlc_flags & PPP_ESCAPED) != 0)
        {
          /* Set escaped to FALSE */

          ctx->ahdlc_flags &= ~PPP_ESCAPED;

          /* If value is 0x7e then silently discard and reset receive packet */

          if (c == 0x7e)
            {
              ahdlc_rx_ready(ctx);
              return 0;
            }

          /* Incoming char = itself xor 20 */

          c = c ^ 0x20;
        }
      else if (c == 0x7e)
        {
          /* Handle frame end */

          if (ctx->ahdlc_rx_crc == CRC_GOOD_VALUE)
            {
              DEBUG1(("\nReceiving packet with good crc value, len %d\n",
                      ctx->ahdlc_rx_count));

              /* we have a good packet, turn off CTS until we are done with this
               * packet
               */

              /* CTS_OFF(); */

#if PPP_STATISTICS
              /* Update statistics */

              ++ctx->ppp_rx_frame_count;
#endif

              /* Remove CRC bytes from packet */

              ctx->ahdlc_rx_count -= 2;

              /* Lock PPP buffer */

              ctx->ahdlc_flags &= ~PPP_RX_READY;

              /* upcall routine must fully process frame before return as
               * returning signifies that buffer belongs to AHDLC again.
               */

              if ((ctx->ahdlc_rx_buffer[0] & 0x1) != 0 &&
                  (ctx->ahdlc_flags & PPP_PFC) != 0)
                {
                  /* Send up packet */

                  ppp_upcall(ctx, (uint16_t)ctx->ahdlc_rx_buffer[0],
                             (FAR uint8_t *) & ctx->ahdlc_rx_buffer[1],
                             (uint16_t)(ctx->ahdlc_rx_count - 1));
                }
              else
                {
                  /* Send up packet */

                  ppp_upcall(ctx,
                             (uint16_t)(ctx->ahdlc_rx_buffer[0] << 8 | ctx->
                                        ahdlc_rx_buffer[1]),
                             (FAR uint8_t *) & ctx->ahdlc_rx_buffer[2],
                             (uint16_t)(ctx->ahdlc_rx_count - 2));
                }

              ctx->ahdlc_tx_offline = 0;        /* The remote side is alive */
              ahdlc_rx_ready(ctx);
              return 0;
            }
          else if (ctx->ahdlc_rx_count > 3)
            {
              DEBUG1(("\nReceiving packet with bad crc value, was 0x%04x len %d\n",
                     ctx->ahdlc_rx_crc, ctx->ahdlc_rx_count));
#ifdef PPP_STATISTICS
              ++ctx->ahdlc_crc_error;
#endif
              /* Shouldn't we dump the packet and not pass it up? */

              /* ppp_upcall((uint16_t)ahdlc_rx_buffer[0], (FAR uint8_t
               * *)&ahdlc_rx_buffer[0], (uint16_t)(ahdlc_rx_count+2));
               * dump_ppp_packet(&ahdlc_rx_buffer[0],ahdlc_rx_count);
               */
            }

          ahdlc_rx_ready(ctx);
          return 0;
        }
      else if (c == 0x7d)
        {
          /* Handle escaped chars */

          ctx->ahdlc_flags |= PPP_ESCAPED;
          return 0;
        }

      /* Try to store char if not too big */

      if (ctx->ahdlc_rx_count >= PPP_RX_BUFFER_SIZE)
        {
#ifdef PPP_STATISTICS
          ++ctx->ahdlc_rx_tobig_error;
#endif
          ahdlc_rx_ready(ctx);
        }
      else
        {
          /* Add CRC in */

          ctx->ahdlc_rx_crc = crcadd(ctx->ahdlc_rx_crc, c);

          /* Do auto ACFC, if packet len is zero discard 0xff and 0x03 */

          if (ctx->ahdlc_rx_count == 0)
            {
              if ((c == 0xff) || (c == 0x03))
                {
                  return 0;
                }
            }

          /* Store char */

          ctx->ahdlc_rx_buffer[ctx->ahdlc_rx_count++] = c;
        }
    }
  else
    {
      /* we are busy and didn't process the character. */

      DEBUG1(("Busy/not active\n"));
      return 1;
    }

  return 0;
}

/****************************************************************************
 * ahdlc_tx_char(char) - write a character to the serial device,
 * escape if necessary.
 *
 * Relies on local global vars   :    ahdlc_tx_crc, ahdlc_flags.
 * Modifies local global vars    :    ahdlc_tx_crc.
 *
 ****************************************************************************/

void ahdlc_tx_char(struct ppp_context_s *ctx, uint16_t protocol, uint8_t c)
{
  /* Add in crc */

  ctx->ahdlc_tx_crc = crcadd(ctx->ahdlc_tx_crc, c);

  /* See if we need to escape char, we always escape 0x7d and 0x7e, in the case
   * of char < 0x20 we only support async map of default or none, so escape if
   * ASYNC map is not set.  We may want to modify this to support a bitmap set
   * ASYNC map.
   */

  if ((c == 0x7d) || (c == 0x7e) || ((c < 0x20) && ((protocol == LCP) ||
                                                    (ctx->
                                                     ahdlc_flags &
                                                     PPP_TX_ASYNC_MAP) == 0)))
    {
      /* Send escape char and xor byte by 0x20 */

      ppp_arch_putchar(ctx, 0x7d);
      c ^= 0x20;
    }

  ppp_arch_putchar(ctx, c);
}

/****************************************************************************
 * ahdlc_tx(protocol,buffer,len) - Transmit a PPP frame.
 *
 *    Buffer contains protocol data, ahdlc_tx adds address, control and
 *    protocol data.
 *
 * Relies on local global vars    :    ahdlc_tx_crc, ahdlc_flags.
 * Modifies local global vars    :    ahdlc_tx_crc.
 *
 ****************************************************************************/

uint8_t ahdlc_tx(struct ppp_context_s *ctx, uint16_t protocol,
                 FAR uint8_t * header, FAR uint8_t * buffer, uint16_t headerlen,
                 uint16_t datalen)
{
  uint16_t i;
  uint8_t c;

  DEBUG1(("\nAHDLC_TX - transmit frame, protocol 0x%04x, length %d  offline %d\n",
         protocol, datalen + headerlen, ctx->ahdlc_tx_offline));

  if (AHDLC_TX_OFFLINE && (ctx->ahdlc_tx_offline++ > AHDLC_TX_OFFLINE))
    {
      ctx->ahdlc_tx_offline = 0;
      DEBUG1(("\nAHDLC_TX to many outstanding TX packets => ppp_reconnect()\n"));
      ppp_reconnect(ctx);
      return 0;
    }

#if PACKET_TX_DEBUG
  DEBUG1(("\n"));
  for (i = 0; i < headerlen; ++i)
    {
      DEBUG1(("0x%02x ", header[i]));
    }

  for (i = 0; i < datalen; ++i)
    {
      DEBUG1(("0x%02x ", buffer[i]));
    }

  DEBUG1(("\n\n"));
#endif

  /* Check to see that physical layer is up, we can assume is some cases */

  /* Write leading 0x7e */

  ppp_arch_putchar(ctx, 0x7e);

  /* Set initial CRC value */

  ctx->ahdlc_tx_crc = 0xffff;

  /* send HDLC control and address if not disabled or of LCP frame type */

  /* if ((0==(ahdlc_flags & PPP_ACFC)) || ((0xc0==buffer[0]) &&
   * (0x21==buffer[1])))
   */

  if ((0 == (ctx->ahdlc_flags & PPP_ACFC)) || (protocol == LCP))
    {
      ahdlc_tx_char(ctx, protocol, 0xff);
      ahdlc_tx_char(ctx, protocol, 0x03);
    }

  /* Write Protocol */

  ahdlc_tx_char(ctx, protocol, (uint8_t)(protocol >> 8));
  ahdlc_tx_char(ctx, protocol, (uint8_t)(protocol & 0xff));

  /* Write header if it exists */

  for (i = 0; i < headerlen; ++i)
    {
      /* Get next byte from buffer */

      c = header[i];

      /* Write it... */

      ahdlc_tx_char(ctx, protocol, c);
    }

  /* Write frame bytes */

  for (i = 0; i < datalen; ++i)
    {
      /* Get next byte from buffer */

      c = buffer[i];

      /* Write it... */

      ahdlc_tx_char(ctx, protocol, c);
    }

  /* Send crc, lsb then msb */

  i = ctx->ahdlc_tx_crc ^ 0xffff;
  ahdlc_tx_char(ctx, protocol, (uint8_t)(i & 0xff));
  ahdlc_tx_char(ctx, protocol, (uint8_t)((i >> 8) & 0xff));

  /* Write trailing 0x7e, probably not needed but it doesn't hurt */

  ppp_arch_putchar(ctx, 0x7e);

#if PPP_STATISTICS
  /* Update statistics */

  ++ctx->ppp_tx_frame_count;
#endif

  return 0;
}
