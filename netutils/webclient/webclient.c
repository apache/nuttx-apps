/****************************************************************************
 * apps/netutils/webclient/webclient.c
 * Implementation of the HTTP client.
 *
 *   Copyright (C) 2007, 2009, 2011-2012, 2014, 2020 Gregory Nutt.
 *   All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Based on uIP which also has a BSD style license:
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2002, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
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

/* This example shows a HTTP client that is able to download web pages
 * and files from web servers. It requires a number of callback
 * functions to be implemented by the module that utilizes the code:
 * webclient_datahandler().
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <debug.h>

#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
#include <sys/un.h>
#endif

#include <nuttx/version.h>

#include "netutils/netlib.h"
#include "netutils/webclient.h"

#if defined(CONFIG_NETUTILS_CODECS)
#  if defined(CONFIG_CODECS_URLCODE)
#    include "netutils/urldecode.h"
#  endif
#  if defined(CONFIG_CODECS_BASE64)
#    include "netutils/base64.h"
#  endif
#else
#  undef CONFIG_CODECS_URLCODE
#  undef CONFIG_CODECS_BASE64
#endif

#ifndef CONFIG_NSH_WGET_USERAGENT
#  if CONFIG_VERSION_MAJOR != 0 || CONFIG_VERSION_MINOR != 0
#    define CONFIG_NSH_WGET_USERAGENT \
     "NuttX/" CONFIG_VERSION_STRING " (; http://www.nuttx.org/)"
#  else
#    define CONFIG_NSH_WGET_USERAGENT \
    "NuttX/6.xx.x (; http://www.nuttx.org/)"
#  endif
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_WEBCLIENT_TIMEOUT
#  define CONFIG_WEBCLIENT_TIMEOUT 10
#endif

#ifndef CONFIG_WEBCLIENT_MAX_REDIRECT
/* The default value 50 is taken from curl's --max-redirs option. */
#  define CONFIG_WEBCLIENT_MAX_REDIRECT 50
#endif

#define HTTPSTATUS_NONE            0
#define HTTPSTATUS_OK              1
#define HTTPSTATUS_MOVED           2
#define HTTPSTATUS_ERROR           3

#define ISO_NL                     0x0a
#define ISO_CR                     0x0d
#define ISO_SPACE                  0x20

#define WGET_MODE_GET              0
#define WGET_MODE_POST             1

/* The following CONN_ flags are for conn::flags.
 */

#define CONN_WANT_READ  WEBCLIENT_POLL_INFO_WANT_READ
#define CONN_WANT_WRITE WEBCLIENT_POLL_INFO_WANT_WRITE

#ifdef CONFIG_DEBUG_ASSERTIONS
#define	_CHECK_STATE(ctx, s)	DEBUGASSERT((ctx)->state == (s))
#define	_SET_STATE(ctx, s)		ctx->state = (s)
#else
#define	_CHECK_STATE(ctx, s)	do {} while (0)
#define	_SET_STATE(ctx, s)		do {} while (0)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum webclient_state_e
  {
    WEBCLIENT_STATE_SOCKET,
    WEBCLIENT_STATE_CONNECT,
    WEBCLIENT_STATE_PREPARE_REQUEST,
    WEBCLIENT_STATE_SEND_REQUEST,
    WEBCLIENT_STATE_SEND_REQUEST_BODY,
    WEBCLIENT_STATE_STATUSLINE,
    WEBCLIENT_STATE_HEADERS,
    WEBCLIENT_STATE_DATA,
    WEBCLIENT_STATE_CHUNKED_HEADER,
    WEBCLIENT_STATE_CHUNKED_DATA,
    WEBCLIENT_STATE_CHUNKED_ENDDATA,
    WEBCLIENT_STATE_CHUNKED_TRAILER,
    WEBCLIENT_STATE_WAIT_CLOSE,
    WEBCLIENT_STATE_CLOSE,
    WEBCLIENT_STATE_DONE,
    WEBCLIENT_STATE_TUNNEL_ESTABLISHED,
  };

/* flags for wget_s::internal_flags */

#define	WGET_FLAG_GOT_CONTENT_LENGTH 1U
#define	WGET_FLAG_CHUNKED            2U
#define	WGET_FLAG_GOT_LOCATION       4U

struct wget_target_s
{
  char scheme[sizeof("https") + 1];
  char hostname[CONFIG_WEBCLIENT_MAXHOSTNAME];
  char filename[CONFIG_WEBCLIENT_MAXFILENAME];
  uint16_t port;     /* The port number to use in the connection */
};

struct wget_s
{
  /* Internal status */

  enum webclient_state_e state;
  uint8_t httpstatus;

  /* These describe the just-received buffer of data */

  FAR char *buffer;  /* user-provided buffer */
  int buflen;        /* Length of the user provided buffer */
  int offset;        /* Offset to the beginning of interesting data */
  int datend;        /* Offset+1 to the last valid byte of data in the buffer */

  /* Buffer HTTP header data and parse line at a time */

  char line[CONFIG_WEBCLIENT_MAXHTTPLINE];
  int  ndx;
  bool skip_to_next_line;

  unsigned int internal_flags; /* OR'ed WGET_FLAG_xxx */
  uintmax_t expected_resp_body_len;
  uintmax_t received_body_len;

  uintmax_t chunk_len;
  uintmax_t chunk_received;

#ifdef CONFIG_WEBCLIENT_GETMIMETYPE
  char mimetype[CONFIG_WEBCLIENT_MAXMIMESIZE];
#endif

  struct wget_target_s target;
  struct wget_target_s proxy;

  bool need_conn_close;
  struct webclient_conn_s *conn;
  unsigned int nredirect;
  int redirected;

  /* progress and todo for the current state (WEBCLIENT_STATE_) */

  off_t state_offset;
  size_t state_len;
  FAR const void *data_buffer;
  size_t data_len;

  FAR struct webclient_context *tunnel;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_http10[]               = "HTTP/1.0";
static const char g_http11[]               = "HTTP/1.1";
#ifdef CONFIG_WEBCLIENT_GETMIMETYPE
static const char g_httpcontenttype[]      = "content-type: ";
#endif
static const char g_httphost[]             = "host: ";
static const char g_httplocation[]         = "location: ";
static const char g_httptransferencoding[] = "transfer-encoding: ";

static const char g_httpuseragentfields[] =
  "User-Agent: "
  CONFIG_NSH_WGET_USERAGENT
  "\r\n\r\n";

static const char g_httpcrnl[]       = "\r\n";

static const char g_httpform[]       = "Content-Type: "
                                       "application/x-www-form-urlencoded";
static const char g_httpcontsize[]   = "Content-Length: ";
static const char g_httpconn_close[] = "Connection: close";
#if 0
static const char g_httpconn[]       = "Connection: Keep-Alive";
static const char g_httpcache[]      = "Cache-Control: no-cache";
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: free_ws
 ****************************************************************************/

static void free_ws(FAR struct wget_s *ws)
{
  if (ws->conn != NULL)
    {
      webclient_conn_free(ws->conn);
    }

  free(ws->tunnel);
  free(ws);
}

/****************************************************************************
 * Name: webclient_static_body_func
 ****************************************************************************/

static int webclient_static_body_func(FAR void *buffer,
                                      FAR size_t *sizep,
                                      FAR const void * FAR *datap,
                                      size_t reqsize,
                                      FAR void *ctx)
{
  *datap = ctx;
  *sizep = reqsize;
  return 0;
}

/****************************************************************************
 * Name: append
 ****************************************************************************/

static char *append(char *dest, char *ep, const char *src)
{
  int len;

  if (dest == NULL)
    {
      return NULL;
    }

  len = strlen(src);
  if (ep - dest < len + 1)
    {
      return NULL;
    }

  memcpy(dest, src, len);
  dest[len] = '\0';
  return dest + len;
}

/****************************************************************************
 * Name: wget_strcpy
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
static char *wget_strcpy(char *dest, const char *src)
{
  int len = strlen(src);

  memcpy(dest, src, len);
  dest[len] = '\0';
  return dest + len;
}
#endif

/****************************************************************************
 * Name: wget_urlencode_strcpy
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
static char *wget_urlencode_strcpy(char *dest, const char *src)
{
  int len = strlen(src);
  int d_len;

  d_len = urlencode_len(src, len);
  urlencode(src, len, dest, &d_len);
  return dest + d_len;
}
#endif

/****************************************************************************
 * Name: wget_parseint
 ****************************************************************************/

static int wget_parseint(const char *cp, uintmax_t *resultp, int base)
{
  char *ep;
  uintmax_t val;

  errno = 0;
  val = strtoumax(cp, &ep, base);
  if (cp == ep)
    {
      return -EINVAL; /* not a number */
    }

  if (*ep != '\0')
    {
      return -EINVAL; /* not a number */
    }

  if (errno != 0)
    {
      DEBUGASSERT(errno == ERANGE);
      DEBUGASSERT(val == UINTMAX_MAX);
      return -errno;
    }

  *resultp = val;
  return 0;
}

/****************************************************************************
 * Name: wget_parsestatus
 ****************************************************************************/

static inline int wget_parsestatus(struct webclient_context *ctx,
                                   struct wget_s *ws)
{
  int offset;
  int ndx;
  char *dest;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      bool got_nl;

