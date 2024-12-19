/****************************************************************************
 * apps/include/netutils/smtp.h
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2007, 2009, 2011, 2015 Gregory Nutt.
 * SPDX-FileCopyrightText: 2002 Adam Dunkels.
 * SPDX-FileContributor: Gregory Nutt <gnutt@nuttx.org>
 * SPDX-FileContributor: Adam Dunkels <adam@sics.se>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
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

#ifndef __APPS_INCLUDE_NETUTILS_SMTP_H
#define __APPS_INCLUDE_NETUTILS_SMTP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/net/netconfig.h>
#include <nuttx/net/ip.h>

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void *smtp_open(void);
void  smtp_configure(FAR void *handle, FAR const char *hostname,
                     FAR const in_addr_t *paddr, FAR const in_port_t *port);
int   smtp_send(FAR void *handle, FAR const char *to, FAR const char *cc,
                FAR const char *from, FAR const char *subject,
                FAR const char *msg, int msglen);
void  smtp_close(FAR void *handle);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_SMTP_H */
