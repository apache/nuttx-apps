/****************************************************************************
 * apps/audioutils/fmsynth/test/fmsynth_eg_test.c
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

#include <stdio.h>

#include <audioutils/fmsynth_eg.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS (48000)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: state_name
 ****************************************************************************/

static const char *state_name(int s)
{
  switch (s)
    {
      case EGSTATE_ATTACK:
        return "Attack    ";
      case EGSTATE_DECAYBREAK:
        return "DecayBreak";
      case EGSTATE_DECAY:
        return "Decay     ";
      case EGSTATE_SUSTAIN:
        return "Sustain   ";
      case EGSTATE_RELEASE:
        return "Release   ";
      case EGSTATE_RELEASED:
      case -1:
        return "RELEASED..";
    }

  return "";
}

/****************************************************************************
 * name: dump_eg
 ****************************************************************************/

static void dump_eg(fmsynth_eg_t *env)
{
  int i;
  fmsynth_egparam_t *last = &env->state_params[EGSTATE_RELEASED];

  printf("===== STATE : %s =======\n", state_name(env->state));
  for (i = -1; i < EGSTATE_RELEASED; i++)
    {
      printf("   [%s] %5d <--------------> [%s] %5d\n",
             state_name(i), last->initval,
             state_name(i + 1), env->state_params[i + 1].initval);
      printf("              per %d\n", env->state_params[i + 1].period);
      printf("              dlt %d\n", env->state_params[i + 1].diff2next);
      last = &env->state_params[i + 1];
    }

  printf("\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: main
 ****************************************************************************/

int main(void)
{
  fmsynth_eg_t *eg;
  fmsynth_eglevels_t levels;

  levels.attack.level = 0.6f;
  levels.attack.period_ms = 10;
  levels.decaybrk.level = 0.3f;
  levels.decaybrk.period_ms = 20;
  levels.decay.level = 0.5f;
  levels.decay.period_ms = 15;
  levels.sustain.level = 0.65f;
  levels.sustain.period_ms = 5;
  levels.release.level = 0.f;
  levels.release.period_ms = 70;

  eg = fmsyntheg_create();
  fmsyntheg_set_param(eg, FS, &levels);
  dump_eg(eg);

  fmsyntheg_start(eg);
  dump_eg(eg);

  while (eg->state != EGSTATE_RELEASED)
    {
      printf("%d\n", fmsyntheg_operate(eg));
    }

  fmsyntheg_delete(eg);

  return 0;
}
