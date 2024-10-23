/****************************************************************************
 * apps/audioutils/alsa-lib/pcm_params.c
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

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "pcm_util.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline int snd_pcm_hw_is_mask(int var)
{
  return var >= SND_PCM_HW_PARAM_FIRST_MASK &&
         var <= SND_PCM_HW_PARAM_LAST_MASK;
}

static inline int snd_pcm_hw_is_range(int var)
{
  return var >= SND_PCM_HW_PARAM_FIRST_RANGE &&
         var <= SND_PCM_HW_PARAM_LAST_RANGE;
}

static int snd_pcm_hw_param_set(FAR snd_pcm_hw_params_t *params,
                                snd_pcm_hw_param_t var, unsigned int val)
{
  FAR snd_interval_t *interval = NULL;

  assert(params);
  interval = &params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL];
  if (snd_pcm_hw_is_mask(var))
    {
      interval->mask = val;
    }
  else if (snd_pcm_hw_is_range(var))
    {
      interval->range.min = val;
    }
  else
    {
      assert(0);
      return -EINVAL;
    }

  return 0;
}

static int snd_pcm_hw_param_get(FAR const snd_pcm_hw_params_t *params,
                                snd_pcm_hw_param_t var,
                                FAR unsigned int *val)
{
  FAR const snd_interval_t *interval;

  assert(params);
  interval = &params->intervals[var - SND_PCM_HW_PARAM_FIRST_INTERVAL];
  if (snd_pcm_hw_is_mask(var))
    {
      *val = interval->mask;
    }
  else
    {
      *val = interval->range.min;
    }

  return 0;
}

#define snd_pcm_hw_param_set(params, var, val)                              \
  snd_pcm_hw_param_set(params, var, (unsigned int)(val))
#define snd_pcm_hw_param_get(params, var, val)                              \
  snd_pcm_hw_param_get(params, var, (FAR unsigned int *)(val))

static int snd_pcm_hw_params_internal(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params)
{
  int period_size;
  int buffer_size;

  assert(pcm && params);

  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_FORMAT, &pcm->format);
  if (!pcm->format)
    {
      pcm->format = SND_PCM_FORMAT_S16;
    }

  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS, &pcm->channels);
  if (!pcm->channels)
    {
      pcm->channels = 2;
    }

  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_RATE, &pcm->sample_rate);
  if (!pcm->sample_rate)
    {
      pcm->sample_rate = 44100;
    }

  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIODS, &pcm->periods);
  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_TIME,
                       &pcm->period_time);
  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_SIZE, &period_size);
  snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_SIZE, &buffer_size);
  if (period_size && buffer_size)
    {
      pcm->periods = buffer_size / period_size;
      pcm->period_time = period_size * 1000 * 1000 / pcm->sample_rate;
    }

  SND_INFO("format:%d, ch:%d, rate:%d, periods:%d, period_time:%d",
           pcm->format, pcm->channels, pcm->sample_rate, pcm->periods,
           pcm->period_time);

  pcm->state = SND_PCM_STATE_SETUP;
  pcm->setup = 1;

  return 0;
}

/****************************************************************************
 * Public HW Functions
 ****************************************************************************/

int snd_pcm_hw_params_any(FAR snd_pcm_t *pcm,
                          FAR snd_pcm_hw_params_t *params)
{
  return 0;
}

int snd_pcm_hw_params_set_format(FAR snd_pcm_t *pcm,
                                 FAR snd_pcm_hw_params_t *params,
                                 snd_pcm_format_t format)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_FORMAT, format);
}

int snd_pcm_hw_params_set_channels(FAR snd_pcm_t *pcm,
                                   FAR snd_pcm_hw_params_t *params,
                                   unsigned int val)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_CHANNELS, val);
}

int snd_pcm_hw_params_get_channels(FAR const snd_pcm_hw_params_t *params,
                                   FAR unsigned int *val)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_CHANNELS, val);
}

int snd_pcm_hw_params_set_rate(FAR snd_pcm_t *pcm,
                               FAR snd_pcm_hw_params_t *params,
                               unsigned int val, int dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_RATE, val);
}

int snd_pcm_hw_params_set_rate_near(FAR snd_pcm_t *pcm,
                                    FAR snd_pcm_hw_params_t *params,
                                    FAR unsigned int *val, FAR int *dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_RATE, *val);
}

int snd_pcm_hw_params_get_rate(FAR const snd_pcm_hw_params_t *params,
                               FAR unsigned int *val, FAR int *dir)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_RATE, val);
}

int snd_pcm_hw_params_set_period_time(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      unsigned int us, int dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_TIME, us);
}

int snd_pcm_hw_params_set_period_time_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR unsigned int *us,
                                           FAR int *dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_TIME, *us);
}

int snd_pcm_hw_params_get_period_time(FAR const snd_pcm_hw_params_t *params,
                                      FAR unsigned int *us, FAR int *dir)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_TIME, us);
}

int snd_pcm_hw_params_set_period_size(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val, int dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_SIZE, val);
}

int snd_pcm_hw_params_set_period_size_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR snd_pcm_uframes_t *val,
                                           FAR int *dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIOD_SIZE, *val);
}

