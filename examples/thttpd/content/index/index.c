/****************************************************************************
 * examples/thttpd/content/index/index.c
 * Generates index.cgi file
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name Gregory Nutt nor the names of its contributors may be
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

#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_THTTPD_BINFS
int index_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  fprintf(stderr, "index.cgi requested from: %s\n", getenv("REMOTE_ADDR"));

  puts(
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\r\n"
    "<html>\r\n"
    "  <head>\r\n"
    "    <title>NuttX examples/thttpd</title>\r\n"
    "  </head>\r\n"
    "  <body bgcolor=\"#fffeec\" text=\"black\">\r\n"
    "\r\n"
    "    <p>\r\n"
    "      These web pages are served by a port of <a href=\"http://acme.com/software/thttpd/\">THTTPD</a>\r\n"
    "      running on top of <a href=\"http://www.nuttx.org\">NuttX</a>.\r\n"
    "    </p>\r\n"
    "    <p>\r\n"
    "      Click on the links below to exercise THTTPD's CGI capability under NuttX.\r\n"
    "      Clicking the links will execute the CGI program from a built-in program\r\n"
    "      residing in a BINFS file system.\r\n"
    "    </p>\r\n"
    "    </ul>\r\n"
    "      <li><a href=\"index.cgi\">Front page</a></li>\r\n"
    "      <li><a href=\"hello\">Say Hello</a></li>\r\n"
    "      <li><a href=\"tasks\">Tasks</a></li>\r\n"
    "      <li><a href=\"netstat\">Network status</a></li>\r\n"
    "    </ul>\r\n"
    "  </body>\r\n"
    "</html>\r\n"
  );

  return 0;
}