      ws->line[ndx] = ws->buffer[offset++];
      got_nl = ws->line[ndx] == ISO_NL;
      if (got_nl || ndx == CONFIG_WEBCLIENT_MAXHTTPLINE - 1)
        {
          if (!got_nl)
            {
              nerr("ERROR: HTTP status line didn't fit "
                   "CONFIG_WEBCLIENT_MAXHTTPLINE: %.*s\n",
                   ndx, ws->line);
              return -E2BIG;
            }

          /* HTTP status line is something like:
           *
           * HTTP/1.1 200 OK
           *
           * https://datatracker.ietf.org/doc/html/rfc7230#section-3.1.2
           *
           * > status-line = HTTP-version SP status-code \
           * >               SP reason-phrase CRLF
           */

          ws->line[ndx] = '\0';
          if ((strncmp(ws->line, g_http10, strlen(g_http10)) == 0) ||
              (strncmp(ws->line, g_http11, strlen(g_http11)) == 0))
            {
              unsigned long http_status;
              char *ep;

              DEBUGASSERT(strlen(g_http10) == 8);
              DEBUGASSERT(strlen(g_http11) == 8);

              if (ws->line[8] != ' ')  /* SP before the status-code */
                {
                  return -EINVAL;
                }

              dest = &(ws->line[9]);  /* the status-code */
              ws->httpstatus = HTTPSTATUS_NONE;

              errno = 0;
              http_status = strtoul(dest, &ep, 10);
              if (ep == dest)
                {
                  return -EINVAL;
                }

              if (*ep != ' ')  /* SP before reason-phrase */
                {
                  return -EINVAL;
                }

              if (errno != 0)
                {
                  return -errno;
                }

              ctx->http_status = http_status;
              ninfo("Got HTTP status %lu\n", http_status);
              if (ctx->http_reason != NULL)
                {
                  strncpy(ctx->http_reason,
                          ep + 1,
                          ctx->http_reason_len - 1);
                  ctx->http_reason[ctx->http_reason_len - 1] = 0;
                }

              /* Check for 2xx (Successful) */

              if ((http_status / 100) == 2)
                {
                  ws->httpstatus = HTTPSTATUS_OK;
                }

              /* Check for 3xx (Redirection)
               * Location: header line will contain the new location.
               *
               * Note: the following 3xx are not redirects.
               *   304 not modified
               *   305 use proxy
               */

              else if ((http_status / 100) == 3 &&
                       (http_status != 304) &&
                       (http_status != 305))
                {
                  ws->httpstatus = HTTPSTATUS_MOVED;
                }
            }
          else
            {
              return -ECONNABORTED;
            }

          /* We're done parsing the status line, so start parsing
           * the HTTP headers.
           */

          ws->state = WEBCLIENT_STATE_HEADERS;
          ws->internal_flags &= ~(WGET_FLAG_GOT_CONTENT_LENGTH |
                                  WGET_FLAG_CHUNKED |
                                  WGET_FLAG_GOT_LOCATION);
          ndx = 0;
          break;
        }
      else
        {
          ndx++;
        }
    }

  ws->offset = offset;
  ws->ndx    = ndx;
  return OK;
}

/****************************************************************************
 * Name: parseurl
 ****************************************************************************/

static int parseurl(FAR const char *url, FAR struct wget_target_s *targ,
                    bool require_port)
{
  struct url_s url_s;
  int ret;

  memset(&url_s, 0, sizeof(url_s));
  url_s.scheme = targ->scheme;
  url_s.schemelen = sizeof(targ->scheme);
  url_s.host = targ->hostname;
  url_s.hostlen = sizeof(targ->hostname);
  url_s.path = targ->filename;
  url_s.pathlen = sizeof(targ->filename);
  ret = netlib_parseurl(url, &url_s);
  if (ret < 0)
    {
      return ret;
    }

  if (url_s.port == 0)
    {
      if (require_port)
        {
          return -EINVAL;
        }

      if (!strcmp(targ->scheme, "https"))
        {
          targ->port = 443;
        }
      else
        {
          targ->port = 80;
        }
    }
  else
    {
      targ->port = url_s.port;
    }

  return 0;
}

/****************************************************************************
 * Name: wget_parseheaders
 ****************************************************************************/

static inline int wget_parseheaders(struct webclient_context *ctx,
                                    struct wget_s *ws)
{
  int offset;
  int ndx;
  int ret = OK;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      bool got_nl;

      ws->line[ndx] = ws->buffer[offset++];
      got_nl = ws->line[ndx] == ISO_NL;
      if (got_nl || ndx == CONFIG_WEBCLIENT_MAXHTTPLINE - 1)
        {
          bool found;

          if (ws->skip_to_next_line)
            {
              if (got_nl)
                {
                  ws->skip_to_next_line = false;
                }

              ndx = 0;
              continue;
            }

          /* We have an entire HTTP header line in ws->line, or
           * our buffer is already full, so we start parsing it.
           */

          found = false;
          if (ndx > 0) /* Should always be true */
            {
              ninfo("Got HTTP header line%s: %.*s\n",
                    got_nl ? "" : " (truncated)",
                    ndx - 1, &ws->line[0]);

              if (ws->line[0] == ISO_CR)
                {
                  /* This was the last header line (i.e., and empty "\r\n"),
                   * so we are done with the headers and proceed with the
                   * actual data.
                   */

                  if ((ws->internal_flags & WGET_FLAG_CHUNKED) != 0)
                    {
                      ws->state = WEBCLIENT_STATE_CHUNKED_HEADER;
                      ndx = 0;
                    }
                  else
                    {
                      if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) != 0)
                        {
                          if (ctx->http_status / 100 == 2)
                            {
                              ninfo("Tunnel established\n");
                              ws->state = WEBCLIENT_STATE_TUNNEL_ESTABLISHED;
                            }
                          else
                            {
                              ninfo("HTTP error from tunnelling proxy: %u\n",
                                    ctx->http_status);
                              ws->state = WEBCLIENT_STATE_DATA;
                            }
                        }
                      else
                        {
                          ws->state = WEBCLIENT_STATE_DATA;
                        }
                    }

                  goto exit;
                }

              /* Truncate the trailing \r\n */

              if (got_nl)
                {
                  ndx--;
                  if (ws->line[ndx] != ISO_CR)
                    {
                      nerr("ERROR: unexpected EOL from the server\n");
                      return -EPROTO;
                    }
                }

              ws->line[ndx] = '\0';

              if (!strchr(ws->line, ':'))
                {
                  if (got_nl)
                    {
                      nerr("ERROR: invalid header possibly due to "
                           "small CONFIG_WEBCLIENT_MAXHTTPLINE\n");
                      return -E2BIG;
                    }

                  nerr("ERROR: invalid header\n");
                  return -EPROTO;
                }

              if (ctx->header_callback)
                {
                  ret = ctx->header_callback(&ws->line[0], !got_nl,
                                             ctx->header_callback_arg);
                  if (ret != 0)
                    {
                      goto exit;
                    }
                }

