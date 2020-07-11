/****************************************************************************
 * netutils/webclient/webclient.c
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

#include <arpa/inet.h>
#include <netinet/in.h>

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

#define WEBCLIENT_STATE_STATUSLINE 0
#define WEBCLIENT_STATE_HEADERS    1
#define WEBCLIENT_STATE_DATA       2
#define WEBCLIENT_STATE_CLOSE      3

#define HTTPSTATUS_NONE            0
#define HTTPSTATUS_OK              1
#define HTTPSTATUS_MOVED           2
#define HTTPSTATUS_ERROR           3

#define ISO_NL                     0x0a
#define ISO_CR                     0x0d
#define ISO_SPACE                  0x20

#define WGET_MODE_GET              0
#define WGET_MODE_POST             1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct wget_s
{
  /* Internal status */

  uint8_t state;
  uint8_t httpstatus;

  uint16_t port;     /* The port number to use in the connection */

  /* These describe the just-received buffer of data */

  FAR char *buffer;  /* user-provided buffer */
  int buflen;        /* Length of the user provided buffer */
  int offset;        /* Offset to the beginning of interesting data */
  int datend;        /* Offset+1 to the last valid byte of data in the buffer */

  /* Buffer HTTP header data and parse line at a time */

  char line[CONFIG_WEBCLIENT_MAXHTTPLINE];
  int  ndx;

#ifdef CONFIG_WEBCLIENT_GETMIMETYPE
  char mimetype[CONFIG_WEBCLIENT_MAXMIMESIZE];
#endif
  char scheme[sizeof("https") + 1];
  char hostname[CONFIG_WEBCLIENT_MAXHOSTNAME];
  char filename[CONFIG_WEBCLIENT_MAXFILENAME];
};

