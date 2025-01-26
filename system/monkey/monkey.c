/****************************************************************************
 * apps/system/monkey/monkey.c
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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "monkey.h"
#include "monkey_recorder.h"
#include "monkey_assert.h"
#include "monkey_log.h"
#include "monkey_dev.h"
#include "monkey_utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MONKEY_DEV_PATH_TOUCH "/dev/input0"
#define MONKEY_DEV_PATH_BUTTON "/dev/buttons"
#define MONKEY_DEV_PATH_UTOUCH "/dev/utouch"
#define MONKEY_DEV_PATH_UBUTTON "/dev/ubutton"

#define MONKEY_DEV_CREATE_MATCH(monkey, type_mask, type)                  \
do {                                                                      \
  if (((type_mask) & MONKEY_DEV_TYPE_##type) == MONKEY_DEV_TYPE_##type)   \
    {                                                                     \
      FAR struct monkey_dev_s *dev;                                       \
      dev = monkey_dev_create(MONKEY_DEV_PATH_##type,                     \
                              MONKEY_DEV_TYPE_##type);                    \
      if (!dev)                                                           \
        {                                                                 \
          goto failed;                                                    \
        }                                                                 \
      (monkey)->devs[(monkey)->dev_num] = dev;                            \
      (monkey)->dev_num++;                                                \
    }                                                                     \
} while (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: monkey_create
 ****************************************************************************/

FAR struct monkey_s *monkey_create(int dev_type_mask)
{
  FAR struct monkey_s *monkey = calloc(1, sizeof(struct monkey_s));
  MONKEY_ASSERT_NULL(monkey);

  if (MONKEY_IS_UINPUT_TYPE(dev_type_mask))
    {
      MONKEY_DEV_CREATE_MATCH(monkey, dev_type_mask, UTOUCH);
      MONKEY_DEV_CREATE_MATCH(monkey, dev_type_mask, UBUTTON);
    }
  else
    {
      MONKEY_DEV_CREATE_MATCH(monkey, dev_type_mask, TOUCH);
      MONKEY_DEV_CREATE_MATCH(monkey, dev_type_mask, BUTTON);
    }

  if (monkey->dev_num == 0)
    {
      MONKEY_LOG_ERROR("NO enabled device detected");
      goto failed;
    }

  MONKEY_ASSERT(monkey->dev_num <= MONKEY_DEV_MAX_NUM);

  MONKEY_LOG_NOTICE("OK");

  return monkey;

failed:
  monkey_delete(monkey);
  return NULL;
}

/****************************************************************************
 * Name: monkey_delete
 ****************************************************************************/

void monkey_delete(FAR struct monkey_s *monkey)
{
  int i;
  MONKEY_ASSERT_NULL(monkey);

  for (i = 0; i < monkey->dev_num; i++)
    {
      monkey_dev_delete(monkey->devs[i]);
    }

  if (monkey->recorder)
    {
      monkey_recorder_delete(monkey->recorder);
      monkey->recorder = NULL;
    }

  free(monkey);
  MONKEY_LOG_NOTICE("OK");
}

/****************************************************************************
 * Name: monkey_config_default_init
 ****************************************************************************/

void monkey_config_default_init(FAR struct monkey_config_s *config)
{
  MONKEY_ASSERT_NULL(config);
  memset(config, 0, sizeof(struct monkey_config_s));
  config->screen.hor_res = 480;
  config->screen.ver_res = 480;
  config->period.min = 100;
  config->period.max = 500;
}

/****************************************************************************
 * Name: monkey_set_config
 ****************************************************************************/

void monkey_set_config(FAR struct monkey_s *monkey,
                       FAR const struct monkey_config_s *config)
{
  MONKEY_ASSERT_NULL(monkey);
  MONKEY_ASSERT_NULL(config);
  monkey->config = *config;
}

/****************************************************************************
 * Name: monkey_set_mode
 ****************************************************************************/

void monkey_set_mode(FAR struct monkey_s *monkey,
                     enum monkey_mode_e mode)
{
  MONKEY_ASSERT_NULL(monkey);
  monkey->mode = mode;
}

/****************************************************************************
 * Name: monkey_set_period
 ****************************************************************************/

void monkey_set_period(FAR struct monkey_s *monkey, uint32_t period)
{
  MONKEY_ASSERT_NULL(monkey);
  monkey->config.period.min = period;
  monkey->config.period.max = period;
}

/****************************************************************************
 * Name: monkey_set_recorder_path
 ****************************************************************************/

bool monkey_set_recorder_path(FAR struct monkey_s *monkey,
                              FAR const char *path)
{
  MONKEY_ASSERT_NULL(monkey);

  if (monkey->mode == MONKEY_MODE_RECORD)
    {
      monkey->recorder = monkey_recorder_create(path,
                          MONKEY_RECORDER_MODE_RECORD);
    }
  else if (monkey->mode == MONKEY_MODE_PLAYBACK)
    {
      monkey->recorder = monkey_recorder_create(path,
                          MONKEY_RECORDER_MODE_PLAYBACK);
    }
  else
    {
      MONKEY_LOG_WARN("mismatched mode: %d", monkey->mode);
    }

  return (monkey->recorder != NULL);
}
