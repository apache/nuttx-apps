/****************************************************************************
 * apps/include/audioutils/fmsynth.h
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

#ifndef __INCLUDE_AUDIOUTILS_FMSYNTH_H
#define __INCLUDE_AUDIOUTILS_FMSYNTH_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <limits.h>
#include <stdint.h>

#include <audioutils/fmsynth_eg.h>
#include <audioutils/fmsynth_op.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FMSYNTH_MAX_VOLUME (SHRT_MAX)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct fmsynth_sound_s
{
  int own_allocate;
  int phase_time;
  int max_phase_time;
  int volume;
  FAR fmsynth_op_t *operators;

  FAR struct fmsynth_sound_s *next_sound;
} fmsynth_sound_t;

typedef CODE void (*fmsynth_tickcb_t)(unsigned long cbarg);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

int fmsynth_initialize(int fs);
FAR fmsynth_sound_t *fmsynthsnd_create(void);
FAR fmsynth_sound_t *create_fmsynthsnd(FAR fmsynth_sound_t *);
void fmsynthsnd_stop(FAR fmsynth_sound_t *snd);
void fmsynthsnd_delete(FAR fmsynth_sound_t *snd);
int fmsynthsnd_set_operator(FAR fmsynth_sound_t *snd, FAR fmsynth_op_t *op);
void fmsynthsnd_set_soundfreq(FAR fmsynth_sound_t *snd, float freq);
void fmsynthsnd_set_volume(FAR fmsynth_sound_t *snd, float vol);
int fmsynthsnd_add_subsound(FAR fmsynth_sound_t *top,
                            FAR fmsynth_sound_t *sub);
int fmsynth_rendering(FAR fmsynth_sound_t *snd,
                      FAR int16_t *sample, int sample_num, int chnum,
                      fmsynth_tickcb_t cb, unsigned long cbarg);

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_AUDIOUTILS_FMSYNTH_H */
