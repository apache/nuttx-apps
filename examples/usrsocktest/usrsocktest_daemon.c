/****************************************************************************
 * apps/examples/usrsocktest/usrsocktest_daemon.c
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nuttx/net/usrsock.h>

#include "defines.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifndef dbg
  #define dbg _warn
#endif

#define usrsocktest_dbg(...) ((void)0)

#define TEST_SOCKET_SOCKID_BASE 10000U
#define TEST_SOCKET_COUNT 8

#ifndef ARRAY_SIZE
#  define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define noinline

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct test_socket_s
{
  bool opened:1;
  bool connected:1;
  bool blocked_connect:1;
  bool block_send:1;
  bool connect_refused:1;
  bool disconnected:1;
  int recv_avail_bytes;
  FAR void *endp;
  struct usrsock_message_req_ack_s pending_resp;
};

struct delayed_cmd_s
{
  sq_entry_t node;
  pthread_t tid;
  sem_t startsem;
  int pipefd;
  uint16_t delay_msec;
  char cmd;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct daemon_priv_s
{
  FAR const struct usrsocktest_daemon_conf_s *conf;
  pthread_t tid;
  bool joined;

  int pipefd[2];
  sem_t wakewaitsem;
  unsigned int sockets_active;
  unsigned int sockets_connected;
  unsigned int sockets_waiting_connect;
  unsigned int sockets_recv_empty;
  unsigned int sockets_not_connected_refused;
  unsigned int sockets_remote_disconnected;
  size_t total_send_bytes;
  size_t total_recv_bytes;
  bool do_not_poll_usrsock;

  struct test_socket_s test_sockets[TEST_SOCKET_COUNT];
  sq_queue_t delayed_cmd_threads;
} g_ub_daemon =
  {
    .joined = true,
    .conf = NULL,
  };

static pthread_mutex_t daemon_mutex = PTHREAD_MUTEX_INITIALIZER;

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct usrsocktest_daemon_conf_s usrsocktest_daemon_defconf =
    USRSOCKTEST_DAEMON_CONF_DEFAULTS;
struct usrsocktest_daemon_conf_s usrsocktest_daemon_config;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_socket_alloc(FAR struct daemon_priv_s *priv)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(priv->test_sockets); i++)
    {
      FAR struct test_socket_s *tsock = &priv->test_sockets[i];

      if (!tsock->opened)
        {
          memset(tsock, 0, sizeof(*tsock));
          tsock->opened = true;
          tsock->block_send = priv->conf->endpoint_block_send;
          tsock->recv_avail_bytes =
              priv->conf->endpoint_recv_avail_from_start ?
                  priv->conf->endpoint_recv_avail : 0;
          priv->sockets_active++;
          if (tsock->recv_avail_bytes == 0)
            priv->sockets_recv_empty++;
          return i + TEST_SOCKET_SOCKID_BASE;
        }
    }

  return -1;
}

static FAR struct test_socket_s *test_socket_get(
  FAR struct daemon_priv_s *priv,
  int sockid)
{
  if (sockid < TEST_SOCKET_SOCKID_BASE)
    {
      return NULL;
    }

  sockid -= TEST_SOCKET_SOCKID_BASE;
  if (sockid >= ARRAY_SIZE(priv->test_sockets))
    {
      return NULL;
    }

  return &priv->test_sockets[sockid];
}

static int test_socket_free(FAR struct daemon_priv_s *priv, int sockid)
{
  FAR struct test_socket_s *tsock = test_socket_get(priv, sockid);

  if (!tsock)
    {
      return -EBADFD;
    }

  if (!tsock->opened)
    {
      return -EFAULT;
    }

  if (tsock->connected)
    {
      priv->sockets_connected--;
      tsock->connected = false;
    }

  if (tsock->blocked_connect)
    {
      priv->sockets_waiting_connect--;
      tsock->blocked_connect = false;
    }

  if (tsock->endp)
    {
      free(tsock->endp);
      usrsocktest_endp_malloc_cnt--;
      tsock->endp = NULL;
    }

  if (tsock->recv_avail_bytes == 0)
    {
      priv->sockets_recv_empty--;
    }

  if (tsock->connect_refused)
    {
      priv->sockets_not_connected_refused--;
    }

  if (tsock->disconnected)
    {
      priv->sockets_remote_disconnected--;
    }

  tsock->opened = false;
  priv->sockets_active--;

  return 0;
}

static int tsock_send_event(int fd, FAR struct daemon_priv_s *priv,
                            FAR struct test_socket_s *tsock, int events)
{
  ssize_t wlen;
  int i;
  FAR struct usrsock_message_socket_event_s event = {
  };

  event.head.flags = USRSOCK_MESSAGE_FLAG_EVENT;
  event.head.msgid = USRSOCK_MESSAGE_SOCKET_EVENT;

  for (i = 0; i < ARRAY_SIZE(priv->test_sockets); i++)
    {
      if (tsock == &priv->test_sockets[i])
        break;
    }

  if (i == ARRAY_SIZE(priv->test_sockets))
    {
      return -EINVAL;
    }

  event.usockid = i + TEST_SOCKET_SOCKID_BASE;
  event.head.events = events;

  wlen = write(fd, &event, sizeof(event));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(event))
    {
      return -ENOSPC;
    }

  return OK;
}

static FAR void *find_endpoint(FAR struct daemon_priv_s *priv,
                               in_addr_t ipaddr)
{
  FAR struct sockaddr_in *endpaddr;
  int ok;

  endpaddr = malloc(sizeof(*endpaddr));
  usrsocktest_endp_malloc_cnt++;
  assert(endpaddr);

  ok = inet_pton(AF_INET, priv->conf->endpoint_addr,
                 &endpaddr->sin_addr.s_addr);
  endpaddr->sin_family = AF_INET;
  endpaddr->sin_port = htons(priv->conf->endpoint_port);
  assert(ok);

  if (endpaddr->sin_addr.s_addr == ipaddr)
    {
      return endpaddr;
    }

  free(endpaddr);
  usrsocktest_endp_malloc_cnt--;
  return NULL;
}

static bool endpoint_connect(FAR struct daemon_priv_s *priv, FAR void *endp,
                             uint16_t port)
{
  FAR struct sockaddr_in *endpaddr = endp;

  if (endpaddr->sin_port == port)
    {
      return true;
    }
  else
    {
      return false;
    }
}

static void get_endpoint_sockaddr(FAR void *endp,
                                  FAR struct sockaddr_in *endpaddr)
{
  *endpaddr = *(FAR struct sockaddr_in *)endp;
}

static ssize_t
read_req(int fd, FAR const struct usrsock_request_common_s *common_hdr,
         FAR void *req, size_t reqsize)
{
  ssize_t rlen;
  int err;

  rlen = read(fd, (uint8_t *)req + sizeof(*common_hdr),
              reqsize - sizeof(*common_hdr));
  if (rlen < 0)
    {
      err = errno;
      usrsocktest_dbg(
          "Error reading %d bytes of request: ret=%d, errno=%d\n",
          reqsize - sizeof(*common_hdr), (int)rlen, errno);
      return -err;
    }

  if (rlen + sizeof(*common_hdr) != reqsize)
    {
      return -EMSGSIZE;
    }

  return rlen;
}

static int socket_request(int fd, FAR struct daemon_priv_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_socket_s *req = hdrbuf;
  int socketid;
  ssize_t wlen;
  struct usrsock_message_req_ack_s resp = {
  };

  /* Validate input. */

  if (req->domain != priv->conf->supported_domain)
    {
      socketid = -EAFNOSUPPORT;
    }
  else if (req->type != priv->conf->supported_type ||
           req->protocol != priv->conf->supported_protocol)
    {
      socketid = -EPROTONOSUPPORT;
    }
  else if (priv->sockets_active >= priv->conf->max_sockets)
    {
      socketid = -EMFILE;
    }
  else
    {
      /* Allocate socket. */

      socketid = test_socket_alloc(priv);
      if (socketid < 0)
        socketid = -ENFILE;
    }

  /* Prepare response. */

  resp.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp.head.flags  = 0;
  resp.head.events = 0;
  resp.xid = req->head.xid;
  resp.result = socketid;

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  return OK;
}

