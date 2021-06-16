/****************************************************************************
 * apps/examples/pf_ieee802154/pf_cmdline.c
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
