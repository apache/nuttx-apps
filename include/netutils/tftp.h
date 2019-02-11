/****************************************************************************
 *  apps/include/netutils/tftp.h
 *
 *   Copyright (C) 2008-2009, 2011, 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *   Copyright (C) 2018 Sebastien Lorquet. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_NETUTILS_TFTP_H
#define __APPS_INCLUDE_NETUTILS_TFTP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <arpa/inet.h>

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: tftp_callback_t
 *
 * Description:  This callback type is used for data exchange with the tftp
 *   protocol handler.
 *
 * Input Parameters:
 *   ctx    - pointer passed to the get or put TFTP function
 *   offset - GET: Always zero
 *            PUT: Data offset within the transmitted file
 *   buf    - GET: Pointer to the received data
 *            PUT: Location of data buffer that will be transferred
 *   len    - GET: Size of the received data (usually 512)
 *            PUT: Size of the provided buffer
 * Return value:
 *   GET: Number of bytes that were written to the destination by the user
 *   PUT: Number of bytes that were retrieved from the user data source
 *
 ****************************************************************************/

typedef ssize_t (*tftp_callback_t)(FAR void *ctx, uint32_t offset,
                                   FAR uint8_t *buf, size_t len);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

int tftpget_cb(FAR const char *remote, in_addr_t addr, bool binary,
               tftp_callback_t cb, FAR void *ctx);
int tftpput_cb(FAR const char *remote, in_addr_t addr, bool binary,
               tftp_callback_t cb, FAR void *ctx);
int tftpget(FAR const char *remote, FAR const char *local, in_addr_t addr,
            bool binary);
int tftpput(FAR const char *local, FAR const char *remote, in_addr_t addr,
            bool binary);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_TFTP_H */
