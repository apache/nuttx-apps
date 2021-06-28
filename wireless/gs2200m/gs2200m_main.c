/****************************************************************************
 * apps/wireless/gs2200m/gs2200m_main.c
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
#include <assert.h>
#include <debug.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <nuttx/net/usrsock.h>
#include <nuttx/wireless/wireless.h>
#include <nuttx/wireless/gs2200m.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* #define GS2200M_TRACE */

#ifdef GS2200M_TRACE
# define gs2200m_printf(v, ...) printf(v, ##__VA_ARGS__)
#else
# define gs2200m_printf(v, ...)
#endif

#ifndef MIN
#  define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#define SOCKET_BASE  10000
#define SOCKET_COUNT 16

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

enum sock_state_e
{
  CLOSED,
  OPENED,
  BOUND,
  CONNECTED,
};

struct usock_s
{
  int8_t   type;
  char     cid;
  enum sock_state_e state;
  uint16_t lport;           /* local port */
  struct sockaddr_in raddr; /* remote addr */
};

struct gs2200m_s
{
  char    *ssid;
  char    *key;
  uint8_t mode;
  uint8_t ch;
  int     gsfd;
  struct usock_s sockets[SOCKET_COUNT];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int socket_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf);
static int close_request(int fd, FAR struct gs2200m_s *priv,
                         FAR void *hdrbuf);
static int connect_request(int fd, FAR struct gs2200m_s *priv,
                           FAR void *hdrbuf);
static int sendto_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf);
static int recvfrom_request(int fd, FAR struct gs2200m_s *priv,
                            FAR void *hdrbuf);
static int setsockopt_request(int fd, FAR struct gs2200m_s *priv,
                              FAR void *hdrbuf);
static int getsockopt_request(int fd, FAR struct gs2200m_s *priv,
                              FAR void *hdrbuf);
static int getsockname_request(int fd, FAR struct gs2200m_s *priv,
                               FAR void *hdrbuf);
static int getpeername_request(int fd, FAR struct gs2200m_s *priv,
                               FAR void *hdrbuf);
static int ioctl_request(int fd, FAR struct gs2200m_s *priv,
                         FAR void *hdrbuf);
static int bind_request(int fd, FAR struct gs2200m_s *priv,
                        FAR void *hdrbuf);
static int listen_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf);
static int accept_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct usrsock_req_handler_s
{
  uint32_t hdrlen;
  int (CODE *fn)(int fd, FAR struct gs2200m_s *priv, FAR void *req);
}
handlers[USRSOCK_REQUEST__MAX] =
{
{
  sizeof(struct usrsock_request_socket_s),
  socket_request,
},
{
  sizeof(struct usrsock_request_close_s),
  close_request,
},
{
  sizeof(struct usrsock_request_connect_s),
  connect_request,
},
{
  sizeof(struct usrsock_request_sendto_s),
  sendto_request,
},
{
  sizeof(struct usrsock_request_recvfrom_s),
  recvfrom_request,
},
{
  sizeof(struct usrsock_request_setsockopt_s),
  setsockopt_request,
},
{
  sizeof(struct usrsock_request_getsockopt_s),
  getsockopt_request,
},
{
  sizeof(struct usrsock_request_getsockname_s),
  getsockname_request,
},
{
  sizeof(struct usrsock_request_getpeername_s),
  getpeername_request,
},
{
  sizeof(struct usrsock_request_bind_s),
  bind_request,
},
{
  sizeof(struct usrsock_request_listen_s),
  listen_request,
},
{
  sizeof(struct usrsock_request_accept_s),
  accept_request,
},
{
  sizeof(struct usrsock_request_ioctl_s),
  ioctl_request,
},
};

static struct gs2200m_s *_daemon;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: _write_to_usock
 ****************************************************************************/

static int _write_to_usock(int fd, void *buf, size_t count)
{
  ssize_t wlen;

  wlen = write(fd, buf, count);

  if (wlen < 0)
    {
      return -errno;
    }

  if (wlen != count)
    {
      return -ENOSPC;
    }

  return OK;
}

/****************************************************************************
 * Name: _send_ack_common
 ****************************************************************************/

static int _send_ack_common(int fd,
                            uint8_t xid,
                            FAR struct usrsock_message_req_ack_s *resp)
{
  resp->head.msgid = USRSOCK_MESSAGE_RESPONSE_ACK;
  resp->head.flags = 0;
  resp->xid = xid;

  /* Send ACK response. */

  return _write_to_usock(fd, resp, sizeof(*resp));
}

