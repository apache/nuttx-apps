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

#include <stdbool.h>
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

/* The following WEBCLIENT_FLAG_xxx constants are for
 * webclient_context::flags.
 */

/* WEBCLIENT_FLAG_NON_BLOCKING tells webclient_perform() to
 * use non-blocking I/O.
 *
 * If this flag is set, webclient_perform() returns -EAGAIN
 * when it would otherwise block for network I/O. In that case,
 * the application should either retry the operation later by calling
 * webclient_perform() again, or abort it by calling webclient_abort().
 * It can also use webclient_get_poll_info() to avoid busy-retrying.
 *
 * If this flag is set, it's the application's responsibility to
 * implement a timeout.
 *
 * If the application specifies tls_ops, it's the application's
 * responsibility to make the TLS implementation to use non-blocking I/O
 * in addition to specifying this flag.
 *
 * Caveat: Even when this flag is set, the current implementation performs
 * the name resolution in a blocking manner.
 */

#define	WEBCLIENT_FLAG_NON_BLOCKING	1U

/* The following WEBCLIENT_FLAG_xxx constants are for
 * webclient_poll_info::flags.
 */

#define	WEBCLIENT_POLL_INFO_WANT_READ	1U
#define	WEBCLIENT_POLL_INFO_WANT_WRITE	2U

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

/* webclient_sink_callback_t: callback to consume body data
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

/* webclient_header_callback_t: callback to consume header data
 *
 * Input Parameters:
 *   line        - A NULL-terminated string containing a header line.
 *   truncated   - Flag for indicating whether the received header line is
 *                 truncated for exceeding the CONFIG_WEBCLIENT_MAXHTTPLINE
 *                 length limit.
 *   arg         - User argument passed to callback.
 */

typedef CODE int (*webclient_header_callback_t)(FAR const char *line,
                                                bool truncated,
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
 * - Fill the buffer (specified by buffer and *sizep) with the data
 *
 * - Update *datap with a buffer filled with the data. In this case,
 *   it can return more than the amount specified *sizep.
 *
 * Either ways, it should update *sizep to the size of the data.
 *
 * A short result is allowed. In that case, this callback will be called
 * again to provide the remaining data.
 *
 * Input Parameters:
 *   buffer  - The buffer to fill.
 *   sizep   - The size of buffer/data in bytes.
 *   datap   - The data to return.
 *   reqsize - The requested size.
 *             Note: This can be larger than *sizep.
 *             The callback can choose either of:
 *               * return more than *sizep data by updating *datap
 *                 with a large enough buffer
 *               * or, just return up to *sizep. (For the rest of data,
 *                 the callback will be called again later.)
 *   ctx     - The value of webclient_context::body_callback_arg.
 *
 * Return value:
 *   0 on success.
 *   A negative errno on error.
 */

typedef CODE int (*webclient_body_callback_t)(
    FAR void *buffer,
    FAR size_t *sizep,
    FAR const void * FAR *datap,
    size_t reqsize,
    FAR void *ctx);

struct webclient_tls_connection;
struct webclient_poll_info;

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
  CODE int (*get_poll_info)(FAR void *ctx,
                            FAR struct webclient_tls_connection *conn,
                            FAR struct webclient_poll_info *info);
};

