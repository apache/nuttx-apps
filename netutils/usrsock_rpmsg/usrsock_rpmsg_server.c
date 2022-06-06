/****************************************************************************
 * apps/netutils/usrsock_rpmsg/usrsock_rpmsg_server.c
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

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include <sys/eventfd.h>
#include <sys/ioctl.h>

#include <nuttx/net/dns.h>
#include <nuttx/net/net.h>
#include <nuttx/rptun/openamp.h>
#ifdef CONFIG_NETDEV_WIRELESS_IOCTL
#  include <nuttx/wireless/wireless.h>
#endif
#include "usrsock_rpmsg.h"

struct usrsock_rpmsg_s
{
  struct file           *eventfp;
  pthread_mutex_t       mutex;
  pthread_cond_t        cond;
  struct iovec          iov[CONFIG_NETUTILS_USRSOCK_NIOVEC];
  struct socket         socks[CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS];
  struct rpmsg_endpoint *epts[CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS];
  struct pollfd         pfds[CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS + 1];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bool usrsock_rpmsg_available(struct socket *psock, int cmd);

static int usrsock_rpmsg_send_ack(struct rpmsg_endpoint *ept,
                                  uint16_t events,
                                  uint64_t xid, int32_t result);
static int usrsock_rpmsg_send_data_ack(struct rpmsg_endpoint *ept,
                                  struct usrsock_message_datareq_ack_s *ack,
                                  uint16_t events,
                                  uint64_t xid, int32_t result,
                                  uint16_t valuelen,
                                  uint16_t valuelen_nontrunc);
static int usrsock_rpmsg_send_event(struct rpmsg_endpoint *ept,
                                    int16_t usockid, uint16_t events);

static int usrsock_rpmsg_socket_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_);
static int usrsock_rpmsg_close_handler(struct rpmsg_endpoint *ept,
                                       void *data, size_t len,
                                       uint32_t src, void *priv_);
static int usrsock_rpmsg_connect_handler(struct rpmsg_endpoint *ept,
                                         void *data, size_t len,
                                         uint32_t src, void *priv_);
static int usrsock_rpmsg_sendto_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_);
static int usrsock_rpmsg_recvfrom_handler(struct rpmsg_endpoint *ept,
                                          void *data, size_t len,
                                          uint32_t src, void *priv_);
static int usrsock_rpmsg_setsockopt_handler(struct rpmsg_endpoint *ept,
                                            void *data, size_t len,
                                            uint32_t src, void *priv_);
static int usrsock_rpmsg_getsockopt_handler(struct rpmsg_endpoint *ept,
                                            void *data, size_t len,
                                            uint32_t src, void *priv_);
static int usrsock_rpmsg_getsockname_handler(struct rpmsg_endpoint *ept,
                                             void *data, size_t len,
                                             uint32_t src, void *priv_);
static int usrsock_rpmsg_getpeername_handler(struct rpmsg_endpoint *ept,
                                             void *data, size_t len,
                                             uint32_t src, void *priv_);
static int usrsock_rpmsg_bind_handler(struct rpmsg_endpoint *ept,
                                      void *data, size_t len,
                                      uint32_t src, void *priv_);
static int usrsock_rpmsg_listen_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_);
static int usrsock_rpmsg_accept_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_);
static int usrsock_rpmsg_ioctl_handler(struct rpmsg_endpoint *ept,
                                       void *data, size_t len,
                                       uint32_t src, void *priv_);
static int usrsock_rpmsg_dns_handler(struct rpmsg_endpoint *ept,
                                     void *data, size_t len,
                                     uint32_t src, void *priv_);

static bool usrsock_rpmsg_ns_match(struct rpmsg_device *rdev, void *priv_,
                                   const char *name, uint32_t dest);
static void usrsock_rpmsg_ns_bind(struct rpmsg_device *rdev, void *priv_,
                                  const char *name, uint32_t dest);
static void usrsock_rpmsg_ns_unbind(struct rpmsg_endpoint *ept);
static int usrsock_rpmsg_ept_cb(struct rpmsg_endpoint *ept, void *data,
                                size_t len, uint32_t src, void *priv);

static int usrsock_rpmsg_notify_poll(struct usrsock_rpmsg_s *priv);
static int usrsock_rpmsg_prepare_poll(struct usrsock_rpmsg_s *priv,
                                      struct pollfd *pfds);
static bool usrsock_rpmsg_process_poll(struct usrsock_rpmsg_s *priv,
                                       struct pollfd *pfds, int count);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const rpmsg_ept_cb g_usrsock_rpmsg_handler[] =
{
  [USRSOCK_REQUEST_SOCKET]      = usrsock_rpmsg_socket_handler,
  [USRSOCK_REQUEST_CLOSE]       = usrsock_rpmsg_close_handler,
  [USRSOCK_REQUEST_CONNECT]     = usrsock_rpmsg_connect_handler,
  [USRSOCK_REQUEST_SENDTO]      = usrsock_rpmsg_sendto_handler,
  [USRSOCK_REQUEST_RECVFROM]    = usrsock_rpmsg_recvfrom_handler,
  [USRSOCK_REQUEST_SETSOCKOPT]  = usrsock_rpmsg_setsockopt_handler,
  [USRSOCK_REQUEST_GETSOCKOPT]  = usrsock_rpmsg_getsockopt_handler,
  [USRSOCK_REQUEST_GETSOCKNAME] = usrsock_rpmsg_getsockname_handler,
  [USRSOCK_REQUEST_GETPEERNAME] = usrsock_rpmsg_getpeername_handler,
  [USRSOCK_REQUEST_BIND]        = usrsock_rpmsg_bind_handler,
  [USRSOCK_REQUEST_LISTEN]      = usrsock_rpmsg_listen_handler,
  [USRSOCK_REQUEST_ACCEPT]      = usrsock_rpmsg_accept_handler,
  [USRSOCK_REQUEST_IOCTL]       = usrsock_rpmsg_ioctl_handler,
  [USRSOCK_RPMSG_DNS_REQUEST]   = usrsock_rpmsg_dns_handler,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool usrsock_rpmsg_available(struct socket *psock, int cmd)
{
  int len;

  if (psock_ioctl(psock, cmd, &len, sizeof(len)) == 0)
    {
      if (len > 0)
        {
          return true;
        }
    }

  return false;
}

static int usrsock_rpmsg_send_ack(struct rpmsg_endpoint *ept,
                                  uint16_t events,
                                  uint64_t xid, int32_t result)
{
  struct usrsock_message_req_ack_s ack;

  ack.head.msgid  = USRSOCK_MESSAGE_RESPONSE_ACK;
  ack.head.flags  = (result == -EINPROGRESS);
  ack.head.events = events;

  ack.xid    = xid;
  ack.result = result;

  return rpmsg_send(ept, &ack, sizeof(ack));
}

static int usrsock_rpmsg_send_data_ack(struct rpmsg_endpoint *ept,
                                  struct usrsock_message_datareq_ack_s *ack,
                                  uint16_t events,
                                  uint64_t xid, int32_t result,
                                  uint16_t valuelen,
                                  uint16_t valuelen_nontrunc)
{
  ack->reqack.head.msgid  = USRSOCK_MESSAGE_RESPONSE_DATA_ACK;
  ack->reqack.head.flags  = 0;
  ack->reqack.head.events = events;

  ack->reqack.xid    = xid;
  ack->reqack.result = result;

  if (result < 0)
    {
      result             = 0;
      valuelen           = 0;
      valuelen_nontrunc  = 0;
    }
  else if (valuelen > valuelen_nontrunc)
    {
      valuelen           = valuelen_nontrunc;
    }

  ack->valuelen          = valuelen;
  ack->valuelen_nontrunc = valuelen_nontrunc;

  return rpmsg_send_nocopy(ept, ack, sizeof(*ack) + valuelen + result);
}

static int usrsock_rpmsg_send_event(struct rpmsg_endpoint *ept,
                                    int16_t usockid, uint16_t events)
{
  struct usrsock_message_socket_event_s event;

  event.head.msgid  = USRSOCK_MESSAGE_SOCKET_EVENT;
  event.head.flags  = USRSOCK_MESSAGE_FLAG_EVENT;
  event.head.events = events;

  event.usockid = usockid;

  return rpmsg_send(ept, &event, sizeof(event));
}

static int usrsock_rpmsg_socket_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_)
{
  struct usrsock_request_socket_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  int i;
  int retr;
  int ret = -ENFILE;

  for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS; i++)
    {
      pthread_mutex_lock(&priv->mutex);
      if (priv->socks[i].s_conn == NULL)
        {
          ret = psock_socket(req->domain, req->type | SOCK_NONBLOCK,
                             req->protocol, &priv->socks[i]);
          pthread_mutex_unlock(&priv->mutex);
          if (ret >= 0)
            {
              priv->epts[i] = ept;
              ret = i; /* Return index as the usockid */
            }

          break;
        }

      pthread_mutex_unlock(&priv->mutex);
    }

  retr = usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
  if (retr >= 0 && ret >= 0 &&
      req->type != SOCK_STREAM && req->type != SOCK_SEQPACKET)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[ret].ptr = &priv->socks[ret];
      priv->pfds[ret].events = POLLIN;
      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
      retr = usrsock_rpmsg_send_event(ept, ret, USRSOCK_EVENT_SENDTO_READY);
    }

  return retr;
}

