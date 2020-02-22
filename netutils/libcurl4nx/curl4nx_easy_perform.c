/****************************************************************************
 * apps/netutils/libcurl4nx/curl4nx_easy_perform.c
 * Implementation of the HTTP client, cURL like interface.
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
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
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <nuttx/version.h>

#include "netutils/netlib.h"
#include "netutils/curl4nx.h"
#include "curl4nx_private.h"

#if defined(CONFIG_NETUTILS_CODECS)
#  if defined(CONFIG_CODECS_URLCODE)
#    define WGET_USE_URLENCODE 1
#    include "netutils/urldecode.h"
#  endif
#  if defined(CONFIG_CODECS_BASE64)
#    include "netutils/base64.h"
#  endif
#else
#  undef CONFIG_CODECS_URLCODE
#  undef CONFIG_CODECS_BASE64
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CURL4NX_STATE_STATUSLINE   0
#define CURL4NX_STATE_STATUSCODE   1
#define CURL4NX_STATE_STATUSREASON 2
#define CURL4NX_STATE_HEADERS      3
#define CURL4NX_STATE_DATA_NORMAL  4
#define CURL4NX_STATE_DATA_CHUNKED 5

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_perform()
 *
 * Description:
 *  Resolve the hostname to an IP address. There are 3 cases:
 *  - direct IP: inet_aton is used
 *  - using internal curl4nx resolution
 *  - DNS resolution, if available
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_resolve
 * Description:
 *   Translate a host name to an IP address (V4 only for the moment)
 *   - either the host is a string with an IP
 *   - either the known hosts were defined by CURL4NXOPT_
 ****************************************************************************/

static int curl4nx_resolve(FAR struct curl4nx_s *handle, FAR char *hostname,
                          FAR struct in_addr *ip)
{
  int ret = inet_aton(hostname, ip);
  if (ret != 0)
    {
      return CURL4NXE_OK; /* IP address is valid */
    }

  curl4nx_warn("Not a valid IP address, trying hostname resolution\n");
  return CURL4NXE_COULDNT_RESOLVE_HOST;
}

/****************************************************************************
 * Name: curl4nx_is_header
 * Description:
 *   Return TRUE if buf contains header, then update off to point at the
 *   beginning of header value
 ****************************************************************************/