int snd_pcm_hw_params_get_period_size(FAR const snd_pcm_hw_params_t *params,
                                      FAR snd_pcm_uframes_t *val,
                                      FAR int *dir)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIOD_SIZE, val);
}

int snd_pcm_hw_params_set_periods(FAR snd_pcm_t *pcm,
                                  FAR snd_pcm_hw_params_t *params,
                                  unsigned int val, int dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIODS, val);
}

int snd_pcm_hw_params_set_periods_near(FAR snd_pcm_t *pcm,
                                       FAR snd_pcm_hw_params_t *params,
                                       FAR unsigned int *val, FAR int *dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_PERIODS, *val);
}

int snd_pcm_hw_params_get_periods(FAR const snd_pcm_hw_params_t *params,
                                  FAR unsigned int *val, FAR int *dir)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_PERIODS, val);
}

int snd_pcm_hw_params_set_buffer_size(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      snd_pcm_uframes_t val)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_SIZE, val);
}

int snd_pcm_hw_params_set_buffer_size_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR snd_pcm_uframes_t *val)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_SIZE, *val);
}

int snd_pcm_hw_params_get_buffer_size(FAR const snd_pcm_hw_params_t *params,
                                      FAR snd_pcm_uframes_t *val)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_SIZE, val);
}

int snd_pcm_hw_params_set_buffer_time(FAR snd_pcm_t *pcm,
                                      FAR snd_pcm_hw_params_t *params,
                                      unsigned int us)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_TIME, us);
}

int snd_pcm_hw_params_set_buffer_time_near(FAR snd_pcm_t *pcm,
                                           FAR snd_pcm_hw_params_t *params,
                                           FAR unsigned int *us,
                                           FAR int *dir)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_BUFFER_TIME, *us);
}

int snd_pcm_hw_params_get_buffer_time(FAR const snd_pcm_hw_params_t *params,
                                      FAR unsigned int *us, FAR int *dir)
{
  return snd_pcm_hw_param_get(params, SND_PCM_HW_PARAM_BUFFER_TIME, us);
}

int snd_pcm_hw_params_set_access(FAR snd_pcm_t *pcm,
                                 FAR snd_pcm_hw_params_t *params,
                                 snd_pcm_access_t access)
{
  return snd_pcm_hw_param_set(params, SND_PCM_HW_PARAM_ACCESS, access);
}

int snd_pcm_hw_params(FAR snd_pcm_t *pcm, FAR snd_pcm_hw_params_t *params)
{
  int ret;
  assert(pcm && params);

  ret = snd_pcm_hw_params_internal(pcm, params);
  if (ret < 0)
    {
      return ret;
    }

  ret = snd_pcm_prepare(pcm);
  return ret;
}

/****************************************************************************
 * Public SW Functions
 ****************************************************************************/

int snd_pcm_sw_params_current(FAR snd_pcm_t *pcm,
                              FAR snd_pcm_sw_params_t *params)
{
  assert(pcm && params);

  memset(params, 0, sizeof(snd_pcm_sw_params_t));
  params->avail_min = pcm->period_frames;
  params->start_threshold = pcm->start_threshold;

  return 0;
}

int snd_pcm_sw_params_set_start_threshold(FAR snd_pcm_t *pcm,
                                          FAR snd_pcm_sw_params_t *params,
                                          snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->start_threshold = val;
  return 0;
}

int snd_pcm_sw_params_get_start_threshold(FAR snd_pcm_sw_params_t *params,
                                          FAR snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->start_threshold;
  return 0;
}

int snd_pcm_sw_params_set_stop_threshold(FAR snd_pcm_t *pcm,
                                         FAR snd_pcm_sw_params_t *params,
                                         snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->stop_threshold = val;
  return 0;
}

int snd_pcm_sw_params_get_stop_threshold(FAR snd_pcm_sw_params_t *params,
                                         FAR snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->stop_threshold;
  return 0;
}

int snd_pcm_sw_params_set_silence_size(FAR snd_pcm_t *pcm,
                                       FAR snd_pcm_sw_params_t *params,
                                       snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->silence_size = val;
  return 0;
}

int snd_pcm_sw_params_get_silence_size(FAR snd_pcm_sw_params_t *params,
                                       FAR snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->silence_size;
  return 0;
}

int snd_pcm_sw_params_set_avail_min(FAR snd_pcm_t *pcm,
                                    FAR snd_pcm_sw_params_t *params,
                                    snd_pcm_uframes_t val)
{
  assert(pcm && params);

  params->avail_min = val;
  return 0;
}

int snd_pcm_sw_params_get_avail_min(FAR const snd_pcm_sw_params_t *params,
                                    FAR snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->avail_min;
  return 0;
}

int snd_pcm_sw_params_get_boundary(FAR const snd_pcm_sw_params_t *params,
                                   FAR snd_pcm_uframes_t *val)
{
  assert(params);

  *val = params->boundary;
  return 0;
}

int snd_pcm_sw_params(FAR snd_pcm_t *pcm, FAR snd_pcm_sw_params_t *params)
{
  assert(pcm && params);

  pcm->start_threshold = params->start_threshold;
  return 0;
}
