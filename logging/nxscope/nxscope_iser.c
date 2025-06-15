/****************************************************************************
 * apps/logging/nxscope/nxscope_iser.c
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

#include <debug.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include <logging/nxscope/nxscope.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxscope_intf_ser_s
{
  FAR struct nxscope_ser_cfg_s *cfg;
  int                           fd;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

static int nxscope_ser_send(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len);
static int nxscope_ser_recv(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxscope_intf_ops_s g_nxscope_ser_ops =
{
  nxscope_ser_send,
  nxscope_ser_recv
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_ser_send
 ****************************************************************************/

static int nxscope_ser_send(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_ser_s *priv = NULL;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_ser_s *)intf->priv;

  /* Write data */

  return write(priv->fd, buff, len);
}

/****************************************************************************
 * Name: nxscope_ser_recv
 ****************************************************************************/

static int nxscope_ser_recv(FAR struct nxscope_intf_s *intf,
                            FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_ser_s *priv = NULL;
  int                            ret  = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_ser_s *)intf->priv;

  /* Read data */

  ret = read(priv->fd, buff, len);

  if (ret < 0)
    {
      if (priv->cfg->nonblock && (errno == EAGAIN))
        {
          ret = 0;
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_ser_init
 ****************************************************************************/

int nxscope_ser_init(FAR struct nxscope_intf_s *intf,
                     FAR struct nxscope_ser_cfg_s *cfg)
{
  FAR struct nxscope_intf_ser_s *priv  = NULL;
  struct termios                 tio;
  int                            ret   = OK;
  int                            flags = 0;

  DEBUGASSERT(intf);
  DEBUGASSERT(cfg);

  /* Allocate priv data */

  intf->priv = zalloc(sizeof(struct nxscope_intf_ser_s));
  if (intf->priv == NULL)
    {
      _err("ERROR: intf->priv alloc failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* Get priv data */

  priv = (FAR struct nxscope_intf_ser_s *)intf->priv;

  /* Connect configuration */

  priv->cfg = (FAR struct nxscope_ser_cfg_s *)cfg;

  /* Connect ops */

  intf->ops = &g_nxscope_ser_ops;

  /* Open serial port */

  flags = O_RDWR;

  if (priv->cfg->nonblock)
    {
      flags |= O_NONBLOCK;
    }

#ifdef CONFIG_SERIAL_REMOVABLE
  do
    {
      /* Try to open the console */

      priv->fd = open(priv->cfg->path, flags);
      if (priv->fd < 0)
        {
          /* ENOTCONN means that the USB device is not yet connected.
           * Anything else is bad.
           */

          if (errno != ENOTCONN)
            {
              break;
            }

          /* Sleep a bit and try again */

          sleep(1);
        }
    }
  while (priv->fd < 0);
#else
  priv->fd = open(priv->cfg->path, flags);
#endif
  if (priv->fd < 0)
    {
      _err("ERROR: failed to open %s %d\n", priv->cfg->path, errno);
      ret = -errno;
      goto errout;
    }

  /* Get the attributes */

  tcgetattr(priv->fd, &tio);

#ifdef CONFIG_SERIAL_TERMIOS
  if (priv->cfg->baud > 0)
    {
      /* Configure a baud rate */

      ret = cfsetspeed(&tio, priv->cfg->baud);
      if (ret < 0)
        {
          _err("ERROR: failed to set baud rate %d\n", errno);
          goto errout;
        }
    }
#endif

  /* Configure RAW mode */

  cfmakeraw(&tio);

  /* Change the attributes now */

  tcsetattr(priv->fd, TCSANOW, &tio);

  /* Initialized */

  intf->initialized = true;

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_ser_deinit
 ****************************************************************************/

void nxscope_ser_deinit(FAR struct nxscope_intf_s *intf)
{
  FAR struct nxscope_intf_ser_s *priv = NULL;

  DEBUGASSERT(intf);

  /* Get priv data */

  priv = (FAR struct nxscope_intf_ser_s *)intf->priv;

  /* Close dev */

  if (priv->fd != -1)
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
