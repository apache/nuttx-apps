/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_tcp.c
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#include <nxmodbus/nxmb_raw.h>
#include <nxmodbus/nxmodbus.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MBAP_HEADER_SIZE 7
#define MBAP_FC_SIZE     8
#define NXMB_TCP_RECV_MS 1000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxmb_tcp_client_s
{
  int      fd;
  uint16_t trans_id;
  time_t   last_activity;
};

struct nxmb_tcp_state_s
{
  struct nxmb_tcp_client_s clients[CONFIG_NXMODBUS_TCP_MAX_CLIENTS];
  int                      listen_fd;
  int                      active_idx;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int nxmb_tcp_init(nxmb_handle_t ctx);
static int nxmb_tcp_deinit(nxmb_handle_t ctx);
static int nxmb_tcp_send(nxmb_handle_t ctx);
static int nxmb_tcp_receive(nxmb_handle_t ctx);

/****************************************************************************
 * Public Data
 ****************************************************************************/

const struct nxmb_transport_ops_s g_nxmb_tcp_ops =
{
  .init    = nxmb_tcp_init,
  .deinit  = nxmb_tcp_deinit,
  .send    = nxmb_tcp_send,
  .receive = nxmb_tcp_receive,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_tcp_put_header
 ****************************************************************************/

static void nxmb_tcp_put_header(FAR const struct nxmb_adu_s *adu,
                                FAR uint8_t *hdr)
{
  nxmb_util_put_u16_be(&hdr[0], adu->trans_id);
  nxmb_util_put_u16_be(&hdr[2], adu->proto_id);
  nxmb_util_put_u16_be(&hdr[4], adu->length);
  hdr[6] = adu->unit_id;
  hdr[7] = adu->fc;
}

/****************************************************************************
 * Name: nxmb_tcp_get_header
 ****************************************************************************/

static void nxmb_tcp_get_header(FAR struct nxmb_adu_s *adu,
                                FAR const uint8_t *hdr)
{
  adu->trans_id = nxmb_util_get_u16_be(&hdr[0]);
  adu->proto_id = nxmb_util_get_u16_be(&hdr[2]);
  adu->length   = nxmb_util_get_u16_be(&hdr[4]);
  adu->unit_id  = hdr[6];
  adu->fc       = hdr[7];
}

/****************************************************************************
 * Name: nxmb_tcp_create_server
 ****************************************************************************/

static int nxmb_tcp_create_server(FAR const struct nxmb_tcp_config_s *cfg)
{
  struct sockaddr_in addr;
  int                listen_fd;
  int                ret;
#ifdef CONFIG_NET_SOCKOPTS
  int                opt = 1;
#endif

  listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0)
    {
      return -errno;
    }

#ifdef CONFIG_NET_SOCKOPTS
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(cfg->port);

  if (cfg->bindaddr != NULL)
    {
      ret = inet_pton(AF_INET, cfg->bindaddr, &addr.sin_addr);
      if (ret != 1)
        {
          close(listen_fd);
          return -EINVAL;
        }
    }
  else
    {
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

  ret = bind(listen_fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      close(listen_fd);
      return -errno;
    }

  ret = listen(listen_fd, CONFIG_NXMODBUS_TCP_MAX_CLIENTS);
  if (ret < 0)
    {
      close(listen_fd);
      return -errno;
    }

  return listen_fd;
}

/****************************************************************************
 * Name: nxmb_tcp_connect_client
 ****************************************************************************/

static int nxmb_tcp_connect_client(FAR const struct nxmb_tcp_config_s *cfg)
{
  struct sockaddr_in addr;
  int                client_fd;
  int                ret;

  if (cfg->host == NULL)
    {
      return -EINVAL;
    }

  client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0)
    {
      return -errno;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(cfg->port);

  ret = inet_pton(AF_INET, cfg->host, &addr.sin_addr);
  if (ret != 1)
    {
      close(client_fd);
      return -EINVAL;
    }

  ret = connect(client_fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      close(client_fd);
      return -errno;
    }

  return client_fd;
}

/****************************************************************************
 * Name: nxmb_tcp_recv_full
 ****************************************************************************/

static int nxmb_tcp_recv_full(int fd, FAR uint8_t *buf, size_t len,
                              int timeout_ms)
{
  struct timeval tv;
  fd_set         readfds;
  size_t         received;
  int            ret;

  received = 0;

  while (received < len)
    {
      FD_ZERO(&readfds);
      FD_SET(fd, &readfds);

      tv.tv_sec  = timeout_ms / 1000;
      tv.tv_usec = (timeout_ms % 1000) * 1000;

      ret = select(fd + 1, &readfds, NULL, NULL, &tv);
      if (ret < 0)
        {
          return -errno;
        }
      else if (ret == 0)
        {
          return -ETIMEDOUT;
        }

      ret = recv(fd, &buf[received], len - received, 0);
      if (ret < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            {
              continue;
            }

          return -errno;
        }
      else if (ret == 0)
        {
          return -ECONNRESET;
        }

      received += ret;
    }

  return received;
}

/****************************************************************************
 * Name: nxmb_tcp_init
 ****************************************************************************/

static int nxmb_tcp_init(nxmb_handle_t ctx)
{
  FAR const struct nxmb_tcp_config_s *cfg;
  FAR struct nxmb_tcp_state_s        *state;
  int                                 fd;
  int                                 i;

  DEBUGASSERT(ctx);

  cfg = &ctx->transport_cfg.tcp;

  state = calloc(1, sizeof(struct nxmb_tcp_state_s));
  if (state == NULL)
    {
      return -ENOMEM;
    }

  state->listen_fd  = -1;
  state->active_idx = -1;

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      state->clients[i].fd = -1;
    }

  if (ctx->is_client)
    {
      fd = nxmb_tcp_connect_client(cfg);
      if (fd < 0)
        {
          free(state);
          return fd;
        }

      state->clients[0].fd            = fd;
      state->clients[0].last_activity = time(NULL);
    }
  else
    {
      fd = nxmb_tcp_create_server(cfg);
      if (fd < 0)
        {
          free(state);
          return fd;
        }

      state->listen_fd = fd;
    }

  ctx->transport_state = state;

  return OK;
}

/****************************************************************************
 * Name: nxmb_tcp_deinit
 ****************************************************************************/

static int nxmb_tcp_deinit(nxmb_handle_t ctx)
{
  FAR struct nxmb_tcp_state_s *state;
  int                          i;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      if (state->clients[i].fd >= 0)
        {
          close(state->clients[i].fd);
        }
    }

