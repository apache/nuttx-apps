/****************************************************************************
 * apps/system/zbus/zbus_iterable_sections.c
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

#include <system/zbus.h>

#include "zbus_priv.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool zbus_iterate_over_channels(
    bool (*iterator_func)(const struct zbus_channel *chan))
{
  FAR struct zbus_channel *chan;

  STRUCT_SECTION_FOREACH(zbus_channel, chan)
    {
      if (!(*iterator_func)(chan))
        {
          return false;
        }
    }

  return true;
}

bool zbus_iterate_over_channels_with_user_data(
    bool (*iterator_func)(const struct zbus_channel *chan, void *user_data),
    void *user_data)
{
  FAR struct zbus_channel *chan;

  STRUCT_SECTION_FOREACH(zbus_channel, chan)
    {
      if (!(*iterator_func)(chan, user_data))
        {
          return false;
        }
    }

  return true;
}

bool zbus_iterate_over_observers(
    bool (*iterator_func)(const struct zbus_observer *obs))
{
  FAR struct zbus_observer *obs;

  STRUCT_SECTION_FOREACH(zbus_observer, obs)
    {
      if (!(*iterator_func)(obs))
        {
          return false;
        }
    }

  return true;
}

bool zbus_iterate_over_observers_with_user_data(
    bool (*iterator_func)(const struct zbus_observer *obs, void *user_data),
    void *user_data)
{
  FAR struct zbus_observer *obs;

  STRUCT_SECTION_FOREACH(zbus_observer, obs)
    {
      if (!(*iterator_func)(obs, user_data))
        {
          return false;
        }
    }

  return true;
}