static int usrsock_rpmsg_close_handler(struct rpmsg_endpoint *ept,
                                       void *data, size_t len,
                                       uint32_t src, void *priv_)
{
  struct usrsock_request_close_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  int ret = -EBADF;

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      priv->pfds[req->usockid].ptr = NULL;
      priv->epts[req->usockid] = NULL;

      /* Signal and wait the poll thread to wakeup */

      pthread_mutex_lock(&priv->mutex);
      usrsock_rpmsg_notify_poll(priv);
      pthread_cond_wait(&priv->cond, &priv->mutex);
      pthread_mutex_unlock(&priv->mutex);

      /* It's safe to close sock here */

      ret = psock_close(&priv->socks[req->usockid]);
    }

  return usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
}

static int usrsock_rpmsg_connect_handler(struct rpmsg_endpoint *ept,
                                         void *data, size_t len,
                                         uint32_t src, void *priv_)
{
  struct usrsock_request_connect_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  bool inprogress = false;
  int retr;
  int ret = -EBADF;

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_connect(&priv->socks[req->usockid],
              (const struct sockaddr *)(req + 1), req->addrlen);
      if (ret == -EINPROGRESS)
        {
          inprogress = true;
          ret = 0;
        }
    }

  retr = usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
  if (retr >= 0 && ret >= 0 && priv->pfds[req->usockid].ptr == NULL)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[req->usockid].ptr = &priv->socks[req->usockid];
      priv->pfds[req->usockid].events = POLLIN;
      if (inprogress)
        {
          priv->pfds[req->usockid].events |= POLLOUT;
        }

      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
      if (!inprogress)
        {
          retr = usrsock_rpmsg_send_event(ept,
            req->usockid, USRSOCK_EVENT_SENDTO_READY);
        }
    }

  return retr;
}