static int curl4nx_is_header(FAR char *buf, int len,
                            FAR const char *header, FAR int *off)
{
  if (strncasecmp(buf, header, strlen(header)))
    {
      return false;
    }

  *off = strlen(header);
  while ((*off) < len && buf[*off] == ' ')
    {
      (*off)++;
    }

  return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: curl4nx_easy_perform()
 ****************************************************************************/

int curl4nx_easy_perform(FAR struct curl4nx_s *handle)
{
  struct sockaddr_in server;
  FILE *             stream;     /* IOSTREAM used to printf in the socket TODO avoid */
  int                rxoff;      /* Current offset within RX buffer */
  char               tmpbuf[16]; /* Buffer to hold small strings */
  int                tmplen;     /* Number of bytes used in tmpbuffer */
  int                state;      /* Current state of the parser */
  int                ret;        /* Return value from internal calls */
  char *             headerbuf;
  int                headerlen;
  char *             end;
  int                cret = CURL4NXE_OK; /* Public return value */
  bool               redirected = false; /* Boolean to manage HTTP redirections */
  bool               chunked = false;
  unsigned long long done = 0;
  int                redircount = 0;

  curl4nx_info("started\n");

  stream = NULL;
  headerbuf = malloc(CONFIG_LIBCURL4NX_MAXHEADERLINE);
  if (!headerbuf)
    {
      cret = CURL4NXE_OUT_OF_MEMORY;
      goto abort;
    }

  if (handle->host[0] == 0 || handle->port == 0)
    {
      /* URL has not been set */

      cret = CURL4NXE_URL_MALFORMAT;
      goto freebuf;
    }

  /* TODO: check that host and port have changed or are the same, so we can
   * recycle the socket for the next request.
   */

  do
    {
      handle->sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (handle->sockfd < 0)
        {
          /* socket failed.  It will set the errno appropriately */

          curl4nx_err("ERROR: socket failed: %d\n", errno);
          cret = CURL4NXE_COULDNT_CONNECT;
          goto freebuf;
        }

      /* TODO: timeouts */

      server.sin_family = AF_INET;
      server.sin_port   = htons(handle->port);
      cret = curl4nx_resolve(handle, handle->host, &server.sin_addr);
      if (cret != CURL4NXE_OK)
        {
          /* Could not resolve host (or malformed IP address) */

          curl4nx_err("ERROR: Failed to resolve hostname\n");
          cret = CURL4NXE_COULDNT_RESOLVE_HOST;
          goto freebuf;
        }

      /* Connect to server.  First we have to set some fields in the
       * 'server' address structure.  The system will assign me an arbitrary
       * local port that is not in use.
       */

      ret = connect(handle->sockfd,
                    (struct sockaddr *)&server, sizeof(struct sockaddr_in));
      if (ret < 0)
        {
          curl4nx_err("ERROR: connect failed: %d\n", errno);
          cret = CURL4NXE_COULDNT_CONNECT;
          goto close;
        }

      curl4nx_info("Connected...\n");

      stream = fdopen(handle->sockfd, "wb");

      /* Send request */

      fprintf(stream, "%s %s HTTP/%d.%d\r\n",
                      handle->method,
                      handle->path,
                      handle->version >> 4,
                      handle->version & 0x0f);

      /* Send headers */

      fprintf(stream, "Host: %s\r\n", handle->host);

      /* For the moment we do not support compression */

      fprintf(stream, "Content-encoding: identity\r\n");

      /* Send more headers */

      /* End of headers */

      fprintf(stream, "\r\n");

      /* TODO send data */

      fflush(stream);
      curl4nx_info("Request sent\n");

      /* Now wait for the result */

      redirected = false; /* for now */
      state      = CURL4NX_STATE_STATUSLINE;
      tmplen     = 0;

      while (1)
        {
          ret = recv(handle->sockfd, handle->rxbuf, handle->rxbufsize, 0);
          if (ret < 0)
            {
              curl4nx_err("RECV failed, errno=%d\n", errno);
              cret = CURL4NXE_RECV_ERROR;
              goto close;
            }
          else if (ret == 0)
            {
              curl4nx_err("Connection lost\n");
              cret = CURL4NXE_GOT_NOTHING;
              goto close;
            }

          curl4nx_info("Received %d bytes\n", ret);
          rxoff = 0;

          if (state == CURL4NX_STATE_STATUSLINE)
            {
              /* Accumulate HTTP/x.y until space */

              while ((tmplen < sizeof(tmpbuf)) && (rxoff < ret))
                {
                  if (handle->rxbuf[rxoff] == ' ')
                    {
                      /* found space after http version */

                      tmpbuf[tmplen] = 0; /* Finish pending string */
                      rxoff++;
                      curl4nx_info("received version: [%s]\n", tmpbuf);
                      tmplen = 0; /* reset buffer to look at code */
                      state = CURL4NX_STATE_STATUSCODE;
                      break;
                    }

                  /* Not a space: accumulate chars */

                  tmpbuf[tmplen] = handle->rxbuf[rxoff];
                  tmplen++;
                  rxoff++;
                }

              /* Check for overflow, version code should not fill the tmpbuf */

              if (tmplen == sizeof(tmpbuf))
                {
                  /* extremely long http version -> invalid response */

                  curl4nx_err("Buffer overflow while reading version\n");
                  cret = CURL4NXE_RECV_ERROR;
                  goto close;
                }

              /* No overflow and no space found: wait for next buffer */
            }

          /* NO ELSE HERE, state may have changed and require new management
           * for the same rx buffer.
           */

          if (state == CURL4NX_STATE_STATUSCODE)
            {
              /* Accumulate response code until space */

              while ((tmplen < sizeof(tmpbuf)) && (rxoff < ret))
                {
                  if (handle->rxbuf[rxoff] == ' ')
                    {
                      /* Found space after http version */

                      tmpbuf[tmplen] = 0; /* Finish pending string */
                      rxoff++;
                      curl4nx_info("received code: [%s]\n", tmpbuf);
                      handle->status = strtol(tmpbuf, &end, 10);

                      if (*end != 0)
                        {
                          curl4nx_err("Bad status code [%s]\n", tmpbuf);
                          cret = CURL4NXE_RECV_ERROR;
                          goto close;
                        }

                      tmplen = 0; /* reset buffer to look at code */
                      state = CURL4NX_STATE_STATUSREASON;
                      break;
                    }

                  /* Not a space: accumulate chars */

                  tmpbuf[tmplen] = handle->rxbuf[rxoff];
                  tmplen++;
                  rxoff++;
                }

              /* Check for overflow, version code should not fill the tmpbuf */

              if (tmplen == sizeof(tmpbuf))
                {
                  /* Extremely long http code -> invalid response */

                  curl4nx_err("Buffer overflow while reading code\n");
                  cret = CURL4NXE_RECV_ERROR;
                  goto close;
                }

              /* No overflow and no space found: wait for next buffer */
            }

          if (state == CURL4NX_STATE_STATUSREASON)
            {
               /* Accumulate response code until CRLF */

              while (rxoff < ret)
                {
                  if (handle->rxbuf[rxoff] == 0x0d ||
                     handle->rxbuf[rxoff] == 0x0a)
                    {
                      /* Accumulate all contiguous CR and LF in any order */

                      tmpbuf[tmplen] = handle->rxbuf[rxoff];
                      tmplen++;
                      if (tmplen == 2)
                        {
                          if (tmpbuf[0] == 0x0d && tmpbuf[1] == 0x0a)
                            {
                              headerlen = 0;
                              handle->content_length = 0;
                              state = CURL4NX_STATE_HEADERS;
                              break;
                            }
                          else
                            {
                              tmplen = 0; /* Reset search for CRLF */
                            }
                        }
                    }
                  else
                    {
                      /* This char is not interesting: reset storage */

                      tmplen = 0;

                      /* curl4nx_info("-> %c\n", handle->rxbuf[rxoff]); */
                    }

                  rxoff++;
                }
            }

          if (state == CURL4NX_STATE_HEADERS)
            {
              while (rxoff < ret)
                {
                  if (handle->rxbuf[rxoff] == 0x0d ||
                     handle->rxbuf[rxoff] == 0x0a)
                    {
                      /* Accumulate all contiguous CR and LF in any order */

                      tmpbuf[tmplen] = handle->rxbuf[rxoff];
                      tmplen++;
                      if (tmplen == 2)
                        {
                          if (tmpbuf[0] == 0x0d && tmpbuf[1] == 0x0a)
                            {
                              if (headerlen == 0) /* Found an empty header */
                                {
                                  curl4nx_info("<End of headers>\n");
                                  rxoff++; /* Skip this char */
                                  state = chunked ?
                                           CURL4NX_STATE_DATA_CHUNKED :
                                           CURL4NX_STATE_DATA_NORMAL;
                                  break;
                                }
                              else
                                {
                                  int off;
                                  curl4nx_iofunc_f func;
                                  headerbuf[headerlen] = 0;
                                  func = handle->headerfunc;
                                  if (func == NULL)
                                    {
                                      func = handle->writefunc;
                                    }

                                  func(headerbuf, headerlen, 1,
                                       handle->headerdata);

                                  /* Find the content-length */

                                  if (curl4nx_is_header(headerbuf, headerlen,
                                                        "content-length:",
                                                        &off))
                                    {
                                      handle->content_length =
                                        strtoull(headerbuf + off, &end, 10);
                                      if (*end != 0)
                                        {
                                          curl4nx_err("Stray chars after "
                                                      "content length!\n");
                                          cret = CURL4NXE_RECV_ERROR;
                                          goto close;
                                        }

                                      curl4nx_info("Found content length: "
                                                   "%llu\n",
                                                   handle->content_length);
                                    }

                                  /* Find the transfer encoding */

                                  if (curl4nx_is_header(headerbuf, headerlen,
                                                        "transfer-encoding:",
                                                        &off))
                                    {
                                      chunked = !strncasecmp(headerbuf + off,
                                                             "chunked", 7);
                                      if (chunked)
                                        {
                                          curl4nx_info("Transfer using "
                                                       "chunked format\n");
                                        }
                                    }

                                  /* Find the location */

                                  if (curl4nx_is_header(headerbuf, headerlen,
                                                        "location:",
                                                        &off))
                                    {
                                      /* Parse the new URL if we get a
                                       * redirection code.
                                       */

                                      if (handle->status >= 300 &&
                                          handle->status < 400)
                                        {
                                          if (handle->flags &
                                             CURL4NX_FLAGS_FOLLOWLOCATION)
                                            {
                                              if ((handle->max_redirs > 0) &&
                                                  (redircount >=
                                                   handle->max_redirs))
                                                {
                                                  curl4nx_info(
                                                    "Too many redirections\n");
                                                  cret = CURL4NXE_TOO_MANY_REDIRECTS;
                                                  goto close;
                                                }

                                              cret =
                                                curl4nx_easy_setopt(handle,
                                                                    CURL4NXOPT_URL,
                                                                    headerbuf + off);
                                              if (cret != CURL4NXE_OK)
                                                {
                                                  goto close;
                                                }

                                              redirected = true;
                                              redircount += 1;
                                              curl4nx_info("REDIRECTION (%d) -> %s\n",
                                                           redircount,
                                                           headerbuf + off);
                                            }
                                        }
                                    }

                                  /* Prepare for next header */

                                  headerlen = 0;
                                  tmplen    = 0; /* Reset search for CRLF */
                                }
                            }
                          else
                            {
                              tmplen = 0; /* Reset search for CRLF */
                            }
                        }
                    }
                  else
                    {
                      tmplen = 0; /* Reset CRLF detection */

                      /* Plus one for final zero */

                      if (headerlen < (CONFIG_LIBCURL4NX_MAXHEADERLINE - 1))
                        {
                          headerbuf[headerlen] = handle->rxbuf[rxoff];
                          headerlen++;
                        }
                      else
                        {
                          curl4nx_warn("prevented header overload\n");
                        }
                    }

                  rxoff++;
                }
            }

          if (state == CURL4NX_STATE_DATA_NORMAL)
            {
              if ((handle->flags & CURL4NX_FLAGS_FAILONERROR) &&
                   handle->status >= 400)
                {
                  cret = CURL4NXE_HTTP_RETURNED_ERROR;
                  goto close;
                }

              curl4nx_info("now in normal data state, rxoff=%d rxlen=%d\n",
                           rxoff, ret);
              done += (ret - rxoff);

              handle->writefunc(handle->rxbuf + rxoff, ret - rxoff, 1,
                                handle->writedata);
              if (handle->content_length != 0)
                {
                  curl4nx_info("Done %llu of %llu\n",
                               done, handle->content_length);

                  if (handle->progressfunc)
                    {
                      handle->progressfunc(handle->progressdata,
                                           handle->content_length,
                                           done, 0, 0);
                    }

                  if (handle->content_length == done)
                    {
                      /* Transfer is complete */

                      goto close;
                    }
                }
            }

          if (state == CURL4NX_STATE_DATA_CHUNKED)
            {
              if ((handle->flags & CURL4NX_FLAGS_FAILONERROR) &&
                  handle->status >= 400)
                {
                  cret = CURL4NXE_HTTP_RETURNED_ERROR;
                  goto close;
                }

              curl4nx_info("now in chunked data state, rxoff=%d rxlen=%d\n",
                           rxoff, ret);
              curl4nx_err("Not supported yet.\n");
              goto close;
            }
        }

      /* Done with this connection - this will also close the socket */

close:
      curl4nx_info("Closing\n");
      if (stream)
        {
          fclose(stream);
        }
    }
  while (redirected);

freebuf:
  free(headerbuf);

abort:
  curl4nx_info("done\n");
  return cret;
}