  if (state->listen_fd >= 0)
    {
      close(state->listen_fd);
    }

  free(state);
  ctx->transport_state = NULL;

  return OK;
}

/****************************************************************************
 * Name: nxmb_tcp_send
 ****************************************************************************/

static int nxmb_tcp_send(nxmb_handle_t ctx)
{
  FAR struct nxmb_tcp_state_s  *state;
  FAR struct nxmb_tcp_client_s *client;
  uint8_t                       header[MBAP_FC_SIZE];
  struct iovec                  iov[2];
  uint16_t                      data_len;
  uint16_t                      total;
  ssize_t                       sent;
  int                           fd;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;

  /* For server mode, respond to the client that sent the request.
   * For client mode, use the single connection (index 0).
   */

  if (ctx->is_client)
    {
      client = &state->clients[0];
    }
  else
    {
      if (state->active_idx < 0)
        {
          return -ENOTCONN;
        }

      client = &state->clients[state->active_idx];
    }

  fd = client->fd;
  if (fd < 0)
    {
      return -ENOTCONN;
    }

  if (ctx->adu.length < 2)
    {
      return -EINVAL;
    }

  /* Populate the MBAP fields before serializing the header */

  ctx->adu.trans_id = client->trans_id;
  ctx->adu.proto_id = 0x0000;

  data_len = ctx->adu.length - 2;

  /* Emit MBAP header + PDU data via writev() to avoid a stack-side
   * copy. The kernel gathers both iovecs into a single TCP segment.
   */

  nxmb_tcp_put_header(&ctx->adu, header);

  iov[0].iov_base = header;
  iov[0].iov_len  = MBAP_FC_SIZE;
  iov[1].iov_base = ctx->adu.data;
  iov[1].iov_len  = data_len;

  total = MBAP_FC_SIZE + data_len;
  sent  = writev(fd, iov, (data_len > 0) ? 2 : 1);
  if (sent != total)
    {
      return (sent < 0) ? -errno : -EIO;
    }

  client->last_activity = time(NULL);

  return OK;
}

/****************************************************************************
 * Name: nxmb_tcp_evict_idle
 *
 * Description:
 *   Close idle client connections that exceed the configured timeout.
 *
 ****************************************************************************/

static void nxmb_tcp_evict_idle(FAR struct nxmb_tcp_state_s *state,
                                time_t now)
{
  int i;

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      if (state->clients[i].fd >= 0 &&
          (now - state->clients[i].last_activity) >
          CONFIG_NXMODBUS_TCP_TIMEOUT_SEC)
        {
          close(state->clients[i].fd);
          state->clients[i].fd = -1;
        }
    }
}

/****************************************************************************
 * Name: nxmb_tcp_accept_new
 *
 * Description:
 *   Accept a pending connection into the first free client slot.
 *
 ****************************************************************************/