static int usrsock_rpmsg_sendto_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_)
{
  struct usrsock_request_sendto_s *req;
  struct usrsock_rpmsg_s *priv = priv_;
  uint16_t events = 0;
  ssize_t ret = -EBADF;
  size_t total;
  int retr;
  int i;

  if (priv->iov[0].iov_base)
    {
      size_t hlen;
      struct msghdr msg =
      {
      };

      req = priv->iov[0].iov_base;
      hlen = sizeof(*req) + req->addrlen;

      total = len;
      for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NIOVEC; i++)
        {
          if (!priv->iov[i].iov_base)
            {
              priv->iov[i].iov_base = data;
              priv->iov[i].iov_len = len;
              rpmsg_hold_rx_buffer(ept, data);
              break;
            }

          total += priv->iov[i].iov_len;
        }

      if (i == CONFIG_NETUTILS_USRSOCK_NIOVEC)
        {
          ret = -ENOMEM;
          goto out;
        }

      /* Partial packet ? continue to fetch */

      if (req->buflen > total - hlen)
        {
          return 0;
        }
      else if (req->buflen < total - hlen)
        {
          ret = -EINVAL;
          goto out;
        }

      /* Skip the sendto header from I/O vector */

      priv->iov[0].iov_base = (char *)priv->iov[0].iov_base + hlen;
      priv->iov[0].iov_len -= hlen;

      msg.msg_name = req->addrlen ? (void *)(req + 1) : NULL;
      msg.msg_namelen = req->addrlen;
      msg.msg_iov = priv->iov;
      msg.msg_iovlen = i + 1;

      ret = psock_sendmsg(&priv->socks[req->usockid], &msg, req->flags);

      /* Recover the I/O vector */

      priv->iov[0].iov_base = (char *)priv->iov[0].iov_base - hlen;
      priv->iov[0].iov_len += hlen;
    }
  else
    {
      req = data;

      if (req->usockid >= 0 &&
          req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
        {
          total = sizeof(*req) + req->addrlen + req->buflen;
          if (total > len)
            {
              priv->iov[0].iov_base = data;
              priv->iov[0].iov_len = len;

              rpmsg_hold_rx_buffer(ept, data);
              return 0;
            }
          else
            {
              ret = psock_sendto(&priv->socks[req->usockid],
                  (const void *)(req + 1) + req->addrlen, req->buflen,
                  req->flags,
                  req->addrlen ? (const struct sockaddr *)(req + 1) : NULL,
                  req->addrlen);
            }
        }
    }

out:

  if (ret > 0 &&
      usrsock_rpmsg_available(&priv->socks[req->usockid], FIONSPACE))
    {
      events |= USRSOCK_EVENT_SENDTO_READY;
    }

  retr = usrsock_rpmsg_send_ack(ept, events, req->head.xid, ret);
  if (retr >= 0 && events == 0)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[req->usockid].events |= POLLOUT;
      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
    }

  if (priv->iov[0].iov_base)
    {
      for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NIOVEC; i++)
        {
          if (priv->iov[i].iov_base == NULL)
            {
              break;
            }

            rpmsg_release_rx_buffer(ept, priv->iov[i].iov_base);
            priv->iov[i].iov_base = NULL;
            priv->iov[i].iov_len = 0;
        }
    }

  return retr;
}