/* Note on webclient_client lifetime
 *
 * (uninitialized)
 *      |
 * webclient_set_defaults
 *      |
 *      v
 *  INITIALIZED
 *      |
 *      |      IN-PROGRESS
 *      |        |
 *      |        +-------------+
 *      |        |             |
 *      |        |         webclient_abort
 *      |        |             |
 *      |        |             v
 *      |        |         ABORTED
 *      |        |
 * webclient_perform
 *      |
 *      +---------------+
 *      |               |
 *      |             non-blocking mode,
 *      |             returns -EAGAIN
 *      |               |
 *      v               v
 *     DONE           IN-PROGRESS
 *
 * (uninitialized):
 *   After the memory for webclient_context is allocated,
 *   it should be initialized with webclient_set_defaults() before
 *   feeding it to other functions taking a webclient_context.
 *
 *   webclient_abort() makes the state back to this state.
 *   If the application wants to reuse the context for another request,
 *   it should initialize it with webclient_set_defaults() again.
 *
 * INITIALIZED:
 *   After calling webclient_set_defaults(), the application can set up
 *   the request parameters by setting the struct fields before the first
 *   call of webclient_perform().
 *   E.g. url, method, buffers, and callbacks.
 *
 *   webclient_set_static_body() can only be used in this state.
 *
 * IN-PROGRESS:
 *   This state only exists for the non-blocking mode.
 *
 *   webclient_get_poll_info() can only be used in this state.
 *
 * ABORTED:
 *   The HTTP operation has been aborted by webclient_abort().
 *
 * DONE:
 *   The HTTP operation has been completed. (Either successfully or not.)
 *
 *   The application can examine the struct fields to see the result.
 *   E.g. http_status and http_reason.
 *
 * ABORTED, DONE:
 *   The application can now dispose the resources associated to the
 *   context.
 *   E.g. buffers
 *
 *   If the application wants to reuse the context for another request,
 *   it should initialize it with webclient_set_defaults() again.
 */

struct webclient_context
{
  /* request parameters
   *
   *   method           - HTTP method like "GET", "POST".
   *                      The default value is "GET".
   *   url              - A pointer to a string containing the full URL.
   *                      (e.g., http://www.nutt.org/index.html, or
   *                       http://192.168.23.1:80/index.html)
   *   unix_socket_path - If not NULL, the path to an AF_LOCAL socket.
   *   headers          - An array of pointers to the extra headers.
   *   nheaders         - The number of elements in the "headers" array.
   *   bodylen          - The size of the request body.
   *   timeout_sec      - The timeout in second.
   *                      This is not meant to cover the entire transaction.
   *                      Instead, this is meant to be an inactive timer.
   *                      That is, if no progress is made during the
   *                      specified amount of time, the operation will fail.
   *                      The default is CONFIG_WEBCLIENT_TIMEOUT, which is
   *                      10 seconds by default.
   */

  FAR const char *method;
  FAR const char *url;
#if defined(CONFIG_WEBCLIENT_NET_LOCAL)
  FAR const char *unix_socket_path;
#endif
  FAR const char * FAR const *headers;
  unsigned int nheaders;
  size_t bodylen;
  unsigned int timeout_sec;

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
   *   flags             - OR'ed WEBCLIENT_FLAG_xxx values.
   */

  FAR char *buffer;
  int buflen;
  wget_callback_t callback;
  webclient_sink_callback_t sink_callback;
  FAR void *sink_callback_arg;
  webclient_header_callback_t header_callback;
  FAR void *header_callback_arg;
  webclient_body_callback_t body_callback;
  FAR void *body_callback_arg;
  FAR const struct webclient_tls_ops *tls_ops;
  FAR void *tls_ctx;
  unsigned int flags;

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

  struct wget_s *ws;

#ifdef CONFIG_DEBUG_ASSERTIONS
  enum webclient_context_state_e
  {
    WEBCLIENT_CONTEXT_STATE_UNINITIALIZED,
    WEBCLIENT_CONTEXT_STATE_INITIALIZED,
    WEBCLIENT_CONTEXT_STATE_IN_PROGRESS,
    WEBCLIENT_CONTEXT_STATE_ABORTED,
    WEBCLIENT_CONTEXT_STATE_DONE,
  } state;
#endif
};

struct webclient_poll_info
{
  /* A file descriptor to wait for i/o. */

  int fd;
  unsigned int flags; /* OR'ed WEBCLIENT_POLL_INFO_xxx flags */
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
void webclient_abort(FAR struct webclient_context *ctx);
void webclient_set_static_body(FAR struct webclient_context *ctx,
                               FAR const void *body,
                               size_t bodylen);
int webclient_get_poll_info(FAR struct webclient_context *ctx,
                            FAR struct webclient_poll_info *info);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_WEBCLIENT_H */
