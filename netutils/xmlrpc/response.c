/****************************************************************************
 * apps/netutils/xmlrpc/response.c
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

/****************************************************************************
 * Based on the embeddable lightweight XML-RPC server code discussed
 * in the article at: http://www.drdobbs.com/web-development/\
 *    an-embeddable-lightweight-xml-rpc-server/184405364
 ****************************************************************************/

/*  Lightweight Embedded XML-RPC Server Response Generator
 *
 *  mtj@cogitollc.com
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "netutils/xmlrpc.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int xmlrpc_insertlength(struct xmlrpc_s *xmlcall)
{
  int xdigit = 1000;
  int i = 0;
  int len;
  int digit;
  char *temp;

  temp = strstr(xmlcall->response, "<?xml");
  len = strlen(temp);

  temp = strstr(xmlcall->response, "xyza");

  do
    {
      digit = (len / xdigit);
      len -= (digit * xdigit);
      xdigit /= 10;

      if ((digit == 0) && (xdigit > 1))
        temp[i++] = ' ';
      else
        temp[i++] = (0x30 + digit);
    }
  while (i < 4);

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int xmlrpc_getinteger(struct xmlrpc_s *xmlcall, int *arg)
{
  if ((xmlcall == NULL) || (arg == NULL))
    {
      return XMLRPC_INTERNAL_ERROR;
    }

  if ((xmlcall->arg < xmlcall->argsize) &&
      (xmlcall->args[xmlcall->arg] == 'i'))
    {
      *arg = xmlcall->arguments[xmlcall->arg++].u.i;
      return 0;
    }

  return XMLRPC_UNEXPECTED_INTEGER_ARG;
}

int xmlrpc_getbool(struct xmlrpc_s *xmlcall, int *arg)
{
  if ((xmlcall == NULL) || (arg == NULL))
    {
      return XMLRPC_INTERNAL_ERROR;
    }

  if ((xmlcall->arg < xmlcall->argsize) &&
      (xmlcall->args[xmlcall->arg] == 'b'))
    {
      *arg = xmlcall->arguments[xmlcall->arg++].u.i;
      return 0;
    }

  return XMLRPC_UNEXPECTED_BOOLEAN_ARG;
}

int xmlrpc_getdouble(struct xmlrpc_s *xmlcall, double *arg)
{
  if ((xmlcall == NULL) || (arg == NULL))
    {
      return XMLRPC_INTERNAL_ERROR;
    }

  if ((xmlcall->arg < xmlcall->argsize) &&
      (xmlcall->args[xmlcall->arg] == 'd'))
    {
      *arg = xmlcall->arguments[xmlcall->arg++].u.d;
      return 0;
    }

  return XMLRPC_UNEXPECTED_DOUBLE_ARG;
}

int xmlrpc_getstring(struct xmlrpc_s *xmlcall, char *arg)
{
  if ((xmlcall == NULL) || (arg == NULL))
    {
      return XMLRPC_INTERNAL_ERROR;
    }

  if ((xmlcall->arg < xmlcall->argsize) &&
      (xmlcall->args[xmlcall->arg] == 's'))
    {
      strcpy(arg, xmlcall->arguments[xmlcall->arg++].u.string);
      return 0;
    }

  return XMLRPC_UNEXPECTED_STRING_ARG;
}

int xmlrpc_buildresponse(struct xmlrpc_s *xmlcall, char *args, ...)
{
  va_list argp;
  int next = 0;
  int index = 0;
  int close = 0;
  int isstruct = 0;
  int i;
  double d;
  char *s;

  if ((xmlcall == NULL) || (args == NULL))
    {
      return -1;
    }

  strlcpy(xmlcall->response, "HTTP/1.1 200 OK\n"
          "Connection: close\n"
          "Content-length: xyza\n"
          "Content-Type: text/xml\n"
          "Server: Lightweight XMLRPC\n\n"
          "<?xml version=\"1.0\"?>\n" "<methodResponse>\n",
          sizeof(xmlcall->response));

  if (xmlcall->error)
    {
      strlcat(xmlcall->response, "  <fault>\n",
              sizeof(xmlcall->response));
    }
  else
    {
      strlcat(xmlcall->response, "  <params><param>\n",
              sizeof(xmlcall->response));
    }

  next = strlen(xmlcall->response);
  va_start(argp, args);

  while (args[index])
    {
      if (isstruct)
        {
          if ((args[index] != '{') && (args[index] != '}'))
            {
              snprintf(&xmlcall->response[next],
                       sizeof(xmlcall->response) - next,
                       "  <member>\n");
              next += strlen(&xmlcall->response[next]);
              snprintf(&xmlcall->response[next],
                       sizeof(xmlcall->response) - next,
                       "    <name>%s</name>\n",
                       va_arg(argp, char *));
              next += strlen(&xmlcall->response[next]);
              close = 1;
            }
        }

      switch (args[index])
        {
        case '{':
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "  <value><struct>\n");
          isstruct = 1;
          break;

        case '}':
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "  </struct></value>\n");
          isstruct = 0;
          break;

        case 'i':
          i = va_arg(argp, int);
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "    <value><int>%d</int></value>\r\n", i);
          break;

        case 'b':
          i = va_arg(argp, int);
          snprintf(&xmlcall->response[next],
                  sizeof(xmlcall->response) - next,
                  "    <value><boolean>%d</boolean></value>\r\n", i);
          break;

        case 'd':
          d = va_arg(argp, double);
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "    <value><double>%f</double></value>\r\n", d);
          break;

        case 's':
          s = va_arg(argp, char *);
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "    <value><string>%s</string></value>\r\n", s);
          break;

        default:
          return (XMLRPC_BAD_RESPONSE_ARG);
          break;
        }

      next += strlen(&xmlcall->response[next]);
      if (close)
        {
          snprintf(&xmlcall->response[next],
                   sizeof(xmlcall->response) - next,
                   "  </member>\n");
          next += strlen(&xmlcall->response[next]);
          close = 0;
        }

      index++;
    }

  va_end(argp);

  if (xmlcall->error)
    {
      strlcat(xmlcall->response, "  </fault>\r\n",
              sizeof(xmlcall->response));
    }
  else
    {
      strlcat(xmlcall->response, "  </param></params>\r\n",
              sizeof(xmlcall->response));
    }

  strlcat(xmlcall->response, "</methodResponse>\r\n",
          sizeof(xmlcall->response));

  xmlrpc_insertlength(xmlcall);
  return 0;
}