              /* Check for specific HTTP header fields. */

#ifdef CONFIG_WEBCLIENT_GETMIMETYPE
              if (strncasecmp(ws->line, g_httpcontenttype,
                              strlen(g_httpcontenttype)) == 0)
                {
                  /* Found Content-type field. */

                  char *dest = strchr(ws->line, ';');
                  if (dest != NULL)
                    {
                      *dest = 0;
                    }

                  strncpy(ws->mimetype, ws->line + strlen(g_httpcontenttype),
                          sizeof(ws->mimetype));
                  found = true;
                }
              else
#endif
              if (strncasecmp(ws->line, g_httplocation,
                              strlen(g_httplocation)) == 0)
                {
                  /* Parse the new host and filename from the URL.
                   */

                  ninfo("Redirect to location: '%s'\n",
                        ws->line + strlen(g_httplocation));
                  ret = parseurl(ws->line + strlen(g_httplocation),
                                 &ws->target, false);
                  if (ret != 0)
                    {
                      goto exit;
                    }

                  ninfo("New hostname='%s' filename='%s'\n",
                        ws->target.hostname, ws->target.filename);
                  ws->internal_flags |= WGET_FLAG_GOT_LOCATION;
                  found = true;
                }
              else if (strncasecmp(ws->line, g_httpcontsize,
                                   strlen(g_httpcontsize)) == 0)
                {
                  found = true;
                  if (got_nl)
                    {
                      ret = wget_parseint(ws->line + strlen(g_httpcontsize),
                                          &ws->expected_resp_body_len, 10);
                      if (ret != 0)
                        {
                          goto exit;
                        }

                      ws->internal_flags |=
                          WGET_FLAG_GOT_CONTENT_LENGTH;
                      ninfo("Content-Length %ju\n",
                            ws->expected_resp_body_len);
                    }
                }
              else if (strncasecmp(ws->line, g_httptransferencoding,
                                   strlen(g_httptransferencoding)) == 0)
                {
                  FAR const char *encodings =
                      ws->line + strlen(g_httptransferencoding);

                  if (strcasecmp(encodings, "chunked"))
                    {
                      nerr("unknown encodings: '%s'\n", encodings);
                      return -EPROTO;
                    }

                  ninfo("transfer encodings: '%s'\n", encodings);
                  ws->internal_flags |= WGET_FLAG_CHUNKED;
                }
            }

          if (found && !got_nl)
            {
              /* We found something we might care.
               * but we couldn't process it correctly.
               */

              nerr("ERROR: truncated a header due to "
                   "small CONFIG_WEBCLIENT_MAXHTTPLINE\n");
              return -E2BIG;
            }

          /* We're done parsing ws->line, so we reset the index.
           *
           * If we haven't seen the entire line yet (!got_nl),
           * skip the rest of the line.
           * Otherwise, start processing the next line.
           */

          ndx = 0;
          ws->skip_to_next_line = !got_nl;
        }
      else
        {
          ndx++;
        }
    }

exit:
  ws->offset = offset;
  ws->ndx    = ndx;
  return ret;
}

/****************************************************************************
 * Name: wget_parsechunkheader
 ****************************************************************************/

static inline int wget_parsechunkheader(struct webclient_context *ctx,
                                        struct wget_s *ws)
{
  int offset;
  int ndx;
  int ret = OK;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      bool got_nl;

      ws->line[ndx] = ws->buffer[offset++];
      got_nl = ws->line[ndx] == ISO_NL;
      if (got_nl || ndx == CONFIG_WEBCLIENT_MAXHTTPLINE - 1)
        {
          bool found_extension = false;

          /* We have an entire header line in ws->line, or
           * our buffer is already full, so we start parsing it.
           */

          if (ndx > 0) /* Should always be true */
            {
              FAR char *semicolon;

              ninfo("Got chunk header line%s: %.*s\n",
                    got_nl ? "" : " (truncated)",
                    ndx - 1, &ws->line[0]);

              if (ws->line[0] == ISO_CR)
                {
                  nerr("ERROR: empty chunk header\n");
                  ret = -EPROTO;
                  break;
                }

              /* Truncate the trailing \r\n */

              if (got_nl)
                {
                  ndx--;
                  if (ws->line[ndx] != ISO_CR)
                    {
                      nerr("ERROR: unexpected EOL from the server\n");
                      ret = -EPROTO;
                      break;
                    }
                }

              ws->line[ndx] = '\0';

              semicolon = strchr(ws->line, ';');
              if (semicolon != NULL)
                {
                  found_extension = true;
                  ninfo("Ignoring extentions in chunk header\n");
                  *semicolon = 0;
                }
            }

          if (!got_nl && !found_extension)
            {
              /* We found something we might care.
               * but we couldn't process it correctly.
               */

              nerr("ERROR: truncated a header due to "
                   "small CONFIG_WEBCLIENT_MAXHTTPLINE\n");
              ret = -E2BIG;
              break;
            }

          ret = wget_parseint(ws->line, &ws->chunk_len, 16);
          if (ret != 0)
            {
              break;
            }

          if (ws->chunk_len != 0)
            {
              ninfo("Receiving a chunk with %ju bytes\n", ws->chunk_len);
              ws->state = WEBCLIENT_STATE_CHUNKED_DATA;
              ws->chunk_received = 0;
            }
          else
            {
              ws->state = WEBCLIENT_STATE_CHUNKED_TRAILER;
            }

          ndx = 0;
          break;
        }
      else
        {
          ndx++;
        }
    }

  ws->offset = offset;
  ws->ndx    = ndx;
  return ret;
}

/****************************************************************************
 * Name: wget_parsechunkenddata
 ****************************************************************************/

static inline int wget_parsechunkenddata(struct webclient_context *ctx,
                                         struct wget_s *ws)
{
  int offset;
  int ndx;
  int ret = OK;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      ws->line[ndx] = ws->buffer[offset++];
      if (ws->line[ndx] == ISO_NL)
        {
          if (ndx == 0)
            {
              ret = -EPROTO;
              break;
            }

          if (ws->line[ndx - 1] != ISO_CR)
            {
              ret = -EPROTO;
              break;
            }

          if (ndx != 1)
            {
              ret = -EPROTO;
              break;
            }

          if (ws->chunk_len == 0)
            {
              ws->state = WEBCLIENT_STATE_CHUNKED_TRAILER;
            }
          else
            {
              ws->state = WEBCLIENT_STATE_CHUNKED_HEADER;
            }

          ndx = 0;
          break;
        }

      ndx++;
    }

  ws->offset = offset;
  ws->ndx    = ndx;
  return ret;
}

/****************************************************************************
 * Name: wget_parsechunktrailer
 ****************************************************************************/

static inline int wget_parsechunktrailer(struct webclient_context *ctx,
                                         struct wget_s *ws)
{
  int offset;
  int ndx;
  int ret = OK;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      ws->line[ndx] = ws->buffer[offset++];
      if (ws->line[ndx] == ISO_NL)
        {
          if (ndx == 0)
            {
              ret = -EPROTO;
              break;
            }

          if (ws->line[ndx - 1] != ISO_CR)
            {
              ret = -EPROTO;
              break;
            }

          if (ndx != 1)
            {
              /* Ignore all non empty lines. */

              ndx = 0;
              continue;
            }

          ws->state = WEBCLIENT_STATE_WAIT_CLOSE;
          break;
        }

      ndx++;
    }

  ws->offset = offset;
  ws->ndx    = ndx;
  return ret;
}

/****************************************************************************
 * Name: wget_gethostip
 *
 * Description:
 *   Call getaddrinfo() to get the IPv4 address associated with a hostname.
 *
 * Input Parameters
 *   hostname - The host name to use in the nslookup.
 *
 * Output Parameters
 *   dest     - The location to return the IPv4 address.
 *
 * Returned Value:
 *   Zero (OK) on success; ERROR on failure.
 *
 ****************************************************************************/

static int wget_gethostip(FAR char *hostname, FAR struct in_addr *dest)
{
#ifdef CONFIG_LIBC_NETDB
  FAR struct addrinfo hint;
  FAR struct addrinfo *info;
  FAR struct sockaddr_in *addr;

  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &hint, &info) != OK)
    {
      return ERROR;
    }

  addr = (FAR struct sockaddr_in *)info->ai_addr;
  memcpy(dest, &addr->sin_addr, sizeof(struct in_addr));

  freeaddrinfo(info);
  return OK;
