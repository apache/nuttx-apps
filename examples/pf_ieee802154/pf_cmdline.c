/****************************************************************************
 * examples/pf_ieee802154/pf_cmdline.c
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netpacket/ieee802154.h>

#include "netutils/netlib.h"
#include "pfieee802154.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct ieee802154_saddr_s g_server_addr;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "\t%s [-p <panid>] <short-addr>|<extended-addr>\n",
          progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<panid> is the PAN ID of the form: xx:xx\n");
  fprintf(stderr, "\t<short-addr> is a short address of the form xx:xx\n");
  fprintf(stderr, "\t<extended-addr>is an extended address of the form\n");
  fprintf(stderr, "\t\txx:xx:xx:xx:xx:xx:xx:xx\n\n");
  fprintf(stderr, "NOTES:\n");
  fprintf(stderr, "\txx is any 2-character hexadecimal string.\n");
  exit(1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * pf_cmdline
 ****************************************************************************/

void pf_cmdline(int argc, char **argv)
{
  int ndx = 1;

  /* Forms:
   *
   * argc=1: progname
   * argc=2: progname <address>
   * argc=4: progname -p <panid> <address>
   */

  if (argc == 4)
    {
      if (strcmp(argv[ndx],  "-p") == 0)
        {
          ndx++;
          if (!netlib_saddrconv(argv[ndx], g_server_addr.s_panid))
            {
              fprintf(stderr, "ERROR: PAN ID is invalid\n");
              show_usage(argv[0]);
            }

          ndx++;
          argc = 2;
        }
      else
        {
          fprintf(stderr, "ERROR: Unrecognized options: %s\n", argv[ndx]);
          show_usage(argv[0]);
        }
    }

  if (argc == 2)
    {
      if (netlib_saddrconv(argv[ndx], g_server_addr.s_saddr))
        {
          g_server_addr.s_mode = IEEE802154_ADDRMODE_SHORT;
        }
      else if (netlib_eaddrconv(argv[ndx], g_server_addr.s_eaddr))
        {
          g_server_addr.s_mode = IEEE802154_ADDRMODE_EXTENDED;
        }
      else
        {
          fprintf(stderr, "ERROR: Server address is invalid\n");
          show_usage(argv[0]);
        }
    }
  else if (argc != 1)
    {
      fprintf(stderr, "ERROR: Unexpected number of arguments\n");
      show_usage(argv[0]);
    }

  if (g_server_addr.s_mode == IEEE802154_ADDRMODE_NONE)
    {
      fprintf(stderr, "ERROR: No server address\n");
      show_usage(argv[0]);
    }
}
