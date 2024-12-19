/****************************************************************************
 * apps/include/audioutils/fmsynth_eg.h
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

#ifndef __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_EG_H
#define __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_EG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <limits.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FMSYNTH_MAX_EGLEVEL  (SHRT_MAX / 8)

#define EGSTATE_ATTACK     (0)
#define EGSTATE_DECAYBREAK (1)
#define EGSTATE_DECAY      (2)
#define EGSTATE_SUSTAIN    (3)
#define EGSTATE_RELEASE    (4)
#define EGSTATE_RELEASED   (5)
#define EGSTATE_MAX        (6)

#define EGSTATE_NUM        EGSTATE_RELEASED

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct fmsynth_eglevel_s
{
  float level;
  int period_ms;
};

typedef struct fmsynth_eglevels_s
{
  struct fmsynth_eglevel_s attack;
  struct fmsynth_eglevel_s decaybrk;
  struct fmsynth_eglevel_s decay;
  struct fmsynth_eglevel_s sustain;
  struct fmsynth_eglevel_s release;
} fmsynth_eglevels_t;

typedef struct fmsynth_egparam_s
{
  int initval;
  int period;
  int diff2next;
} fmsynth_egparam_t;

typedef struct fmsynth_eg_s
{
  int state;
  int state_counter;
  fmsynth_egparam_t state_params[EGSTATE_MAX];
} fmsynth_eg_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

FAR fmsynth_eg_t *create_fmsyntheg(fmsynth_eg_t *eg);
FAR fmsynth_eg_t *fmsyntheg_create(void);
void fmsyntheg_delete(FAR fmsynth_eg_t *eg);
int fmsyntheg_set_param(FAR fmsynth_eg_t *eg,
                        int fs, FAR fmsynth_eglevels_t *levels);
void fmsyntheg_start(FAR fmsynth_eg_t *eg);
void fmsyntheg_stop(FAR fmsynth_eg_t *eg);
int fmsyntheg_operate(FAR fmsynth_eg_t *eg);

#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_EG_H */
