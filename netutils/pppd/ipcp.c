/****************************************************************************
 * apps/netutils/pppd/ipcp.c
 * PPP IPCP (intrnet protocol) Processor/Handler
 *
 *   Version: 0.1 Original Version Jun 3, 2000
 *   Copyright (C) 2000, Mycal Labs www.mycal.com
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
#include "ppp_arch.h"
#include "ipcp.h"
#include "ppp.h"
#include "ahdlc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if PPP_DEBUG
#  define DEBUG1(x) debug_printf x
#else
#  define DEBUG1(x)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* In the future add compression protocol and name servers (possibly for servers
 * only)
 */

static const uint8_t g_ipcplist[] =
{
  IPCP_IPADDRESS,
  0
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: printip
 ****************************************************************************/

#if PPP_DEBUG
void printip(struct in_addr ip2)
{
  char *ip = (FAR uint8_t *) & ip2.s_addr;
  DEBUG1((" %d.%d.%d.%d ", ip[0], ip[1], ip[2], ip[3]));
}
#else
#  define printip(x)
#endif

/****************************************************************************
 * Name: ipcp_init
 ****************************************************************************/

void ipcp_init(FAR struct ppp_context_s *ctx)
{
  DEBUG1(("ipcp init\n"));

  ctx->ipcp_state = 0;
  ctx->ipcp_retry = 0;
  ctx->ipcp_prev_seconds = 0;

  memset(&ctx->local_ip, 0, sizeof(struct in_addr));
#ifdef IPCP_GET_PEER_IP
  memset(&ctx->peer_ip, 0, sizeof(struct in_addr));
#endif
#ifdef IPCP_GET_PRI_DNS
  memset(&ctx->pri_dns_addr, 0, sizeof(struct in_addr));
#endif
#ifdef IPCP_GET_SEC_DNS
  memset(&ctx->sec_dns_addr, 0, sizeof(struct in_addr));
#endif
}

/****************************************************************************
 * Name: ipcp_rx
 *
 * Description:
 *   IPCP RX protocol Handler
 *
 ****************************************************************************/

void ipcp_rx(FAR struct ppp_context_s *ctx, FAR uint8_t * buffer,
             uint16_t count)
{
  FAR uint8_t *bptr = buffer;
  uint16_t len;

  DEBUG1(("IPCP len %d\n", count));

  switch (*bptr++)
    {
    case CONF_REQ:
      /* Parse request and see if we can ACK it */

      ++bptr;
      len = (*bptr++ << 8);
      len |= *bptr++;

      /* len-=2; */

      DEBUG1(("check lcplist\n"));
      if (scan_packet(ctx, IPCP, g_ipcplist, buffer, bptr, (uint16_t)(len - 4)))
        {
          DEBUG1(("option was bad\n"));
        }
      else
        {
          DEBUG1(("IPCP options are good\n"));

          /* Parse out the results */

          /* lets try to implement what peer wants */

          /* Reject any protocol not */

          /* Error? if we we need to send a config Reject ++++ this is good for
           * a subroutine.
           */

           /* All we should get is the peer IP address */

          if (IPCP_IPADDRESS == *bptr++)
            {
              /* Dump length */

              ++bptr;
#ifdef IPCP_GET_PEER_IP
              ((FAR uint8_t *) & ctx->peer_ip)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->peer_ip)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->peer_ip)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->peer_ip)[3] = *bptr++;

              DEBUG1(("Peer IP "));
              printip(ctx->peer_ip);
              DEBUG1(("\n"));

              netlib_set_dripv4addr((char *)ctx->ifname, &ctx->peer_ip);
#else
              bptr += 4;
#endif
            }
          else
            {
              DEBUG1(("HMMMM this shouldn't happen IPCP1\n"));
            }

#if 0
          if (error)
            {
              /* Write the config NAK packet we've built above, take on the
               * header
               */

              bptr = buffer;
              *bptr++ = CONF_NAK;       /* Write Conf_rej */
              *bptr++;

              /* tptr++; *//* skip over ID */

              /* Write new length */

              *bptr++ = 0;
              *bptr = tptr - buffer;

              /* Write the reject frame */

              DEBUG1(("Writing NAK frame \n"));
              ahdlc_tx(IPCP, buffer, (uint16_t)(tptr - buffer));
              DEBUG1(("- End NAK Write frame\n"));
            }
          else
            {
            }
#endif
          /* If we get here then we are OK, lets send an ACK and tell the rest
           * of our modules our negotiated config.
           */

          ctx->ipcp_state |= IPCP_RX_UP;
          DEBUG1(("Send IPCP ACK!\n"));
          bptr = buffer;
          *bptr++ = CONF_ACK;   /* Write Conf_ACK */
          bptr++;               /* Skip ID (send same one) */

          /* Set stuff */

          /* ppp_flags |= tflag; */

          DEBUG1(("SET- stuff -- are we up? c=%d dif=%d \n",
                  count, (uint16_t)(bptr - buffer)));

          /* Write the ACK frame */

          DEBUG1(("Writing ACK frame \n"));

          /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

          ahdlc_tx(ctx, IPCP, 0, buffer, 0, count /* bptr-buffer */);
          DEBUG1(("- End ACK Write frame\n"));
        }
      break;

    case CONF_ACK:             /* config Ack */
      DEBUG1(("CONF ACK\n"));

      /* Parse out the results Dump the ID and get the length. */

      /* Dump the ID */

      bptr++;

      /* Get the length */

      len = (*bptr++ << 8);
      len |= *bptr++;

