/****************************************************************************
 * apps/wireless/wapi/src/util.h
 *
 *  Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of  source code must  retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of  conditions and the  following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND  FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_WIRELESS_WAPI_SRC_UTIL_H
#define __APPS_WIRELESS_WAPI_SRC_UTIL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WAPI_IOCTL_STRERROR(cmd) \
  fprintf( \
    stderr, "%s:%d:%s():ioctl(%s): %s\n", \
    __FILE__, __LINE__, __func__, \
    wapi_ioctl_command_name(cmd), strerror(errno))

#define WAPI_STRERROR(fmt, ...) \
  fprintf( \
    stderr, "%s:%d:%s():" fmt ": %s\n", \
    __FILE__, __LINE__, __func__, \
    ## __VA_ARGS__, strerror(errno))

#define WAPI_ERROR(fmt, ...) \
  fprintf( \
    stderr, "%s:%d:%s(): " fmt , \
    __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define WAPI_VALIDATE_PTR(ptr) \
  if (!ptr) \
  { \
    WAPI_ERROR("Null pointer: %s.\n", #ptr); \
    return -1; \
  }

/****************************************************************************
 *  Public Function Prototypes
 ****************************************************************************/

FAR const char *wapi_ioctl_command_name(int cmd);

#endif /* __APPS_WIRELESS_WAPI_SRC_UTIL_H */
