/****************************************************************************
 * apps/system/zmodem/host/nuttx/config.h
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