static int close_request(int fd, FAR struct daemon_priv_s *priv,
                         FAR void *hdrbuf)
{
  FAR struct usrsock_request_close_s *req = hdrbuf;
  ssize_t wlen;
  int ret;
  struct usrsock_message_req_ack_s resp = {
  };

  /* Check if this socket exists. */

  ret = test_socket_free(priv, req->usockid);

  /* Prepare response. */

  resp.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp.head.events = 0;
  resp.xid = req->head.xid;
  if (priv->conf->delay_all_responses)
    {
      resp.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.result = -EAGAIN;
    }
  else
    {
      resp.head.flags = 0;
      resp.result = ret;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (priv->conf->delay_all_responses)
    {
      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.result = ret;
      resp.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }
    }

  return OK;
}

static int connect_request(int fd, FAR struct daemon_priv_s *priv,
                           FAR void *hdrbuf)
{
  FAR struct usrsock_request_connect_s *req = hdrbuf;
  struct sockaddr_in addr;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  ssize_t rlen;
  int ret = 0;
  struct usrsock_message_req_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is already connected. */

  if (tsock->connected)
    {
      ret = -EISCONN;
      goto prepare;
    }

  /* Check if address size ok. */

  if (req->addrlen > sizeof(addr))
    {
      ret = -EFAULT;
      goto prepare;
    }

  /* Read address. */

  rlen = read(fd, &addr, sizeof(addr));
  if (rlen < 0 || rlen < req->addrlen)
    {
      ret = -EFAULT;
      goto prepare;
    }

  /* Check address family. */

  if (addr.sin_family != priv->conf->supported_domain)
    {
      ret = -EAFNOSUPPORT;
      goto prepare;
    }

  /* Check if there is endpoint with target address */

  tsock->endp = find_endpoint(priv, addr.sin_addr.s_addr);
  if (!tsock->endp)
    {
      ret = -ENETUNREACH;
      goto prepare;
    }

  /* Check if there is port open at endpoint */

  if (!endpoint_connect(priv, tsock->endp, addr.sin_port))
    {
      free(tsock->endp);
      usrsocktest_endp_malloc_cnt--;
      tsock->endp = NULL;
      ret = -ECONNREFUSED;
      goto prepare;
    }

  ret = OK;

prepare:

  /* Prepare response. */

  resp.xid = req->head.xid;
  resp.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp.head.flags  = 0;
  resp.head.events = 0;

  if (priv->conf->endpoint_block_connect)
    {
      resp.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.result = ret;

      /* Mark connection as blocked */

      priv->sockets_waiting_connect++;
      tsock->blocked_connect = true;
      tsock->pending_resp = resp;
    }
  else if (priv->conf->delay_all_responses)
    {
      resp.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.result = -EINPROGRESS;
      tsock->blocked_connect = false;
    }
  else
    {
      if (ret == OK)
        {
          priv->sockets_connected++;
          tsock->connected = true;
        }

      resp.head.flags = 0;
      resp.result = ret;
      tsock->blocked_connect = false;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (priv->conf->endpoint_block_connect)
    {
      tsock->pending_resp.head.flags &=
        ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
    }
  else
    {
      int events;

      if (priv->conf->delay_all_responses)
        {
          pthread_mutex_unlock(&daemon_mutex);
          usleep(50 * 1000);
          pthread_mutex_lock(&daemon_mutex);

          /* Previous write was acknowledgment to request, informing that
           * request is still in progress. Now write actual completion
           * response.
           */

          resp.result = ret;

          if (ret == OK)
            {
              priv->sockets_connected++;
              tsock->connected = true;
            }

          resp.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;

          wlen = write(fd, &resp, sizeof(resp));
          if (wlen < 0)
            {
              return -errno;
            }

          if (wlen != sizeof(resp))
            {
              return -ENOSPC;
            }
        }

      events = 0;
      if (!tsock->block_send)
        {
          events |= USRSOCK_EVENT_SENDTO_READY;
        }

      if (tsock->recv_avail_bytes > 0)
        {
          events |= USRSOCK_EVENT_RECVFROM_AVAIL;
        }

      if (events)
        {
          wlen = tsock_send_event(fd, priv, tsock, events);
          if (wlen < 0)
            {
              return wlen;
            }
        }
    }

  return OK;
}

static int sendto_request(int fd, FAR struct daemon_priv_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_sendto_s *req = hdrbuf;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  ssize_t rlen;
  int ret = 0;
  uint8_t sendbuf[16];
  int sendbuflen = 0;
  struct usrsock_message_req_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is connected. */

  if (!tsock->connected)
    {
      ret = -ENOTCONN;
      goto prepare;
    }

  /* Check if address size non-zero. */

  if (req->addrlen > 0)
    {
      ret = -EISCONN; /* connection-mode socket do not accept address */
      goto prepare;
    }

  /* Can send? */

  if (!tsock->block_send)
    {
      /* Check if request has data. */

      if (req->buflen > 0)
        {
          sendbuflen = req->buflen;
          if (sendbuflen > sizeof(sendbuf))
            sendbuflen = sizeof(sendbuf);

          /* Read data. */

          rlen = read(fd, sendbuf, sendbuflen);
          if (rlen < 0 || rlen < sendbuflen)
            {
              ret = -EFAULT;
              goto prepare;
            }

          /* Debug print */

          usrsocktest_dbg("got %d bytes of data: '%.*s'\n",
                          sendbuflen, sendbuflen, sendbuf);
        }
    }
  else
    {
      ret = -EAGAIN; /* blocked. */
      goto prepare;
    }

  ret = sendbuflen;

prepare:

  /* Prepare response. */

  resp.xid = req->head.xid;
  resp.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp.head.flags  = 0;
  resp.head.events = 0;

  if (priv->conf->delay_all_responses)
    {
      resp.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.result = -EINPROGRESS;
    }
  else
    {
      if (ret > 0)
        {
          priv->total_send_bytes += ret;
        }

      resp.head.flags = 0;
      resp.result = ret;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (priv->conf->delay_all_responses)
    {
      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.result = ret;

      if (ret > 0)
        {
          priv->total_send_bytes += ret;
        }

      resp.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }
    }

  if (!tsock->block_send)
    {
      /* Let kernel-side know that there is space for more send data. */

      wlen = tsock_send_event(fd, priv, tsock, USRSOCK_EVENT_SENDTO_READY);
      if (wlen < 0)
        {
          return wlen;
        }
    }

  return OK;
}

static int recvfrom_request(int fd, FAR struct daemon_priv_s *priv,
                            FAR void *hdrbuf)
{
  FAR struct usrsock_request_recvfrom_s *req = hdrbuf;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  size_t i;
  int ret = 0;
  size_t outbuflen;
  struct sockaddr_in endpointaddr;
  struct usrsock_message_datareq_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is connected. */

  if (!tsock->connected)
    {
      ret = -ENOTCONN;
      goto prepare;
    }

  get_endpoint_sockaddr(tsock->endp, &endpointaddr);

  /* Do we have recv data available? */

  if (tsock->recv_avail_bytes > 0)
    {
      outbuflen = req->max_buflen;

      if (outbuflen > tsock->recv_avail_bytes)
        {
          outbuflen = tsock->recv_avail_bytes;
        }
    }
  else
    {
      ret = -EAGAIN; /* blocked. */
      goto prepare;
    }

  ret = outbuflen;

prepare:

  /* Prepare response. */

  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid  = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags  = 0;
  resp.reqack.head.events = 0;

  if (priv->conf->delay_all_responses)
    {
      resp.reqack.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.reqack.result = -EINPROGRESS;
      resp.valuelen = 0;
      resp.valuelen_nontrunc = 0;

      /* Send ack response. */

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }

      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
      resp.reqack.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
    }

  resp.reqack.head.flags = 0;
  resp.reqack.result = ret;
  if (ret >= 0)
    {
      priv->total_recv_bytes += ret;
      resp.valuelen_nontrunc = sizeof(endpointaddr);
      resp.valuelen = resp.valuelen_nontrunc;
      if (resp.valuelen > req->max_addrlen)
        resp.valuelen = req->max_addrlen;
    }
  else
    {
      resp.valuelen = 0;
      resp.valuelen_nontrunc = 0;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      wlen = write(fd, &endpointaddr, resp.valuelen);
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != resp.valuelen)
        {
          return -ENOSPC;
        }
    }

  if (resp.reqack.result > 0)
    {
      /* Send buffer */

      for (i = 0; i < resp.reqack.result; i++)
        {
          char tmp = 'a' + i;

          /* Check if MSG_PEEK flag is specified. */

          if ((req->flags & MSG_PEEK) != MSG_PEEK)
            {
              tsock->recv_avail_bytes--;
            }

          wlen = write(fd, &tmp, 1);
          if (wlen < 0)
            {
              return -errno;
            }

          if (wlen != 1)
            {
              return -ENOSPC;
            }
        }

      if (tsock->recv_avail_bytes == 0)
        {
          priv->sockets_recv_empty++;
        }
    }

  if (tsock->recv_avail_bytes > 0)
    {
      /* Let kernel-side know that there is more recv data. */

      wlen = tsock_send_event(fd, priv, tsock, USRSOCK_EVENT_RECVFROM_AVAIL);
      if (wlen < 0)
        {
          return wlen;
        }
    }

  return OK;
}