#else
  /* No host name support */

  /* Convert strings to numeric IPv4 address */

  int ret = inet_pton(AF_INET, hostname, dest);

  /* The inet_pton() function returns 1 if the conversion succeeds. It will
   * return 0 if the input is not a valid IPv4 dotted-decimal string or -1
   * with errno set to EAFNOSUPPORT if the address family argument is
   * unsupported.
   */

  return (ret > 0) ? OK : ERROR;
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: webclient_conn_send
 ****************************************************************************/

ssize_t webclient_conn_send(FAR struct webclient_conn_s *conn,
                            FAR const void *buffer, size_t len)
{
  if (conn->tls)
    {
      return conn->tls_ops->send(conn->tls_ctx, conn->tls_conn, buffer, len);
    }

  while (true)
    {
      ssize_t ret = send(conn->sockfd, buffer, len, 0);
      if (ret == -1)
        {
          if (errno == EINTR)
            {
              continue;
            }

          if (errno == EAGAIN)
            {
              conn->flags |= CONN_WANT_WRITE;
            }

          return -errno;
        }

      return ret;
    }
}

/****************************************************************************
 * Name: webclient_conn_recv
 ****************************************************************************/

ssize_t webclient_conn_recv(FAR struct webclient_conn_s *conn,
                            FAR void *buffer, size_t len)
{
  if (conn->tls)
    {
      return conn->tls_ops->recv(conn->tls_ctx, conn->tls_conn, buffer, len);
    }

  while (true)
    {
      ssize_t ret = recv(conn->sockfd, buffer, len, 0);
      if (ret == -1)
        {
          if (errno == EINTR)
            {
              continue;
            }

          if (errno == EAGAIN)
            {
              conn->flags |= CONN_WANT_READ;
            }

          return -errno;
        }

      return ret;
    }
}

/****************************************************************************
 * Name: webclient_conn_close
 ****************************************************************************/

void webclient_conn_close(FAR struct webclient_conn_s *conn)
{
  if (conn->tls)
    {
      conn->tls_ops->close(conn->tls_ctx, conn->tls_conn);
    }
  else
    {
      close(conn->sockfd);
    }
}

/****************************************************************************
 * Name: webclient_conn_free
 ****************************************************************************/

void webclient_conn_free(FAR struct webclient_conn_s *conn)
{
  DEBUGASSERT(conn != NULL);
  free(conn);
}

/****************************************************************************
 * Name: webclient_perform
 *
 * Returned Value:
 *               0: if the operation completed successfully;
 *  Negative errno: On a failure
 *
 ****************************************************************************/