/****************************************************************************
 * Name: gs2200m_socket_alloc
 ****************************************************************************/

static int16_t gs2200m_socket_alloc(FAR struct gs2200m_s *priv, int type)
{
  FAR struct usock_s *usock;
  int16_t i;

  for (i = 0; i < SOCKET_COUNT; i++)
    {
      usock = &priv->sockets[i];

      if (CLOSED == usock->state)
        {
          memset(usock, 0, sizeof(*usock));
          usock->cid = 'z'; /* Invalidate cid */
          usock->state = OPENED;
          usock->type  = type;
          return i + SOCKET_BASE;
        }
    }

  return -1;
}

/****************************************************************************
 * Name: gs2200m_socket_get
 ****************************************************************************/

static FAR struct usock_s *gs2200m_socket_get(FAR struct gs2200m_s *priv,
                                             int sockid)
{
  if (sockid < SOCKET_BASE)
    {
      return NULL;
    }

  sockid -= SOCKET_BASE;

  if (sockid >= SOCKET_COUNT)
    {
      return NULL;
    }

  return &priv->sockets[sockid];
}

/****************************************************************************
 * Name: gs2200m_find_socket_by_cid
 ****************************************************************************/

static FAR struct usock_s *
gs2200m_find_socket_by_cid(FAR struct gs2200m_s *priv,
                           char cid)
{
  FAR struct usock_s *ret = NULL;
  int i;

  for (i = 0; i < SOCKET_COUNT; i++)
    {
      if (priv->sockets[i].cid == cid)
        {
          ret = &priv->sockets[i];
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: gs2200m_socket_free
 ****************************************************************************/

static int gs2200m_socket_free(FAR struct gs2200m_s *priv, int sockid)
{
  FAR struct usock_s *usock = gs2200m_socket_get(priv, sockid);

  if (!usock)
    {
      return -EBADFD;
    }

  if (CLOSED == usock->state)
    {
      return -EFAULT;
    }

  usock->state = CLOSED;
  usock->cid = 'z'; /* invalid */

  return 0;
}

/****************************************************************************
 * Name: read_req
 ****************************************************************************/

static ssize_t
read_req(int fd, FAR const struct usrsock_request_common_s *com_hdr,
         FAR void *req, size_t reqsize)
{
  ssize_t rlen;

  rlen = read(fd, (uint8_t *)req + sizeof(*com_hdr),
              reqsize - sizeof(*com_hdr));

  if (rlen < 0)
    {
      return -errno;
    }

  if (rlen + sizeof(*com_hdr) != reqsize)
    {
      return -EMSGSIZE;
    }

  return rlen;
}

/****************************************************************************
 * Name: usrsock_request
 ****************************************************************************/

static int usrsock_request(int fd, FAR struct gs2200m_s *priv)
{
  FAR struct usrsock_request_common_s *com_hdr;
  uint8_t hdrbuf[16];
  ssize_t rlen;

  com_hdr = (FAR void *)hdrbuf;
  rlen = read(fd, com_hdr, sizeof(*com_hdr));

  if (rlen < 0)
    {
      return -errno;
    }

  if (rlen != sizeof(*com_hdr))
    {
      return -EMSGSIZE;
    }

  if (com_hdr->reqid >= USRSOCK_REQUEST__MAX ||
      !handlers[com_hdr->reqid].fn)
    {
      ASSERT(false);
      return -EIO;
    }

  assert(handlers[com_hdr->reqid].hdrlen < sizeof(hdrbuf));

  rlen = read_req(fd, com_hdr, hdrbuf,
                  handlers[com_hdr->reqid].hdrlen);

  if (rlen < 0)
    {
      return rlen;
    }

  return handlers[com_hdr->reqid].fn(fd, priv, hdrbuf);
}

/****************************************************************************
 * Name: usock_send_event
 ****************************************************************************/

static int usock_send_event(int fd, FAR struct gs2200m_s *priv,
                            FAR struct usock_s *usock, int events)
{
  FAR struct usrsock_message_socket_event_s event;
  int i;

  memset(&event, 0, sizeof(event));
  event.head.flags = USRSOCK_MESSAGE_FLAG_EVENT;
  event.head.msgid = USRSOCK_MESSAGE_SOCKET_EVENT;

  for (i = 0; i < SOCKET_COUNT; i++)
    {
      if (usock == &priv->sockets[i])
        {
          break;
        }
    }

  if (i == SOCKET_COUNT)
    {
      return -EINVAL;
    }

  event.usockid = i + SOCKET_BASE;
  event.events  = events;

  return _write_to_usock(fd, &event, sizeof(event));
}

/****************************************************************************
 * Name: socket_request
 ****************************************************************************/

static int socket_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_socket_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  FAR struct usock_s *usock;
  int16_t usockid;
  int ret;

  gs2200m_printf("%s: start type=%d \n",
                 __func__, req->type);

  /* Check domain requested */

  if (req->domain != AF_INET)
    {
      usockid = -EAFNOSUPPORT;
    }
  else
    {
      /* Allocate socket. */

      usockid = gs2200m_socket_alloc(priv, req->type);
      ASSERT(0 < usockid);
    }

  /* Send ACK response */

  memset(&resp, 0, sizeof(resp));
  resp.result = usockid;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  if (req->type == SOCK_DGRAM)
    {
      /* NOTE: If the socket type is DGRAM, it's ready to send
       * a packet after creating user socket.
       */

      usock = gs2200m_socket_get(priv, usockid);
      usock_send_event(fd, priv, usock,
                       USRSOCK_EVENT_SENDTO_READY);
    }

  gs2200m_printf("%s: end \n", __func__);
  return OK;
}

/****************************************************************************
 * Name: close_request
 ****************************************************************************/

static int close_request(int fd, FAR struct gs2200m_s *priv,
                         FAR void *hdrbuf)
{
  FAR struct usrsock_request_close_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  struct gs2200m_close_msg clmsg;
  FAR struct usock_s *usock;
  char cid;
  int ret = 0;

  gs2200m_printf("%s: start \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  cid = usock->cid;

  if ((BOUND != usock->state) && (CONNECTED != usock->state))
    {
      ret = -EBADFD;
      goto errout;
    }

  memset(&clmsg, 0, sizeof(clmsg));
  clmsg.cid = cid;
  ioctl(priv->gsfd, GS2200M_IOC_CLOSE, (unsigned long)&clmsg);

errout:

  /* Send ACK response */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  /* Free socket */

  ret = gs2200m_socket_free(priv, req->usockid);

  gs2200m_printf("%s: end \n", __func__);

  return OK;
}

/****************************************************************************
 * Name: connect_request
 ****************************************************************************/

static int connect_request(int fd, FAR struct gs2200m_s *priv,
                           FAR void *hdrbuf)
{
  FAR struct usrsock_request_connect_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  struct gs2200m_connect_msg cmsg;
  struct sockaddr_in addr;
  FAR struct usock_s *usock;
  int events;
  ssize_t wlen;
  ssize_t rlen;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: start \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is already connected. */

  if (CONNECTED == usock->state)
    {
      ret = -EISCONN;
      goto prepare;
    }

  memset(&cmsg, 0, sizeof(cmsg));

  /* Check if this socket is already connected. */

  if (BOUND == usock->state)
    {
      if (usock->type == SOCK_STREAM)
        {
          ret = -EISCONN;
          goto prepare;
        }
      else
        {
          /* Firstly, close the socket */

          struct gs2200m_close_msg clmsg;
          memset(&clmsg, 0, sizeof(clmsg));
          clmsg.cid = usock->cid;

          ioctl(priv->gsfd, GS2200M_IOC_CLOSE, (unsigned long)&clmsg);

          /* Copy the local port info */

          cmsg.lport = usock->lport;

          usock->state = OPENED;
        }
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

  if (addr.sin_family != AF_INET)
    {
      ret = -EAFNOSUPPORT;
      goto prepare;
    }

  snprintf(cmsg.addr, sizeof(cmsg.addr), "%s",
           inet_ntoa(addr.sin_addr));
  snprintf(cmsg.port, sizeof(cmsg.port), "%d",
           ntohs(addr.sin_port));

  cmsg.cid  = 'z'; /* set to invalid */
  cmsg.type = usock->type;

  ret = ioctl(priv->gsfd, GS2200M_IOC_CONNECT,
              (unsigned long)&cmsg);

  if (0 == ret)
    {
      usock->cid   = cmsg.cid;
      usock->state = CONNECTED;
      usock->raddr = addr;
    }
  else
    {
      ret = -errno;
    }

prepare:

  /* Send ACK response. */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  events = USRSOCK_EVENT_SENDTO_READY;
  wlen   = usock_send_event(fd, priv, usock, events);

  if (wlen < 0)
    {
      return wlen;
    }

  gs2200m_printf("%s: end \n", __func__);
  return OK;
}

/****************************************************************************
 * Name: sendto_request
 ****************************************************************************/

static int sendto_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_sendto_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  struct gs2200m_send_msg smsg;
  FAR struct usock_s *usock;
  uint8_t *sendbuf = NULL;
  ssize_t wlen;
  ssize_t rlen;
  int nret;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: start (buflen=%d) \n",
                 __func__, req->buflen);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is connected. */

  if (SOCK_STREAM == usock->type && CONNECTED != usock->state)
    {
      ret = -ENOTCONN;
      goto prepare;
    }

  /* Check if the address size is non-zero.
   * connection-mode socket does not accept address
   */

  if (usock->type == SOCK_STREAM && req->addrlen > 0)
    {
      ret = -EISCONN;
      goto prepare;
    }

  memset(&smsg, 0, sizeof(smsg));

  smsg.is_tcp = (usock->type == SOCK_STREAM) ? true : false;

  /* For UDP, addlen must be provided */

  if (usock->type == SOCK_DGRAM)
    {
      if (CONNECTED != usock->state)
        {
          if (req->addrlen == 0)
            {
              ret = -EINVAL;
              goto prepare;
            }

          /* In UDP case, read the address. */

          rlen = read(fd, &smsg.addr, sizeof(smsg.addr));

          if (rlen < 0 || rlen < req->addrlen)
            {
              ret = -EFAULT;
              goto prepare;
            }
        }
      else if (CONNECTED == usock->state)
        {
          /* Copy remote address */

          smsg.addr = usock->raddr;
        }
      else
        {
          ASSERT(false);
        }

      gs2200m_printf("%s: addr: %s:%d",
                     __func__,
                     inet_ntoa(smsg.addr.sin_addr),
                     ntohs(smsg.addr.sin_port));
    }

  /* Check if the request has data. */

  if (req->buflen > 0)
    {
      sendbuf = calloc(1, req->buflen);
      ASSERT(sendbuf);

      /* Read data from usrsock. */

      rlen = read(fd, sendbuf, req->buflen);

      if (rlen < 0 || rlen < req->buflen)
        {
          ret = -EFAULT;
          goto prepare;
        }

      smsg.cid = usock->cid;
      smsg.buf = sendbuf;
      smsg.len = req->buflen;

      nret = ioctl(priv->gsfd, GS2200M_IOC_SEND,
                   (unsigned long)&smsg);

      if (usock->cid != smsg.cid)
        {
          /* cid is newly assigned (bound) */

          usock->cid = smsg.cid;
          usock->state = BOUND;
        }

      if (0 != nret)
        {
          ret = -errno;
          goto prepare;
        }

      /* return length which gs2200m sent */

      ret = smsg.len;
    }

prepare:

  if (sendbuf)
    {
      free(sendbuf);
    }

  /* Send ACK response. */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  /* Let kernel-side know that there is space for more send data. */

  wlen = usock_send_event(fd, priv, usock,
                          USRSOCK_EVENT_SENDTO_READY);

  if (wlen < 0)
    {
      return wlen;
    }

  gs2200m_printf("%s: end \n", __func__);

  return OK;
}

/****************************************************************************
 * Name: recvfrom_request
 ****************************************************************************/

static int recvfrom_request(int fd, FAR struct gs2200m_s *priv,
                            FAR void *hdrbuf)
{
  FAR struct usrsock_request_recvfrom_s *req = hdrbuf;
  struct usrsock_message_datareq_ack_s resp;
  struct gs2200m_recv_msg rmsg;
  FAR struct usock_s *usock;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: start (req->max_buflen=%d) \n",
                 __func__, req->max_buflen);

  memset(&rmsg, 0, sizeof(rmsg));

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* Check if this socket is connected. */

  if (SOCK_STREAM == usock->type && CONNECTED != usock->state)
    {
      ret = -ENOTCONN;
      goto prepare;
    }

  rmsg.cid = usock->cid;
  rmsg.reqlen = req->max_buflen;
  rmsg.is_tcp = (usock->type == SOCK_STREAM) ? true : false;
  rmsg.flags = req->flags;

  if (0 < req->max_buflen)
    {
      rmsg.buf = calloc(1, req->max_buflen);
      ASSERT(rmsg.buf);

      ret = ioctl(priv->gsfd, GS2200M_IOC_RECV,
                  (unsigned long)&rmsg);
    }

  if (0 == ret)
    {
      ret = rmsg.len;
    }
  else
    {
      ret = -errno;
    }

  if (!rmsg.is_tcp)
    {
      gs2200m_printf("%s: from (%s:%d) \n",
                     __func__,
                     inet_ntoa(rmsg.addr.sin_addr),
                     ntohs(rmsg.addr.sin_port));
    }

prepare:

  /* Prepare response. */

  memset(&resp, 0, sizeof(resp));
  resp.reqack.result = ret;
  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags = 0;

  if (0 <= ret)
    {
      resp.valuelen_nontrunc = sizeof(rmsg.addr);
      resp.valuelen = MIN(resp.valuelen_nontrunc,
                          req->max_addrlen);

      if ((0 == rmsg.len) && (0 != rmsg.reqlen))
        {
          usock_send_event(fd, priv, usock,
                           USRSOCK_EVENT_REMOTE_CLOSED
                           );
        }
    }

  /* Send response. */

  ret = _write_to_usock(fd, &resp, sizeof(resp));

  if (0 > ret)
    {
      goto err_out;
    }

  if (0 < resp.valuelen)
    {
      /* Send address (value) */

      ret = _write_to_usock(fd, &rmsg.addr, resp.valuelen);

      if (0 > ret)
        {
          goto err_out;
        }
    }

  if (resp.reqack.result > 0)
    {
      /* Send buffer */

      ret = _write_to_usock(fd, rmsg.buf, resp.reqack.result);

      if (0 > ret)
        {
          goto err_out;
        }
    }

err_out:

  gs2200m_printf("%s: *** end ret=%d \n", __func__, ret);

  if (rmsg.buf)
    {
      free(rmsg.buf);
    }

  return ret;
}

/****************************************************************************
 * Name: bind_request
 ****************************************************************************/

static int bind_request(int fd, FAR struct gs2200m_s *priv,
                        FAR void *hdrbuf)
{
  FAR struct usrsock_request_bind_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  struct gs2200m_bind_msg bmsg;
  FAR struct usock_s *usock;
  struct sockaddr_in addr;
  ssize_t rlen;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);
  if (!usock)
    {
      ret = -EBADFD;
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

  if (addr.sin_family != AF_INET)
    {
      ret = -EAFNOSUPPORT;
      goto prepare;
    }

  snprintf(bmsg.port, sizeof(bmsg.port), "%d", ntohs(addr.sin_port));
  bmsg.cid    = 'z'; /* set to invalid */
  bmsg.is_tcp = (usock->type == SOCK_STREAM) ? true : false;

  ret = ioctl(priv->gsfd, GS2200M_IOC_BIND, (unsigned long)&bmsg);

  if (0 == ret)
    {
      usock->cid   = bmsg.cid;
      usock->lport = ntohs(addr.sin_port);
      usock->state = BOUND;
    }

prepare:

  /* Send ACK response. */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  gs2200m_printf("%s: end \n", __func__);
  return OK;
}

/****************************************************************************
 * Name: listen_request
 ****************************************************************************/

static int listen_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_listen_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  FAR struct usock_s *usock;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
    }

  /* Send ACK response. */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;
  ret = _send_ack_common(fd, req->head.xid, &resp);

  if (0 > ret)
    {
      return ret;
    }

  gs2200m_printf("%s: end \n", __func__);
  return ret;
}

