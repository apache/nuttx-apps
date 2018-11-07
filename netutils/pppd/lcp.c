/****************************************************************************
 * netutils/pppd/lcp.c
 * Link Configuration Protocol Handler
 *
 *   Version - 0.1 Original Version June 3, 2000 -
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

#include <nuttx/config.h>

#include "ppp_conf.h"
#include "ppp_arch.h"
#include "ppp.h"
#include "ahdlc.h"
#include "lcp.h"

#include "netutils/pppd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if PPP_DEBUG
#  define DEBUG1(x) debug_printf x
#  define DEBUG2(x) debug_printf x
#else
#  define DEBUG1(x)
#  define DEBUG2(x)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* We need this when we neg our direction.
 * uint8_t lcp_tx_options;
 */

/* Define the supported parameters for this module here. */

static const uint8_t g_lcplist[] =
{
  LPC_MAGICNUMBER,
  LPC_PFC,
  LPC_ACFC,
#ifdef CONFIG_NETUTILS_PPPD_PAP
  LPC_AUTH,
#endif
  LPC_ACCM,
  LPC_MRU,
  0
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * lcp_init() - Initialize the LCP engine to startup values
 *
 ****************************************************************************/

void lcp_init(struct ppp_context_s *ctx)
{
  ctx->lcp_state = 0;
  ctx->lcp_retry = 0;
}

/****************************************************************************
 * lcp_rx() - Receive an LCP packet and process it.
 *    This routine receives a LCP packet in buffer of length count.
 *    Process it here, support for CONF_REQ, CONF_ACK, CONF_NACK, CONF_REJ or
 *    TERM_REQ.
 *
 ****************************************************************************/

void lcp_rx(struct ppp_context_s *ctx, uint8_t * buffer, uint16_t count)
{
  FAR uint8_t *bptr = buffer;
  FAR uint8_t *tptr;
  uint8_t error = 0;
  uint8_t id;
  uint16_t len;
  uint16_t j;

  switch (*bptr++)
    {
    case CONF_REQ:             /* config request */
      /* Parse request and see if we can ACK it */

      id = *bptr++;
      UNUSED(id);
      len = (*bptr++ << 8);
      len |= *bptr++;

      /* len -= 2; */

      /* In case of new peer connection */

      ipcp_init(ctx);
      ctx->lcp_state &= ~LCP_RX_UP;

      DEBUG1(("received [LCP Config Request id %u\n", id));
      if (scan_packet
          (ctx, (uint16_t)LCP, g_lcplist, buffer, bptr, (uint16_t)(len - 4)))
        {
          /* Must do the -4 here, !scan packet */

          DEBUG1((" options were rejected\n"));
        }
      else
        {
          /* Lets try to implement what peer wants */

          tptr = bptr;
          error = 0;

          /* First scan for unknown values */

          while (bptr < (buffer + len))
            {
              switch (*bptr++)
                {
                case LPC_MRU:  /* mru */
                  j = *bptr++;
                  j -= 2;

                  if (j == 2)
                    {
                      ctx->ppp_tx_mru = ((int)*bptr++) << 8;
                      ctx->ppp_tx_mru |= *bptr++;
                      DEBUG1(("<mru %d> ", ctx->ppp_tx_mru));
                    }
                  else
                    {
                      DEBUG1(("<mru ?? > "));
                    }
                  break;

                case LPC_ACCM:
                  bptr++;       /* skip length */
                  j = *bptr++;
                  j += *bptr++;
                  j += *bptr++;
                  j += *bptr++;

                  if (j == 0)
                    {
                      /* OK */

                      DEBUG1(("<asyncmap sum=0x%04x>", j));
                      ctx->ahdlc_flags |= PPP_TX_ASYNC_MAP;
                    }
                  else if (j == 0x3fc)
                    {
                      /* OK */

                      DEBUG1(("<asyncmap sum=0x%04x>, assume 0xffffffff", j));
                      ctx->ahdlc_flags &= ~PPP_TX_ASYNC_MAP;
                    }
                  else
                    {
                      /* Fail.  We only support default or all zeros */

                      DEBUG1(("We only support default or all zeros for ACCM "));
                      error = 1;
                      *tptr++ = LPC_ACCM;
                      *tptr++ = 0x6;
                      *tptr++ = 0;
                      *tptr++ = 0;
                      *tptr++ = 0;
                      *tptr++ = 0;
                    }
                  break;

#ifdef CONFIG_NETUTILS_PPPD_PAP
                case LPC_AUTH:
                  bptr++;
                  if ((*bptr++ == 0xc0) && (*bptr++ == 0x23))
                    {
                      /* Negotiate PAP */

                      DEBUG1(("<auth pap> "));
                      ctx->lcp_state |= LCP_RX_AUTH;
                    }
                  else
                    {
                      /* We only support PAP */

                      DEBUG1(("<auth ?? >"));
                      error = 1;
                      *tptr++ = LPC_AUTH;
                      *tptr++ = 0x4;
                      *tptr++ = 0xc0;
                      *tptr++ = 0x23;
                    }
                  break;
#endif                                 /* CONFIG_NETUTILS_PPPD_PAP */

                case LPC_MAGICNUMBER:
                  DEBUG1(("<magic > "));

                  /* Compare incoming number to our number (not implemented) */

                  bptr++;       /* For now just dump */
                  bptr++;
                  bptr++;
                  bptr++;
                  bptr++;
                  break;

                case LPC_PFC:
                  bptr++;
                  DEBUG1(("<pcomp> "));
                  ctx->ahdlc_flags |= PPP_PFC;
                  break;

                case LPC_ACFC:
                  bptr++;
                  DEBUG1(("<accomp> "));
                  ctx->ahdlc_flags |= PPP_ACFC;
                  break;
                }
            }

          /* Error? if we we need to send a config Reject ++++ this is good for
           * a subroutine.
           */

          if (error)
            {
              /* Write the config NAK packet we've built above, take on the
               * header
               */

              bptr = buffer;
              *bptr++ = CONF_NAK;   /* Write Conf_rej */
              bptr++;               /* tptr++; skip over ID */

              /* Write new length */

              *bptr++ = 0;
              *bptr = tptr - buffer;

              /* Write the reject frame */

              DEBUG1(("\nWriting NAK frame \n"));

              /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

              ahdlc_tx(ctx, LCP, 0, buffer, 0, (uint16_t)(tptr - buffer));
              DEBUG1(("- end NAK Write frame\n"));
            }
          else
            {
              /* If we get here then we are OK, lets send an ACK and tell the
               * rest of our modules our negotiated config.
               */

              DEBUG1(("\nSend ACK!\n"));
              bptr = buffer;
              *bptr++ = CONF_ACK;  /* Write Conf_ACK */
              bptr++;              /* Skip ID (send same one) */

              /* Set stuff */

              /* ppp_flags |= tflag;
               * DEBUG2("SET- stuff -- are we up? c=%d dif=%d \n", count,
               * (uint16_t)(bptr-buffer));
               */

              /* Write the ACK frame */

              DEBUG2(("Writing ACK frame \n"));

              /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

              ahdlc_tx(ctx, LCP, 0, buffer, 0, count /* bptr-buffer */);
              DEBUG2(("- end ACK Write frame\n"));
              ctx->lcp_state |= LCP_RX_UP;
            }
        }
      break;

    case CONF_ACK:             /* config Ack Anytime we do an ack reset the
                                * timer to force send. */
      DEBUG1(("LCP-ACK - "));

      /* Check that ID matches one sent */

      if (*bptr++ == ctx->ppp_id)
        {
          /* Change state to PPP up. */

          DEBUG1((">>>>>>>> good ACK id up! %d\n", ctx->ppp_id));

          /* Copy negotiated values over */

          ctx->lcp_state |= LCP_TX_UP;
        }
      else
        {
          DEBUG1(("*************++++++++++ bad id %d\n", ctx->ppp_id));
        }
      break;

    case CONF_NAK:             /* Config Nack */
      DEBUG1(("LCP-CONF NAK\n"));
      ctx->ppp_id++;
      break;

    case CONF_REJ:             /* Config Reject */
      DEBUG1(("LCP-CONF REJ\n"));
      ctx->ppp_id++;
      break;

    case TERM_REQ:             /* Terminate Request */
      DEBUG1(("LCP-TERM-REQ -"));
      bptr = buffer;
      *bptr++ = TERM_ACK;       /* Write TERM_ACK */

      /* Write the reject frame */

      DEBUG1(("Writing TERM_ACK frame \n"));

      /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

      ahdlc_tx(ctx, LCP, 0, buffer, 0, count);

      ctx->lcp_state &= ~LCP_TX_UP;
      ctx->lcp_state |= LCP_TERM_PEER;
      break;

    case TERM_ACK:
      DEBUG1(("LCP-TERM ACK\n"));
      break;

    case ECHO_REQ:
      if ((ctx->lcp_state & LCP_TX_UP) && (ctx->lcp_state & LCP_RX_UP))
        {
          bptr = buffer;
          *bptr++ = ECHO_REP;   /* Write ECHO-REPLY */

          bptr += 3;            /* Skip id and length */

          *bptr++ = 0;          /* Zero Magic */
          *bptr++ = 0;
          *bptr++ = 0;
          *bptr++ = 0;

          /* Write the echo reply frame */

          DEBUG1(("\nWriting ECHO-REPLY frame \n"));

          /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

          ahdlc_tx(ctx, LCP, 0, buffer, 0, count);
          DEBUG1(("- end ECHO-REPLY Write frame\n"));
        }
      break;

    case ECHO_REP:
      DEBUG1(("LCP-ECHO REPLY\n"));
      if ((ctx->lcp_state & LCP_TX_UP) && (ctx->lcp_state & LCP_RX_UP))
        {
          ctx->ppp_id++;
        }
      break;

    default:
      DEBUG1(("LCP unknown packet: %02x", *(bptr - 1)));
      break;
    }
}

/****************************************************************************
 * Name: lcp_disconnect
 ****************************************************************************/

void lcp_disconnect(struct ppp_context_s *ctx, uint8_t id)
{
  uint8_t buffer[4];
  FAR uint8_t *bptr = buffer;

  *bptr++ = TERM_REQ;
  *bptr++ = id;
  *bptr++ = 0;
  *bptr++ = 4;

  ahdlc_tx(ctx, LCP, 0, buffer, 0, bptr - buffer);
}

/****************************************************************************
 * Name: lcp_echo_request
 ****************************************************************************/

void lcp_echo_request(struct ppp_context_s *ctx)
{
  uint8_t buffer[8];
  FAR uint8_t *bptr;
  uint16_t t;
  LCPPKT *pkt;

  if ((ctx->lcp_state & LCP_TX_UP) && (ctx->lcp_state & LCP_RX_UP))
    {
      if ((ppp_arch_clock_seconds() - ctx->lcp_prev_seconds) >
          LCP_ECHO_INTERVAL)
        {
          ctx->lcp_prev_seconds = ppp_arch_clock_seconds();

          pkt = (LCPPKT *) buffer;

          /* Configure-Request only here, write id */

          pkt->code = ECHO_REQ;
          pkt->id = ctx->ppp_id;

          bptr = pkt->data;

          *bptr++ = 0;
          *bptr++ = 0;
          *bptr++ = 0;
          *bptr++ = 0;

          /* Write length */

          t = bptr - buffer;
          pkt->len = htons(t);  /* length here - code and ID + */

          DEBUG1((" len %d\n", t));

          /* Send packet
           * Send packet ahdlc_txz(procol,header,data,headerlen,datalen);
           */

          DEBUG1(("\nWriting ECHO-REQUEST frame \n"));
          ahdlc_tx(ctx, LCP, 0, buffer, 0, t);
          DEBUG1(("- end ECHO-REQUEST Write frame\n"));
        }
    }
}

/****************************************************************************
 * lcp_task(buffer) - This routine see if a lcp request needs to be sent
 *    out.  It uses the passed buffer to form the packet.  This formed LCP
 *    request is what we negotiate for sending options on the link.
 *
 *    Currently we negotiate : Magic Number Only, but this will change.
 *
 ****************************************************************************/

void lcp_task(FAR struct ppp_context_s *ctx, FAR uint8_t * buffer)
{
  FAR uint8_t *bptr;
  uint16_t t;
  LCPPKT *pkt;

  /* lcp tx not up and hasn't timed out then lets see if we need to send a
   * request
   */

  if (!(ctx->lcp_state & LCP_TX_UP) && !(ctx->lcp_state & LCP_TX_TIMEOUT))
    {
      /* Check if we have a request pending */

      if ((ppp_arch_clock_seconds() - ctx->lcp_prev_seconds) > LCP_TIMEOUT)
        {
          ctx->lcp_prev_seconds = ppp_arch_clock_seconds();

          DEBUG1(("\nSending LCP request packet - "));

          /* No pending request, lets build one */

          pkt = (LCPPKT *) buffer;

          /* Configure-Request only here, write id */

          pkt->code = CONF_REQ;
          pkt->id = ctx->ppp_id;

          bptr = pkt->data;

          /* Write options */

          *bptr++ = LPC_ACCM;
          *bptr++ = 0x6;
          *bptr++ = 0xff;
          *bptr++ = 0xff;
          *bptr++ = 0xff;
          *bptr++ = 0xff;

#if 0
          /* Write magic number */

          DEBUG1(("LPC_MAGICNUMBER -"));
          *bptr++ = LPC_MAGICNUMBER;
          *bptr++ = 0x6;

          /* bptr++ = random_rand() & 0xff;
           * bptr++ = random_rand() & 0xff;
           * bptr++ = random_rand() & 0xff;
           * bptr++ = random_rand() & 0xff;
           */

          *bptr++ = 0x11;
          *bptr++ = 0x11;
          *bptr++ = 0x11;
          *bptr++ = 0x11;
#endif

#if 0
          /* Authentication protocol */

          if ((lcp_tx_options & LCP_OPT_AUTH) && 0)
            {
              /* If turned on, we only negotiate PAP */

              *bptr++ = LPC_AUTH;
              *bptr++ = 0x4;
              *bptr++ = 0xc0;
              *bptr++ = 0x23;
            }

          /* PFC */

          if ((lcp_tx_options & LCP_OPT_PFC) && 0)
            {
              /* If turned on, we only negotiate PAP */

              *bptr++ = LPC_PFC;
              *bptr++ = 0x2;
            }

          /* ACFC */

          if ((lcp_tx_options & LCP_OPT_ACFC) && 0)
            {
              /* If turned on, we only negotiate PAP */

              *bptr++ = LPC_ACFC;
              *bptr++ = 0x2;
            }
#endif

          /* Write length */

          t = bptr - buffer;
          pkt->len = htons(t);  /* length here - code and ID + */

          DEBUG1((" len %d\n", t));

          /* Send packet
           * Send packet ahdlc_txz(procol,header,data,headerlen,datalen);
           */

          ahdlc_tx(ctx, LCP, 0, buffer, 0, t);

          /* Inc retry */

          ctx->lcp_retry++;

          /* Have we timed out? */

          if (ctx->lcp_retry > LCP_RETRY_COUNT)
            {
              ctx->lcp_state |= LCP_TX_TIMEOUT;
            }
        }
    }
}