static int setsockopt_request(int fd, FAR struct daemon_priv_s *priv,
                              FAR void *hdrbuf)
{
  FAR struct usrsock_request_setsockopt_s *req = hdrbuf;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  ssize_t rlen;
  int ret = 0;
  int value;
  struct usrsock_message_req_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  if (req->level != SOL_SOCKET)
    {
      usrsocktest_dbg("setsockopt: level=%d not supported\n", req->level);
      ret = -ENOPROTOOPT;
      goto prepare;
    }

  if (req->option != SO_REUSEADDR)
    {
      usrsocktest_dbg("setsockopt: option=%d not supported\n", req->option);
      ret = -ENOPROTOOPT;
      goto prepare;
    }

  if (req->valuelen < sizeof(value))
    {
      ret = -EINVAL;
      goto prepare;
    }

  /* Read value. */

  rlen = read(fd, &value, sizeof(value));
  if (rlen < 0 || rlen < sizeof(value))
    {
      ret = -EFAULT;
      goto prepare;
    }

  /* Debug print */

  usrsocktest_dbg("setsockopt: option=%d value=%d\n", req->option, value);

  ret = OK;

prepare:

  /* Prepare response. */

  resp.xid = req->head.xid;
  resp.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp.head.flags  = 0;
  resp.head.events = 0;

  if (priv->conf->delay_all_responses)
    {
      resp.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.result = -EINPROGRESS;
    }
  else
    {
      resp.head.flags = 0;
      resp.result = ret;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (priv->conf->delay_all_responses)
    {
      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.result = ret;
      resp.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }
    }

  return OK;
}