int webclient_perform(FAR struct webclient_context *ctx)
{
  struct wget_s *ws;
  struct timeval tv;
  char *dest;
  char *ep;
  struct webclient_conn_s *conn;
  FAR const struct webclient_tls_ops *tls_ops = ctx->tls_ops;
  FAR const char *method = ctx->method;
  FAR void *tls_ctx = ctx->tls_ctx;
  unsigned int i;
  int len;
  int ret;

#ifdef CONFIG_DEBUG_ASSERTIONS
  DEBUGASSERT(ctx->state == WEBCLIENT_CONTEXT_STATE_INITIALIZED ||
              (ctx->state == WEBCLIENT_CONTEXT_STATE_IN_PROGRESS &&
               (ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0));
#endif

#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
  if (ctx->unix_socket_path != NULL && ctx->proxy != NULL)
    {
      nerr("ERROR: proxy with AF_LOCAL is not implemented");
      _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
      return -ENOTSUP;
    }
#endif

  /* Initialize the state structure */

  if (ctx->ws == NULL)
    {
      ws = calloc(1, sizeof(struct wget_s));
      if (!ws)
        {
          _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
          return -errno;
        }

      ws->conn = calloc(1, sizeof(struct webclient_conn_s));
      if (!ws->conn)
        {
          free_ws(ws);
          _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
          return -errno;
        }

      ws->buffer = ctx->buffer;
      ws->buflen = ctx->buflen;

      /* Parse the hostname (with optional port number) and filename
       * from the URL.
       */

      if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) == 0)
        {
          ret = parseurl(ctx->url, &ws->target, false);
          if (ret != 0)
            {
              nwarn("WARNING: Malformed URL: %s\n", ctx->url);
              free_ws(ws);
              _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
              return ret;
            }
        }

      if (ctx->proxy != NULL)
        {
          /* Note: reject a proxy string w/o port number specified.
           * It's better to be explicit because the default number varies
           * among HTTP client implementations.
           * (80, 1080, 3128, 8080, ...)
           */

          ret = parseurl(ctx->proxy, &ws->proxy, true);
          if (ret != 0)
            {
              nerr("ERROR: Malformed proxy setting: %s\n", ctx->proxy);
              free_ws(ws);
              _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
              return ret;
            }

          if (strcmp(ws->proxy.scheme, "http") ||
              strcmp(ws->proxy.filename, "/"))
            {
              nerr("ERROR: Unsupported proxy setting: %s\n", ctx->proxy);
              free_ws(ws);
              _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
              return -ENOTSUP;
            }
        }

      ws->state = WEBCLIENT_STATE_SOCKET;
      ctx->ws = ws;
    }

  ws = ctx->ws;

  ninfo("hostname='%s' filename='%s'\n", ws->target.hostname,
        ws->target.filename);

  /* The following sequence may repeat indefinitely if we are redirected */

  conn = ws->conn;
  do
    {
      if (ws->state == WEBCLIENT_STATE_SOCKET)
        {
          if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) != 0)
            {
              conn->tls = false;
            }
          else if (!strcmp(ws->target.scheme, "https") && tls_ops != NULL)
            {
              conn->tls = true;
              conn->tls_ops = tls_ops;
              conn->tls_ctx = ctx->tls_ctx;
            }
          else if (!strcmp(ws->target.scheme, "http"))
            {
              conn->tls = false;
            }
          else
            {
              nerr("ERROR: unsupported scheme: %s\n", ws->target.scheme);
              free_ws(ws);
              _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
              return -ENOTSUP;
            }

          /* Re-initialize portions of the state structure that could have
           * been left from the previous time through the loop and should not
           * persist with the new connection.
           */

          ws->httpstatus = HTTPSTATUS_NONE;
          ws->offset     = 0;
          ws->datend     = 0;
          ws->ndx        = 0;
          ws->redirected = 0;

          if (conn->tls)
            {
#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
              if (ctx->unix_socket_path != NULL)
                {
                  nerr("ERROR: TLS on AF_LOCAL socket is not implemented\n");
                  free_ws(ws);
                  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
                  return -ENOTSUP;
                }
#endif

              if (ctx->proxy != NULL)
                {
                  FAR struct webclient_context *tunnel;

                  DEBUGASSERT(ws->tunnel == NULL);

                  if (tls_ops->init_connection == NULL)
                    {
                      nerr("ERROR: TLS over proxy is not implemented\n");
                      ret = -ENOTSUP;
                      goto errout_with_errno;
                    }

                  /* Create a temporary context to establish a tunnel. */

                  ws->tunnel = tunnel = calloc(1, sizeof(*ws->tunnel));
                  if (tunnel == NULL)
                    {
                      ret = -ENOMEM;
                      goto errout_with_errno;
                    }

                  webclient_set_defaults(tunnel);
                  tunnel->method = "CONNECT";
                  tunnel->flags |= WEBCLIENT_FLAG_TUNNEL;
                  tunnel->tunnel_target_host = ws->target.hostname;
                  tunnel->tunnel_target_port = ws->target.port;
                  tunnel->proxy = ctx->proxy;
                  tunnel->proxy_headers = ctx->proxy_headers;
                  tunnel->proxy_nheaders = ctx->proxy_nheaders;

                  /* Inherit some configurations.
                   *
                   * Revisit: should there be independent configurations?
                   */

                  tunnel->protocol_version = ctx->protocol_version;
                  if ((ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0)
                    {
                      tunnel->flags |= WEBCLIENT_FLAG_NON_BLOCKING;
                    }

                  /* Abuse the buffer of the original request.
                   * It's safe with the current usage.
                   */

                  tunnel->buffer = ctx->buffer;
                  tunnel->buflen = ctx->buflen;
                }
            }
          else
            {
              int domain;

#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
              if (ctx->unix_socket_path != NULL)
                {
                  domain = PF_LOCAL;
                }
              else
#endif
                {
                  domain = PF_INET;
                }

              /* Create a socket */

              conn->sockfd = socket(domain, SOCK_STREAM, 0);
              if (conn->sockfd < 0)
                {
                  ret = -errno;
                  nerr("ERROR: socket failed: %d\n", errno);
                  goto errout_with_errno;
                }

              ws->need_conn_close = true;

              if ((ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0)
                {
                  int flags = fcntl(conn->sockfd, F_GETFL, 0);
                  ret = fcntl(conn->sockfd, F_SETFL, flags | O_NONBLOCK);
                  if (ret == -1)
                    {
                      ret = -errno;
                      nerr("ERROR: F_SETFL failed: %d\n", ret);
                      goto errout_with_errno;
                    }
                }
              else
                {
                  /* Set send and receive timeout values */

                  tv.tv_sec  = ctx->timeout_sec;
                  tv.tv_usec = 0;

                  setsockopt(conn->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                             (FAR const void *)&tv, sizeof(struct timeval));
                  setsockopt(conn->sockfd, SOL_SOCKET, SO_SNDTIMEO,
                             (FAR const void *)&tv, sizeof(struct timeval));
                }
            }

          ws->state = WEBCLIENT_STATE_CONNECT;
        }

      if (ws->state == WEBCLIENT_STATE_CONNECT)
        {
          if (ws->tunnel != NULL)
            {
              ret = webclient_perform(ws->tunnel);
              if (ret == 0)
                {
                  unsigned int http_status = ws->tunnel->http_status;

                  if (http_status / 100 != 2)
                    {
                      nerr("HTTP-level error %u on tunnel attempt\n",
                           http_status);

                      /* 407 Proxy Authentication Required */

                      if (http_status == 407)
                        {
                          ret = -EPERM;
                        }
                      else
                        {
                          ret = -EIO;
                        }
                    }
                }

              if (ret == 0)
                {
                  FAR struct webclient_conn_s *tunnel_conn;

                  webclient_get_tunnel(ws->tunnel, &tunnel_conn);
                  DEBUGASSERT(tunnel_conn != NULL);
                  DEBUGASSERT(!tunnel_conn->tls);
                  free(ws->tunnel);
                  ws->tunnel = NULL;

                  if (conn->tls)
                    {
                      /* Revisit: tunnel_conn here should have
                       * timeout configured already.
                       * Configuring it again here is redundant.
                       */

                      ret = tls_ops->init_connection(tls_ctx,
                                                     tunnel_conn,
                                                     ws->target.hostname,
                                                     ctx->timeout_sec,
                                                     &conn->tls_conn);
                      if (ret == 0)
                        {
                          /* Note: tunnel_conn has been consumed by
                           * tls_ops->init_connection
                           */

                          ws->need_conn_close = true;
                        }
                      else
                        {
                          /* Note: restarting tls_ops->init_connection
                           * is not implemented
                           */

                          DEBUGASSERT(ret != -EAGAIN &&
                                      ret != -EINPROGRESS &&
                                      ret != -EALREADY);
                          webclient_conn_close(tunnel_conn);
                          webclient_conn_free(tunnel_conn);
                        }
                    }
                  else
                    {
                      conn->sockfd = tunnel_conn->sockfd;
                      ws->need_conn_close = true;
                      webclient_conn_free(tunnel_conn);
                    }
                }
            }
          else if (conn->tls)
            {
              char port_str[sizeof("65535")];

#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
              if (ctx->unix_socket_path != NULL)
                {
                  nerr("ERROR: TLS on AF_LOCAL socket is not implemented\n");
                  free_ws(ws);
                  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
                  return -ENOTSUP;
                }
#endif

              snprintf(port_str, sizeof(port_str), "%u", ws->target.port);
              ret = tls_ops->connect(tls_ctx, ws->target.hostname, port_str,
                                     ctx->timeout_sec,
                                     &conn->tls_conn);
              if (ret == 0)
                {
                  ws->need_conn_close = true;
                }
            }
          else
            {
#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
              struct sockaddr_un server_un;
#endif
              struct sockaddr_in server_in;
              const struct sockaddr *server_address;
              socklen_t server_address_len;

#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
              if (ctx->unix_socket_path != NULL)
                {
                  memset(&server_un, 0, sizeof(server_un));
                  server_un.sun_family = AF_LOCAL;
                  strncpy(server_un.sun_path, ctx->unix_socket_path,
                          sizeof(server_un.sun_path));
#if !defined(__NuttX__) && !defined(__linux__)
                  server_un.sun_len = SUN_LEN(&server_un);
#endif
                  server_address = (const struct sockaddr *)&server_un;
                  server_address_len = sizeof(server_un);
                }
              else
#endif
                {
                  /* Get the server address from the host name */

                  FAR struct wget_target_s *target;

                  if (ctx->proxy != NULL)
                    {
                      target = &ws->proxy;
                    }
                  else
                    {
                      target = &ws->target;
                    }

                  server_in.sin_family = AF_INET;
                  server_in.sin_port   = htons(target->port);
                  ret = wget_gethostip(target->hostname,
                                       &server_in.sin_addr);
                  if (ret < 0)
                    {
                      /* Could not resolve host (or malformed IP address) */

                      nwarn("WARNING: Failed to resolve hostname\n");
                      free_ws(ws);
                      _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
                      return -EHOSTUNREACH;
                    }

                  server_address = (const struct sockaddr *)&server_in;
                  server_address_len = sizeof(struct sockaddr_in);
                }

              /* Connect to server.  First we have to set some fields in the
               * 'server' address structure.  The system will assign me an
               * arbitrary local port that is not in use.
               */

              while (true)
                {
                  ret = connect(conn->sockfd, server_address,
                                server_address_len);
                  if (ret == -1)
                    {
                      ret = -errno;
                      DEBUGASSERT(ret < 0);
                      if (ret == -EINTR)
                        {
                          continue;
                        }

                      if (ret == -EISCONN)
                        {
                          ret = 0;
                        }
                    }
                  break;
                }
            }

          if (ret < 0)
            {
              nerr("ERROR: connect failed: %d\n", errno);
              goto errout_with_errno;
            }

          ws->state = WEBCLIENT_STATE_PREPARE_REQUEST;
        }

      if (ws->state == WEBCLIENT_STATE_PREPARE_REQUEST)
        {
          /* Send the request */

          dest = ws->buffer;
          ep = ws->buffer + ws->buflen;
          dest = append(dest, ep, method);
          dest = append(dest, ep, " ");

          if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) != 0)
            {
              /* Use authority-form for a tunnel
               *
               * https://datatracker.ietf.org/doc/html/rfc7231#section-4.3.6
               * https://datatracker.ietf.org/doc/html/rfc7230#section-5.3.3
               */

              char port_str[sizeof("65535")];

              dest = append(dest, ep, ctx->tunnel_target_host);
              dest = append(dest, ep, ":");
              snprintf(port_str, sizeof(port_str), "%u",
                       ctx->tunnel_target_port);
              dest = append(dest, ep, port_str);
            }
          else if (ctx->proxy != NULL)
            {
              /* Use absolute-form for a proxy
               *
               * https://datatracker.ietf.org/doc/html/rfc7230#section-5.3.2
               */

              char port_str[sizeof("65535")];

              dest = append(dest, ep, ws->target.scheme);
              dest = append(dest, ep, "://");
              dest = append(dest, ep, ws->target.hostname);
              dest = append(dest, ep, ":");
              snprintf(port_str, sizeof(port_str), "%u", ws->target.port);
              dest = append(dest, ep, port_str);
              dest = append(dest, ep, ws->target.filename);
            }
          else
            {
              /* Otherwise, use origin-form */

#ifndef WGET_USE_URLENCODE
              dest = append(dest, ep, ws->target.filename);
#else
              /* TODO: should we use wget_urlencode_strcpy? */

              dest = append(dest, ep, ws->target.filename);
#endif
            }

          dest = append(dest, ep, " ");
          if (ctx->protocol_version == WEBCLIENT_PROTOCOL_VERSION_HTTP_1_0)
            {
              dest = append(dest, ep, g_http10);
            }
          else if (ctx->protocol_version ==
                   WEBCLIENT_PROTOCOL_VERSION_HTTP_1_1)
            {
              dest = append(dest, ep, g_http11);
            }
          else
            {
              ret = -EINVAL;
              goto errout_with_errno;
            }

          dest = append(dest, ep, g_httpcrnl);

          /* Note about proxy and Host header:
           *
           * https://datatracker.ietf.org/doc/html/rfc7230#section-5.4
           * > A client MUST send a Host header field in an HTTP/1.1
           * > request even if the request-target is in the absolute-form,
           * > since this allows the Host information to be forwarded
           * > through ancient HTTP/1.0 proxies that might not have
           * > implemented Host.
           *
           * > When a proxy receives a request with an absolute-form of
           * > request-target, the proxy MUST ignore the received Host
           * > header field (if any) and instead replace it with the host
           * > information of the request-target.
           */

          dest = append(dest, ep, g_httphost);
          dest = append(dest, ep, ws->target.hostname);
          dest = append(dest, ep, g_httpcrnl);

          /* Append user-specified headers.
           *
           * - For non-proxied request,
           *   only send "headers".
           *
           * - For proxied request, (traditional http proxy)
           *   send both of "headers" and "proxy_headers".
           *
           * - For tunneling request, (WEBCLIENT_FLAG_TUNNEL)
           *   only send "proxy_headers".
           */

          if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) == 0)
            {
              for (i = 0; i < ctx->nheaders; i++)
                {
                  dest = append(dest, ep, ctx->headers[i]);
                  dest = append(dest, ep, g_httpcrnl);
                }
            }

          if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) != 0 ||
              ctx->proxy != NULL)
            {
              for (i = 0; i < ctx->proxy_nheaders; i++)
                {
                  dest = append(dest, ep, ctx->proxy_headers[i]);
                  dest = append(dest, ep, g_httpcrnl);
                }
            }

          if (ctx->bodylen)
            {
              char post_size[sizeof("18446744073709551615")];

              dest = append(dest, ep, g_httpcontsize);
              sprintf(post_size, "%zu", ctx->bodylen);
              dest = append(dest, ep, post_size);
              dest = append(dest, ep, g_httpcrnl);
            }

          if (ctx->protocol_version == WEBCLIENT_PROTOCOL_VERSION_HTTP_1_1)
            {
              /* We don't implement persistect connections. */

              dest = append(dest, ep, g_httpconn_close);
              dest = append(dest, ep, g_httpcrnl);
            }

          dest = append(dest, ep, g_httpuseragentfields);

          if (dest == NULL)
            {
              ret = -E2BIG;
              goto errout_with_errno;
            }

          len = dest - ws->buffer;

          ws->state = WEBCLIENT_STATE_SEND_REQUEST;
          ws->state_offset = 0;
          ws->state_len = len;
        }

      if (ws->state == WEBCLIENT_STATE_SEND_REQUEST)
        {
          ssize_t ssz;

          ssz = webclient_conn_send(conn,
                                    ws->buffer + ws->state_offset,
                                    ws->state_len);
          if (ssz < 0)
            {
              ret = ssz;
              nerr("ERROR: send failed: %d\n", -ret);
              goto errout_with_errno;
            }

          if (ssz >= 0)
            {
              ws->state_offset += ssz;
              ws->state_len -= ssz;
              if (ws->state_len == 0)
                {
                  ws->state = WEBCLIENT_STATE_SEND_REQUEST_BODY;
                  ws->state_offset = 0;
                  ws->state_len = ctx->bodylen;
                  ws->data_buffer = NULL;
                  ninfo("Sending %zu bytes request body\n", ws->state_len);
                }
            }
        }

      if (ws->state == WEBCLIENT_STATE_SEND_REQUEST_BODY)
        {
          /* In this state,
           *
           * ws->data_buffer  the data given by the user
           * ws->data_offset  the byte offset in the entire body
           * ws->data_len     the byte size of the data in ws->data_buffer
           * ws->state_offset the send offset in ws->data_buffer
           * ws->state_len    the number of remaining bytes to send (total)
           */

          if (ws->state_len == 0)
            {
              ninfo("Finished sending request body\n");
              ws->state = WEBCLIENT_STATE_STATUSLINE;
            }
          else if (ws->data_buffer == NULL)
            {
              FAR const void *input_buffer;
              size_t input_buffer_size = ws->buflen;

              size_t todo = ws->state_len;
              if (input_buffer_size > todo)
                {
                  input_buffer_size = todo;
                }

              input_buffer = ws->buffer;
              ret = ctx->body_callback(ws->buffer,
                                       &input_buffer_size,
                                       &input_buffer,
                                       todo,
                                       ctx->body_callback_arg);
              if (ret < 0)
                {
                  nerr("ERROR: body_callback failed: %d\n", -ret);
                  goto errout_with_errno;
                }

              ninfo("Got %zu bytes body chunk / total remaining %zu bytes\n",
                    input_buffer_size, ws->state_len);
              ws->data_buffer = input_buffer;
              ws->data_len = input_buffer_size;
              ws->state_offset = 0;
            }
          else
            {
              size_t bytes_to_send = ws->data_len - ws->state_offset;

              DEBUGASSERT(bytes_to_send <= ws->state_len);
              ssize_t ssz = webclient_conn_send(conn,
                                                ws->data_buffer +
                                                ws->state_offset,
                                                bytes_to_send);
              if (ssz < 0)
                {
                  ret = ssz;
                  nerr("ERROR: send failed: %d\n", -ret);
                  goto errout_with_errno;
                }

              ninfo("Sent %zd bytes request body at offset %ju "
                    "in the %zu bytes chunk, "
                    "total remaining %zu bytes\n",
                    ssz,
                    (uintmax_t)ws->state_offset,
                    ws->data_len,
                    ws->state_len);
              ws->state_len -= ssz;
              ws->state_offset += ssz;
              DEBUGASSERT(ws->state_offset <= ws->data_len);
              if (ws->state_offset == ws->data_len)
                {
                  ws->data_buffer = NULL;
                }
            }
        }

      /* Now loop to get the file sent in response to the GET.  This
       * loop continues until either we read the end of file (nbytes == 0)
       * or until we detect that we have been redirected.
       */

      if (ws->state == WEBCLIENT_STATE_STATUSLINE ||
          ws->state == WEBCLIENT_STATE_HEADERS ||
          ws->state == WEBCLIENT_STATE_DATA ||
          ws->state == WEBCLIENT_STATE_CHUNKED_HEADER ||
          ws->state == WEBCLIENT_STATE_CHUNKED_DATA)
        {
          for (; ; )
            {
              if (ws->datend - ws->offset == 0)
                {
                  size_t want = ws->buflen;
                  ssize_t ssz;

                  ninfo("Reading new data\n");
                  if ((ctx->flags & WEBCLIENT_FLAG_TUNNEL) != 0)
                    {
                      /* When tunnelling, we want to avoid troubles
                       * with reading the starting payload of the tunnelled
                       * protocol here, in case it's a server-speaks-first
                       * protocol.
                       */

                      want = 1;
                    }

                  ssz = webclient_conn_recv(conn, ws->buffer, want);
                  if (ssz < 0)
                    {
                      ret = ssz;
                      nerr("ERROR: recv failed: %d\n", -ret);
                      goto errout_with_errno;
                    }
                  else if (ssz == 0)
                    {
                      if (ws->state != WEBCLIENT_STATE_DATA &&
                          ws->state != WEBCLIENT_STATE_WAIT_CLOSE)
                        {
                          nerr("Connection lost unexpectedly\n");
                          ret = -ECONNABORTED;
                          goto errout_with_errno;
                        }

                      if ((ws->internal_flags &
                           WGET_FLAG_GOT_CONTENT_LENGTH) != 0 &&
                          ws->expected_resp_body_len !=
                          ws->received_body_len)
                        {
                          nerr("Unexpected response body length: "
                               "%ju != %ju\n",
                               ws->expected_resp_body_len,
                               ws->received_body_len);
                          ret = -EPROTO;
                          goto errout_with_errno;
                        }

                      ninfo("Connection lost\n");
                      ws->state = WEBCLIENT_STATE_CLOSE;
                      ws->redirected = 0;
                      break;
                    }

                  ninfo("Got %zd bytes data\n", ssz);
                  ws->offset = 0;
                  ws->datend = ssz;
                }

              /* Handle initial parsing of the status line */

              if (ws->state == WEBCLIENT_STATE_STATUSLINE)
                {
                  ret = wget_parsestatus(ctx, ws);
                  if (ret < 0)
                    {
                      goto errout_with_errno;
                    }
                }

              /* Parse the HTTP data */

              if (ws->state == WEBCLIENT_STATE_HEADERS)
                {
                  ret = wget_parseheaders(ctx, ws);
                  if (ret < 0)
                    {
                      goto errout_with_errno;
                    }
                }

              /* Parse the chunk header */

              if (ws->state == WEBCLIENT_STATE_CHUNKED_HEADER)
                {
                  ret = wget_parsechunkheader(ctx, ws);
                  if (ret < 0)
                    {
                      goto errout_with_errno;
                    }
                }

              if (ws->state == WEBCLIENT_STATE_CHUNKED_ENDDATA)
                {
                  ret = wget_parsechunkenddata(ctx, ws);
                  if (ret < 0)
                    {
                      goto errout_with_errno;
                    }
                }

              if (ws->state == WEBCLIENT_STATE_CHUNKED_TRAILER)
                {
                  ret = wget_parsechunktrailer(ctx, ws);
                  if (ret < 0)
                    {
                      goto errout_with_errno;
                    }
                }

              if (ws->state == WEBCLIENT_STATE_WAIT_CLOSE)
                {
                  uintmax_t received = ws->datend - ws->offset;
                  if (received != 0)
                    {
                      nerr("Unexpected %ju bytes data received", received);
                      ret = -EPROTO;
                      goto errout_with_errno;
                    }
                }

              /* Dispose of the data payload */

              if (ws->state == WEBCLIENT_STATE_DATA ||
                  ws->state == WEBCLIENT_STATE_CHUNKED_DATA)
                {
                  if (ws->httpstatus != HTTPSTATUS_MOVED)
                    {
                      uintmax_t received = ws->datend - ws->offset;
                      FAR char *orig_buffer = ws->buffer;
                      int orig_buflen = ws->buflen;

                      if ((ws->internal_flags & WGET_FLAG_GOT_LOCATION) != 0)
                        {
                          nwarn("WARNING: Unexpected Location header\n");
                        }

                      if (ws->state == WEBCLIENT_STATE_CHUNKED_DATA)
                        {
                          uintmax_t chunk_left =
                              ws->chunk_len - ws->chunk_received;

                          if (received > chunk_left)
                            {
                              received = chunk_left;
                            }

                          ws->chunk_received += received;
                        }

                      ninfo("Processing resp body %ju - %ju\n",
                            ws->received_body_len,
                            ws->received_body_len + received);
                      ws->received_body_len += received;

                      /* Let the client decide what to do with the
                       * received file.
                       */

                      if (received == 0)
                        {
                          /* We don't have data to give to the client yet. */
                        }
                      else if (ctx->sink_callback)
                        {
                          ret = ctx->sink_callback(&ws->buffer, ws->offset,
                                                   ws->offset + received,
                                                   &ws->buflen,
                                                   ctx->sink_callback_arg);
                          if (ret != 0)
                            {
                              goto errout_with_errno;
                            }
                        }
                      else if (ctx->callback)
                        {
                          ctx->callback(&ws->buffer, ws->offset,
                                        ws->offset + received,
                                        &ws->buflen, ctx->sink_callback_arg);
                        }

                      ws->offset += received;

                      /* The buffer swapping API doesn't work for
                       * HTTP 1.1 chunked transfer because the buffer here
                       * might already contain the next chunk header.
                       */

                      if (ctx->protocol_version ==
                          WEBCLIENT_PROTOCOL_VERSION_HTTP_1_1)
                        {
                          if (orig_buffer != ws->buffer ||
                              orig_buflen != ws->buflen)
                            {
                              ret = -EINVAL;
                              goto errout_with_errno;
                            }
                        }

                      if (ws->state == WEBCLIENT_STATE_CHUNKED_DATA)
                        {
                          if (ws->chunk_len == ws->chunk_received)
                            {
                              ws->state = WEBCLIENT_STATE_CHUNKED_ENDDATA;
                              ws->ndx = 0;
                            }
                        }
                    }
                  else
                    {
                      if ((ws->internal_flags & WGET_FLAG_GOT_LOCATION) == 0)
                        {
                          nerr("ERROR: Redirect w/o Location header\n");
                          ret = -EPROTO;
                          goto errout_with_errno;
                        }

                      ws->nredirect++;
                      if (ws->nredirect > CONFIG_WEBCLIENT_MAX_REDIRECT)
                        {
                          nerr("ERROR: too many redirects (%u)\n",
                               ws->nredirect);
                          ret = -ELOOP;
                          goto errout_with_errno;
                        }

                      ws->state = WEBCLIENT_STATE_CLOSE;
                      ws->redirected = 1;
                      break;
                    }
                }

              if (ws->state == WEBCLIENT_STATE_TUNNEL_ESTABLISHED)
                {
                  break;
                }
            }
        }

      if (ws->state == WEBCLIENT_STATE_CLOSE)
        {
          webclient_conn_close(conn);
          ws->need_conn_close = false;
          if (ws->redirected)
            {
              ws->state = WEBCLIENT_STATE_SOCKET;
            }
          else
            {
              ws->state = WEBCLIENT_STATE_DONE;
            }
        }
    }
  while (ws->state != WEBCLIENT_STATE_DONE &&
         ws->state != WEBCLIENT_STATE_TUNNEL_ESTABLISHED);

  if (ws->state == WEBCLIENT_STATE_DONE)
    {
      free_ws(ws);
      _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
    }
  else
    {
      _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_TUNNEL_ESTABLISHED);
    }

  return OK;

