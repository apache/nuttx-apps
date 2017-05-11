/****************************************************************************
 * netutils/pppd/ppp.c
 * PPP Processor/Handler
 *
 *   Version: 0.1 Original Version Jun 3, 2000
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
#include "ipcp.h"
#include "lcp.h"

#ifdef CONFIG_NETUTILS_PPPD_PAP
#include "pap.h"
#endif /* CONFIG_NETUTILS_PPPD_PAP */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if PPP_DEBUG
#  define DEBUG1(x) debug_printf x
#else
#  define DEBUG1(x)
#endif

/* Set the debug message level */

#define    PACKET_RX_DEBUG 1

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Unknown Protocol Handler, sends reject
 *
 ****************************************************************************/

static void ppp_reject_protocol(struct ppp_context_s *ctx, u16_t protocol,
                                u8_t *buffer, u16_t count)
{
  u16_t    i;
  u8_t *dptr, *sptr;
  LCPPKT *pkt;

  /* first copy rejected packet back, start from end and work forward,
     +++ Pay attention to buffer management when updated. Assumes fixed
     PPP blocks. */

  DEBUG1(("Rejecting Protocol\n"));
  if ((count + 6) > PPP_RX_BUFFER_SIZE)
    {
      /* This is a fatal error +++ do something about it. */

      DEBUG1(("Cannot Reject Protocol, PKT to big\n"));
      return;
    }

  dptr = buffer + count + 6;
  sptr = buffer + count;
  for (i = 0; i < count; ++i)
    {
      *dptr-- = *sptr--;
    }

  pkt = (LCPPKT *)buffer;
  pkt->code = PROT_REJ;        /* Write Conf_rej */
  /*pkt->id = tid++;*/            /* write tid */
  pkt->len = htons(count + 6);
  *((u16_t *)(&pkt->data[0])) = htons(protocol);

  ahdlc_tx(ctx, LCP, buffer, 0, (u16_t)(count + 6), 0);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dump_ppp_packet
 ****************************************************************************/

#if PACKET_RX_DEBUG
void dump_ppp_packet(u8_t *buffer, u16_t len)
{
  int i;

  DEBUG1(("\n"));
  for (i = 0;i < len; ++i)
    {
      if ((i & 0x1f) == 0x10)
        {
          DEBUG1(("\n"));
        }

      DEBUG1(("0x%02x ",buffer[i]));
    }

  DEBUG1(("\n\n"));
}
#endif

/****************************************************************************
 * Initialize and start PPP engine.  This just sets things up to
 * starting values.  This can stay a private method.
 *
 ****************************************************************************/

void ppp_init(struct ppp_context_s *ctx)
{
#ifdef PPP_STATISTICS
  ctx->ppp_rx_frame_count = 0;
#endif
  ctx->ppp_flags = 0;
  ctx->ip_no_data_time = 0;
  ctx->ppp_id = 0;

#ifdef CONFIG_NETUTILS_PPPD_PAP
  pap_init(ctx);
#endif /* CONFIG_NETUTILS_PPPD_PAP */
  ipcp_init(ctx);
  lcp_init(ctx);

  ahdlc_init(ctx);
  ahdlc_rx_ready(ctx);
}

/****************************************************************************
 * raise_ppp() - This routine will try to bring up a PPP connection,
 *  It is blocking. In the future we probably want to pass a
 *  structure with all the options on bringing up a PPP link, like
 *  server/client, DSN server, username password for PAP... +++ for
 *  now just use config and bit defines
 *
 ****************************************************************************/

#if 0
u16_t ppp_raise(u8_t config, u8_t *username, u8_t *password)
{
  u16_t    status = 0;

  /* Initialize PPP engine */
  /* init_ppp(); */

  pap_init();
  ipcp_init();
  lcp_init();

  /* Enable PPP */

  ppp_flags = PPP_RX_READY;

  /* Try to bring up the layers */

  while (status == 0)
    {
#ifdef SYSTEM_POLLER
      /* If the serial interrupt is not hooked to ahdlc_rx, or the
         system needs to handle other stuff while were blocking, call
         the system poller.*/

      system_poller();
#endif

      /* Call the lcp task to bring up the LCP layer */

      lcp_task(ppp_tx_buffer);

      /* If LCP is up, neg next layer */

      if (lcp_state & LCP_TX_UP)
        {
          /* If LCP wants PAP, try to authenticate, else bring up IPCP */

          if ((lcp_state & LCP_RX_AUTH) && (!(pap_state & PAP_TX_UP)))
            {
              pap_task(ppp_tx_buffer,username,password);
            }
          else
            {
              ipcp_task(ppp_tx_buffer);
            }
        }

      /* If IPCP came up then our link should be up. */

      if ((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP))
        {
          break;
        }

      status = check_ppp_errors();
    }

  return status;
}
#endif

/****************************************************************************
 * Name: ppp_connect
 ****************************************************************************/

void ppp_connect(struct ppp_context_s *ctx)
{
  /* Initialize PPP engine */

  /* init_ppp(); */

#ifdef CONFIG_NETUTILS_PPPD_PAP
  pap_init(ctx);
#endif /* CONFIG_NETUTILS_PPPD_PAP */
  ipcp_init(ctx);
  lcp_init(ctx);

  /* Enable PPP */

  ctx->ppp_flags = PPP_RX_READY;
}

/****************************************************************************
 * Name: ppp_send
 ****************************************************************************/

void ppp_send(struct ppp_context_s *ctx)
{
  /* If IPCP came up then our link should be up. */

  if ((ctx->ipcp_state & IPCP_TX_UP) && (ctx->ipcp_state & IPCP_RX_UP))
    {
      ahdlc_tx(ctx, IPV4, 0, ctx->ip_buf, 0, ctx->ip_len);
    }
}

/****************************************************************************
 * Name: ppp_poll
 ****************************************************************************/

void ppp_poll(struct ppp_context_s *ctx)
{
  u8_t c;

  ctx->ip_len = 0;

  ++ctx->ip_no_data_time;
  if (ctx->ip_no_data_time > PPP_IP_TIMEOUT)
    {
      ppp_reconnect(ctx);
      return;
    }

  if (!(ctx->ppp_flags & PPP_RX_READY))
    {
      return;
    }

  while (ctx->ip_len == 0 && ppp_arch_getchar(ctx, &c))
    {
      ahdlc_rx(ctx, c);
    }

  /* If IPCP came up then our link should be up. */

  if ((ctx->ipcp_state & IPCP_TX_UP) && (ctx->ipcp_state & IPCP_RX_UP))
    {
      lcp_echo_request(ctx, ctx->ip_buf);
      return;
    }

  /* Call the lcp task to bring up the LCP layer */

  lcp_task(ctx, ctx->ip_buf);

  /* If LCP is up, neg next layer */

  if ((ctx->lcp_state & LCP_TX_UP) && (ctx->lcp_state & LCP_RX_UP))
    {
      /* If LCP wants PAP, try to authenticate, else bring up IPCP */
#ifdef CONFIG_NETUTILS_PPPD_PAP
      if ((ctx->lcp_state & LCP_RX_AUTH) && (!(ctx->pap_state & PAP_TX_UP)))
        {
          pap_task(ctx, ctx->ip_buf);
        }
      else
        {
          ipcp_task(ctx, ctx->ip_buf);
        }
#else
      if (ctx->lcp_state & LCP_RX_AUTH)
        {
          /* lcp is asking for authentication but we do not support this.
           * This should be communicated upstream but we do not have an
           * interface for that right now, so just ignore it; nothing can be
           * done.  This also should not have been hit because upcall does
           * not know about the pap message type.
           */

          DEBUG1(("Asking for PAP, but we do not know PAP\n"));
        }
      else
        {
          ipcp_task(ctx, ctx->ip_buf);
        }
#endif /* CONFIG_NETUTILS_PPPD_PAP */
    }
}

/****************************************************************************
 * Name: ppp_upcall
 *
 * Description:
 *   This is where valid PPP frames from the ahdlc layer are sent to be
 *   processed and demuxed.
 *
 ****************************************************************************/

void ppp_upcall(struct ppp_context_s *ctx, u16_t protocol, u8_t *buffer, u16_t len)
{
#ifdef PPP_STATISTICS
  ++ctx->ppp_rx_frame_count;

#ifdef PPP_DEBUG
  dump_ppp_packet(buffer, len);
#endif

#endif /* PPP_STATISTICS */

  /* Check to see if we have a packet waiting to be processed */

  if (ctx->ppp_flags & PPP_RX_READY)
    {
      /* Demux on protocol field */

      switch (protocol)
        {
        case LCP:  /* We must support some level of LCP */
          DEBUG1(("LCP Packet - "));
          lcp_rx(ctx, buffer, len);
          DEBUG1(("\n"));
          break;

#ifdef CONFIG_NETUTILS_PPPD_PAP
        case PAP:  /* PAP should be compile in optional */
          DEBUG1(("PAP Packet - "));
          pap_rx(ctx, buffer, len);
          DEBUG1(("\n"));
          break;
#endif /* CONFIG_NETUTILS_PPPD_PAP */

        case IPCP: /* IPCP should be compile in optional. */
          DEBUG1(("IPCP Packet - "));
          ipcp_rx(ctx, buffer, len);
          DEBUG1(("\n"));
          break;

        case IPV4: /* We must support IPV4 */
          DEBUG1(("IPV4 Packet---\n"));
          memcpy(ctx->ip_buf, buffer, len);
          ctx->ip_len = len;
          ctx->ip_no_data_time = 0;
          DEBUG1(("\n"));
          break;

        default:
          DEBUG1(("Unknown PPP Packet Type 0x%04x - ",protocol));
          ppp_reject_protocol(ctx, protocol, buffer, len);
          DEBUG1(("\n"));
          break;
        }
    }
}

/****************************************************************************
 * scan_packet(list,buffer,len)
 *
 * list = list of supported ID's
 * *buffer pointer to the first code in the packet
 * length of the codespace
 *
 ****************************************************************************/

u16_t scan_packet(struct ppp_context_s *ctx, u16_t protocol, const u8_t *list,
                  u8_t *buffer, u8_t *options, u16_t len)
{
  const u8_t *tlist;
  u8_t *bptr;
  u8_t *tptr;
  u8_t bad = 0;
  u8_t i, j, good;

  bptr = tptr = options;

  /* Scan through the packet and see if it has any unsupported codes */

  while (bptr < options + len)
    {
      /* Get code and see if it matches somwhere in the list, if not
         we don't support it */

      i = *bptr++;

      /* DEBUG2("%x - ",i); */

      tlist = list;
      good = 0;
      while (*tlist)
        {
          /* DEBUG2("%x ",*tlist); */
          if (i == *tlist++)
            {
              good = 1;
              break;
            }
        }

      if (!good)
        {
          /* We don't understand it, write it back */

          DEBUG1(("We don't understand option 0x%02x\n",i));
          bad = 1;
          *tptr++ = i;
          j = *tptr++ = *bptr++;
          for (i = 0; i < j - 2; ++i)
            {
              *tptr++ = *bptr++;
            }
        }
      else
        {
          /* Advance over to next option */

          bptr += *bptr - 1;
        }
  }

  /* Bad? if we we need to send a config Reject */

  if (bad)
    {
      /* Write the config Rej packet we've built above, take on the header */

      bptr = buffer;
      *bptr++ = CONF_REJ;  /* Write Conf_rej */
      bptr++;              /* skip over ID */
      *bptr++ = 0;
      *bptr = tptr - buffer;

      /* Length right here? */

      /* Write the reject frame */

      DEBUG1(("Writing Reject frame --\n"));
      ahdlc_tx(ctx, protocol, buffer, 0, (u16_t)(tptr - buffer), 0);
      DEBUG1(("\nEnd writing reject \n"));
    }

  return bad;
}