static int getsockopt_request(int fd, FAR struct daemon_priv_s *priv,
                              FAR void *hdrbuf)
{
  FAR struct usrsock_request_getsockopt_s *req = hdrbuf;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  int ret = 0;
  int value;
  struct usrsock_message_datareq_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  if (req->level != SOL_SOCKET)
    {
      usrsocktest_dbg("getsockopt: level=%d not supported\n", req->level);
      ret = -ENOPROTOOPT;
      goto prepare;
    }

  if (req->option != SO_REUSEADDR)
    {
      usrsocktest_dbg("getsockopt: option=%d not supported\n", req->option);
      ret = -ENOPROTOOPT;
      goto prepare;
    }

  if (req->max_valuelen < sizeof(value))
    {
      ret = -EINVAL;
      goto prepare;
    }

  value = 0;
  ret = OK;

prepare:

  /* Prepare response. */

  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid  = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags  = 0;
  resp.reqack.head.events = 0;

  if (priv->conf->delay_all_responses)
    {
      resp.reqack.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.reqack.result = -EINPROGRESS;
      resp.valuelen = 0;
      resp.valuelen_nontrunc = 0;

      /* Send ack response. */

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }

      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
      resp.reqack.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
    }

  resp.reqack.head.flags = 0;
  resp.reqack.result = ret;
  if (ret >= 0)
    {
      resp.valuelen = sizeof(value);
    }
  else
    {
      resp.valuelen = 0;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      wlen = write(fd, &value, resp.valuelen);
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != resp.valuelen)
        {
          return -ENOSPC;
        }
    }

  return OK;
}