static int usrsock_rpmsg_recvfrom_handler(struct rpmsg_endpoint *ept,
                                          void *data, size_t len_,
                                          uint32_t src, void *priv_)
{
  struct usrsock_request_recvfrom_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
  socklen_t outaddrlen = req->max_addrlen;
  socklen_t inaddrlen = req->max_addrlen;
  size_t buflen = req->max_buflen;
  ssize_t ret = -EBADF;
  uint16_t events = 0;
  uint32_t len;
  int retr;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (sizeof(*ack) + inaddrlen + buflen > len)
    {
      buflen = len - sizeof(*ack) - inaddrlen;
    }

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_recvfrom(&priv->socks[req->usockid],
              (void *)(ack + 1) + inaddrlen, buflen, req->flags,
              outaddrlen ? (struct sockaddr *)(ack + 1) : NULL,
              outaddrlen ? &outaddrlen : NULL);
      if (ret > 0)
        {
          if (outaddrlen < inaddrlen)
            {
              memcpy((void *)(ack + 1) + outaddrlen,
                     (void *)(ack + 1) + inaddrlen, ret);
            }

          if (usrsock_rpmsg_available(&priv->socks[req->usockid], FIONREAD))
            {
              events |= USRSOCK_EVENT_RECVFROM_AVAIL;
            }
        }
    }

  retr = usrsock_rpmsg_send_data_ack(ept,
            ack, events, req->head.xid, ret, inaddrlen, outaddrlen);
  if (retr >= 0 && events == 0)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[req->usockid].events |= POLLIN;
      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
    }

  return retr;
}