static void nxmb_tcp_accept_new(FAR struct nxmb_tcp_state_s *state)
{
  int fd;
  int i;

  fd = accept(state->listen_fd, NULL, NULL);
  if (fd < 0)
    {
      return;
    }

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      if (state->clients[i].fd < 0)
        {
          state->clients[i].fd            = fd;
          state->clients[i].last_activity = time(NULL);
          return;
        }
    }

  /* No free slot -- reject the connection */

  close(fd);
}

/****************************************************************************
 * Name: nxmb_tcp_read_frame
 *
 * Description:
 *   Read a complete Modbus TCP frame from a client connection.
 *   On error (except timeout), the connection is closed.
 *
 ****************************************************************************/

static int nxmb_tcp_read_frame(nxmb_handle_t ctx,
                               FAR struct nxmb_tcp_client_s *client)
{
  uint8_t  header[MBAP_FC_SIZE];
  uint16_t data_len;
  int      ret;

  ret = nxmb_tcp_recv_full(client->fd, header, MBAP_FC_SIZE,
                           NXMB_TCP_RECV_MS);
  if (ret < 0)
    {
      if (ret == -ETIMEDOUT || ret == -EAGAIN)
        {
          return -EAGAIN;
        }

      close(client->fd);
      client->fd = -1;
      return ret;
    }

  nxmb_tcp_get_header(&ctx->adu, header);

  if (ctx->adu.proto_id != 0x0000)
    {
      close(client->fd);
      client->fd = -1;
      return -EPROTO;
    }

  if (ctx->adu.length < 2)
    {
      close(client->fd);
      client->fd = -1;
      return -EINVAL;
    }

  data_len = ctx->adu.length - 2;

  if (data_len > NXMB_ADU_DATA_MAX)
    {
      close(client->fd);
      client->fd = -1;
      return -EMSGSIZE;
    }

  if (data_len > 0)
    {
      ret = nxmb_tcp_recv_full(client->fd, ctx->adu.data, data_len,
                               NXMB_TCP_RECV_MS);
      if (ret < 0)
        {
          close(client->fd);
          client->fd = -1;
          return ret;
        }
    }

  client->trans_id      = ctx->adu.trans_id;
  client->last_activity = time(NULL);

  return ctx->adu.length;
}

static int nxmb_tcp_receive(nxmb_handle_t ctx)
{
  FAR struct nxmb_tcp_state_s *state;
  struct timeval               tv;
  fd_set                       readfds;
  time_t                       now;
  int                          maxfd;
  int                          ret;
  int                          i;

  DEBUGASSERT(ctx && ctx->transport_state);

  state = ctx->transport_state;
  state->active_idx = -1;

  /* Client mode: single connection, read directly */

  if (ctx->is_client)
    {
      if (state->clients[0].fd < 0)
        {
          return -ENOTCONN;
        }

      state->active_idx = 0;
      return nxmb_tcp_read_frame(ctx, &state->clients[0]);
    }

  /* Server mode: multiplex listen socket + all client connections */

  now = time(NULL);
  nxmb_tcp_evict_idle(state, now);

  FD_ZERO(&readfds);
  maxfd = -1;

  if (state->listen_fd >= 0)
    {
      FD_SET(state->listen_fd, &readfds);
      maxfd = state->listen_fd;
    }

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      if (state->clients[i].fd >= 0)
        {
          FD_SET(state->clients[i].fd, &readfds);
          if (state->clients[i].fd > maxfd)
            {
              maxfd = state->clients[i].fd;
            }
        }
    }

  if (maxfd < 0)
    {
      return -EAGAIN;
    }

  tv.tv_sec  = 0;
  tv.tv_usec = 50000;

  ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
  if (ret < 0)
    {
      return -errno;
    }

  if (ret == 0)
    {
      return -EAGAIN;
    }

  /* Accept new connections if listen socket is ready */

  if (state->listen_fd >= 0 && FD_ISSET(state->listen_fd, &readfds))
    {
      nxmb_tcp_accept_new(state);
    }

  /* Check each client for incoming data (first ready wins).
   * In server mode, a per-client read failure (disconnect, protocol
   * error, etc.) must not be fatal - nxmb_tcp_read_frame has already
   * closed the offending fd, and the listen socket remains open for new
   * connections. Translate any such error to -EAGAIN so the caller
   * keeps polling.
   */

  for (i = 0; i < CONFIG_NXMODBUS_TCP_MAX_CLIENTS; i++)
    {
      if (state->clients[i].fd >= 0 &&
          FD_ISSET(state->clients[i].fd, &readfds))
        {
          state->active_idx = i;
          ret = nxmb_tcp_read_frame(ctx, &state->clients[i]);
          if (ret < 0 && ret != -EAGAIN)
            {
              state->active_idx = -1;
              return -EAGAIN;
            }

          return ret;
        }
    }

  return -EAGAIN;
}