static int getsockname_request(int fd, FAR struct daemon_priv_s *priv,
                               FAR void *hdrbuf)
{
  FAR struct usrsock_request_getsockname_s *req = hdrbuf;
  FAR struct test_socket_s *tsock;
  ssize_t wlen;
  int ret = 0;
  struct sockaddr_in addr;
  struct usrsock_message_datareq_ack_s resp = {
  };

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  /* Check if this socket exists. */

  tsock = test_socket_get(priv, req->usockid);
  if (!tsock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  ret = inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12345);
  ret = ret == 1 ? 0 : -EINVAL;

prepare:

  /* Prepare response. */

  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid  = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags  = 0;
  resp.reqack.head.events = 0;

  if (priv->conf->delay_all_responses)
    {
      resp.reqack.head.flags = USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp.reqack.result = -EINPROGRESS;
      resp.valuelen = 0;
      resp.valuelen_nontrunc = 0;

      /* Send ack response. */

      wlen = write(fd, &resp, sizeof(resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(resp))
        {
          return -ENOSPC;
        }

      pthread_mutex_unlock(&daemon_mutex);
      usleep(50 * 1000);
      pthread_mutex_lock(&daemon_mutex);

      /* Previous write was acknowledgment to request, informing that request
       * is still in progress. Now write actual completion response.
       */

      resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
      resp.reqack.head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
    }

  resp.reqack.head.flags = 0;
  resp.reqack.result = ret;
  if (ret >= 0)
    {
      resp.valuelen = sizeof(addr);
      resp.valuelen_nontrunc = sizeof(addr);
      if (resp.valuelen > req->max_addrlen)
        {
          resp.valuelen = req->max_addrlen;
        }
    }
  else
    {
      resp.valuelen = 0;
      resp.valuelen_nontrunc = 0;
    }

  /* Send response. */

  wlen = write(fd, &resp, sizeof(resp));
  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != sizeof(resp))
    {
      return -ENOSPC;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      wlen = write(fd, &addr, resp.valuelen);
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != resp.valuelen)
        {
          return -ENOSPC;
        }
    }

  return OK;
}

static int handle_usrsock_request(int fd, FAR struct daemon_priv_s *priv)
{
  static const struct
  {
    unsigned int hdrlen;
    int (CODE *fn)(int fd, FAR struct daemon_priv_s *priv, FAR void *req);
  }

  handlers[USRSOCK_REQUEST__MAX] =
    {
      [USRSOCK_REQUEST_SOCKET] =
        {
          sizeof(struct usrsock_request_socket_s),
          socket_request,
        },

      [USRSOCK_REQUEST_CLOSE] =
        {
          sizeof(struct usrsock_request_close_s),
          close_request,
        },

      [USRSOCK_REQUEST_CONNECT] =
        {
          sizeof(struct usrsock_request_connect_s),
          connect_request,
        },

      [USRSOCK_REQUEST_SENDTO] =
        {
          sizeof(struct usrsock_request_sendto_s),
          sendto_request,
        },

      [USRSOCK_REQUEST_RECVFROM] =
        {
          sizeof(struct usrsock_request_recvfrom_s),
          recvfrom_request,
        },

      [USRSOCK_REQUEST_SETSOCKOPT] =
        {
          sizeof(struct usrsock_request_setsockopt_s),
          setsockopt_request,
        },

      [USRSOCK_REQUEST_GETSOCKOPT] =
        {
          sizeof(struct usrsock_request_getsockopt_s),
          getsockopt_request,
        },

      [USRSOCK_REQUEST_GETSOCKNAME] =
        {
          sizeof(struct usrsock_request_getsockname_s),
          getsockname_request,
        },
    };

  uint8_t hdrbuf[16];
  FAR struct usrsock_request_common_s *common_hdr = (FAR void *)hdrbuf;
  ssize_t rlen;

  rlen = read(fd, common_hdr, sizeof(*common_hdr));
  if (rlen < 0)
    {
      return -errno;
    }

  if (rlen != sizeof(*common_hdr))
    {
      return -EMSGSIZE;
    }

  if (common_hdr->reqid >= USRSOCK_REQUEST__MAX ||
      !handlers[common_hdr->reqid].fn)
    {
      usrsocktest_dbg("Unknown request type: %d\n", common_hdr->reqid);
      return -EIO;
    }

  assert(handlers[common_hdr->reqid].hdrlen < sizeof(hdrbuf));

  rlen = read_req(fd, common_hdr, hdrbuf,
                  handlers[common_hdr->reqid].hdrlen);
  if (rlen < 0)
    {
      return rlen;
    }

  return handlers[common_hdr->reqid].fn(fd, priv, hdrbuf);
}