static int usrsock_rpmsg_setsockopt_handler(struct rpmsg_endpoint *ept,
                                            void *data, size_t len,
                                            uint32_t src, void *priv_)
{
  struct usrsock_request_setsockopt_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  int ret = -EBADF;

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_setsockopt(&priv->socks[req->usockid],
              req->level, req->option, req + 1, req->valuelen);
    }

  return usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
}

static int usrsock_rpmsg_getsockopt_handler(struct rpmsg_endpoint *ept,
                                            void *data, size_t len_,
                                            uint32_t src, void *priv_)
{
  struct usrsock_request_getsockopt_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
  socklen_t optlen = req->max_valuelen;
  int ret = -EBADF;
  uint32_t len;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_getsockopt(&priv->socks[req->usockid],
              req->level, req->option, ack + 1, &optlen);
    }

  return usrsock_rpmsg_send_data_ack(ept,
          ack, 0, req->head.xid, ret, optlen, optlen);
}

static int usrsock_rpmsg_getsockname_handler(struct rpmsg_endpoint *ept,
                                             void *data, size_t len_,
                                             uint32_t src, void *priv_)
{
  struct usrsock_request_getsockname_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
  socklen_t outaddrlen = req->max_addrlen;
  socklen_t inaddrlen = req->max_addrlen;
  int ret = -EBADF;
  uint32_t len;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_getsockname(&priv->socks[req->usockid],
              (struct sockaddr *)(ack + 1), &outaddrlen);
    }

  return usrsock_rpmsg_send_data_ack(ept,
          ack, 0, req->head.xid, ret, inaddrlen, outaddrlen);
}

static int usrsock_rpmsg_getpeername_handler(struct rpmsg_endpoint *ept,
                                             void *data, size_t len_,
                                             uint32_t src, void *priv_)
{
  struct usrsock_request_getpeername_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
  socklen_t outaddrlen = req->max_addrlen;
  socklen_t inaddrlen = req->max_addrlen;
  int ret = -EBADF;
  uint32_t len;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_getpeername(&priv->socks[req->usockid],
              (struct sockaddr *)(ack + 1), &outaddrlen);
    }

  return usrsock_rpmsg_send_data_ack(ept,
          ack, 0, req->head.xid, ret, inaddrlen, outaddrlen);
}

static int usrsock_rpmsg_bind_handler(struct rpmsg_endpoint *ept,
                                      void *data, size_t len,
                                      uint32_t src, void *priv_)
{
  struct usrsock_request_bind_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  int ret = -EBADF;

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_bind(&priv->socks[req->usockid],
              (const struct sockaddr *)(req + 1), req->addrlen);
    }

  return usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
}

static int usrsock_rpmsg_listen_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len,
                                        uint32_t src, void *priv_)
{
  struct usrsock_request_listen_s *req = data;
  struct usrsock_rpmsg_s *priv = priv_;
  int retr;
  int ret = -EBADF;

  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = psock_listen(&priv->socks[req->usockid], req->backlog);
    }

  retr = usrsock_rpmsg_send_ack(ept, 0, req->head.xid, ret);
  if (retr >= 0 && ret >= 0)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[req->usockid].ptr = &priv->socks[req->usockid];
      priv->pfds[req->usockid].events = POLLIN;
      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
    }

  return retr;
}

