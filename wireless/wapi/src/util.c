/****************************************************************************
 * apps/wireless/wapi/src/util.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include "include/wireless/wapi.h"
#include "util.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WAPI_IOCTL_COMMAND_NAMEBUFSIZ 128      /* Is fairly enough to print an
                                                * integer. */

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
  return socket(AF_INET, SOCK_DGRAM, 0);
}

/****************************************************************************
 * Name: wapi_get_ifnames
 *
 * Description:
 *   Parses WAPI_PROC_NET_WIRELESS.
 *
 * Returned Value:
 *   list Pushes collected  wapi_string_t into this list.
 *
 ****************************************************************************/

int wapi_get_ifnames(FAR wapi_list_t *list)
{
  FILE *fp;
  int ret;
  size_t tmpsize = WAPI_PROC_LINE_SIZE * sizeof(char);
  char tmp[WAPI_PROC_LINE_SIZE];

  WAPI_VALIDATE_PTR(list);

  /* Open file for reading. */

  fp = fopen(WAPI_PROC_NET_WIRELESS, "r");
  if (!fp)
    {
      WAPI_STRERROR("fopen(\"%s\", \"r\")", WAPI_PROC_NET_WIRELESS);
      return -1;
    }

  /* Skip first two lines. */

  if (!fgets(tmp, tmpsize, fp) || !fgets(tmp, tmpsize, fp))
    {
      WAPI_ERROR("Invalid \"%s\" content!\n", WAPI_PROC_NET_WIRELESS);
      return -1;
    }

  /* Iterate over available lines. */

  ret = 0;
  while (fgets(tmp, tmpsize, fp))
    {
      char *beg;
      char *end;
      wapi_string_t *string;

      /* Locate the interface name region. */

      for (beg = tmp; *beg && isspace(*beg); beg++);
      for (end = beg; *end && *end != ':'; end++);

      /* Allocate both wapi_string_t and char vector. */

      string = malloc(sizeof(wapi_string_t));
      if (string)
        {
          string->data = malloc(end - beg + sizeof(char));
        }

      if (!string || !string->data)
        {
          WAPI_STRERROR("malloc()");
          ret = -1;
          break;
        }

      /* Copy region into the buffer. */

      snprintf(string->data, (end - beg + sizeof(char)), "%s", beg);

      /* Push string into the list. */

      string->next = list->head.string;
      list->head.string = string;
    }

  fclose(fp);
  return ret;
}

/****************************************************************************
 * Name: wapi_get_ifnames
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
