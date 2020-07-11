/****************************************************************************
 * apps/include/netutils/webclient.h
 * Header file for the HTTP client
 *
 *   Copyright (C) 2007, 2009, 2011, 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Based remotely on the uIP webclient which also has a BSD style license:
 *
 *   Author: Adam Dunkels <adam@dunkels.com>
 *   Copyright (c) 2002, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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

#ifndef __APPS_INCLUDE_NETUTILS_WEBCLIENT_H
#define __APPS_INCLUDE_NETUTILS_WEBCLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_WEBCLIENT_MAXHTTPLINE
#  define CONFIG_WEBCLIENT_MAXHTTPLINE 200
#endif

#ifndef CONFIG_WEBCLIENT_MAXMIMESIZE
#  define CONFIG_WEBCLIENT_MAXMIMESIZE 32
#endif

#ifndef CONFIG_WEBCLIENT_MAXHOSTNAME
#  define CONFIG_WEBCLIENT_MAXHOSTNAME 40
#endif

#ifndef CONFIG_WEBCLIENT_MAXFILENAME
#  define CONFIG_WEBCLIENT_MAXFILENAME 100
#endif

#if defined(CONFIG_NETUTILS_CODECS)
#  if defined(CONFIG_CODECS_URLCODE)
#    define WGET_USE_URLENCODE 1
#  endif
#endif

/****************************************************************************
 * Public types
 ****************************************************************************/

/* wget calls a user provided function of the following type to process
 * each received chuck of the incoming file data.  If the system has a file
 * system, then it may just write the data to a file.  Or it may buffer the
 * file in memory.  To facilitate this latter case, the caller may modify
 * the buffer address in this callback by writing to buffer and buflen. This
 * may be used, for example, to implement double buffering.
 *
 * Input Parameters:
 *   buffer - A pointer to a pointer to a buffer.  If the callee wishes to
 *       change the buffer address, it may do so in the callback by writing
 *       to buffer.
 *   offset - Offset to the beginning of valid data in the buffer.  Offset
 *       is used to skip over any HTTP header info that may be at the
 *       beginning of the buffer.
 *   datend - The end+1 offset of valid data in the buffer.  The total number
 *       of valid bytes is datend - offset.
 *   buflen - A pointer to the length of the buffer.  If the callee wishes
 *       to change the size of the buffer, it may write to buflen.
 *   arg    - User argument passed to callback.
 */

typedef void (*wget_callback_t)(FAR char **buffer, int offset,
                                int datend, FAR int *buflen, FAR void *arg);

/* webclient_sink_callback_t: callback to consume data
 *
 * Same as wget_callback_t, but allowed to fail.
 *
 * Input Parameters:
 *   Same as wget_callback_t
 *
 * Return value:
 *   0 on success.
 *   A negative errno on error.
 */

typedef CODE int (*webclient_sink_callback_t)(FAR char **buffer, int offset,
                                              int datend, FAR int *buflen,
                                              FAR void *arg);

/* webclient_body_callback_t: a callback to provide request body
 *
 * This callback can be called multiple times to provide
 * webclient_context::bodylen bytes of the data.
 * It's the responsibility of this callback to maintain the current
 * "offset" of the data. (similarly to fread(3))
 *
 * An implementation of this callback should perform either of
 * the followings:
 *
 * - fill the buffer (specified by buffer and *sizep) with the data
 * - update *datap with a buffer filled with the data
 *
 * Either ways, it should update *sizep to the size of the data.
 *
 * A short result is allowed. In that case, this callback will be called
 * again to provide the remaining data.
 *
 * Input Parameters:
 *   buffer - The buffer to fill.
 *   sizep  - The size of buffer/data in bytes.
 *   datap  - The data to return.
 *   ctx    - The value of webclient_context::body_callback_arg.
 *
 * Return value:
 *   0 on success.
 *   A negative errno on error.
 */

