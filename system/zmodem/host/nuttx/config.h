/****************************************************************************
 * include/nuttx/compiler.h
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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

#ifndef __APPS_SYSTEM_ZMODEM_HOST_NUTTX_CONFIG_H
#define __APPS_SYSTEM_ZMODEM_HOST_NUTTX_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Environment stuff */

#define OK 0
#define ERROR -1
#define FAR
#define DEBUGASSERT assert

#define _GNU_SOURCE 1

/* Configuration */

#define CONFIG_SYSTEM_ZMODEM 1
#define CONFIG_SYSTEM_ZMODEM_DEVNAME "/dev/ttyS0"
#define CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE 512
#define CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE 1024
#define CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE 512
#define CONFIG_SYSTEM_ZMODEM_MOUNTPOINT "/tmp"
#undef  CONFIG_SYSTEM_ZMODEM_RCVSAMPLE
#undef  CONFIG_SYSTEM_ZMODEM_SENDATTN
#define CONFIG_SYSTEM_ZMODEM_ALWAYSSINT 1
#undef  CONFIG_SYSTEM_ZMODEM_SENDBRAK
#define CONFIG_SYSTEM_ZMODEM_RESPTIME 10
#define CONFIG_SYSTEM_ZMODEM_CONNTIME 30
#define CONFIG_SYSTEM_ZMODEM_SERIALNO 1
#define CONFIG_SYSTEM_ZMODEM_MAXERRORS 20
#define CONFIG_SYSTEM_ZMODEM_WRITESIZE 0
#define CONFIG_SYSTEM_ZMODEM_MOUNTPOINT "/tmp"

/* Cannot control pre-emption from Linux (don't need to) */

#define sched_lock()
#define sched_unlock()

/****************************************************************************
 * Inline Functions
 ****************************************************************************/

static inline void *zalloc(unsigned long size)
{
  void *ret = malloc(size);
  if (ret)
    {
      memset(ret, 0, size);
    }
  return ret;
}

#endif /* __APPS_SYSTEM_ZMODEM_HOST_NUTTX_CONFIG_H */