struct conn
{
  bool tls;
  int sockfd;
  struct webclient_tls_connection *tls_conn;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_http10[]          = "HTTP/1.0";
static const char g_http11[]          = "HTTP/1.1";
#ifdef CONFIG_WEBCLIENT_GETMIMETYPE
static const char g_httpcontenttype[] = "content-type: ";
#endif
static const char g_httphost[]        = "host: ";
static const char g_httplocation[]    = "location: ";

static const char g_httpuseragentfields[] =
  "Connection: close\r\n"
  "User-Agent: "
  CONFIG_NSH_WGET_USERAGENT
  "\r\n\r\n";

static const char g_httpcrnl[]      = "\r\n";

static const char g_httpform[]      = "Content-Type: "
                                      "application/x-www-form-urlencoded";
static const char g_httpcontsize[]  = "Content-Length: ";
#if 0
static const char g_httpconn[]      = "Connection: Keep-Alive";
static const char g_httpcache[]     = "Cache-Control: no-cache";
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: conn_send
 ****************************************************************************/

static ssize_t conn_send(struct webclient_context *ctx, struct conn *conn,
                         const void *buffer, size_t len)
{
  if (conn->tls)
    {
      return ctx->tls_ops->send(ctx->tls_ctx, conn->tls_conn, buffer, len);
    }
  else
    {
      ssize_t ret = send(conn->sockfd, buffer, len, 0);
      if (ret == -1)
        {
          return -errno;
        }

      return ret;
    }
}

/****************************************************************************
 * Name: conn_send_all
 ****************************************************************************/

static int conn_send_all(struct webclient_context *ctx, struct conn *conn,
                         const void *buffer, size_t len)
{
  size_t left = len;
  ssize_t ret;

  while (left > 0)
    {
      ret = conn_send(ctx, conn, buffer, left);
      if (ret < 0)
        {
          goto errout;
        }

      buffer += ret;
      left -= ret;
    }

  return len;
errout:
  return ret;
}

/****************************************************************************
 * Name: conn_recv
 ****************************************************************************/

static ssize_t conn_recv(struct webclient_context *ctx, struct conn *conn,
                         void *buffer, size_t len)
{
  if (conn->tls)
    {
      return ctx->tls_ops->recv(ctx->tls_ctx, conn->tls_conn, buffer, len);
    }
  else
    {
      ssize_t ret = recv(conn->sockfd, buffer, len, 0);
      if (ret == -1)
        {
          return -errno;
        }

      return ret;
    }
}

/****************************************************************************
 * Name: conn_close
 ****************************************************************************/

static void conn_close(struct webclient_context *ctx, struct conn *conn)
{
  if (conn->tls)
    {
      ctx->tls_ops->close(ctx->tls_ctx, conn->tls_conn);
    }
  else
    {
      close(conn->sockfd);
    }
}

/****************************************************************************
 * Name: webclient_static_body_func
 ****************************************************************************/

static int webclient_static_body_func(FAR void *buffer,
                                      FAR size_t *sizep,
                                      FAR const void * FAR *datap,
                                      FAR void *ctx)
{
  *datap = ctx;
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
      ws->line[ndx] = ws->buffer[offset];
      if (ws->line[ndx] == ISO_NL)
        {
          ws->line[ndx] = '\0';
          if ((strncmp(ws->line, g_http10, strlen(g_http10)) == 0) ||
              (strncmp(ws->line, g_http11, strlen(g_http11)) == 0))
            {
              unsigned long http_status;
              char *ep;

              dest = &(ws->line[9]);
              ws->httpstatus = HTTPSTATUS_NONE;

              errno = 0;
              http_status = strtoul(dest, &ep, 10);
              if (ep == dest)
                {
                  return -EINVAL;
                }

              if (*ep != ' ')
                {
                  return -EINVAL;
                }

              if (errno != 0)
                {
                  return -errno;
                }

              ctx->http_status = http_status;
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
               */

              else if ((http_status / 100) == 3)
                {
                  ws->httpstatus = HTTPSTATUS_MOVED;
                }
            }
          else
            {
              return - ECONNABORTED;
            }

          /* We're done parsing the status line, so start parsing
           * the HTTP headers.
           */

          ws->state = WEBCLIENT_STATE_HEADERS;
          break;
        }
      else
        {
          offset++;
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

static int parseurl(FAR const char *url, FAR struct wget_s *ws)
{
  struct url_s url_s;
  int ret;

  memset(&url_s, 0, sizeof(url_s));
  url_s.scheme = ws->scheme;
  url_s.schemelen = sizeof(ws->scheme);
  url_s.host = ws->hostname;
  url_s.hostlen = sizeof(ws->hostname);
  url_s.path = ws->filename;
  url_s.pathlen = sizeof(ws->filename);
  ret = netlib_parseurl(url, &url_s);
  if (ret < 0)
    {
      return ret;
    }

  if (url_s.port == 0)
    {
      if (!strcmp(ws->scheme, "https"))
        {
          ws->port = 443;
        }
      else
        {
          ws->port = 80;
        }
    }
  else
    {
      ws->port = url_s.port;
    }

  return 0;
}

/****************************************************************************
 * Name: wget_parseheaders
 ****************************************************************************/

static inline int wget_parseheaders(struct wget_s *ws)
{
  int offset;
  int ndx;
  int ret = OK;

  offset = ws->offset;
  ndx    = ws->ndx;

  while (offset < ws->datend)
    {
      ws->line[ndx] = ws->buffer[offset];
      if (ws->line[ndx] == ISO_NL)
        {
          /* We have an entire HTTP header line in s.line, so
           * we parse it.
           */

          if (ndx > 0) /* Should always be true */
            {
              if (ws->line[0] == ISO_CR)
                {
                  /* This was the last header line (i.e., and empty "\r\n"),
                   * so we are done with the headers and proceed with the
                   * actual data.
                   */

                  ws->state = WEBCLIENT_STATE_DATA;
                  goto exit;
                }

              /* Truncate the trailing \r\n */

              ws->line[ndx - 1] = '\0';

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
                  ret = parseurl(ws->line + strlen(g_httplocation), ws);
                  ninfo("New hostname='%s' filename='%s'\n",
                        ws->hostname, ws->filename);
                }
            }

          /* We're done parsing this line, so we reset the index to the start
           * of the next line.
           */

          ndx = 0;
        }
      else
        {
          ndx++;
        }

      offset++;
    }

exit:
  ws->offset = ++offset;
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
 * Name: webclient_perform
 *
 * Returned Value:
 *               0: if the operation completed successfully;
 *  Negative errno: On a failure
 *
 ****************************************************************************/

int webclient_perform(FAR struct webclient_context *ctx)
{
  struct sockaddr_in server;
  struct wget_s *ws;
  struct timeval tv;
  bool redirected;
  unsigned int nredirect = 0;
  char *dest;
  char *ep;
  struct conn conn;
  FAR const struct webclient_tls_ops *tls_ops = ctx->tls_ops;
  FAR const char *method = ctx->method;
  FAR void *tls_ctx = ctx->tls_ctx;
  unsigned int i;
  int len;
  int ret;

  /* Initialize the state structure */

  ws = calloc(1, sizeof(struct wget_s));
  if (!ws)
    {
      return -errno;
    }

  ws->buffer = ctx->buffer;
  ws->buflen = ctx->buflen;

  /* Parse the hostname (with optional port number) and filename
   * from the URL.
   */

  ret = parseurl(ctx->url, ws);
  if (ret != 0)
    {
      nwarn("WARNING: Malformed URL: %s\n", ctx->url);
      free(ws);
      return ret;
    }

  ninfo("hostname='%s' filename='%s'\n", ws->hostname, ws->filename);

  /* The following sequence may repeat indefinitely if we are redirected */

  do
    {
      if (!strcmp(ws->scheme, "https") && tls_ops != NULL)
        {
          conn.tls = true;
        }
      else if (!strcmp(ws->scheme, "http"))
        {
          conn.tls = false;
        }
      else
        {
          nerr("ERROR: unsupported scheme: %s\n", ws->scheme);
          free(ws);
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

      if (conn.tls)
        {
          char port_str[sizeof("65535")];

          snprintf(port_str, sizeof(port_str), "%u", ws->port);
          ret = tls_ops->connect(tls_ctx, ws->hostname, port_str,
                                 CONFIG_WEBCLIENT_TIMEOUT, &conn.tls_conn);
        }
      else
        {
          /* Create a socket */

          conn.sockfd = socket(AF_INET, SOCK_STREAM, 0);
          if (conn.sockfd < 0)
            {
              /* socket failed.  It will set the errno appropriately */

              nerr("ERROR: socket failed: %d\n", errno);
              free(ws);
              return -errno;
            }

          /* Set send and receive timeout values */

          tv.tv_sec  = CONFIG_WEBCLIENT_TIMEOUT;
          tv.tv_usec = 0;

          setsockopt(conn.sockfd, SOL_SOCKET, SO_RCVTIMEO,
                     (FAR const void *)&tv, sizeof(struct timeval));
          setsockopt(conn.sockfd, SOL_SOCKET, SO_SNDTIMEO,
                     (FAR const void *)&tv, sizeof(struct timeval));

          /* Get the server address from the host name */

          server.sin_family = AF_INET;
          server.sin_port   = htons(ws->port);
          ret = wget_gethostip(ws->hostname, &server.sin_addr);
          if (ret < 0)
            {
              /* Could not resolve host (or malformed IP address) */

              nwarn("WARNING: Failed to resolve hostname\n");
              ret = -EHOSTUNREACH;
              goto errout_with_errno;
            }

          /* Connect to server.  First we have to set some fields in the
           * 'server' address structure.  The system will assign me an
           * arbitrary local port that is not in use.
           */

          ret = connect(conn.sockfd, (struct sockaddr *)&server,
                        sizeof(struct sockaddr_in));
          if (ret == -1)
            {
              ret = -errno;
            }
        }

      if (ret < 0)
        {
          nerr("ERROR: connect failed: %d\n", errno);
          free(ws);
          return ret;
        }

      /* Send the request */

      dest = ws->buffer;
      ep = ws->buffer + ws->buflen;
      dest = append(dest, ep, method);
      dest = append(dest, ep, " ");

#ifndef WGET_USE_URLENCODE
      dest = append(dest, ep, ws->filename);
#else
      /* TODO: should we use wget_urlencode_strcpy? */

      dest = append(dest, ep, ws->filename);
#endif

      dest = append(dest, ep, " ");
      dest = append(dest, ep, g_http10);
      dest = append(dest, ep, g_httpcrnl);
      dest = append(dest, ep, g_httphost);
      dest = append(dest, ep, ws->hostname);
      dest = append(dest, ep, g_httpcrnl);

      for (i = 0; i < ctx->nheaders; i++)
        {
          dest = append(dest, ep, ctx->headers[i]);
          dest = append(dest, ep, g_httpcrnl);
        }

      if (ctx->bodylen)
        {
          char post_size[sizeof("18446744073709551615")];

          dest = append(dest, ep, g_httpcontsize);
          sprintf(post_size, "%zu", ctx->bodylen);
          dest = append(dest, ep, post_size);
          dest = append(dest, ep, g_httpcrnl);
        }

      dest = append(dest, ep, g_httpuseragentfields);

      if (dest == NULL)
        {
          ret = -E2BIG;
          goto errout_with_errno;
        }

      len = dest - ws->buffer;

      ret = conn_send_all(ctx, &conn, ws->buffer, len);

      if (ret >= 0)
        {
          size_t sent = 0;

          while (sent < ctx->bodylen)
            {
              FAR const void *input_buffer;
              size_t input_buffer_size = ws->buflen;

              size_t todo = ctx->bodylen - sent;
              if (input_buffer_size > todo)
                {
                  input_buffer_size = todo;
                }

              input_buffer = ws->buffer;
              ret = ctx->body_callback(ws->buffer,
                                       &input_buffer_size,
                                       &input_buffer,
                                       ctx->body_callback_arg);
              if (ret < 0)
                {
                  nerr("ERROR: body_callback failed: %d\n", -ret);
                  goto errout_with_errno;
                }

              ret = conn_send_all(ctx, &conn, input_buffer,
                                  input_buffer_size);
              if (ret < 0)
                {
                  break;
                }

              sent += input_buffer_size;
            }
        }

      if (ret < 0)
        {
          nerr("ERROR: send failed: %d\n", -ret);
          goto errout_with_errno;
        }

      /* Now loop to get the file sent in response to the GET.  This
       * loop continues until either we read the end of file (nbytes == 0)
       * or until we detect that we have been redirected.
       */

      ws->state  = WEBCLIENT_STATE_STATUSLINE;
      redirected = false;
      for (; ; )
        {
          ws->datend = conn_recv(ctx, &conn, ws->buffer, ws->buflen);
          if (ws->datend < 0)
            {
              ret = ws->datend;
              nerr("ERROR: recv failed: %d\n", -ret);
              goto errout_with_errno;
            }
          else if (ws->datend == 0)
            {
              ninfo("Connection lost\n");
              break;
            }

          /* Handle initial parsing of the status line */

          ws->offset = 0;
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
              ret = wget_parseheaders(ws);
              if (ret < 0)
                {
                  goto errout_with_errno;
                }
            }

          /* Dispose of the data payload */

          if (ws->state == WEBCLIENT_STATE_DATA)
            {
              if (ws->httpstatus != HTTPSTATUS_MOVED)
                {
                  /* Let the client decide what to do with the
                   * received file.
                   */

                  if (ctx->sink_callback)
                    {
                      ret = ctx->sink_callback(&ws->buffer, ws->offset,
                                               ws->datend, &ws->buflen,
                                               ctx->sink_callback_arg);
                      if (ret != 0)
                        {
                          goto errout_with_errno;
                        }
                    }
                  else
                    {
                      ctx->callback(&ws->buffer, ws->offset, ws->datend,
                                    &ws->buflen, ctx->sink_callback_arg);
                    }
                }
              else
                {
                  redirected = true;
                  nredirect++;
                  if (nredirect > CONFIG_WEBCLIENT_MAX_REDIRECT)
                    {
                      nerr("ERROR: too many redirects (%u)\n", nredirect);
                      goto errout;
                    }

                  break;
                }
            }
        }

      conn_close(ctx, &conn);
    }
  while (redirected);

  free(ws);
  return OK;

errout:
  ret = -errno;
errout_with_errno:
  conn_close(ctx, &conn);

  free(ws);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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
  ctx->method = "GET";
}

/****************************************************************************
 * Name: webclient_set_static_body
 ****************************************************************************/

void webclient_set_static_body(FAR struct webclient_context *ctx,
                               FAR const void *body,
                               size_t bodylen)
{
  ctx->body_callback = webclient_static_body_func;
  ctx->body_callback_arg = (void *)body; /* discard const */
  ctx->bodylen = bodylen;
}