static int usrsock_rpmsg_accept_handler(struct rpmsg_endpoint *ept,
                                        void *data, size_t len_,
                                        uint32_t src, void *priv_)
{
  struct usrsock_request_accept_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
  socklen_t outaddrlen = req->max_addrlen;
  socklen_t inaddrlen = req->max_addrlen;
  int ret = -EBADF;
  uint32_t len;
  int i = 0;
  int retr;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      ret = -ENFILE; /* Assume no free socket handler */
      for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS; i++)
        {
          pthread_mutex_lock(&priv->mutex);
          if (priv->socks[i].s_conn == NULL)
            {
              ret = psock_accept(&priv->socks[req->usockid],
                      outaddrlen ? (struct sockaddr *)(ack + 1) : NULL,
                      outaddrlen ? &outaddrlen : NULL, &priv->socks[i]);
              pthread_mutex_unlock(&priv->mutex);
              if (ret >= 0)
                {
                  int nonblock = 1;

                  psock_ioctl(&priv->socks[i], FIONBIO, &nonblock);
                  priv->epts[i] = ept;

                  /* Append index as usockid to the payload */

                  if (outaddrlen <= inaddrlen)
                    {
                      *(int16_t *)((void *)(ack + 1) + outaddrlen) = i;
                    }
                  else
                    {
                      *(int16_t *)((void *)(ack + 1) + inaddrlen) = i;
                    }

                  ret = sizeof(int16_t); /* Return usockid size */
                }

              break;
            }

          pthread_mutex_unlock(&priv->mutex);
        }
    }

  retr = usrsock_rpmsg_send_data_ack(ept,
    ack, 0, req->head.xid, ret, inaddrlen, outaddrlen);
  if (retr >= 0 && ret >= 0)
    {
      pthread_mutex_lock(&priv->mutex);
      priv->pfds[i].ptr = &priv->socks[i];
      priv->pfds[i].events = POLLIN;
      priv->pfds[req->usockid].events |= POLLIN;
      usrsock_rpmsg_notify_poll(priv);
      pthread_mutex_unlock(&priv->mutex);
      usrsock_rpmsg_send_event(ept, i, USRSOCK_EVENT_SENDTO_READY);
    }

  return retr;
}

static int usrsock_rpmsg_ioctl_handler(struct rpmsg_endpoint *ept,
                                       void *data, size_t len_,
                                       uint32_t src, void *priv_)
{
  struct usrsock_request_ioctl_s *req = data;
  struct usrsock_message_datareq_ack_s *ack;
  struct usrsock_rpmsg_s *priv = priv_;
#ifdef CONFIG_NETDEV_WIRELESS_IOCTL
  struct iwreq *wlreq;
  struct iwreq *wlack;
#endif
  int ret = -EBADF;
  uint32_t len;

  ack = rpmsg_get_tx_payload_buffer(ept, &len, true);
  if (req->usockid >= 0 &&
      req->usockid < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS)
    {
      memcpy(ack + 1, req + 1, len_ - sizeof(*req));
#ifdef CONFIG_NETDEV_WIRELESS_IOCTL
      wlreq = (struct iwreq *)(req + 1);
      wlack = (struct iwreq *)(ack + 1);
      if (WL_IS80211POINTERCMD(req->cmd))
        {
          wlack->u.data.pointer = wlack + 1;
        }
#endif

      ret = psock_ioctl(&priv->socks[req->usockid],
              req->cmd, (unsigned long)(ack + 1));

#ifdef CONFIG_NETDEV_WIRELESS_IOCTL
      if (WL_IS80211POINTERCMD(req->cmd))
        {
          if (ret >= 0)
            {
              ret = wlreq->u.data.length;
            }

          wlack->u.data.pointer = wlreq->u.data.pointer;
        }
#endif
    }

  return usrsock_rpmsg_send_data_ack(ept,
           ack, 0, req->head.xid, ret, req->arglen, req->arglen);
}

static int usrsock_rpmsg_dns_handler(struct rpmsg_endpoint *ept, void *data,
                                     size_t len, uint32_t src, void *priv_)
{
#ifdef CONFIG_NETDB_DNSCLIENT
  struct usrsock_rpmsg_dns_request_s *dns = data;

  dns_add_nameserver((struct sockaddr *)(dns + 1), dns->addrlen);
#endif

  return 0;
}