typedef CODE int (*webclient_body_callback_t)(
    FAR void *buffer,
    FAR size_t *sizep,
    FAR const void * FAR *datap,
    FAR void *ctx);

struct webclient_tls_connection;

struct webclient_tls_ops
{
  CODE int (*connect)(FAR void *ctx,
                      FAR const char *hostname, FAR const char *port,
                      unsigned int timeout_second,
                      FAR struct webclient_tls_connection **connp);
  CODE ssize_t (*send)(FAR void *ctx,
                       FAR struct webclient_tls_connection *conn,
                       FAR const void *buf, size_t len);
  CODE ssize_t (*recv)(FAR void *ctx,
                       FAR struct webclient_tls_connection *conn,
                       FAR void *buf, size_t len);
  CODE int (*close)(FAR void *ctx,
                    FAR struct webclient_tls_connection *conn);
};

struct webclient_context
{
  /* request parameters
   *
   *   method       - HTTP method like "GET", "POST".
   *                  The default value is "GET".
   *   url          - A pointer to a string containing the full URL.
   *                  (e.g., http://www.nutt.org/index.html, or
   *                   http://192.168.23.1:80/index.html)
   *   headers      - An array of pointers to the extra headers.
   *   nheaders     - The number of elements in the "headers" array.
   *   bodylen      - The size of the request body.
   */

  FAR const char *method;
  FAR const char *url;
  FAR const char * FAR const *headers;
  unsigned int nheaders;
  size_t bodylen;

  /* other parameters
   *
   *   buffer            - A user provided buffer to receive the file data
   *                       (also used for the outgoing GET request)
   *                       It should be large enough to hold the whole
   *                       request header. It should also be large enough
   *                       to hold the whole response header.
   *   buflen            - The size of the user provided buffer
   *   sink_callback     - As data is obtained from the host, this function
   *                       is to dispose of each block of file data as it is
   *                       received.
   *   callback          - a compat version of sink_callback.
   *   sink_callback_arg - User argument passed to callback.
   *   body_callback     - A callback function to provide the request body.
   *   body_callback_arg - User argument passed to body_callback.
   *   tls_ops           - A vector to implement TLS operations.
   *                       NULL means no https support.
   *   tls_ctx           - A user pointer to be passed to tls_ops as it is.
   */

  FAR char *buffer;
  int buflen;
  wget_callback_t callback;
  webclient_sink_callback_t sink_callback;
  FAR void *sink_callback_arg;
  webclient_body_callback_t body_callback;
  FAR void *body_callback_arg;
  FAR const struct webclient_tls_ops *tls_ops;
  FAR void *tls_ctx;

  /* results
   *
   *   http_status     - HTTP status code
   *   http_reason     - A buffer to store HTTP reason phrase.
   *                     If NULL, the reason phrase will be discarded.
   *   http_reason_len - The size of the http_reason buffer
   *                     A reason phrase longer than the buffer size
   *                     will be silently truncated.
   */

  unsigned int http_status;
  FAR char *http_reason;
  size_t http_reason_len;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#ifdef WGET_USE_URLENCODE
FAR char *web_post_str(FAR char *buffer, FAR int *size, FAR char *name,
                       FAR char *value);
FAR char *web_posts_str(FAR char *buffer, FAR int *size, FAR char **name,
                        FAR char **value, int len);
int web_post_strlen(FAR char *name, FAR char *value);
int web_posts_strlen(FAR char **name, FAR char **value, int len);
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
         wget_callback_t callback, FAR void *arg);

int wget_post(FAR const char *url, FAR const char *posts, FAR char *buffer,
              int buflen, wget_callback_t callback, FAR void *arg);

void webclient_set_defaults(FAR struct webclient_context *ctx);
int webclient_perform(FAR struct webclient_context *ctx);
void webclient_set_static_body(FAR struct webclient_context *ctx,
                               FAR const void *body,
                               size_t bodylen);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_WEBCLIENT_H */