errout_with_errno:
  if ((ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0 &&
      (ret == -EAGAIN || ret == -EINPROGRESS || ret == -EALREADY))
    {
      if (ret == -EINPROGRESS || ret == -EALREADY)
        {
          conn->flags |= CONN_WANT_WRITE;
        }

      _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_IN_PROGRESS);
      return -EAGAIN;
    }

  if (ws->need_conn_close)
    {
      webclient_conn_close(conn);
    }

  free_ws(ws);
  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
  return ret;
}

/****************************************************************************
 * Name: webclient_abort
 *
 * Description:
 *  This function is used to free the resources used by webclient_perform()
 *  in case of non blocking I/O.
 *
 *  After webclient_perform() returned -EAGAIN, the application can either
 *  retry the operation by calling webclient_perform() again or abort
 *  the operation by calling this function.
 *
 ****************************************************************************/

void webclient_abort(FAR struct webclient_context *ctx)
{
  _CHECK_STATE(ctx, WEBCLIENT_CONTEXT_STATE_IN_PROGRESS);
  DEBUGASSERT((ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0);

  struct wget_s *ws = ctx->ws;

  if (ws == NULL)
    {
      return;
    }

  if (ws->need_conn_close)
    {
      struct webclient_conn_s *conn = ws->conn;

      webclient_conn_close(conn);
    }

  if (ws->tunnel != NULL)
    {
      webclient_abort(ws->tunnel);
    }

  free_ws(ws);
  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_ABORTED);
}

