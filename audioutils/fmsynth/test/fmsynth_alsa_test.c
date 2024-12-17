/****************************************************************************
 * apps/audioutils/fmsynth/test/fmsynth_alsa_test.c
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
#include <termios.h>
#include <fcntl.h>

#include <alsa/asoundlib.h>

#include <audioutils/fmsynth_eg.h>
#include <audioutils/fmsynth_op.h>
#include <audioutils/fmsynth.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS (48000)

#define CHANNEL_NUM (2)
#define RESAMPLING_ALSA (1)
#define LATENCY_ALSA (10000)

#define SAMPLE_NUM (FS / 20)
#define BUFF_LENGTH (SAMPLE_NUM * CHANNEL_NUM)

#define CODE_C_FREQ (261.625565f)
#define CODE_D_FREQ (293.6647674f)
#define CODE_E_FREQ (329.6275561f)
#define CODE_F_FREQ (349.2282305f)
#define CODE_G_FREQ (391.9954347f)
#define CODE_A_FREQ (440.f)
#define CODE_B_FREQ (493.8833009f)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int16_t samples[BUFF_LENGTH];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: init_alsa
 ****************************************************************************/

static snd_pcm_t *init_alsa(int fs)
{
  int ret;
  snd_pcm_t *hndl = NULL;

  ret = snd_pcm_open(&hndl, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (ret < 0)
    {
      printf("sdn_pcm_open error\n");
      return NULL;
    }

  ret = snd_pcm_set_params(hndl,
                           SND_PCM_FORMAT_S16,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           CHANNEL_NUM, fs, RESAMPLING_ALSA, LATENCY_ALSA);
  if (ret != 0)
    {
      printf("sdn_pcm_set_params error\n");
      snd_pcm_close(hndl);
      return NULL;
    }

  return hndl;
}

/****************************************************************************
 * name: set_nonblocking
 ****************************************************************************/

static int set_nonblocking(struct termios *saved)
{
  struct termios settings;

  tcgetattr(0, saved);
  settings = *saved;

  settings.c_lflag &= ~(ECHO | ICANON);
  settings.c_cc[VTIME] = 0;
  settings.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &settings);
  fcntl(0, F_SETFL, O_NONBLOCK);

  return OK;
}

/****************************************************************************
 * name: store_setting
 ****************************************************************************/

static void store_setting(struct termios *saved)
{
  tcsetattr(0, TCSANOW, saved);
}

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
  int running;
  int dump_count = 0;
  int dump_enable = 0;

  fmsynth_eglevels_t levels;
  fmsynth_sound_t *snd1;
  fmsynth_op_t *envop;
  fmsynth_op_t *fbop;
  snd_pcm_t *hndl = NULL;

  struct termios save_param;

  hndl = init_alsa(FS);
  if (!hndl)
    {
      printf("Init alsa error\n");
      return -1;
    }

  /* Initialize FM synthesizer */

  fmsynth_initialize(FS);

  /* Operator setup */

  envop  = fmsynthop_create();
  fbop   = fmsynthop_create();

  set_levels(&levels, 0.6f, 100, 0.3f, 300, 0.1f, 500, 0.f, 0, 0.f, 70);

  fmsynthop_set_envelope(envop, &levels);
  fmsynthop_select_opfunc(envop, FMSYNTH_OPFUNC_SIN);

  fmsynthop_set_envelope(fbop, &levels);
  fmsynthop_select_opfunc(fbop, FMSYNTH_OPFUNC_SIN);
  fmsynthop_bind_feedback(fbop, fbop, 0.6f);

  fmsynthop_parallel_subop(envop, fbop);

  /* Sound setup */

  snd1 = fmsynthsnd_create();
  fmsynthsnd_set_operator(snd1, envop);
  fmsynthsnd_set_soundfreq(snd1, CODE_C_FREQ);

  set_nonblocking(&save_param);
  running = 1;
  while (running)
    {
      switch (getchar())
        {
          case 'c':
            fmsynthsnd_set_soundfreq(snd1, CODE_C_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Do\n");
            break;
          case 'd':
            fmsynthsnd_set_soundfreq(snd1, CODE_D_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Le\n");
            break;
          case 'e':
            fmsynthsnd_set_soundfreq(snd1, CODE_E_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Mi\n");
            break;
          case 'f':
            fmsynthsnd_set_soundfreq(snd1, CODE_F_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Fha\n");
            break;
          case 'g':
            fmsynthsnd_set_soundfreq(snd1, CODE_G_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("So\n");
            break;
          case 'a':
            fmsynthsnd_set_soundfreq(snd1, CODE_A_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Ra\n");
            break;
          case 'b':
            fmsynthsnd_set_soundfreq(snd1, CODE_B_FREQ);
            if (dump_enable)
              {
                dump_count = FS;
                dump_enable = 0;
                printf("DUMP: ");
              }

            printf("Shi\n");
            break;
          case 'z':
            dump_enable = 1;
            printf("Dump next code\n");
            break;
          case 'q':
            running = 0;
            break;
        }

      fmsynth_rendering(snd1, samples, BUFF_LENGTH, CHANNEL_NUM, NULL, 0);

      if (dump_count)
        {
          for (int i = 0; i < BUFF_LENGTH; i += 2)
            {
              printf("%d\n", samples[i]);
            }

          dump_count -= SAMPLE_NUM;
        }

      snd_pcm_writei(hndl, samples, SAMPLE_NUM);
    }

  snd_pcm_drain(hndl);
  snd_pcm_close(hndl);

  fmsynthop_delete(envop);
  fmsynthop_delete(fbop);

  fmsynthsnd_delete(snd1);

  store_setting(&save_param);

  return 0;
}