#ifdef CONFIG_NETDB_DNSCLIENT
static int usrsock_rpmsg_send_dns_event(void *arg,
                                        struct sockaddr *addr,
                                        socklen_t addrlen)
{
  struct rpmsg_endpoint *ept = arg;
  struct usrsock_rpmsg_dns_event_s *dns;
  uint32_t len;

  dns = rpmsg_get_tx_payload_buffer(ept, &len, true);

  dns->head.msgid = USRSOCK_RPMSG_DNS_EVENT;
  dns->head.flags = USRSOCK_MESSAGE_FLAG_EVENT;

  dns->addrlen = addrlen;
  memcpy(dns + 1, addr, addrlen);

  return rpmsg_send_nocopy(ept, dns, sizeof(*dns) + addrlen);
}
#endif

static bool usrsock_rpmsg_ns_match(struct rpmsg_device *rdev, void *priv_,
                                   const char *name, uint32_t dest)
{
  return !strcmp(name, USRSOCK_RPMSG_EPT_NAME);
}

static void usrsock_rpmsg_ns_bind(struct rpmsg_device *rdev, void *priv_,
                                  const char *name, uint32_t dest)
{
  struct usrsock_rpmsg_s *priv = priv_;
  struct rpmsg_endpoint *ept;
  int ret;

  ept = zalloc(sizeof(struct rpmsg_endpoint));
  if (!ept)
    {
      return;
    }

  ept->priv = priv;

  ret = rpmsg_create_ept(ept, rdev, USRSOCK_RPMSG_EPT_NAME,
                         RPMSG_ADDR_ANY, dest,
                         usrsock_rpmsg_ept_cb, usrsock_rpmsg_ns_unbind);
  if (ret)
    {
      free(ept);
      return;
    }

#ifdef CONFIG_NETDB_DNSCLIENT
  dns_register_notify(usrsock_rpmsg_send_dns_event, ept);
#endif
}

static void usrsock_rpmsg_ns_unbind(struct rpmsg_endpoint *ept)
{
  struct usrsock_rpmsg_s *priv = ept->priv;
  struct socket *socks[CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS];
  int count = 0;
  int i;

#ifdef CONFIG_NETDB_DNSCLIENT
  dns_unregister_notify(usrsock_rpmsg_send_dns_event, ept);
#endif

  /* Collect all socks belong to the dead client */

  for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS; i++)
    {
      if (priv->epts[i] == ept)
        {
          socks[count++] = &priv->socks[i];
          priv->pfds[i].ptr = NULL;
          priv->epts[i] = NULL;
        }
    }

  /* Signal and wait the poll thread to wakeup */

  pthread_mutex_lock(&priv->mutex);
  usrsock_rpmsg_notify_poll(priv);
  pthread_cond_wait(&priv->cond, &priv->mutex);
  pthread_mutex_unlock(&priv->mutex);

  /* It's safe to close all socks here */

  for (i = 0; i < count; i++)
    {
      psock_close(socks[i]);
    }

  rpmsg_destroy_ept(ept);
}

static int usrsock_rpmsg_ept_cb(struct rpmsg_endpoint *ept, void *data,
                                size_t len, uint32_t src, void *priv_)
{
  struct usrsock_request_common_s *common = data;
  struct usrsock_rpmsg_s *priv = priv_;

  if (priv->iov[0].iov_base)
    {
      return usrsock_rpmsg_sendto_handler(ept, data, len, src, priv);
    }
  else if (common->reqid >= 0 && common->reqid <= USRSOCK_REQUEST__MAX)
    {
      return g_usrsock_rpmsg_handler[common->reqid](ept, data, len,
                                                    src, priv);
    }

  return -EINVAL;
}

static int usrsock_rpmsg_notify_poll(struct usrsock_rpmsg_s *priv)
{
  eventfd_t value = 1ULL;

  return file_write(priv->eventfp, &value, sizeof(value));
}

static int usrsock_rpmsg_prepare_poll(struct usrsock_rpmsg_s *priv,
                                      struct pollfd *pfds)
{
  int count = 0;
  int i;

  pthread_mutex_lock(&priv->mutex);

  /* Signal the worker it's safe to close sock */

  pthread_cond_signal(&priv->cond);

