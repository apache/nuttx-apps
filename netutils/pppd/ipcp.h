/****************************************************************************
 * netutils/pppd/ipcp.h
 * Internet Protocol Control Protocol header file
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

#ifndef __APPS_NETUTILS_PPPD_IPCP_H
#define __APPS_NETUTILS_PPPD_IPCP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ppp_arch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IPCP Option Types */

#define IPCP_IPADDRESS        0x03
#define IPCP_PRIMARY_DNS      0x81
#define IPCP_SECONDARY_DNS    0x83

/* IPCP state machine flags */

#define IPCP_TX_UP            0x01
#define IPCP_RX_UP            0x02
#define IPCP_IP_BIT           0x04
#define IPCP_TX_TIMEOUT       0x08
#define IPCP_PRI_DNS_BIT      0x10
#define IPCP_SEC_DNS_BIT      0x20

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ppp_context_s;

typedef struct  _ipcp
{
  uint8_t  code;
  uint8_t  id;
  uint16_t len;
  uint8_t  data[0];
} IPCPPKT;

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

void ipcp_init(FAR struct ppp_context_s *ctx);
void ipcp_task(FAR struct ppp_context_s *ctx, FAR uint8_t *buffer);
void ipcp_rx(FAR struct ppp_context_s *ctx, FAR uint8_t *, uint16_t);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_IPCP_H */
