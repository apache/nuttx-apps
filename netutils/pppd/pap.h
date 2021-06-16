/****************************************************************************
 * apps/netutils/pppd/pap.h
 * PAP header file
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifndef __APPS_NETUTILS_PPPD_PAP_H
#define __APPS_NETUTILS_PPPD_PAP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "ppp_arch.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PAP state machine flags */
/* Client only */

#define PAP_TX_UP         0x01

/* Server only */

#define PAP_RX_UP         0x02

#define PAP_RX_AUTH_FAIL  0x10
#define PAP_TX_AUTH_FAIL  0x20
#define PAP_RX_TIMEOUT    0x40
#define PAP_TX_TIMEOUT    0x80

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct ppp_context_s;

typedef struct _pappkt
{
  uint8_t code;
  uint8_t id;
  uint16_t len;
  uint8_t data[0];
} PAPPKT;

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

/* Function prototypes */

void pap_init(FAR struct ppp_context_s *ctx);
void pap_rx(FAR struct ppp_context_s *ctx, FAR uint8_t *, uint16_t);
void pap_task(FAR struct ppp_context_s *ctx, FAR uint8_t *buffer);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_NETUTILS_PPPD_PAP_H */
