/****************************************************************************
 * apps/examples/webserver/cgi.c
 * Web server script interface
 * Author: Adam Dunkels <adam@sics.se>
 *
 * Copyright (c) 2001-2006, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include <nuttx/net/netstats.h>
#include "netutils/httpd.h"

#include "cgi.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_HTTPDFILESTATS
HTTPD_CGI_CALL(file, "file-stats", file_stats);
#endif

#ifdef CONFIG_NETUTILS_HTTPDNETSTATS
HTTPD_CGI_CALL(net, "net-stats", net_stats);
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: net_stats
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_HTTPDNETSTATS
static void net_stats(struct httpd_state *pstate, char *ptr)
{
  char buffer[16];
  int i;
  bool chunked_http_tx = 0;

#if defined(CONFIG_NETUTILS_HTTPD_ENABLE_CHUNKED_ENCODING)
  chunked_http_tx = pstate->ht_chunked;
#endif

  for (i = 0; i < sizeof(g_netstats) / sizeof(net_stats_t); i++)
    {
      snprintf(buffer, 16, "%5u\n", ((net_stats_t *)&g_netstats)[i]);
      httpd_send_datachunk(pstate->ht_sockfd, buffer, strlen(buffer),
                           chunked_http_tx);
    }
}
#endif

/****************************************************************************
 * Name: file_stats
 ****************************************************************************/

#ifdef CONFIG_NETUTILS_HTTPDFILESTATS
static void file_stats(struct httpd_state *pstate, char *ptr)
{
  char buffer[16];
  char *pcount = strchr(ptr, ' ') + 1;
  bool chunked_http_tx = 0;

#if defined(CONFIG_NETUTILS_HTTPD_ENABLE_CHUNKED_ENCODING)
  chunked_http_tx = pstate->ht_chunked;
#endif

  snprintf(buffer, 16, "%5u", httpd_fs_count(pcount));
  httpd_send_datachunk(pstate->ht_sockfd, buffer, strlen(buffer),
                       chunked_http_tx);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: cgi_register
 ****************************************************************************/

void cgi_register(void)
{
#ifdef CONFIG_NETUTILS_HTTPDFILESTATS
  httpd_cgi_register(&file);
#endif

#ifdef CONFIG_NETUTILS_HTTPDNETSTATS
  httpd_cgi_register(&net);
#endif
}
