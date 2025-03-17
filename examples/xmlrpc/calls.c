/****************************************************************************
 * apps/examples/xmlrpc/calls.c
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2012 Max Holtzberg. All rights reserved.
 * SPDX-FileCopyrightText: 2002 Cogito LLC.  All rights reserved.
 * SPDX-FileContributor: Max Holtzberg <mh@uvc.de>
 *
 *  Redistribution and use in source and binary forms, with or
 *  without modification, is hereby granted without fee provided
 *  that the following conditions are met:
 *
 *    1.  Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer.
 *    2.  Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the
 *        following disclaimer in the documentation and/or other
 *        materials provided with the distribution.
 *    3.  Neither the name of Cogito LLC nor the names of its
 *        contributors may be used to endorse or promote products
 *        derived from this software without specific prior
 *        written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY COGITO LLC AND CONTRIBUTORS 'AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COGITO LLC
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARAY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/

/* Based on the embeddable lightweight XML-RPC server code discussed
 * in the article at: http://www.drdobbs.com/web-development/\
 *    an-embeddable-lightweight-xml-rpc-server/184405364
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include "netutils/xmlrpc.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int calls_get_device_stats(struct xmlrpc_s *xmlcall);

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct xmlrpc_entry_s get_device_stats =
{
  .name = "get_device_stats",
  .func = calls_get_device_stats
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int calls_get_device_stats(struct xmlrpc_s *xmlcall)
{
  char username[CONFIG_XMLRPC_STRINGSIZE + 1];
  char password[CONFIG_XMLRPC_STRINGSIZE + 1];
  char lastcmd[CONFIG_XMLRPC_STRINGSIZE + 1];
  char curstate[CONFIG_XMLRPC_STRINGSIZE + 1];
  int request = 0;
  int status;
  int ret;

  do
    {
      ret = xmlrpc_getstring(xmlcall, username);
      if (ret != XMLRPC_NO_ERROR)
        {
          break;
        }

      ret = xmlrpc_getstring(xmlcall, password);
      if (ret != XMLRPC_NO_ERROR)
        {
          break;
        }

      ret = xmlrpc_getinteger(xmlcall, &request);
      if (ret != XMLRPC_NO_ERROR)
        {
          break;
        }
    }
  while (0);

  if (ret == XMLRPC_NO_ERROR)
    {
      /* Dummy up some data... */

      status = 1;
      strlcpy(lastcmd, "reboot", sizeof(lastcmd));
      strlcpy(curstate, "Normal Operation", sizeof(curstate));

      ret = xmlrpc_buildresponse(xmlcall, "{iss}",
                                 "status", status,
                                 "lastCommand", lastcmd,
                                 "currentState", curstate);
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void calls_register(void)
{
  xmlrpc_register(&get_device_stats);
}