static int unblock_sendto(int fd, FAR struct daemon_priv_s *priv,
                          FAR struct test_socket_s *tsock)
{
  if (tsock->block_send)
    {
      int ret;

      tsock->block_send = false;

      ret = tsock_send_event(fd, priv, tsock, USRSOCK_EVENT_SENDTO_READY);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

static int reset_recv_avail(int fd, FAR struct daemon_priv_s *priv,
                            FAR struct test_socket_s *tsock)
{
  if (tsock->recv_avail_bytes == 0)
    {
      int ret;

      priv->sockets_recv_empty--;

      tsock->recv_avail_bytes = priv->conf->endpoint_recv_avail;

      ret = tsock_send_event(fd, priv, tsock,
                             USRSOCK_EVENT_RECVFROM_AVAIL);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

static int disconnect_connection(int fd, FAR struct daemon_priv_s *priv,
                                 FAR struct test_socket_s *tsock)
{
  if (tsock->connected)
    {
      int ret;

      tsock->disconnected = true;
      tsock->connected = false;

      priv->sockets_connected--;
      priv->sockets_remote_disconnected++;

      ret = tsock_send_event(fd, priv, tsock, USRSOCK_EVENT_REMOTE_CLOSED);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

static int establish_blocked_connection(int fd,
                                        FAR struct daemon_priv_s *priv,
                                        FAR struct test_socket_s *tsock)
{
  if (tsock->blocked_connect)
    {
      FAR struct usrsock_message_req_ack_s *resp = &tsock->pending_resp;
      ssize_t wlen;
      int events;

      if (resp->result == OK)
        {
          priv->sockets_connected++;
          tsock->connected = true;
        }

      tsock->blocked_connect = false;

      priv->sockets_waiting_connect--;
      resp->head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp->head.events = 0;

      wlen = write(fd, resp, sizeof(*resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(*resp))
        {
          return -ENOSPC;
        }

      events = 0;
      if (!tsock->block_send)
        {
          events |= USRSOCK_EVENT_SENDTO_READY;
        }

      if (tsock->recv_avail_bytes > 0)
        {
          events |= USRSOCK_EVENT_RECVFROM_AVAIL;
        }

      if (events)
        {
          wlen = tsock_send_event(fd, priv, tsock, events);
          if (wlen < 0)
            {
              return wlen;
            }
        }
    }

  return OK;
}

static int fail_blocked_connection(int fd, FAR struct daemon_priv_s *priv,
                                   FAR struct test_socket_s *tsock)
{
  if (tsock->blocked_connect)
    {
      FAR struct usrsock_message_req_ack_s *resp = &tsock->pending_resp;
      ssize_t wlen;

      resp->result = -ECONNREFUSED;
      priv->sockets_not_connected_refused++;
      tsock->connect_refused = true;
      tsock->blocked_connect = false;

      priv->sockets_waiting_connect--;
      resp->head.flags &= ~USRSOCK_MESSAGE_FLAG_REQ_IN_PROGRESS;
      resp->head.events = 0;

      wlen = write(fd, resp, sizeof(*resp));
      if (wlen < 0)
        {
          return -errno;
        }

      if (wlen != sizeof(*resp))
        {
          return -ENOSPC;
        }
    }

  return OK;
}

static int for_each_connection(int fd, FAR struct daemon_priv_s *priv,
                               int (CODE *iter_fn)(
                                   int fd,
                                   FAR struct daemon_priv_s *priv,
                                   FAR struct test_socket_s *tsock))
{
  int i;

  for (i = 0; i < ARRAY_SIZE(priv->test_sockets); i++)
    {
      FAR struct test_socket_s *tsock = &priv->test_sockets[i];

      if (tsock->opened)
        {
          int ret = iter_fn(fd, priv, tsock);
          if (ret < 0)
            {
              return ret;
            }
        }
    }

  return OK;
}

static FAR void *usrsocktest_daemon(FAR void *param)
{
  FAR struct daemon_priv_s *priv = param;
  bool stopped;
  int ret;
  int fd;

  usrsocktest_dbg("\n");

  priv->sockets_active = 0;

  fd = open("/dev/usrsock", O_RDWR);
  if (fd < 0)
    {
      ret = -errno;
      goto errout;
    }

  do
    {
      int npfds = 0;
      int usrsock_pfdpos = -1;
      int pipe_pdfpos = -1;
      struct pollfd pfd[2] = {
      };

      stopped = false;

      /* Wait for request from kernel side. */

      pthread_mutex_lock(&daemon_mutex);
      if (!priv->do_not_poll_usrsock && fd >= 0)
        {
          pfd[npfds].fd = fd;
          pfd[npfds].events = POLLIN;
          usrsock_pfdpos = npfds++;
        }

      pfd[npfds].fd = priv->pipefd[0];
      pfd[npfds].events = POLLIN;
      pipe_pdfpos = npfds++;
      pthread_mutex_unlock(&daemon_mutex);

      ret = poll(pfd, npfds, -1);
      if (ret < 0)
        {
          /* Error? */

          ret = -errno;
          goto errout;
        }

      if (usrsock_pfdpos >= 0 && (pfd[usrsock_pfdpos].revents & POLLIN))
        {
          pthread_mutex_lock(&daemon_mutex);
          ret = handle_usrsock_request(fd, priv);
          pthread_mutex_unlock(&daemon_mutex);
          if (ret < 0)
            {
              goto errout;
            }
        }

      if (pipe_pdfpos >= 0 && (pfd[pipe_pdfpos].revents & POLLIN))
        {
          char in;

          if (read(pfd[pipe_pdfpos].fd, &in, 1) == 1)
            {
              pthread_mutex_lock(&daemon_mutex);

              switch (in)
                {
                case 'S':
                  stopped = true;
                  ret = 0;
                  break;
                case 's':
                  stopped = false;
                  ret = 0;
                  break;
                case 'E':
                  ret = for_each_connection(fd, priv,
                                            &establish_blocked_connection);
                  break;
                case 'F':
                  ret = for_each_connection(fd, priv,
                                            &fail_blocked_connection);
                  break;
                case 'D':
                  ret = for_each_connection(fd, priv,
                                            &disconnect_connection);
                  break;
                case 'W':
                  ret = for_each_connection(fd, priv,
                                            &unblock_sendto);
                  break;
                case 'r':
                  ret = for_each_connection(fd, priv,
                                            &reset_recv_avail);
                  break;
                case 'K':

                  /* Kill usrsockdev */

                  if (fd >= 0)
                    {
                      close(fd);
                      fd = -1;
                    }

                  break;
                case '*':
                  sem_post(&priv->wakewaitsem);
                  break; /* woke thread. */
                }

              pthread_mutex_unlock(&daemon_mutex);

              if (ret < 0)
                {
                  goto errout;
                }
            }
        }

      usleep(1);
    }
  while (!stopped);

  ret = OK;
errout:
  if (fd >= 0)
    {
      close(fd);
    }

  usrsocktest_dbg("ret: %d\n", ret);

  return (FAR void *)(intptr_t)ret;
}

static int get_daemon_value(FAR struct daemon_priv_s *priv,
                            FAR void *dst, FAR const void *src, size_t len)
{
  int ret = 0;

  if ((uintptr_t)src < (uintptr_t)priv ||
      (uintptr_t)src >= (uintptr_t)priv + sizeof(*priv) || len <= 0)
    {
      /* Not daemon value */

      return -EINVAL;
    }

  pthread_mutex_lock(&daemon_mutex);

  if (priv->conf == NULL)
    {
      /* Not running? */

      ret = -ENODEV;
      goto out;
    }

  memmove(dst, src, len);

out:
  pthread_mutex_unlock(&daemon_mutex);

  return ret;
}

static FAR void *delayed_cmd_thread(FAR void *priv)
{
  FAR struct delayed_cmd_s *cmd = priv;

  if (cmd->delay_msec)
    {
      sem_post(&cmd->startsem);
    }

  usleep(cmd->delay_msec * 1000);

  write(cmd->pipefd, &cmd->cmd, 1);

  if (!cmd->delay_msec)
    {
      sem_post(&cmd->startsem);
    }

  return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int usrsocktest_daemon_start(
  FAR const struct usrsocktest_daemon_conf_s *conf)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  pthread_attr_t attr;
  int ret;

  usrsocktest_dbg("\n");

  pthread_mutex_lock(&daemon_mutex);

  if (priv->conf != NULL || !priv->joined)
    {
      /* Already running? */

      ret = -EALREADY;
      goto out;
    }

  /* Clear daemon private data. */

  memset(priv, 0, sizeof(*priv));

  /* Allocate pipe for daemon commands. */

  ret = pipe(priv->pipefd);
  if (ret != OK)
    {
      ret = -errno;
      goto out;
    }

  ret = pthread_attr_init(&attr);
  if (ret != OK)
    {
      ret = -ret;
      goto errout_closepipe;
    }

  sem_init(&priv->wakewaitsem, 0, 0);

  priv->joined = false;
  priv->conf = conf;

  ret = pthread_create(&priv->tid, &attr, usrsocktest_daemon, priv);
  if (ret != OK)
    {
      sem_destroy(&priv->wakewaitsem);
      priv->joined = true;
      priv->conf = NULL;
      ret = -ret;
      goto errout_closepipe;
    }

errout_closepipe:
  if (ret != OK)
    {
      close(priv->pipefd[0]);
      close(priv->pipefd[1]);
    }

out:
  pthread_mutex_unlock(&daemon_mutex);
  usrsocktest_dbg("ret: %d\n", ret);
  return ret;
}

int usrsocktest_daemon_stop(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  FAR struct delayed_cmd_s *item;
  FAR struct delayed_cmd_s *next;
  FAR pthread_addr_t retval;
  char stopped;
  int ret;
  int i;

  usrsocktest_dbg("\n");

  pthread_mutex_lock(&daemon_mutex);

  if (priv->conf == NULL)
    {
      /* Not running? */

      ret = -ENODEV;
      goto out;
    }

  item = (void *)sq_peek(&priv->delayed_cmd_threads);
  while (item)
    {
      next = (void *)sq_next(&item->node);

      pthread_mutex_unlock(&daemon_mutex);
      pthread_join(item->tid, &retval);
      pthread_mutex_lock(&daemon_mutex);
      sq_rem(&item->node, &priv->delayed_cmd_threads);
      free(item);
      usrsocktest_dcmd_malloc_cnt--;

      item = next;
    }

  pthread_mutex_unlock(&daemon_mutex);
  stopped = 'S';
  write(priv->pipefd[1], &stopped, 1);

  ret = pthread_join(priv->tid, &retval);
  pthread_mutex_lock(&daemon_mutex);
  if (ret != OK)
    {
      ret = -ret;
      goto out;
    }

  for (i = 0; i < ARRAY_SIZE(priv->test_sockets); i++)
    {
      if (priv->test_sockets[i].opened && priv->test_sockets[i].endp != NULL)
        {
          free(priv->test_sockets[i].endp);
          priv->test_sockets[i].endp = NULL;
          usrsocktest_endp_malloc_cnt--;
        }
    }

  priv->conf = NULL;
  close(priv->pipefd[0]);
  close(priv->pipefd[1]);
  sem_destroy(&priv->wakewaitsem);

  priv->joined = true;
  ret = (intptr_t)retval;

out:
  pthread_mutex_unlock(&daemon_mutex);
  usrsocktest_dbg("ret: %d\n", ret);
  return ret;
}

int usrsocktest_daemon_get_num_active_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_active, sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_get_num_connected_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_connected, sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_get_num_waiting_connect_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_waiting_connect,
                         sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_get_num_recv_empty_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_recv_empty, sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

ssize_t usrsocktest_daemon_get_send_bytes(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  size_t ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->total_send_bytes, sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

ssize_t usrsocktest_daemon_get_recv_bytes(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  size_t ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->total_recv_bytes, sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_get_num_unreachable_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_not_connected_refused,
                         sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_get_num_remote_disconnected_sockets(void)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  int err;

  err = get_daemon_value(priv, &ret, &priv->sockets_remote_disconnected,
                         sizeof(ret));
  if (err < 0)
    {
      return err;
    }

  return ret;
}

int usrsocktest_daemon_pause_usrsock_handling(bool pause)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  int ret;
  char cmd = '*';

  pthread_mutex_lock(&daemon_mutex);

  if (priv->conf == NULL)
    {
      /* Not running? */

      pthread_mutex_unlock(&daemon_mutex);
      return -ENODEV;
    }

  priv->do_not_poll_usrsock = pause;

  write(priv->pipefd[1], &cmd, 1);
  ret = OK;

  pthread_mutex_unlock(&daemon_mutex);

  sem_wait(&priv->wakewaitsem);

  return ret;
}

bool usrsocktest_send_delayed_command(const char cmd,
                                      unsigned int delay_msec)
{
  FAR struct daemon_priv_s *priv = &g_ub_daemon;
  pthread_attr_t attr;
  FAR struct delayed_cmd_s *delayed_cmd;
  int ret;

  if (priv->conf == NULL)
    {
      /* Not running? */

      return false;
    }

  delayed_cmd = calloc(1, sizeof(*delayed_cmd));
  if (!delayed_cmd)
    {
      return false;
    }

  usrsocktest_dcmd_malloc_cnt++;

  delayed_cmd->delay_msec = delay_msec;
  delayed_cmd->cmd = cmd;
  delayed_cmd->pipefd = priv->pipefd[1];
  sem_init(&delayed_cmd->startsem, 0, 0);

  ret = pthread_attr_init(&attr);
  if (ret != OK)
    {
      free(delayed_cmd);
      usrsocktest_dcmd_malloc_cnt--;
      return false;
    }

  ret = pthread_create(&delayed_cmd->tid, &attr, delayed_cmd_thread,
                       delayed_cmd);
  if (ret != OK)
    {
      free(delayed_cmd);
      usrsocktest_dcmd_malloc_cnt--;
      return false;
    }

  while (sem_wait(&delayed_cmd->startsem) != OK);

  sq_addlast(&delayed_cmd->node, &priv->delayed_cmd_threads);

  return true;
}

bool usrsocktest_daemon_establish_waiting_connections(void)
{
  return usrsocktest_send_delayed_command('E', 0);
}
