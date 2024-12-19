/****************************************************************************
 * apps/include/canutils/lely/config.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef __APPS_INCLUDE_CANUTILS_LELY_CONFIG_H
#define __APPS_INCLUDE_CANUTILS_LELY_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* General NuttX port configuration */

#define _WIN32                 0
#define _WIN64                 0
#define LELY_NO_ERRNO          0
#define LELY_NO_MALLOC         0
#define LELY_NO_THREADS        0
#define LELY_NO_STDIO          0
#define LELY_NO_ATOMICS        0
#define HAVE_SYS_IOCTL_H       1
#define LELY_HAVE_ITIMERSPEC   1
#define LELY_HAVE_SYS_TYPES_H  1
#define LELY_HAVE_STRINGS_H    1
#define LELY_HAVE_UCHAR_H      0
#define _POSIX_C_SOURCE        200112L

/* --disable-daemon */

#define LELY_NO_DAEMON 1

/* --disable-cxx */

#ifdef CONFIG_HAVE_CXX
#  define LELY_NO_CXX 0
#else
#  define LELY_NO_CXX 1
#endif

/* SocketCAN support */

#ifdef CONFIG_NET_CAN
#  define LELY_HAVE_SOCKET_CAN 1
#else
#  define LELY_HAVE_SOCKET_CAN 0
#endif

/* --disable-canfd */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_CANFD
#  define LELY_NO_CANFD 1
#else
#  define LELY_NO_CANFD 0
#endif

/* --disable-diag */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_DIAG
#  define LELY_NO_DIAG 0
#else
#  define LELY_NO_DIAG 1
#endif

/* --disable-dcf */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_DCF
#  define LELY_NO_CO_DCF 0
#else
#  define LELY_NO_CO_DCF 1
#endif

/* --disable-obj-default */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_OBJDEFAULT
#  define LELY_NO_CO_OBJ_DEFAULT 0
#else
#  define LELY_NO_CO_OBJ_DEFAULT 1
#endif

/* --disable-obj-file */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_OBJFILE
#  define LELY_NO_CO_OBJ_FILE 0
#else
#  define LELY_NO_CO_OBJ_FILE 1
#endif

/* --disable-obj-limits */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_OBJLIMITS
#  define LELY_NO_CO_OBJ_LIMITS 0
#else
#  define LELY_NO_CO_OBJ_LIMITS 1
#endif

/* --disable-obj-name */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_OBJNAME
#  define LELY_NO_CO_OBJ_NAME 0
#else
#  define LELY_NO_CO_OBJ_NAME 1
#endif

/* --disable-obj-upload */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_OBJUPLOAD
#  define LELY_NO_CO_OBJ_UPLOAD 0
#else
#  define LELY_NO_CO_OBJ_UPLOAD 1
#endif

/* --disable-sdev */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_SDEV
#  define LELY_NO_CO_SDEV 0
#else
#  define LELY_NO_CO_SDEV 1
#endif

/* --disable-csdo */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_CSDO
#  define LELY_NO_CO_CSDO 0
#else
#  define LELY_NO_CO_CSDO 1
#endif

/* --disable-rpdo */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_RPDO
#  define LELY_NO_CO_RPDO 0
#else
#  define LELY_NO_CO_RPDO 1
#endif

/* --disable-tpdo */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_TPDO
#  define LELY_NO_CO_TPDO 0
#else
#  define LELY_NO_CO_TPDO 1
#endif

/* --disable-mpdo */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_MPDO
#  define LELY_NO_CO_MPDO 0
#else
#  define LELY_NO_CO_MPDO 1
#endif

/* --disable-sync */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_SYNC
#  define LELY_NO_CO_SYNC 0
#else
#  define LELY_NO_CO_SYNC 1
#endif

/* --disable-time */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_TIME
#  define LELY_NO_CO_TIME 0
#else
#  define LELY_NO_CO_TIME 1
#endif

/* --disable-emcy */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_EMCY
#  define LELY_NO_CO_EMCY 0
#else
#  define LELY_NO_CO_EMCY 1
#endif

/* --disable-lss */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_LSS
#  define LELY_NO_CO_LSS 0
#else
#  define LELY_NO_CO_LSS 1
#endif

/* --disable-wtm */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_WTM
#  define LELY_NO_CO_WTM 0
#else
#  define LELY_NO_CO_WTM 1
#endif

/* --disable-master */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_MASTER
#  define LELY_NO_CO_MASTER 0
#else
#  define LELY_NO_CO_MASTER 1
#endif

/* --disable-ng */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_NG
#  define LELY_NO_CO_NG 0
#else
#  define LELY_NO_CO_NG 1
#endif

/* --disable-nmt-boot */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_NMTBOOT
#  define LELY_NO_CO_NMT_BOOT 0
#else
#  define LELY_NO_CO_NMT_BOOT 1
#endif

/* --disable-nmt-cfg */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_NMTCFG
#  define LELY_NO_CO_NMT_CFG 0
#else
#  define LELY_NO_CO_NMT_CFG 1
#endif

/* --disable-gw */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_GW
#  define LELY_NO_CO_GW 0
#else
#  define LELY_NO_CO_GW 1
#endif

/* --disable-gw-txt */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_GW_TXT
#  define LELY_NO_CO_GW_TXT 0
#else
#  define LELY_NO_CO_GW_TXT 1
#endif

/* --disable-coapp-master */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_COAPP_MASTER
#  define LELY_NO_CO_COAPP_MASTER 0
#else
#  define LELY_NO_CO_COAPP_MASTER 1
#endif

/* --disable-coapp-slave */

#ifdef CONFIG_CANUTILS_LELYCANOPEN_COAPP_SLAVE
#  define LELY_NO_CO_COAPP_SLAVE 0
#else
#  define LELY_NO_CO_COAPP_SLAVE 1
#endif

#endif  /* __APPS_INCLUDE_CANUTILS_LELY_CONFIG_H */