#if 0
      /* Parse ACK and set data */

      while (bptr < buffer + len)
        {
          switch (*bptr++)
            {
            case IPCP_IPADDRESS:
              /* Dump length */

              bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[3] = *bptr++;
              break;

#  ifdef IPCP_GET_PRI_DNS
            case IPCP_PRIMARY_DNS:
              bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[3] = *bptr++;
              break;
#  endif

#  ifdef IPCP_GET_SEC_DNS
            case IPCP_SECONDARY_DNS:
              bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[3] = *bptr++;
              break;
#  endif

            default:
              DEBUG1(("IPCP CONFIG_ACK problem1\n"));
            }
        }
#endif

      ctx->ipcp_state |= IPCP_TX_UP;

      /* ppp_ipcp_state &= ~IPCP_RX_UP; */

      DEBUG1(("were up! \n"));
      printip(ctx->local_ip);
#ifdef IPCP_GET_PRI_DNS
      printip(ctx->pri_dns_addr);
#endif
#ifdef IPCP_GET_SEC_DNS
      printip(ctx->sec_dns_addr);
#endif
      DEBUG1(("\n"));
      break;

    case CONF_NAK:             /* Config Nack */
      DEBUG1(("CONF NAK\n"));

      /* Dump the ID */

      bptr++;

      /* Get the length */

      len = (*bptr++ << 8);
      len |= *bptr++;

      /* Parse ACK and set data */

      while (bptr < buffer + len)
        {
          switch (*bptr++)
            {
            case IPCP_IPADDRESS:
              /* Dump length */

              bptr++;

              ((FAR uint8_t *) & ctx->local_ip)[0] = (char)*bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[1] = (char)*bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[2] = (char)*bptr++;
              ((FAR uint8_t *) & ctx->local_ip)[3] = (char)*bptr++;

              netlib_ifup((char *)ctx->ifname);
              netlib_set_ipv4addr((char *)ctx->ifname, &ctx->local_ip);
              break;

#ifdef IPCP_GET_PRI_DNS
            case IPCP_PRIMARY_DNS:
              bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->pri_dns_addr)[3] = *bptr++;
              netlib_set_ipv4dnsaddr(&ctx->pri_dns_addr);
              break;
#endif

#ifdef IPCP_GET_SEC_DNS
            case IPCP_SECONDARY_DNS:
              bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[0] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[1] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[2] = *bptr++;
              ((FAR uint8_t *) & ctx->sec_dns_addr)[3] = *bptr++;
              netlib_set_ipv4dnsaddr(&ctx->sec_dns_addr);
              break;
#endif

            default:
              DEBUG1(("IPCP CONFIG_ACK problem 2\n"));
            }
        }

      ctx->ppp_id++;

      printip(ctx->local_ip);
#ifdef IPCP_GET_PRI_DNS
      printip(ctx->pri_dns_addr);
#endif
#ifdef IPCP_GET_PRI_DNS
      printip(ctx->sec_dns_addr);
