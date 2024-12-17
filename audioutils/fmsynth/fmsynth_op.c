/****************************************************************************
 * apps/audioutils/fmsynth/fmsynth_op.c
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
#include <audioutils/fmsynth_op.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PHASE_ADJUST(th) \
        (((th) < 0 ? (FMSYNTH_PI) - (th) : (th)) % (FMSYNTH_PI * 2))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const short s_sintbl[] =
{
  0xff37, /* Extra data for linear completion */

  /* Actual sin table of half PI [256] */

  0x0000, 0x00c9, 0x0192, 0x025b,
  0x0324, 0x03ed, 0x04b6, 0x057e,
  0x0647, 0x0710, 0x07d9, 0x08a1,
  0x096a, 0x0a32, 0x0afb, 0x0bc3,
  0x0c8b, 0x0d53, 0x0e1b, 0x0ee3,
  0x0fab, 0x1072, 0x1139, 0x1200,
  0x12c7, 0x138e, 0x1455, 0x151b,
  0x15e1, 0x16a7, 0x176d, 0x1833,
  0x18f8, 0x19bd, 0x1a82, 0x1b46,
  0x1c0b, 0x1ccf, 0x1d93, 0x1e56,
  0x1f19, 0x1fdc, 0x209f, 0x2161,
  0x2223, 0x22e4, 0x23a6, 0x2467,
  0x2527, 0x25e7, 0x26a7, 0x2767,
  0x2826, 0x28e5, 0x29a3, 0x2a61,
  0x2b1e, 0x2bdb, 0x2c98, 0x2d54,
  0x2e10, 0x2ecc, 0x2f86, 0x3041,
  0x30fb, 0x31b4, 0x326d, 0x3326,
  0x33de, 0x3496, 0x354d, 0x3603,
  0x36b9, 0x376f, 0x3824, 0x38d8,
  0x398c, 0x3a3f, 0x3af2, 0x3ba4,
  0x3c56, 0x3d07, 0x3db7, 0x3e67,
  0x3f16, 0x3fc5, 0x4073, 0x4120,
  0x41cd, 0x4279, 0x4325, 0x43d0,
  0x447a, 0x4523, 0x45cc, 0x4674,
  0x471c, 0x47c3, 0x4869, 0x490e,
  0x49b3, 0x4a57, 0x4afa, 0x4b9d,
  0x4c3f, 0x4ce0, 0x4d80, 0x4e20,
  0x4ebf, 0x4f5d, 0x4ffa, 0x5097,
  0x5133, 0x51ce, 0x5268, 0x5301,
  0x539a, 0x5432, 0x54c9, 0x555f,
  0x55f4, 0x5689, 0x571d, 0x57b0,
  0x5842, 0x58d3, 0x5963, 0x59f3,
  0x5a81, 0x5b0f, 0x5b9c, 0x5c28,
  0x5cb3, 0x5d3d, 0x5dc6, 0x5e4f,
  0x5ed6, 0x5f5d, 0x5fe2, 0x6067,
  0x60eb, 0x616e, 0x61f0, 0x6271,
  0x62f1, 0x6370, 0x63ee, 0x646b,
  0x64e7, 0x6562, 0x65dd, 0x6656,
  0x66ce, 0x6745, 0x67bc, 0x6831,
  0x68a5, 0x6919, 0x698b, 0x69fc,
  0x6a6c, 0x6adb, 0x6b4a, 0x6bb7,
  0x6c23, 0x6c8e, 0x6cf8, 0x6d61,
  0x6dc9, 0x6e30, 0x6e95, 0x6efa,
  0x6f5e, 0x6fc0, 0x7022, 0x7082,
  0x70e1, 0x7140, 0x719d, 0x71f9,
  0x7254, 0x72ae, 0x7306, 0x735e,
  0x73b5, 0x740a, 0x745e, 0x74b1,
  0x7503, 0x7554, 0x75a4, 0x75f3,
  0x7640, 0x768d, 0x76d8, 0x7722,
  0x776b, 0x77b3, 0x77f9, 0x783f,
  0x7883, 0x78c6, 0x7908, 0x7949,
  0x7989, 0x79c7, 0x7a04, 0x7a41,
  0x7a7c, 0x7ab5, 0x7aee, 0x7b25,
  0x7b5c, 0x7b91, 0x7bc4, 0x7bf7,
  0x7c29, 0x7c59, 0x7c88, 0x7cb6,
  0x7ce2, 0x7d0e, 0x7d38, 0x7d61,
  0x7d89, 0x7db0, 0x7dd5, 0x7df9,
  0x7e1c, 0x7e3e, 0x7e5e, 0x7e7e,
  0x7e9c, 0x7eb9, 0x7ed4, 0x7eef,
  0x7f08, 0x7f20, 0x7f37, 0x7f4c,
  0x7f61, 0x7f74, 0x7f86, 0x7f96,
  0x7fa6, 0x7fb4, 0x7fc1, 0x7fcd,
  0x7fd7, 0x7fe0, 0x7fe8, 0x7fef,
  0x7ff5, 0x7ff9, 0x7ffc, 0x7ffe,

  0x7fff, /* Extra data for linear completion */
};