/****************************************************************************
 * Name: web_post_str
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
char *web_post_str(FAR char *buffer, int *size, FAR char *name,
                   FAR char *value)
{
  char *dst = buffer;
  buffer = wget_strcpy(buffer, name);
  buffer = wget_strcpy(buffer, "=");
  buffer = wget_urlencode_strcpy(buffer, value);
  *size  = buffer - dst;
  return dst;
}
#endif

/****************************************************************************
 * Name: web_post_strlen
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
int web_post_strlen(FAR char *name, FAR char *value)
{
  return strlen(name) + urlencode_len(value, strlen(value)) + 1;
}
#endif

/****************************************************************************
 * Name: web_posts_str
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
char *web_posts_str(FAR char *buffer, int *size, FAR char **name,
                    FAR char **value, int len)
{
  char *dst = buffer;
  int wlen;
  int i;

  for (i = 0; i < len; i++)
    {
      if (i > 0)
        {
          buffer = wget_strcpy(buffer, "&");
        }

      wlen    = *size;
      buffer  = web_post_str(buffer, &wlen, name[i], value[i]);
      buffer += wlen;
    }

  *size = buffer - dst;
  return dst;
}
#endif

/****************************************************************************
 * Name: web_posts_strlen
 ****************************************************************************/

#ifdef WGET_USE_URLENCODE
int web_posts_strlen(FAR char **name, FAR char **value, int len)
{
  int wlen = 0;
  int i;

  for (i = 0; i < len; i++)
    {
      wlen += web_post_strlen(name[i], value[i]);
    }

  return wlen + len - 1;
}
#endif

