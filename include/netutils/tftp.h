/****************************************************************************
 * apps/include/netutils/tftp.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
