/****************************************************************************
 * apps/system/nxinit/property_simple.c
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

#include "action.h"
#include "property.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_property_set(FAR struct init_poller_s *ctx,
                      FAR const char *key, FAR const char *value)
{
  init_debug("Setprop key:%s value:%s", key, value);
  init_action_trigger_event(ctx->am, key, value);
  return 0;
}

void init_property_handler(FAR struct init_poller_s *ctx)
{
  UNUSED(ctx);
}

void init_property_deinit(FAR struct init_poller_s *ctx)
{
  UNUSED(ctx);
}

int init_property_init(FAR struct init_poller_s *ctx)
{
  ctx->am->prop = ctx;
  ctx->pfd->fd = -1;
  return 0;
}
