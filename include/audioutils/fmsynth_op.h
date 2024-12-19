/****************************************************************************
 * apps/include/audioutils/fmsynth_op.h
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

#ifndef __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_OP_H
#define __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_OP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <audioutils/fmsynth_eg.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FMSYNTH_PI (0x10000)

#define FMSYNTH_OPFUNC_SIN      (0)
#define FMSYNTH_OPFUNC_TRIANGLE (1)
#define FMSYNTH_OPFUNC_SAWTOOTH (2)
#define FMSYNTH_OPFUNC_SQUARE   (3)
#define FMSYNTH_OPFUNC_NUM      (4)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef CODE int (*opfunc_t)(int theta);

typedef struct fmsynth_op_s
{
  FAR fmsynth_eg_t *eg;
  opfunc_t wavegen;
  struct fmsynth_op_s *cascadeop;
  struct fmsynth_op_s *parallelop;

  int own_allocate;
  FAR int *feedback_ref;
  int feedback_val;
  int feedbackrate;
  int last_sigval;

  float freq_rate;
  float sound_freq;
  float delta_phase;
  float current_phase;
} fmsynth_op_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

int fmsynthop_set_samplerate(int fs);

FAR fmsynth_op_t *fmsynthop_create(void);
FAR fmsynth_op_t *create_fmsynthop(FAR fmsynth_op_t *op,
                                   FAR fmsynth_eg_t *eg);
void fmsynthop_delete(FAR fmsynth_op_t *op);
int fmsynthop_select_opfunc(FAR fmsynth_op_t *op, int type);
int fmsynthop_set_envelope(FAR fmsynth_op_t *op,
                           FAR fmsynth_eglevels_t *levels);
int fmsynthop_cascade_subop(FAR fmsynth_op_t *op,
                            FAR fmsynth_op_t *subop);
int fmsynthop_parallel_subop(FAR fmsynth_op_t *op,
                             FAR fmsynth_op_t *subop);
int fmsynthop_bind_feedback(FAR fmsynth_op_t *op,
                            FAR fmsynth_op_t *subop, float ratio);
int fmsynthop_update_feedback(FAR fmsynth_op_t *op);
void fmsynthop_set_soundfreq(FAR fmsynth_op_t *op, float freq);
void fmsynthop_set_soundfreqrate(FAR fmsynth_op_t *op, float rate);
void fmsynthop_start(FAR fmsynth_op_t *op);
void fmsynthop_stop(FAR fmsynth_op_t *op);
int fmsynthop_operate(FAR fmsynth_op_t *op, int phase_time);

#ifdef __cplusplus
}
#endif

#endif  /* __APPS_INCLUDE_AUDIOUTILS_FMSYNTH_OP_H */
