/****************************************************************************
 * apps/lte/alt1250/alt1250_container.c
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

#include <string.h>
#include <nuttx/queue.h>

#include "alt1250_dbg.h"
#include "alt1250_daemon.h"
#include "alt1250_container.h"

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

static struct postproc_s postproc_obj[CONFIG_LTE_ALT1250_CONTAINERS];
static struct alt_container_s container_obj[CONFIG_LTE_ALT1250_CONTAINERS];
static sq_queue_t free_containers;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: init_containers
 ****************************************************************************/

void init_containers(void)
{
  int i;

  memset(&container_obj, 0, sizeof(container_obj));

  sq_init(&free_containers);

  for (i = 0; i < CONFIG_LTE_ALT1250_CONTAINERS; i++)
    {
      container_obj[i].priv = (unsigned long)&postproc_obj[i];
      sq_addlast(&container_obj[i].node, &free_containers);
    }
}

/****************************************************************************
 * name: container_alloc
 ****************************************************************************/

FAR struct alt_container_s *container_alloc(void)
{
  FAR struct alt_container_s *ret;

  ret = (FAR struct alt_container_s *)sq_peek(&free_containers);
  if (ret)
    {
      sq_rem(&ret->node, &free_containers);
      clear_container(ret);
    }
  else
    {
      dbg_alt1250("No more container\n");
    }

  return ret;
}

/****************************************************************************
 * name: container_free
 ****************************************************************************/

void container_free(FAR struct alt_container_s *container)
{
  dbg_alt1250("Container free <cmdid : 0x%08lx\n>\n", container->cmdid);

  sq_addlast(&container->node, &free_containers);
}

/****************************************************************************
 * name: set_container_ids
 ****************************************************************************/

void set_container_ids(FAR struct alt_container_s *container,
                       int16_t usockid, uint32_t cmdid)
{
  container->sock = usockid;
  container->cmdid = cmdid;
}

/****************************************************************************
 * name: set_container_argument
 ****************************************************************************/

void set_container_argument(FAR struct alt_container_s *container,
                            FAR void *argparams[], size_t paramsz)
{
  container->inparam = argparams;
  container->inparamlen = paramsz;
}

/****************************************************************************
 * name: set_container_response
 ****************************************************************************/

void set_container_response(FAR struct alt_container_s *container,
                            FAR void *respparams[], size_t paramsz)
{
  container->outparam = respparams;
  container->outparamlen = paramsz;
}

/****************************************************************************
 * name: set_container_postproc
 ****************************************************************************/

void set_container_postproc(FAR struct alt_container_s *container,
                            FAR postproc_hdlr_t func, unsigned long arg)
{
  FAR struct postproc_s *pp = (FAR struct postproc_s *)container->priv;
  pp->hdlr = func;
  pp->priv = arg;
}

/****************************************************************************
 * name: clear_container
 ****************************************************************************/

void clear_container(FAR struct alt_container_s *container)
{
  unsigned long priv = container->priv;
  memset(container, 0, sizeof(struct alt_container_s));
  container->priv = priv;
}

/****************************************************************************
 * name: container_free_all
 ****************************************************************************/

void container_free_all(FAR struct alt_container_s *head)
{
  FAR struct alt_container_s *c;

  while (head)
    {
      c = head;
      head = (FAR struct alt_container_s *)sq_next(&c->node);
      container_free(c);
    }
}

/****************************************************************************
 * name: container_pick_listtop
 ****************************************************************************/

FAR struct alt_container_s *container_pick_listtop(
                FAR struct alt_container_s **head)
{
  FAR struct alt_container_s *ret = *head;

  if (ret)
    {
      *head = (FAR struct alt_container_s *)sq_next(&ret->node);
      sq_next(&ret->node) = NULL;
    }

  return ret;
}