/****************************************************************************
 * Name: accept_request
 ****************************************************************************/

static int accept_request(int fd, FAR struct gs2200m_s *priv,
                          FAR void *hdrbuf)
{
  FAR struct usrsock_request_accept_s *req = hdrbuf;
  struct usrsock_message_datareq_ack_s resp;
  struct gs2200m_accept_msg amsg;
  FAR struct usock_s *usock;
  FAR struct usock_s *new_usock = NULL;
  int ret = 0;
  int16_t usockid; /* usockid for new client */

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  /* TODO: need to check if specified socket exists */

  /* Call gs2200m driver to obtain new cid for client */

  amsg.cid  = usock->cid; /* Set server cid */
  ret = ioctl(priv->gsfd, GS2200M_IOC_ACCEPT, (unsigned long)&amsg);

  if (-1 == ret)
    {
      goto prepare;
    }

  /* allocate socket. */

  usockid   = gs2200m_socket_alloc(priv, SOCK_STREAM);
  ASSERT(0 < usockid);
  new_usock = gs2200m_socket_get(priv, usockid);

  /* Set cid for the new_usock to be used in gs2200m driver */

  new_usock->cid   = amsg.cid;
  new_usock->state = CONNECTED;
  new_usock->raddr = amsg.addr;

prepare:

  /* Prepare response. */

  memset(&resp, 0, sizeof(resp));
  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags = 0;

  if (0 == ret)
    {
      resp.reqack.result = 2; /* new_usock->raddr + usock */
      resp.valuelen_nontrunc = sizeof(new_usock->raddr);
      resp.valuelen = resp.valuelen_nontrunc;
    }
  else
    {
      resp.reqack.result = ret;
      resp.valuelen = 0;
    }

  /* Send response. */

  ret = _write_to_usock(fd, &resp, sizeof(resp));

  if (0 > ret)
    {
      goto err_out;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      ret = _write_to_usock(fd, &new_usock->raddr, resp.valuelen);

      if (0 > ret)
        {
          goto err_out;
        }

      /* Send new usockid info */

      ret = _write_to_usock(fd, &usockid, sizeof(usockid));

      if (0 > ret)
        {
          goto err_out;
        }

      /* Set events ofr new_usock */

      usock_send_event(fd, priv, new_usock,
                       USRSOCK_EVENT_SENDTO_READY
                       );
    }

err_out:
  gs2200m_printf("%s: end \n", __func__);
  return ret;
}