  for (i = 0; i < CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS; i++)
    {
      if (priv->pfds[i].ptr)
        {
          pfds[count] = priv->pfds[i];
          pfds[count++].events |= POLLERR | POLLHUP | POLLSOCK;
        }
    }

  pfds[count].ptr = priv->eventfp;
  pfds[count++].events = POLLIN | POLLFILE;

  pthread_mutex_unlock(&priv->mutex);

  return count;
}

static bool usrsock_rpmsg_process_poll(struct usrsock_rpmsg_s *priv,
                                       struct pollfd *pfds, int count)
{
  bool prepare = false;
  int i;

  for (i = 0; i < count; i++)
    {
      pthread_mutex_lock(&priv->mutex);

      if (pfds[i].ptr == priv->eventfp)
        {
          if (pfds[i].revents & POLLIN)
            {
              eventfd_t value;

              file_read(priv->eventfp, &value, sizeof(value));
            }

          prepare = true;
        }
      else
        {
          int j;

          j = (struct socket *)pfds[i].ptr - priv->socks;

          if (priv->epts[j] != NULL)
            {
              int events = 0;

              if (pfds[i].revents & POLLIN)
                {
                  events |= USRSOCK_EVENT_RECVFROM_AVAIL;

                  /* Stop poll in until recv get called */

                  pfds[i].events &= ~POLLIN;
                  priv->pfds[j].events &= ~POLLIN;
                }

              if (pfds[i].revents & POLLOUT)
                {
                  events |= USRSOCK_EVENT_SENDTO_READY;

                  /* Stop poll out until send get called */

                  pfds[i].events &= ~POLLOUT;
                  priv->pfds[j].events &= ~POLLOUT;
                }

              if (pfds[i].revents & (POLLHUP | POLLERR))
                {
                  events |= USRSOCK_EVENT_REMOTE_CLOSED;

                  /* Check data that has not been recv */

                  if (usrsock_rpmsg_available(&priv->socks[j], FIONREAD))
                    {
                      events |= USRSOCK_EVENT_RECVFROM_AVAIL;
                    }

                  /* Stop poll at all */

                  pfds[i].ptr = NULL;
                  priv->pfds[j].ptr = NULL;
                }

              if (events != 0)
                {
                  usrsock_rpmsg_send_event(priv->epts[j], j, events);
                }
            }
        }

      pthread_mutex_unlock(&priv->mutex);
    }

  return prepare;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  struct pollfd pfds[CONFIG_NETUTILS_USRSOCK_NSOCK_DESCRIPTORS + 1];
  struct usrsock_rpmsg_s *priv;
  bool prepare = true;
  int ret;
  int fd;

  priv = calloc(1, sizeof(*priv));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  pthread_mutex_init(&priv->mutex, NULL);
  pthread_cond_init(&priv->cond, NULL);

  fd = eventfd(0, 0);
  if (fd < 0)
    {
      ret = -errno;
      goto free_priv;
    }

  ret = fs_getfilep(fd, &priv->eventfp);
  if (ret < 0)
    {
      goto free_fd;
    }

  ret = rpmsg_register_callback(priv,
                                NULL,
                                NULL,
                                usrsock_rpmsg_ns_match,
                                usrsock_rpmsg_ns_bind);
  if (ret < 0)
    {
      goto free_fd;
    }

  while (1)
    {
      /* Collect all socks which need monitor */

      if (prepare)
        {
          ret = usrsock_rpmsg_prepare_poll(priv, pfds);
        }

      /* Monitor the state change from them */

      if (poll(pfds, ret, -1) > 0)
        {
          /* Process all changed socks */

          prepare = usrsock_rpmsg_process_poll(priv, pfds, ret);
        }
    }

  rpmsg_unregister_callback(priv,
                            NULL,
                            NULL,
                            usrsock_rpmsg_ns_match,
                            usrsock_rpmsg_ns_bind);
free_fd:
  close(fd);
free_priv:
  pthread_cond_destroy(&priv->cond);
  pthread_mutex_destroy(&priv->mutex);
  free(priv);
  return ret;
}
