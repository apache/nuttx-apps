/****************************************************************************
 * apps/audioutils/fmsynth/test/fmsynth_op_test.c
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

#include <audioutils/fmsynth_op.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS (48000)
#define SOUNDFREQ (261.f)
#define TEST_LOOP (FS * (30 + 10 + 100 + 30 + 1) / 1000)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: set_levels
 ****************************************************************************/

static fmsynth_eglevels_t *set_levels(fmsynth_eglevels_t *level,
                                      float atk_lvl, int atk_peri,
                                      float decbrk_lvl, int decbrk_peri,
                                      float dec_lvl, int dec_peri,
                                      float sus_lvl, int sus_peri,
                                      float rel_lvl, int rel_peri)
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

  fmsynth_op_t *envop;
  fmsynth_op_t *fbop;
  fmsynth_op_t *triop;
  fmsynth_op_t *mainop;
  fmsynth_op_t *subop;

  fmsynth_op_t *conv1;
  fmsynth_op_t *conv2;

  phase_time = 0;
  fmsynthop_set_samplerate(FS);

  set_levels(&levels, 0.12f, 10, 0.06f, 20, 0.1f, 16, 0.1f, 5, 0.f, 70);

  envop  = fmsynthop_create();
  fmsynthop_set_envelope(envop, &levels);
  fmsynthop_select_opfunc(envop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_set_soundfreq(envop, SOUNDFREQ);
  fmsynthop_start(envop);

  triop  = fmsynthop_create();
  fmsynthop_select_opfunc(triop, FMSYNTH_OPFUNC_TRIANGLE);
  fmsynthop_set_soundfreq(triop, SOUNDFREQ);
  fmsynthop_start(triop);

  fbop   = fmsynthop_create();
  fmsynthop_select_opfunc(fbop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_set_soundfreq(fbop, SOUNDFREQ);
  fmsynthop_bind_feedback(fbop, fbop, 0.6f);
  fmsynthop_start(fbop);

  mainop = fmsynthop_create();
  subop  = fmsynthop_create();
  fmsynthop_select_opfunc(mainop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_select_opfunc(subop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_cascade_subop(mainop, subop);
  fmsynthop_set_soundfreq(mainop, SOUNDFREQ);
  fmsynthop_set_soundfreqrate(subop, 2.f);
  fmsynthop_start(mainop);

  printf("idx,EnvTest,FeedbackTest,CascadeTest,Triangle\n");
  while (phase_time < TEST_LOOP)
    {
      fmsynthop_update_feedback(envop);
      fmsynthop_update_feedback(fbop);
      fmsynthop_update_feedback(mainop);
      fmsynthop_update_feedback(triop);

      printf("%d,%d,%d,%d,%d\n",
            phase_time,
            fmsynthop_operate(envop, phase_time),
            fmsynthop_operate(fbop, phase_time),
            fmsynthop_operate(mainop, phase_time),
            fmsynthop_operate(triop, phase_time));

      phase_time++;
    }

  fmsynthop_delete(envop);
  fmsynthop_delete(fbop);
  fmsynthop_delete(mainop);
  fmsynthop_delete(subop);

  return 0;
}