static int local_fs;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: pseudo_sin256
 ****************************************************************************/

static int pseudo_sin256(int theta)
{
  int short_sin;
  int rest;
  int phase;
  int tblidx;

  theta = PHASE_ADJUST(theta);

  rest   = theta & 0x7f;
  phase  = theta / (FMSYNTH_PI / 2);
  tblidx = (theta % (FMSYNTH_PI / 2)) >> 7;

  if (phase & 0x01)
    {
      tblidx = 257 - tblidx;
      short_sin = s_sintbl[tblidx];
      short_sin = short_sin
                + (((s_sintbl[tblidx - 1] - short_sin) * rest) >> 7);
    }
  else
    {
      short_sin = s_sintbl[tblidx + 1];
      short_sin = short_sin
                + (((s_sintbl[tblidx + 2] - short_sin) * rest) >> 7);
    }

  return phase & 0x02 ? -short_sin : short_sin;
}

/****************************************************************************
 * name: triangle_wave
 ****************************************************************************/

static int triangle_wave(int theta)
{
  int ret = 0;
  int phase;
  int offset;
  int slope;

  theta = PHASE_ADJUST(theta);
  phase  = theta / (FMSYNTH_PI / 2);
  offset = theta % (FMSYNTH_PI / 2);

  switch (phase)
    {
      case 0:
        ret = 0;
        slope = SHRT_MAX;
        break;
      case 1:
        ret = SHRT_MAX;
        slope = -SHRT_MAX;
        break;
      case 2:
        ret = 0;
        slope = -SHRT_MAX;
        break;
      case 3:
        ret = -SHRT_MAX;
        slope = SHRT_MAX;
        break;
      default:
        ret = 0;
        slope = SHRT_MAX;
        break;
    }

  return ret + ((slope * offset) >> 15);
}

/****************************************************************************
 * name: sawtooth_wave
 ****************************************************************************/

static int sawtooth_wave(int theta)
{
  theta = PHASE_ADJUST(theta);
  return (theta >> 1) - SHRT_MAX;
}

/****************************************************************************
 * name: square_wave
 ****************************************************************************/

static int square_wave(int theta)
{
  theta = PHASE_ADJUST(theta);
  return theta < FMSYNTH_PI ? SHRT_MAX : -SHRT_MAX;
}

/****************************************************************************
 * name: update_parameters
 ****************************************************************************/

