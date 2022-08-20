/****************************************************************************
 * apps/netutils/pppd/pap.c
 * PAP processor for the PPP module
 *
 *   Version: 0.1 Original Version Jun 3, 2000
 *   Copyright (C) 2000, Mycal Labs www.mycal.com - -
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

#include "netutils/pppd.h"
#include "ppp_conf.h"
#include "ppp_arch.h"
#include "ppp.h"
#include "pap.h"
#include "lcp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if PPP_DEBUG
#  define DEBUG1(x) debug_printf x
#else
#  define DEBUG1(x)
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pap_init
 ****************************************************************************/

void pap_init(struct ppp_context_s *ctx)
{
  ctx->pap_retry = 0;
  ctx->pap_state = 0;
  ctx->pap_prev_seconds = 0;
}

/****************************************************************************
 * pap_rx() - PAP RX protocol Handler
 *
 ****************************************************************************/

void pap_rx(struct ppp_context_s *ctx, FAR uint8_t * buffer, uint16_t count)
{
  FAR uint8_t *bptr = buffer;
  uint8_t len;

  switch (*bptr++)
    {
    case CONF_REQ:
      DEBUG1(("CONF REQ - only for server, no support\n"));
      break;

    case CONF_ACK:             /* config Ack */
      DEBUG1(("CONF ACK - PAP good - "));

      /* Display message if debug */

      bptr += 3;
      len = *bptr++;
      *(bptr + len) = 0;
      DEBUG1((" %s\n", bptr));
      ctx->pap_state |= PAP_TX_UP;
      break;

    case CONF_NAK:
      DEBUG1(("CONF NAK - Failed Auth - "));
      ctx->pap_state |= PAP_TX_AUTH_FAIL;

      /* Display message if debug */

      bptr += 3;
      len = *bptr++;
      *(bptr + len) = 0;
      DEBUG1((" %s\n", bptr));
      break;
    }
}

/****************************************************************************
 * pap_task() - This task needs to be called every so often during the PAP
 *    negotiation phase.  This task sends PAP REQ packets.
 *
 ****************************************************************************/

void pap_task(FAR struct ppp_context_s *ctx, FAR uint8_t * buffer)
{
  FAR uint8_t *bptr;
  uint16_t t;
  PAPPKT *pkt;

  /* If LCP is up and PAP negotiated, try to bring up PAP */

  if (!(ctx->pap_state & PAP_TX_UP) && !(ctx->pap_state & PAP_TX_TIMEOUT))
    {
      /* Do we need to send a PAP auth packet? Check if we have a request
       * pending.
       */

      if ((ppp_arch_clock_seconds() - ctx->pap_prev_seconds) > PAP_TIMEOUT)
        {
          ctx->pap_prev_seconds = ppp_arch_clock_seconds();

          /* We need to send a PAP authentication request */

          DEBUG1(("\nSending PAP Request packet - "));

          /* Build a PAP request packet */

          pkt = (PAPPKT *) buffer;

          /* Configure-Request only here, write id */

          pkt->code = CONF_REQ;
          pkt->id = ctx->ppp_id;
          bptr = pkt->data;

          /* Write options */

          /* Write peer-ID length */

          t = strlen((char *)ctx->settings->pap_username);
          *bptr++ = (uint8_t)t;

          /* Write peer-ID */

          bptr = memcpy(bptr, ctx->settings->pap_username, t);
          bptr += t;

          /* Write passwd length */

          t = strlen((char *)ctx->settings->pap_password);
          *bptr++ = (uint8_t)t;

          /* Write passwd */

          bptr = memcpy(bptr, ctx->settings->pap_password, t);
          bptr += t;

          /* Write length */

          t = bptr - buffer;

          /* length here - code and ID + */

          pkt->len = htons(t);

          DEBUG1((" Len %d\n", t));

          /* Send packet */

          ahdlc_tx(ctx, PAP, buffer, 0, t, 0);

          ctx->pap_retry++;

          /* Have we failed? */

          if (ctx->pap_retry > PAP_RETRY_COUNT)
            {
              DEBUG1(("PAP - timeout\n"));
              ctx->pap_state |= PAP_TX_TIMEOUT;
            }
        }
    }
}
