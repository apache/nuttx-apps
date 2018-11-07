/****************************************************************************
 * netutils/pppd/lpc.h
 * Link Configuration Protocol header file
 *
 *   Version: .1 Original Version June 3, 2000
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
 *      This product includes software developed by Mike Johnson/Mycal Labs
 *        www.mycal.net.
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

#ifndef __APPS_NETUTILS_PPPD_LCP_H
#define __APPS_NETUTILS_PPPD_LCP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ppp_arch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LCP Option Types */

#define LPC_VENDERX        0x0
#define LPC_MRU            0x1
#define LPC_ACCM           0x2
#define LPC_AUTH           0x3
#define LPC_QUALITY        0x4
#define LPC_MAGICNUMBER    0x5
#define LPC_PFC            0x7
#define LPC_ACFC           0x8

/* LCP state machine flags */

#define LCP_TX_UP          0x1
#define LCP_RX_UP          0x2

#define LCP_RX_AUTH        0x10

/* LCP request for auth */

#define LCP_TERM_PEER      0x20

/* LCP Terminated by peer */

#define LCP_RX_TIMEOUT     0x40
#define LCP_TX_TIMEOUT     0x80

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ppp_context_s;

typedef struct _lcppkt
{
  uint8_t code;
  uint8_t id;
  uint16_t len;
  uint8_t data[0];
} LCPPKT;

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

void lcp_init(FAR struct ppp_context_s *ctx);
void lcp_rx(FAR struct ppp_context_s *ctx, FAR uint8_t *, uint16_t);
void lcp_task(FAR struct ppp_context_s *ctx, FAR uint8_t *buffer);
void lcp_disconnect(FAR struct ppp_context_s *ctx, uint8_t id);
void lcp_echo_request(FAR struct ppp_context_s *ctx);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_LCP_H */
