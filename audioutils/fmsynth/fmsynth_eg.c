/****************************************************************************
 * apps/audioutils/fmsynth/fmsynth_eg.c
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

#include <stdlib.h>
#include <limits.h>

#include <audioutils/fmsynth_eg.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONVERT_INITVAL(lv) (int)((lv) * FMSYNTH_MAX_EGLEVEL)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: set_egparams
 ****************************************************************************/

static int set_egparams(int fs,
                        FAR fmsynth_egparam_t *param,
                        FAR struct fmsynth_eglevel_s *target_level,
                        FAR struct fmsynth_eglevel_s *last_level)
{
  param->initval   = CONVERT_INITVAL(last_level->level);
  param->period    = fs * target_level->period_ms / 1000;
  param->diff2next = CONVERT_INITVAL(target_level->level)
                    - CONVERT_INITVAL(last_level->level);

  if (param->initval < -FMSYNTH_MAX_EGLEVEL ||
      param->initval > FMSYNTH_MAX_EGLEVEL || param->period < 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: create_fmsyntheg
 ****************************************************************************/

FAR fmsynth_eg_t *create_fmsyntheg(FAR fmsynth_eg_t *eg)
{
  int i;

  if (eg)
    {
      eg->state = EGSTATE_RELEASED;
      eg->state_counter = 0;
      for (i = 0; i < EGSTATE_MAX; i++)
        {
          eg->state_params[i].initval = 0;
          eg->state_params[i].period = 0;
        }

      eg->state_params[EGSTATE_RELEASED].initval = FMSYNTH_MAX_EGLEVEL;
    }

  return eg;
}

/****************************************************************************
 * name: fmsyntheg_create
 ****************************************************************************/

FAR fmsynth_eg_t *fmsyntheg_create(void)
{
  FAR fmsynth_eg_t *ret = (FAR fmsynth_eg_t *)malloc(sizeof(fmsynth_eg_t));

  return create_fmsyntheg(ret);
}

/****************************************************************************
 * name: fmsyntheg_delete
 ****************************************************************************/

void fmsyntheg_delete(FAR fmsynth_eg_t *eg)
{
  if (eg != NULL)
    {
      free(eg);
    }
}

/****************************************************************************
 * name: fmsyntheg_set_param
 ****************************************************************************/

int fmsyntheg_set_param(FAR fmsynth_eg_t *eg,
                        int fs, FAR fmsynth_eglevels_t *levels)
{
  int errcnt = 0;

  if (fs <= 0)
    {
      return ERROR;
    }

  errcnt += set_egparams(fs, &eg->state_params[EGSTATE_ATTACK],
                         &levels->attack, &levels->release);

  errcnt += set_egparams(fs, &eg->state_params[EGSTATE_DECAYBREAK],
                         &levels->decaybrk, &levels->attack);

  errcnt += set_egparams(fs, &eg->state_params[EGSTATE_DECAY],
                         &levels->decay, &levels->decaybrk);

  errcnt += set_egparams(fs, &eg->state_params[EGSTATE_SUSTAIN],
                         &levels->sustain, &levels->decay);

  errcnt += set_egparams(fs, &eg->state_params[EGSTATE_RELEASE],
                         &levels->release, &levels->sustain);

  eg->state_params[EGSTATE_RELEASED].initval =
        CONVERT_INITVAL(levels->release.level);

  return errcnt ? ERROR : OK;
}

/****************************************************************************
 * name: fmsyntheg_start
 ****************************************************************************/

void fmsyntheg_start(FAR fmsynth_eg_t *eg)
{
  eg->state = EGSTATE_ATTACK;
  eg->state_counter = 0;
}

/****************************************************************************
 * name: fmsyntheg_stop
 ****************************************************************************/

void fmsyntheg_stop(FAR fmsynth_eg_t *eg)
{
  eg->state = EGSTATE_RELEASED;
  eg->state_counter = 0;
}

/****************************************************************************
 * name: fmsyntheg_operate
 ****************************************************************************/

int fmsyntheg_operate(FAR fmsynth_eg_t *eg)
{
  int val;
  FAR fmsynth_egparam_t *param = &eg->state_params[eg->state];

  val = param->initval;

  if (eg->state != EGSTATE_RELEASED)
    {
      if (eg->state_counter >= eg->state_params[eg->state].period)
        {
          /* Reset the counter */

          eg->state_counter = 0;

          /* Search next available state */

          do
            {
              eg->state++;
            }
          while (eg->state < EGSTATE_RELEASED
               && eg->state_params[eg->state].period == 0);

          val = eg->state_params[eg->state].initval;
        }
      else
        {
          val = val + param->diff2next * eg->state_counter / param->period;
          eg->state_counter++;
        }
    }

  return val;
}
