/****************************************************************************
 * netutils/pppd/ppp.h
 * PPP header file
 *
 *   Version: 0.1 Original Version June 3, 2000
 *   (c)2000 Mycal Labs, All Rights Reserved
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

#ifndef __APPS_NETUTILS_PPPD_PPP_H
#define __APPS_NETUTILS_PPPD_PPP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "netutils/chat.h"

#include "ppp_conf.h"
#include "ahdlc.h"
#include "lcp.h"
#include "ipcp.h"
#include "ppp_arch.h"

#ifdef CONFIG_NETUTILS_PPPD_PAP
#  include "pap.h"
#endif /* CONFIG_NETUTILS_PPPD_PAP */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CRC_GOOD_VALUE      0xf0b8

/* ppp_rx_status values */

#define PPP_RX_IDLE         0
#define PPP_READY           1

/* ppp flags */

#define PPP_ESCAPED         0x1
#define PPP_RX_READY        0x2
#define PPP_RX_ASYNC_MAP    0x8
#define PPP_TX_ASYNC_MAP    0x8
#define PPP_PFC             0x10
#define PPP_ACFC            0x20

/* Supported PPP Protocols */

#define LCP                 0xc021
#define PAP                 0xc023
#define IPCP                0x8021
#define IPV4                0x0021

/* LCP codes packet types */

#define CONF_REQ            0x1
#define CONF_ACK            0x2
#define CONF_NAK            0x3
#define CONF_REJ            0x4
#define TERM_REQ            0x5
#define TERM_ACK            0x6
#define PROT_REJ            0x8
#define ECHO_REQ            0x9
#define ECHO_REP            0xa

/* Raise PPP config bits */

#define USE_PAP             0x1
#define USE_NOACCMBUG       0x2
#define USE_GETDNS          0x4

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* PPP context definition */

struct ppp_context_s
{
  /* IP Buffer */

  u8_t  ip_buf[PPP_RX_BUFFER_SIZE];
  u16_t ip_len;

  /* Main status */

  u8_t  ppp_flags;
  u8_t  ppp_status;
  u16_t ppp_tx_mru;
  u8_t  ppp_id;

  /* IP timeout */

  u16_t ip_no_data_time;

  /* Interfaces */

  int   if_fd;
  u8_t  ifname[IFNAMSIZ];

  /* Addresses */

  struct in_addr local_ip;
#ifdef IPCP_GET_PEER_IP
  struct in_addr peer_ip;
#endif
#ifdef IPCP_GET_PRI_DNS
  struct in_addr pri_dns_addr;
#endif
#ifdef IPCP_GET_SEC_DNS
  struct in_addr sec_dns_addr;
#endif

  /* LCP */

  u8_t   lcp_state;
  u16_t  lcp_tx_mru;
  u8_t   lcp_retry;
  time_t lcp_prev_seconds;

#ifdef CONFIG_NETUTILS_PPPD_PAP
  /* PAP */

  u8_t   pap_state;
  u8_t   pap_retry;
  time_t pap_prev_seconds;
#endif /* CONFIG_NETUTILS_PPPD_PAP */

  /* IPCP */

  u8_t   ipcp_state;
  u8_t   ipcp_retry;
  time_t ipcp_prev_seconds;

  /* AHDLC */

  u8_t  ahdlc_rx_buffer[PPP_RX_BUFFER_SIZE];
  u16_t ahdlc_tx_crc;     /* Running tx CRC */
  u16_t ahdlc_rx_crc;     /* Running rx CRC */
  u16_t ahdlc_rx_count;   /* Number of rx bytes processed, cur frame */
  u8_t  ahdlc_flags;      /* ahdlc state flags, see above */
  u8_t  ahdlc_tx_offline;

  /* Statistics counters */

#ifdef PPP_STATISTICS
  u16_t ahdlc_crc_error;
  u16_t ahdlc_rx_tobig_error;
  u32_t ppp_rx_frame_count;
  u32_t ppp_tx_frame_count;
#endif

  /* Chat controls */

  struct chat_ctl ctl;

  /* PPPD Settings */

  struct pppd_settings_s *settings;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/*
 * Function Prototypes
 */
void ppp_init(struct ppp_context_s *ctx);
void ppp_connect(struct ppp_context_s *ctx);

extern void ppp_reconnect(struct ppp_context_s *ctx);

void ppp_send(struct ppp_context_s *ctx);
void ppp_poll(struct ppp_context_s *ctx);

void ppp_upcall(struct ppp_context_s *ctx, u16_t, u8_t *, u16_t);
u16_t scan_packet(struct ppp_context_s *ctx, u16_t, const u8_t *list,
                  u8_t *buffer, u8_t *options, u16_t len);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_PPP_H */
