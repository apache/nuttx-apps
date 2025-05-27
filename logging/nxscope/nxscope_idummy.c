/****************************************************************************
 * apps/logging/nxscope/nxscope_idummy.c
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

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <logging/nxscope/nxscope.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nxscope_intf_dummy_s
{
  FAR struct nxscope_dummy_cfg_s *cfg;
};

/****************************************************************************
 * Private Function Protototypes
 ****************************************************************************/

static int nxscope_dummy_send(FAR struct nxscope_intf_s *intf,
                              FAR uint8_t *buff, int len);
static int nxscope_dummy_recv(FAR struct nxscope_intf_s *intf,
                              FAR uint8_t *buff, int len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct nxscope_intf_ops_s g_nxscope_dummy_ops =
{
  nxscope_dummy_send,
  nxscope_dummy_recv
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_dummy_send
 ****************************************************************************/

static int nxscope_dummy_send(FAR struct nxscope_intf_s *intf,
                              FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_dummy_s *priv = NULL;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  _info("nxscope_dummy_send\n");

  /* Get priv data */

  priv = (FAR struct nxscope_intf_dummy_s *)intf->priv;

  UNUSED(priv);

  /* Dump send buffer */

  lib_dumpbuffer("nxscope_dummy_send", buff, len);

  return OK;
}

/****************************************************************************
 * Name: nxscope_dummy_recv
 ****************************************************************************/

static int nxscope_dummy_recv(FAR struct nxscope_intf_s *intf,
                              FAR uint8_t *buff, int len)
{
  FAR struct nxscope_intf_dummy_s *priv = NULL;

  DEBUGASSERT(intf);
  DEBUGASSERT(intf->priv);

  _info("nxscope_dummy_recv\n");

  /* Get priv data */

  priv = (FAR struct nxscope_intf_dummy_s *)intf->priv;

  UNUSED(priv);

  /* Dump recv buffer */

  lib_dumpbuffer("nxscope_dummy_recv", buff, len);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxscope_dummy_init
 ****************************************************************************/

int nxscope_dummy_init(FAR struct nxscope_intf_s *intf,
                       FAR struct nxscope_dummy_cfg_s *cfg)
{
  FAR struct nxscope_intf_dummy_s *priv = NULL;
  int                              ret  = OK;

  DEBUGASSERT(intf);
  DEBUGASSERT(cfg);

  _info("nxscope_dummy_init\n");

  /* Allocate priv data */

  intf->priv = zalloc(sizeof(struct nxscope_intf_dummy_s));
  if (intf->priv == NULL)
    {
      _err("ERROR: intf->priv alloc failed %d\n", errno);
      ret = -errno;
      goto errout;
    }

  /* Get priv data */

  priv = (FAR struct nxscope_intf_dummy_s *)intf->priv;

  /* Connect configuration */

  priv->cfg = cfg;

  /* Connect ops */

  intf->ops = &g_nxscope_dummy_ops;

  /* Initialized */

  intf->initialized = true;

errout:
  return ret;
}

/****************************************************************************
 * Name: nxscope_dummy_deinit
 ****************************************************************************/

void nxscope_dummy_deinit(FAR struct nxscope_intf_s *intf)
{
  FAR struct nxscope_intf_dummy_s *priv = NULL;

  DEBUGASSERT(intf);

  _info("nxscope_dummy_deinit\n");

  /* Get priv data */

  priv = (FAR struct nxscope_intf_dummy_s *)intf->priv;

  if (priv != NULL)
    {
      free(priv);
    }

  /* Reset structure */

  memset(intf, 0, sizeof(struct nxscope_intf_s));
}
