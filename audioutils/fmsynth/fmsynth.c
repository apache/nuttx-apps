/****************************************************************************
 * apps/audioutils/fmsynth/fmsynth.c
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
#include <audioutils/fmsynth.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WRAP_ROUND_TIME_SEC (10)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int max_phase_time;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: fetch_feedback
 ****************************************************************************/

static void fetch_feedback(FAR fmsynth_op_t *ops)
{
  while (ops != NULL)
    {
      fmsynthop_update_feedback(ops);
      ops = ops->parallelop;
    }
}

/****************************************************************************
 * name: update_phase
 ****************************************************************************/

static void update_phase(FAR fmsynth_sound_t *snd)
{
  snd->phase_time++;
  if (snd->phase_time >= max_phase_time)
    {
      snd->phase_time = 0;
    }
}

/****************************************************************************
 * name: sound_modulate
 ****************************************************************************/

static int sound_modulate(FAR fmsynth_sound_t *snd)
{
  int out = 0;
  FAR fmsynth_op_t *op;

  if (snd->operators == NULL)
    {
      return out;
    }

  fetch_feedback(snd->operators);

  for (op = snd->operators; op != NULL; op = op->parallelop)
    {
      out += fmsynthop_operate(op, snd->phase_time);
    }

  update_phase(snd);

  return out * snd->volume / FMSYNTH_MAX_VOLUME;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: fmsynth_initialize
 ****************************************************************************/

int fmsynth_initialize(int fs)
{
  max_phase_time = fs * WRAP_ROUND_TIME_SEC;
  return fmsynthop_set_samplerate(fs);
}

/****************************************************************************
 * name: create_fmsynthsnd
 ****************************************************************************/

FAR fmsynth_sound_t *create_fmsynthsnd(FAR fmsynth_sound_t *snd)
{
  if (snd)
    {
      snd->own_allocate = 0;
      snd->phase_time = 0;
      snd->volume = FMSYNTH_MAX_VOLUME;
      snd->operators = NULL;
      snd->next_sound = NULL;
    }

  return snd;
}

/****************************************************************************
 * name: fmsynthsnd_create
 ****************************************************************************/

FAR fmsynth_sound_t *fmsynthsnd_create(void)
{
  FAR fmsynth_sound_t *ret;
  ret = (FAR fmsynth_sound_t *)malloc(sizeof(fmsynth_sound_t));

  if (ret)
    {
      create_fmsynthsnd(ret);
      ret->own_allocate = 1;
    }

  return ret;
}

/****************************************************************************
 * name: fmsynthsnd_delete
 ****************************************************************************/

void fmsynthsnd_delete(FAR fmsynth_sound_t *snd)
{
  if (snd != NULL && snd->own_allocate == 1)
    {
      free(snd);
    }
}

/****************************************************************************
 * name: fmsynthsnd_set_operator
 ****************************************************************************/

int fmsynthsnd_set_operator(FAR fmsynth_sound_t *snd, FAR fmsynth_op_t *op)
{
  snd->operators = op;

  return OK;
}

/****************************************************************************
 * name: fmsynthsnd_set_soundfreq
 ****************************************************************************/

void fmsynthsnd_set_soundfreq(FAR fmsynth_sound_t *snd, float freq)
{
  FAR fmsynth_op_t *op;

  for (op = snd->operators; op != NULL; op = op->parallelop)
    {
      fmsynthop_set_soundfreq(op, freq);
      fmsynthop_start(op);
    }
}

/****************************************************************************
 * name: fmsynthsnd_stop
 ****************************************************************************/

void fmsynthsnd_stop(FAR fmsynth_sound_t *snd)
{
  FAR fmsynth_op_t *op;

  for (op = snd->operators; op != NULL; op = op->parallelop)
    {
      fmsynthop_stop(op);
    }
}

/****************************************************************************
 * name: fmsynthsnd_set_volume
 ****************************************************************************/

void fmsynthsnd_set_volume(FAR fmsynth_sound_t *snd, float vol)
{
  snd->volume = vol * FMSYNTH_MAX_VOLUME;
}

/****************************************************************************
 * name: fmsynthsnd_add_subsound
 ****************************************************************************/

int fmsynthsnd_add_subsound(FAR fmsynth_sound_t *top,
                            FAR fmsynth_sound_t *sub)
{
  FAR fmsynth_sound_t *s = top;

  if (!top || !sub)
    {
      return ERROR;
    }

  for (s = top; s->next_sound; s = s->next_sound);

  s->next_sound = sub;

  return OK;
}

/****************************************************************************
 * name: fmsynth_rendering
 ****************************************************************************/

int fmsynth_rendering(FAR fmsynth_sound_t *snd,
                      FAR int16_t *sample, int sample_num, int chnum,
                      fmsynth_tickcb_t cb, unsigned long cbarg)
{
  int i;
  int ch;
  int out;
  FAR fmsynth_sound_t *itr;

  for (i = 0; i < sample_num; i += chnum)
    {
      out = 0;
      for (itr = snd; itr != NULL; itr = itr->next_sound)
        {
          out = out + sound_modulate(itr);
        }

      for (ch = 0; ch < chnum; ch++)
        {
          *sample++ = (int16_t)out;
        }

      if (cb != NULL)
        {
          cb(cbarg);
        }
    }

  if (i > sample_num)
    {
      i -= chnum;
    }

  /* Return total bytes stored in the buffer */

  return i * sizeof(int16_t);
}