#endif
      DEBUG1(("\n"));
      break;

    case CONF_REJ:             /* Config Reject */
      DEBUG1(("CONF REJ\n"));

      /* Remove the offending options */

      ctx->ppp_id++;

      /* Dump the ID */

      bptr++;

      /* Get the length */

      len = (*bptr++ << 8);
      len |= *bptr++;

      /* Parse ACK and set data */

      while (bptr < buffer + len)
        {
          switch (*bptr++)
            {
            case IPCP_IPADDRESS:
              ctx->ipcp_state |= IPCP_IP_BIT;
              bptr += 5;
              break;

#ifdef IPCP_GET_PRI_DNS
            case IPCP_PRIMARY_DNS:
              ctx->ipcp_state |= IPCP_PRI_DNS_BIT;
              bptr += 5;
              break;
#endif

#ifdef IPCP_GET_PRI_DNS
            case IPCP_SECONDARY_DNS:
              ctx->ipcp_state |= IPCP_SEC_DNS_BIT;
              bptr += 5;
              break;
#endif

            default:
              DEBUG1(("IPCP this shouldn't happen 3\n"));
            }
        }
      break;

    default:
      DEBUG1(("-Unknown 4\n"));
    }
}

/****************************************************************************
 * Name: ipcp_task
 ****************************************************************************/

void ipcp_task(FAR struct ppp_context_s *ctx, FAR uint8_t * buffer)
{
  FAR uint8_t *bptr;
  uint16_t t;
  IPCPPKT *pkt;

  /* IPCP tx not up and hasn't timed out then lets see if we need to send a
   * request
   */

  if (!(ctx->ipcp_state & IPCP_TX_UP) && !(ctx->ipcp_state & IPCP_TX_TIMEOUT))
    {
      /* Check if we have a request pending */

      if ((ppp_arch_clock_seconds() - ctx->ipcp_prev_seconds) > IPCP_TIMEOUT)
        {
          ctx->ipcp_prev_seconds = ppp_arch_clock_seconds();

          /* No pending request, lets build one */

          pkt = (IPCPPKT *) buffer;

          /* Configure-Request only here, write id */

          pkt->code = CONF_REQ;
          pkt->id = ctx->ppp_id;

          bptr = pkt->data;

          /* Write options, we want IP address, and DNS addresses if set.
           * Write zeros for IP address the first time
           */

          *bptr++ = IPCP_IPADDRESS;
          *bptr++ = 0x6;
          *bptr++ = (uint8_t)((FAR uint8_t *) & ctx->local_ip)[0];
          *bptr++ = (uint8_t)((FAR uint8_t *) & ctx->local_ip)[1];
          *bptr++ = (uint8_t)((FAR uint8_t *) & ctx->local_ip)[2];
          *bptr++ = (uint8_t)((FAR uint8_t *) & ctx->local_ip)[3];

#ifdef IPCP_GET_PRI_DNS
          if ((ctx->ipcp_state & IPCP_PRI_DNS_BIT) == 0)
            {
              /* Write zeros for IP address the first time */

              *bptr++ = IPCP_PRIMARY_DNS;
              *bptr++ = 0x6;
              *bptr++ = ((FAR uint8_t *) & ctx->pri_dns_addr)[0];
              *bptr++ = ((FAR uint8_t *) & ctx->pri_dns_addr)[1];
              *bptr++ = ((FAR uint8_t *) & ctx->pri_dns_addr)[2];
              *bptr++ = ((FAR uint8_t *) & ctx->pri_dns_addr)[3];
            }
#endif

#ifdef IPCP_GET_SEC_DNS
          if ((ctx->ipcp_state & IPCP_SEC_DNS_BIT) == 0)
            {
              /* Write zeros for IP address the first time */

              *bptr++ = IPCP_SECONDARY_DNS;
              *bptr++ = 0x6;
              *bptr++ = ((FAR uint8_t *) & ctx->sec_dns_addr)[0];
              *bptr++ = ((FAR uint8_t *) & ctx->sec_dns_addr)[1];
              *bptr++ = ((FAR uint8_t *) & ctx->sec_dns_addr)[2];
              *bptr++ = ((FAR uint8_t *) & ctx->sec_dns_addr)[3];
            }
#endif

          /* Write length */

          t = bptr - buffer;

          /* length here - code and ID + */

          pkt->len = htons(t);

          DEBUG1(("\n**Sending IPCP Request packet\n"));

          /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */

          ahdlc_tx(ctx, IPCP, 0, buffer, 0, t);

          /* Inc retry */

          ctx->ipcp_retry++;

          /* Have we timed out? (combine the timers?) */

          if (ctx->ipcp_retry > IPCP_RETRY_COUNT)
            {
              ctx->ipcp_state |= IPCP_TX_TIMEOUT;
            }
        }
    }
}