/****************************************************************************
 * Name: setsockopt_request
 ****************************************************************************/

static int setsockopt_request(int fd, FAR struct gs2200m_s *priv,
                              FAR void *hdrbuf)
{
  FAR struct usrsock_request_setsockopt_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  FAR struct usock_s *usock;
  ssize_t rlen;
  int ret = 0;
  int value;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  if (req->level != SOL_SOCKET)
    {
      gs2200m_printf("setsockopt: level=%d not supported\n",
                     __func__, req->level);
      ret = -ENOPROTOOPT;
      goto prepare;
    }

  if (req->option != SO_REUSEADDR)
    {
      gs2200m_printf("setsockopt: option=%d not supported\n",
                     __func__, req->option);
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

  gs2200m_printf("setsockopt: option=%d value=%d\n",
                 __func__, req->option, value);

  ret = OK;

prepare:

  /* Send ACK response */

  memset(&resp, 0, sizeof(resp));
  resp.result = ret;

  ret = _send_ack_common(fd, req->head.xid, &resp);

  gs2200m_printf("%s: end (ret=%d) \n", __func__, ret);
  return ret;
}

/****************************************************************************
 * Name: getsockopt_request
 ****************************************************************************/

static int getsockopt_request(int fd, FAR struct gs2200m_s *priv,
                              FAR void *hdrbuf)
{
  DEBUGASSERT(false);
  return -ENOSYS;
}

/****************************************************************************
 * Name: getsockname_request
 ****************************************************************************/

static int getsockname_request(int fd, FAR struct gs2200m_s *priv,
                               FAR void *hdrbuf)
{
  FAR struct usrsock_request_getsockname_s *req = hdrbuf;
  struct usrsock_message_datareq_ack_s resp;
  struct gs2200m_name_msg nmsg;
  FAR struct usock_s *usock;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  memset(&nmsg, 0, sizeof(nmsg));
  nmsg.cid = usock->cid;
  nmsg.local = true; /* Obtain local address & port */

  ret = ioctl(priv->gsfd, GS2200M_IOC_NAME, (unsigned long)&nmsg);

prepare:

  /* Prepare response. */

  memset(&resp, 0, sizeof(resp));
  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags = 0;
  resp.reqack.result = ret;

  if (0 == ret)
    {
      resp.valuelen_nontrunc = sizeof(nmsg.addr);
      resp.valuelen = resp.valuelen_nontrunc;

      if (resp.valuelen > req->max_addrlen)
        {
          resp.valuelen = req->max_addrlen;
        }
    }
  else
    {
      resp.valuelen_nontrunc = 0;
      resp.valuelen = 0;
    }

  /* Send response. */

  ret = _write_to_usock(fd, &resp, sizeof(resp));

  if (0 > ret)
    {
      goto err_out;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      ret = _write_to_usock(fd, &nmsg.addr, resp.valuelen);

      if (0 > ret)
        {
          goto err_out;
        }
    }

err_out:
  gs2200m_printf("%s: end \n", __func__);
  return ret;
}

/****************************************************************************
 * Name: getpeername_request
 ****************************************************************************/

static int getpeername_request(int fd, FAR struct gs2200m_s *priv,
                               FAR void *hdrbuf)
{
  FAR struct usrsock_request_getpeername_s *req = hdrbuf;
  struct usrsock_message_datareq_ack_s resp;
  FAR struct usock_s *usock;
  int ret = 0;

  DEBUGASSERT(priv);
  DEBUGASSERT(req);

  gs2200m_printf("%s: called **** \n", __func__);

  /* Check if this socket exists. */

  usock = gs2200m_socket_get(priv, req->usockid);

  if (!usock)
    {
      ret = -EBADFD;
      goto prepare;
    }

  if (CONNECTED != usock->state)
    {
      ret = -ENOTCONN;
    }

prepare:

  /* Prepare response. */

  memset(&resp, 0, sizeof(resp));
  resp.reqack.xid = req->head.xid;
  resp.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  resp.reqack.head.flags = 0;
  resp.reqack.result = ret;

  if (0 == ret)
    {
      resp.valuelen_nontrunc = sizeof(usock->raddr);
      resp.valuelen = resp.valuelen_nontrunc;

      if (resp.valuelen > req->max_addrlen)
        {
          resp.valuelen = req->max_addrlen;
        }
    }
  else
    {
      resp.valuelen_nontrunc = 0;
      resp.valuelen = 0;
    }

  /* Send response. */

  ret = _write_to_usock(fd, &resp, sizeof(resp));

  if (0 > ret)
    {
      goto err_out;
    }

  if (resp.valuelen > 0)
    {
      /* Send address (value) */

      ret = _write_to_usock(fd, &usock->raddr, resp.valuelen);

      if (0 > ret)
        {
          goto err_out;
        }
    }

err_out:
  gs2200m_printf("%s: end \n", __func__);
  return ret;
}

/****************************************************************************
 * Name: ioctl_request
 ****************************************************************************/

static int ioctl_request(int fd, FAR struct gs2200m_s *priv,
                         FAR void *hdrbuf)
{
  FAR struct usrsock_request_ioctl_s *req = hdrbuf;
  struct usrsock_message_req_ack_s resp;
  struct usrsock_message_datareq_ack_s resp2;
  struct gs2200m_ifreq_msg imsg;
  bool getreq = false;
  int ret = -EINVAL;

  memset(&imsg.ifr, 0, sizeof(imsg.ifr));

  switch (req->cmd)
    {
      case SIOCGIFADDR:
      case SIOCGIFHWADDR:
      case SIOCGIWNWID:
      case SIOCGIWFREQ:
      case SIOCGIWSENS:
        getreq = true;
        break;

      case SIOCSIFADDR:
      case SIOCSIFDSTADDR:
      case SIOCSIFNETMASK:

        read(fd, &imsg.ifr, sizeof(imsg.ifr));
        break;

      default:
        break;
    }

  imsg.cmd = req->cmd;
  ret = ioctl(priv->gsfd, GS2200M_IOC_IFREQ, (unsigned long)&imsg);

  if (!getreq)
    {
      /* Send ACK response */

      memset(&resp, 0, sizeof(resp));
      resp.result = ret;
      ret = _send_ack_common(fd, req->head.xid, &resp);

      if (0 > ret)
        {
          return ret;
        }
    }

  if (getreq)
    {
      resp2.reqack.result = ret;
      resp2.reqack.xid = req->head.xid;
      resp2.reqack.head.msgid = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
      resp2.reqack.head.flags = 0;
      resp2.valuelen_nontrunc = sizeof(imsg.ifr);
      resp2.valuelen = sizeof(imsg.ifr);

      _write_to_usock(fd, &resp2, sizeof(resp2));

      /* Return struct ifreq address */

      _write_to_usock(fd, &imsg.ifr, resp2.valuelen);
    }

  return ret;
}

/****************************************************************************
 * Name: gs2200m_loop
 ****************************************************************************/

static int gs2200m_loop(FAR struct gs2200m_s *priv)
{
  struct gs2200m_assoc_msg amsg;
  FAR struct usock_s *usock;
  struct pollfd fds[2];
  int  fd[2];
  char cid;
  int  ret;

  fd[0] = open("/dev/usrsock", O_RDWR);
  ASSERT(0 <= fd[0]);

  fd[1] = open("/dev/gs2200m", O_RDWR);
  ASSERT(0 <= fd[1]);
  priv->gsfd = fd[1];

  amsg.ssid = priv->ssid;
  amsg.key  = priv->key;
  amsg.mode = priv->mode;
  amsg.ch   = priv->ch;
  while (true)
    {
      ret = ioctl(priv->gsfd, GS2200M_IOC_ASSOC, (unsigned long)&amsg);

      if (0 == ret)
        {
          break;
        }

      fprintf(stderr, "association failed : retrying\n");
    }

  while (true)
    {
      memset(fds, 0, sizeof(fds));

      /* Check events from usrsock and gs2200m */

      fds[0].fd     = fd[0];
      fds[0].events = POLLIN;
      fds[1].fd     = fd[1];
      fds[1].events = POLLIN;

      ret = poll(fds, 2, -1);
      ASSERT(0 < ret);

      if (fds[0].revents & POLLIN)
        {
          ret = usrsock_request(fd[0], priv);
          ASSERT(0 == ret);
        }

      if (fds[1].revents & POLLIN)
        {
          gs2200m_printf("=== %s: event from /dev/gs2200m \n",
                         __func__);

          /* retrieve cid from gs2200m driver */

          cid = 'z';
          ret = read(fd[1], &cid, sizeof(cid));
          ASSERT(ret == sizeof(cid));

          /* find usock by the cid */

          usock = gs2200m_find_socket_by_cid(priv, cid);

          if (NULL == usock)
            {
              gs2200m_printf("=== %s: cid=%c not found (ignored) \n",
                             __func__, cid);
            }
          else
            {
              /* send event to call xxxx_request() */

              usock_send_event(fd[0], priv, usock,
                               USRSOCK_EVENT_RECVFROM_AVAIL);
            }
        }
    }

  close(fd[1]);
  close(fd[0]);

  gs2200m_printf("finished: ret=%d\n", __func__, ret);

  return ret;
}

/****************************************************************************
 * Name: _show_usage
 ****************************************************************************/

static void _show_usage(FAR char *cmd)
{
  fprintf(stderr,
          "Usage: %s [-a [ch]] ssid passphrase(key) \n\n", cmd);
  fprintf(stderr,
          "AP mode : specify -a option (optionally with channel) with ssid\n"
          "          and 8 to 63 ascii passphrase for WPA2-PSK \n"
          "          or 10 hex digits key for WEP \n");
  fprintf(stderr,
          "STA mode: specify ssid and passphrase for WPA/WPA2 PSK \n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int  option;
  int  ret;
  bool ap_mode = false;

  if (_daemon)
    {
      fprintf(stderr, "%s is already running! \n", argv[0]);
      return -1;
    }

  _daemon = calloc(sizeof(struct gs2200m_s), 1);
  ASSERT(_daemon);

  _daemon->mode = 0; /* default mode = 0 (station) */

  while ((option = getopt(argc, argv, "a:")) != ERROR)
    {
      switch (option)
        {
          case 'a':
            _daemon->mode = 1; /* ap mode */
            _daemon->ch   = 1;

            if (5 == argc)
              {
                _daemon->ch   = (int)atoi(optarg);
              }

            ap_mode = true;
            break;
        }
    }

  if ((ap_mode && (4 != argc) && (5 != argc))
      || (!ap_mode && 3 != argc))
    {
      _show_usage(argv[0]);
      ret = ERROR;
      goto errout;
    }

  _daemon->ssid = argv[argc - 2];
  _daemon->key  = argv[argc - 1];

  ret = gs2200m_loop(_daemon);

errout:

  if (_daemon)
    {
      free(_daemon);
      _daemon = NULL;
    }

  return ret;
}
