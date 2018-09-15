/****************************************************************************
 * apps/wireless/wapi/src/util.c
 *
 *   Copyright (C) 2011, 2017Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Adapted for Nuttx from WAPI:
 *
 *   Copyright (c) 2010, Volkan YAZICI <volkan.yazici@gmail.com>
 *   All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "wireless/wapi.h"
#include "util.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Size of the command buffer */

#define WAPI_IOCTL_COMMAND_NAMEBUFSIZ 24

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static char g_ioctl_command_namebuf[WAPI_IOCTL_COMMAND_NAMEBUFSIZ];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wapi_make_socket
 *
 * Description:
 *   Creates an AF_INET socket to be used in ioctl() calls.
 *
 * Returned Value:
 *   Non-negative on success.
 *
 ****************************************************************************/

int wapi_make_socket(void)
{
  return socket(PF_INETX, SOCK_WAPI, 0);
}

/****************************************************************************
 * Name: wapi_ioctl_command_name
 *
 * Description:
 *   Return name string for IOCTL command
 *
 * Returned Value:
 *   Name string for IOCTL command
 *
 ****************************************************************************/

FAR const char *wapi_ioctl_command_name(int cmd)
{
  switch (cmd)
    {
    case SIOCADDRT:
      return "SIOCADDRT";

    case SIOCDELRT:
      return "SIOCDELRT";

    case SIOCGIFADDR:
      return "SIOCGIFADDR";

    case SIOCGIWAP:
      return "SIOCGIWAP";

    case SIOCGIWESSID:
      return "SIOCGIWESSID";

    case SIOCGIWFREQ:
      return "SIOCGIWFREQ";

    case SIOCGIWMODE:
      return "SIOCGIWMODE";

    case SIOCGIWRANGE:
      return "SIOCGIWRANGE";

    case SIOCGIWRATE:
      return "SIOCGIWRATE";

    case SIOCGIWSCAN:
      return "SIOCGIWSCAN";

    case SIOCGIWTXPOW:
      return "SIOCGIWTXPOW";

    case SIOCSIFADDR:
      return "SIOCSIFADDR";

    case SIOCSIWAP:
      return "SIOCSIWAP";

    case SIOCSIWESSID:
      return "SIOCSIWESSID";

    case SIOCSIWFREQ:
      return "SIOCSIWFREQ";

    case SIOCSIWMODE:
      return "SIOCSIWMODE";

    case SIOCSIWRATE:
      return "SIOCSIWRATE";

    case SIOCSIWSCAN:
      return "SIOCSIWSCAN";

    case SIOCSIWTXPOW:
      return "SIOCSIWTXPOW";

    default:
      snprintf(g_ioctl_command_namebuf, WAPI_IOCTL_COMMAND_NAMEBUFSIZ,
               "0x%x", cmd);
      return g_ioctl_command_namebuf;
    }
}
