/****************************************************************************
 * apps/audioutils/fmsynth/test/fmsynth_test.c
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
#include <stdint.h>

#include <audioutils/fmsynth_eg.h>
#include <audioutils/fmsynth_op.h>
#include <audioutils/fmsynth.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS (48000)
#define SOUNDFREQ (3000.f)
#define TEST_LENGTH ((FS / 1000) * (10 + 20 + 16 + 5 + 30))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int16_t my_sample[TEST_LENGTH];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: set_levels
 ****************************************************************************/

static fmsynth_eglevels_t *set_levels(fmsynth_eglevels_t *level,
                                      int atk_lvl, int atk_peri,
                                      int decbrk_lvl, int decbrk_peri,
                                      int dec_lvl, int dec_peri,
                                      int sus_lvl, int sus_peri,
                                      int rel_lvl, int rel_peri)
{
  level->attack.level = atk_lvl;
  level->attack.period_ms = atk_peri;
  level->decaybrk.level = decbrk_lvl;
  level->decaybrk.period_ms = decbrk_peri;
  level->decay.level = dec_lvl;
  level->decay.period_ms = dec_peri;
  level->sustain.level = sus_lvl;
  level->sustain.period_ms = sus_peri;
  level->release.level = rel_lvl;
  level->release.period_ms = rel_peri;

  return level;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: main
 ****************************************************************************/

int main(void)
{
  int phase_time;

  fmsynth_eglevels_t levels;
  fmsynth_sound_t *snd1;
  fmsynth_op_t *envop;
  fmsynth_op_t *fbop;

  /* Initialize FM synthesizer */

  fmsynth_initialize(FS);

  /* Operator setup */

  envop  = fmsynthop_create();
  fbop   = fmsynthop_create();

  set_levels(&levels, 0.12f, 10, 0.06f, 20, 0.1f, 16, 0.1f, 5, 0.f, 70);

  fmsynthop_set_envelope(envop, &levels);
  fmsynthop_select_opfunc(envop, FMSYNTH_OPFUNC_SIN);

  fmsynthop_select_opfunc(fbop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_bind_feedback(fbop, fbop, 0.6f);

  fmsynthop_parallel_subop(envop, fbop);

  /* Sound setup */

  snd1 = fmsynthsnd_create();
  fmsynthsnd_set_operator(snd1, envop);
  fmsynthsnd_set_soundfreq(snd1, SOUNDFREQ);

  fmsynth_rendering(snd1, my_sample, TEST_LENGTH, 1, NULL, 0);

  for (int i = 0; i < TEST_LENGTH; i++)
    {
      printf("%d\n", my_sample[i]);
    }

  fmsynthop_delete(envop);
  fmsynthop_delete(fbop);

  fmsynthsnd_delete(snd1);

  return 0;
}
