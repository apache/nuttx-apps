/****************************************************************************
 * apps/logging/nxscope/nxscope_iudp.c
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

#include <nuttx/debug.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <logging/nxscope/nxscope.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxscope_intf_udp_s
{
  FAR struct nxscope_udp_cfg_s *cfg;
  int                           fd;
  bool                          peer_valid;
  struct sockaddr_in            peer;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

static int nxscope_udp_send(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len);
static int nxscope_udp_recv(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxscope_intf_ops_s g_nxscope_udp_ops =
{
  nxscope_udp_send,
  nxscope_udp_recv
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_udp_send
 ****************************************************************************/

static int nxscope_udp_send(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_udp_s *priv = NULL;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_udp_s *)intf->priv;

  if (!priv->peer_valid)
    {
      return -ENOTCONN;
    }

  return sendto(priv->fd, buff, len, 0,
                (FAR const struct sockaddr *)&priv->peer,
                sizeof(priv->peer));
}

/****************************************************************************
 * Name: nxscope_udp_recv
 ****************************************************************************/

static int nxscope_udp_recv(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_udp_s *priv = NULL;
  struct sockaddr_in             from;
  socklen_t                      fromlen;
  int                            ret = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_udp_s *)intf->priv;

  fromlen = sizeof(from);
  ret     = recvfrom(priv->fd, buff, len, 0, (FAR struct sockaddr *)&from,
                     &fromlen);

  if (ret < 0)
    {
      if (priv->cfg->nonblock && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
          ret = 0;
        }

      return ret;
    }

  priv->peer       = from;
  priv->peer_valid = true;

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_udp_init
 ****************************************************************************/

int nxscope_udp_init(FAR struct nxscope_intf_s *intf,
                     FAR struct nxscope_udp_cfg_s *cfg)
{
  FAR struct nxscope_intf_udp_s *priv = NULL;
  struct sockaddr_in             addr;
  int                            ret   = OK;
  int                            flags = 0;

  DEBUGASSERT(intf);
  DEBUGASSERT(cfg);

  /* Allocate priv data */

  intf->priv = zalloc(sizeof(struct nxscope_intf_udp_s));
  if (intf->priv == NULL)
    {
      _err("ERROR: intf->priv alloc failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* Get priv data */

  priv = (FAR struct nxscope_intf_udp_s *)intf->priv;
  priv->fd = -1;

  /* Connect configuration */

  priv->cfg = (FAR struct nxscope_udp_cfg_s *)cfg;

  /* Connect ops */

  intf->ops = &g_nxscope_udp_ops;

  /* Open UDP socket */

  priv->fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (priv->fd < 0)
    {
      _err("ERROR: failed to create UDP socket %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* Configure non-blocking if enabled */

  if (priv->cfg->nonblock)
    {
      flags = fcntl(priv->fd, F_GETFL, 0);
      if (flags < 0)
        {
          _err("ERROR: failed to get socket flags %d\n", errno);
          ret = -errno;
          goto errout;
        }

      ret = fcntl(priv->fd, F_SETFL, flags | O_NONBLOCK);
      if (ret < 0)
        {
          _err("ERROR: failed to set O_NONBLOCK %d\n", errno);
          ret = -errno;
          goto errout;
        }
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port        = htons(priv->cfg->port);

  ret = bind(priv->fd, (FAR struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      _err("ERROR: failed to bind UDP port %u errno=%d\n",
           priv->cfg->port,
           errno);
      ret = -errno;
      goto errout;
    }

  /* Initialized */

  intf->initialized = true;
  return OK;

errout:
  if (priv != NULL && priv->fd >= 0)
    {
      close(priv->fd);
      priv->fd = -1;
    }

  return ret;
}

/****************************************************************************
 * Name: nxscope_udp_deinit
 ****************************************************************************/

void nxscope_udp_deinit(FAR struct nxscope_intf_s *intf)
{
  FAR struct nxscope_intf_udp_s *priv = NULL;

  DEBUGASSERT(intf);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_udp_s *)intf->priv;

  if (priv != NULL && priv->fd != -1)
    {
      close(priv->fd);
    }

  if (priv != NULL)
    {
      free(priv);
    }

  /* Reset structure */

  memset(intf, 0, sizeof(struct nxscope_intf_s));
}