/****************************************************************************
 * Name: wget
 *
 * Description:
 *   Obtain the requested file from an HTTP server using the GET method.
 *
 * Input Parameters
 *   url      - A pointer to a string containing either the full URL to
 *              the file to get (e.g., http://www.nutt.org/index.html, or
 *              http://192.168.23.1:80/index.html).
 *   buffer   - A user provided buffer to receive the file data (also
 *              used for the outgoing GET request
 *   buflen   - The size of the user provided buffer
 *   callback - As data is obtained from the host, this function is
 *              to dispose of each block of file data as it is received.
 *   arg      - User argument passed to callback.
 *
 * Returned Value:
 *   0: if the GET operation completed successfully;
 *  -1: On a failure with errno set appropriately
 *
 ****************************************************************************/

int wget(FAR const char *url, FAR char *buffer, int buflen,
         wget_callback_t callback, FAR void *arg)
{
  struct webclient_context ctx;
  int ret;
  webclient_set_defaults(&ctx);
  ctx.method = "GET";
  ctx.url = url;
  ctx.buffer = buffer;
  ctx.buflen = buflen;
  ctx.callback = callback;
  ctx.sink_callback_arg = arg;
  ret = webclient_perform(&ctx);
  if (ret < 0)
    {
      errno = -ret;
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: wget_post
 ****************************************************************************/

int wget_post(FAR const char *url, FAR const char *posts, FAR char *buffer,
              int buflen, wget_callback_t callback, FAR void *arg)
{
  static const char *headers = g_httpform;
  struct webclient_context ctx;
  int ret;
  webclient_set_defaults(&ctx);
  ctx.method = "POST";
  ctx.url = url;
  ctx.buffer = buffer;
  ctx.buflen = buflen;
  ctx.callback = callback;
  ctx.sink_callback_arg = arg;
  ctx.headers = &headers;
  ctx.nheaders = 1;
  webclient_set_static_body(&ctx, posts, strlen(posts));
  ret = webclient_perform(&ctx);
  if (ret < 0)
    {
      errno = -ret;
      return ERROR;
    }

  return OK;
}

/****************************************************************************
 * Name: webclient_set_defaults
 ****************************************************************************/

void webclient_set_defaults(FAR struct webclient_context *ctx)
{
  memset(ctx, 0, sizeof(*ctx));
  ctx->protocol_version = WEBCLIENT_PROTOCOL_VERSION_HTTP_1_0;
  ctx->method = "GET";
  ctx->timeout_sec = CONFIG_WEBCLIENT_TIMEOUT;
  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_INITIALIZED);
}

/****************************************************************************
 * Name: webclient_set_static_body
 ****************************************************************************/

void webclient_set_static_body(FAR struct webclient_context *ctx,
                               FAR const void *body,
                               size_t bodylen)
{
  _CHECK_STATE(ctx, WEBCLIENT_CONTEXT_STATE_INITIALIZED);

  ctx->body_callback = webclient_static_body_func;
  ctx->body_callback_arg = (void *)body; /* discard const */
  ctx->bodylen = bodylen;
}

/****************************************************************************
 * Name: webclient_get_poll_info
 *
 * Description:
 *   This function retrieves the information necessary
 *   to wait for events when using non blocking I/O.
 *
 *   When using WEBCLIENT_FLAG_NON_BLOCKING, webclient_perform() can
 *   return -EAGAIN when it would otherwise block for I/O.
 *   In that case, the application can use this function to
 *   get the information necessary to wait for the I/O events
 *   using poll()/select(), by populating the the given
 *   webclient_poll_info structure with the information.
 *
 *   The following is an example to use this function to handle EAGAIN.
 *
 *        retry:
 *          ret = webclient_perform(&ctx);
 *          if (ret == -EAGAIN)
 *            {
 *              struct webclient_poll_info info;
 *              struct pollfd pfd;
 *
 *              ret = webclient_get_poll_info(&ctx, &info);
 *              if (ret != 0)
 *                {
 *                  ...
 *                }
 *
 *              memset(&pfd, 0, sizeof(pfd));
 *              pfd.fd = info.fd;
 *              if ((info.flags & WEBCLIENT_POLL_INFO_WANT_READ) != 0)
 *                {
 *                  pfd.events |= POLLIN;
 *                }
 *
 *              if ((info.flags & WEBCLIENT_POLL_INFO_WANT_WRITE) != 0)
 *                {
 *                  pfd.events |= POLLOUT;
 *                }
 *
 *              ret = poll(&pfd, 1, -1);
 *              if (ret != 0)
 *                {
 *                  ...
 *                }
 *
 *              goto retry;
 *            }
 *
 ****************************************************************************/

int webclient_get_poll_info(FAR struct webclient_context *ctx,
                             FAR struct webclient_poll_info *info)
{
  struct wget_s *ws;
  struct webclient_conn_s *conn;

  _CHECK_STATE(ctx, WEBCLIENT_CONTEXT_STATE_IN_PROGRESS);
  DEBUGASSERT((ctx->flags & WEBCLIENT_FLAG_NON_BLOCKING) != 0);

  ws = ctx->ws;
  if (ws == NULL)
    {
      return -EINVAL;
    }

  if (ws->tunnel != NULL)
    {
      return webclient_get_poll_info(ws->tunnel, info);
    }

  conn = ws->conn;
  if (conn->tls)
    {
      return ctx->tls_ops->get_poll_info(ctx->tls_ctx, conn->tls_conn, info);
    }

  info->fd = conn->sockfd;
  info->flags = conn->flags & (CONN_WANT_READ | CONN_WANT_WRITE);
  conn->flags &= ~(CONN_WANT_READ | CONN_WANT_WRITE);
  return 0;
}

/****************************************************************************
 * Name: webclient_get_tunnel
 *
 * Description:
 *   This function is used to get the webclient_conn_s, which describes
 *   the tunneled connection.
 *
 *   This function should be used exactly once after a successful
 *   call of webclient_perform with WEBCLIENT_FLAG_TUNNEL, with
 *   http_status 2xx.
 *
 *   This function also disposes the given webclient_context.
 *   The context will be invalid after a call of this function.
 *
 *   It's the caller's responsibility to close the returned
 *   webclient_conn_s, either using webclient_conn_close() or
 *   using the internal knowledge about the structure. (E.g.
 *   It's sometimes convenient/efficient for the caller to
 *   only keep conn->sockfd descriptor and free the rest of the
 *   structure using webclient_conn_free(). In that case, it will
 *   need to close() the descriptor after finishing on it.)
 *
 *   It's the caller's responsibility to free the returned
 *   webclient_conn_s using webclient_conn_free().
 *
 ****************************************************************************/

void webclient_get_tunnel(FAR struct webclient_context *ctx,
                          FAR struct webclient_conn_s **connp)
{
  struct wget_s *ws;
  struct webclient_conn_s *conn;

  _CHECK_STATE(ctx, WEBCLIENT_CONTEXT_STATE_TUNNEL_ESTABLISHED);
  DEBUGASSERT(ctx->http_status / 100 == 2);
  ws = ctx->ws;
  DEBUGASSERT(ws != NULL);
  conn = ws->conn;
  DEBUGASSERT(conn != NULL);
  *connp = conn;
  ws->conn = NULL;
  free_ws(ws);
  _SET_STATE(ctx, WEBCLIENT_CONTEXT_STATE_DONE);
}
