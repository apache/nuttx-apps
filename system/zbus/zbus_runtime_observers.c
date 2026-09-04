/****************************************************************************
 * apps/system/zbus/zbus_runtime_observers.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * Copyright (c) 2026 NuttX port
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.  You may obtain
 * a copy of the License at
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

#include <stdlib.h>

#include <system/zbus.h>

#include "zbus_priv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zbus_chan_add_obs
 ****************************************************************************/

int zbus_chan_add_obs(const struct zbus_channel *chan,
                      const struct zbus_observer *obs, int32_t timeout_ms)
{
  struct zbus_observer_node *obs_nd;
  struct zbus_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(obs != NULL, "obs is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  err = zbus_sem_take(&chan->data->sem, &d);
  if (err)
    {
      return err;
    }

  /* Reject observers already statically attached to the channel */

  for (int16_t i = chan->data->observers_start_idx,
       limit = chan->data->observers_end_idx; i < limit; i++)
    {
      struct zbus_channel_observation *observation;

      STRUCT_SECTION_GET(zbus_channel_observation, i, &observation);

      if (observation->obs == obs)
        {
          sem_post(&chan->data->sem);
          return -EEXIST;
        }
    }

  /* Reject observers already dynamically attached to the channel */

  list_for_every_entry(&chan->data->observers, obs_nd,
                       struct zbus_observer_node, node)
    {
      if (obs_nd->obs == obs)
        {
          sem_post(&chan->data->sem);
          return -EALREADY;
        }
    }

  obs_nd = malloc(sizeof(*obs_nd));
  if (obs_nd == NULL)
    {
      sem_post(&chan->data->sem);
      return -ENOMEM;
    }

  obs_nd->obs = obs;
  list_add_tail(&chan->data->observers, &obs_nd->node);

  sem_post(&chan->data->sem);

  return 0;
}

/****************************************************************************
 * Name: zbus_chan_rm_obs
 ****************************************************************************/

int zbus_chan_rm_obs(const struct zbus_channel *chan,
                     const struct zbus_observer *obs, int32_t timeout_ms)
{
  struct zbus_observer_node *obs_nd;
  struct zbus_observer_node *tmp;
  struct zbus_deadline d;
  int err;

  _ZBUS_ASSERT(chan != NULL, "chan is required");
  _ZBUS_ASSERT(obs != NULL, "obs is required");

  zbus_port_init_once();

  zbus_deadline_init(timeout_ms, &d);

  err = zbus_sem_take(&chan->data->sem, &d);
  if (err)
    {
      return err;
    }

  list_for_every_entry_safe(&chan->data->observers, obs_nd, tmp,
                            struct zbus_observer_node, node)
    {
      if (obs_nd->obs == obs)
        {
          list_delete(&obs_nd->node);
          free(obs_nd);

          sem_post(&chan->data->sem);
          return 0;
        }
    }

  sem_post(&chan->data->sem);

  return -ENODATA;
}