static void update_parameters(FAR fmsynth_op_t *op)
{
  if (local_fs != 0)
    {
      op->delta_phase = 2 * FMSYNTH_PI * op->sound_freq * op->freq_rate
                        / (float)local_fs;
    }
  else
    {
      op->delta_phase = 0.f;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: fmsynthop_set_samplerate
 ****************************************************************************/

int fmsynthop_set_samplerate(int fs)
{
  if (fs < 0)
    {
      return ERROR;
    }

  local_fs = fs;
  return OK;
}

/****************************************************************************
 * name: create_fmsynthop
 ****************************************************************************/

FAR fmsynth_op_t *create_fmsynthop(FAR fmsynth_op_t *op,
                                   FAR fmsynth_eg_t *eg)
{
  if (op)
    {
      op->eg = eg;

      op->own_allocate  = 0;
      op->wavegen       = NULL;
      op->cascadeop     = NULL;
      op->parallelop    = NULL;
      op->feedback_ref  = NULL;
      op->feedback_val  = 0;
      op->feedbackrate  = 0;
      op->last_sigval   = 0;
      op->freq_rate     = 1.f;
      op->sound_freq    = 0.f;
      op->delta_phase   = 0.f;
      op->current_phase = 0.f;
    }

  return op;
}

/****************************************************************************
 * name: fmsynthop_create
 ****************************************************************************/

FAR fmsynth_op_t *fmsynthop_create(void)
{
  FAR fmsynth_op_t *ret;

  ret = (FAR fmsynth_op_t *)malloc(sizeof(fmsynth_op_t));

  if (ret)
    {
      ret->eg = fmsyntheg_create();
      if (!ret->eg)
        {
          free(ret);
          return NULL;
        }

      create_fmsynthop(ret, ret->eg);
      ret->own_allocate = 1;
    }

  return ret;
}

/****************************************************************************
 * name: fmsynthop_delete
 ****************************************************************************/

void fmsynthop_delete(FAR fmsynth_op_t *op)
{
  if (op != NULL && op->own_allocate == 1)
    {
      if (op->eg)
        {
          fmsyntheg_delete(op->eg);
        }

      free(op);
    }
}

/****************************************************************************
 * name: fmsynthop_select_opfunc
 ****************************************************************************/

int fmsynthop_select_opfunc(FAR fmsynth_op_t *op, int type)
{
  int ret = ERROR;

  if (op != NULL)
    {
      switch (type)
        {
          case FMSYNTH_OPFUNC_SIN:
            op->wavegen = pseudo_sin256;
            ret = OK;
            break;

          case FMSYNTH_OPFUNC_TRIANGLE:
            op->wavegen = triangle_wave;
            ret = OK;
            break;

          case FMSYNTH_OPFUNC_SAWTOOTH:
            op->wavegen = sawtooth_wave;
            ret = OK;
            break;

          case FMSYNTH_OPFUNC_SQUARE:
            op->wavegen = square_wave;
            ret = OK;
            break;
        }
    }

  return ret;
}

/****************************************************************************
 * name: fmsynthop_set_envelope
 ****************************************************************************/

int fmsynthop_set_envelope(FAR fmsynth_op_t *op,
                           FAR fmsynth_eglevels_t *levels)
{
  if (local_fs >= 0 && op && levels)
    {
      return fmsyntheg_set_param(op->eg, local_fs, levels);
    }

  return ERROR;
}

/****************************************************************************
 * name: fmsynthop_cascade_subop
 ****************************************************************************/

int fmsynthop_cascade_subop(FAR fmsynth_op_t *op,
                            FAR fmsynth_op_t *subop)
{
  FAR fmsynth_op_t *tmp;

  if (!op || !subop)
    {
      return ERROR;
    }

  for (tmp = op; tmp->cascadeop; tmp = tmp->cascadeop);

  tmp->cascadeop = subop;

  return OK;
}

/****************************************************************************
 * name: fmsynthop_parallel_subop
 ****************************************************************************/

int fmsynthop_parallel_subop(FAR fmsynth_op_t *op,
                             FAR fmsynth_op_t *subop)
{
  FAR fmsynth_op_t *tmp;

  if (!op || !subop)
    {
      return ERROR;
    }

  for (tmp = op; tmp->parallelop; tmp = tmp->parallelop);

  tmp->parallelop = subop;

  return OK;
}

/****************************************************************************
 * name: fmsynthop_bind_feedback
 ****************************************************************************/

int fmsynthop_bind_feedback(FAR fmsynth_op_t *op,
                            FAR fmsynth_op_t *subop, float ratio)
{
  if (!op || !subop)
    {
      return ERROR;
    }

  op->feedbackrate = (int)((float)FMSYNTH_MAX_EGLEVEL * ratio);
  op->feedback_ref = &subop->last_sigval;

  return OK;
}

/****************************************************************************
 * name: fmsynthop_update_feedback
 ****************************************************************************/

int fmsynthop_update_feedback(FAR fmsynth_op_t *op)
{
  FAR fmsynth_op_t *tmp;

  for (tmp = op->cascadeop; tmp != NULL; tmp = tmp->parallelop)
    {
      fmsynthop_update_feedback(tmp);
    }

  if (op->feedback_ref)
    {
      op->feedback_val = *op->feedback_ref * op->feedbackrate
                       / FMSYNTH_MAX_EGLEVEL;
    }

  return OK;
}

/****************************************************************************
 * name: fmsynthop_set_soundfreq
 ****************************************************************************/

void fmsynthop_set_soundfreq(FAR fmsynth_op_t *op, float freq)
{
  FAR fmsynth_op_t *tmp;

  op->sound_freq = freq;
  update_parameters(op);

  for (tmp = op->cascadeop; tmp != NULL; tmp = tmp->parallelop)
    {
      fmsynthop_set_soundfreq(tmp, freq);
    }
}

/****************************************************************************
 * name: fmsynthop_set_soundfreqrate
 ****************************************************************************/

void fmsynthop_set_soundfreqrate(FAR fmsynth_op_t *op, float rate)
{
  op->freq_rate = rate;
  update_parameters(op);
}

/****************************************************************************
 * name: fmsynthop_start
 ****************************************************************************/

void fmsynthop_start(FAR fmsynth_op_t *op)
{
  FAR fmsynth_op_t *tmp;

  fmsyntheg_start(op->eg);

  for (tmp = op->cascadeop; tmp; tmp = tmp->parallelop)
    {
      fmsynthop_start(tmp);
    }
}

/****************************************************************************
 * name: fmsynthop_stop
 ****************************************************************************/

void fmsynthop_stop(FAR fmsynth_op_t *op)
{
  FAR fmsynth_op_t *tmp;

  fmsyntheg_stop(op->eg);

  for (tmp = op->cascadeop; tmp; tmp = tmp->parallelop)
    {
      fmsynthop_stop(tmp);
    }
}

/****************************************************************************
 * name: fmsynthop_operate
 ****************************************************************************/

int fmsynthop_operate(FAR fmsynth_op_t *op, int phase_time)
{
  int val;
  int val2;
  int phase;
  FAR fmsynth_op_t *subop;

  op->current_phase = phase_time ? op->current_phase + op->delta_phase : 0.f;

  val = (int)op->current_phase;
  val2 = (val / (2 * FMSYNTH_PI));

  phase = (int)val + op->feedback_val;
  op->current_phase = op->current_phase - (float)(val2 * (2 * FMSYNTH_PI));

  subop = op->cascadeop;

  while (subop)
    {
      phase += fmsynthop_operate(subop, phase_time);
      subop = subop->parallelop;
    }

  op->last_sigval = fmsyntheg_operate(op->eg) * op->wavegen(phase)
                  / FMSYNTH_MAX_EGLEVEL;

  return op->last_sigval;
}
