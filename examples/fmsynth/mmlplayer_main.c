/****************************************************************************
 * apps/examples/fmsynth/mmlplayer_main.c
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

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <limits.h>

#include <nuttx/audio/audio.h>
#include <audioutils/fmsynth.h>
#include <audioutils/nxaudio.h>
#include <audioutils/mml_parser.h>

#include "operator_algorithm.h"
#include "music_scale.h"
#include "mmlplayer_score.h"

/****************************************************************************
 * Pre-processor
 ****************************************************************************/

#define APP_FS        (48000)
#define APP_BPS       (16)
#define APP_CHNUM     (2)
#define CARRIER_LEVEL (25.f / 100.f)

#define APP_DEFAULT_VOL (1000)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct app_options
{
  int volume;
  int mode;
};

struct mmlplayer_s
{
  struct nxaudio_s nxaudio;

  /* Right hand sound */

  FAR fmsynth_sound_t *rsound[2]; /* Need 2 sounds for CHORD */
  FAR fmsynth_op_t    *rop[2];    /* Need 2 sounds for CHORD */

  int rtick;
  FAR char *rscore;
  struct music_macro_lang_s rmml;

  /* Left hand sound */

  FAR fmsynth_sound_t *lsound;
  FAR fmsynth_op_t    *lop;
  int ltick;
  FAR char *lscore;
  struct music_macro_lang_s lmml;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void app_dequeue_cb(unsigned long arg,
                           FAR struct ap_buffer_s *apb);
static void app_complete_cb(unsigned long arg);
static void app_user_cb(unsigned long arg,
                        FAR struct audio_msg_s *msg, FAR bool *running);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct mmlplayer_s g_mmlplayer;
static bool g_running = true;

static struct nxaudio_callbacks_s cbs =
{
  app_dequeue_cb,
  app_complete_cb,
  app_user_cb
};

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * name: print_note
 ****************************************************************************/

static void print_note(bool LR, int index, int length)
{
  printf("%c: O%d%c : %d\n", LR ? 'R' : 'L',
                index / 12, "CcDdEFfGgAaB"[index % 12], length);
}

/****************************************************************************
 * name: print_chord
 ****************************************************************************/

static void print_chord(bool LR, int index1, int index2, int length)
{
  printf("%c: [O%d%c, O%d%c] : %d\n", LR ? 'R' : 'L',
                index1 / 12, "CcDdEFfGgAaB"[index1 % 12],
                index2 / 12, "CcDdEFfGgAaB"[index2 % 12],
                length);
}

/****************************************************************************
 * name: update_righthand_note
 ****************************************************************************/

static void update_righthand_note(FAR struct mmlplayer_s *fmmsc)
{
  int mml_ret;
  struct mml_result_s mml_result;

  fmmsc->rtick = 0;
  do
    {
      mml_ret = parse_mml(&fmmsc->rmml, &fmmsc->rscore, &mml_result);
      switch (mml_ret)
        {
          case MML_TYPE_NOTE:
            fmmsc->rtick = mml_result.length;
            fmsynthsnd_set_soundfreq(fmmsc->rsound[0],
                                     musical_scale[mml_result.note_idx[0]]);
            fmsynthsnd_set_volume(fmmsc->rsound[0], CARRIER_LEVEL);
            fmsynthsnd_set_volume(fmmsc->rsound[1], 0.f);
            print_note(1, mml_result.note_idx[0], mml_result.length);
            break;

          case MML_TYPE_CHORD:
            fmmsc->rtick = mml_result.length;
            fmsynthsnd_set_soundfreq(fmmsc->rsound[0],
                                    musical_scale[mml_result.note_idx[0]]);
            fmsynthsnd_set_soundfreq(fmmsc->rsound[1],
                                    musical_scale[mml_result.note_idx[1]]);
            fmsynthsnd_set_volume(fmmsc->rsound[0], CARRIER_LEVEL);
            fmsynthsnd_set_volume(fmmsc->rsound[1], CARRIER_LEVEL);
            print_chord(1, mml_result.note_idx[0], mml_result.note_idx[1],
                                                        mml_result.length);
            break;

          case MML_TYPE_REST:
            fmmsc->rtick = mml_result.length;
            fmsynthsnd_set_volume(fmmsc->rsound[0], 0.f);
            fmsynthsnd_set_volume(fmmsc->rsound[1], 0.f);
            printf("R: Rest : %d\n", mml_result.length);
            break;

          default:

            /* Do nothing */

            break;
        }
    }
  while (!fmmsc->rtick && mml_ret != MML_TYPE_EOF);
}

/****************************************************************************
 * name: update_lefthand_note
 ****************************************************************************/

static void update_lefthand_note(FAR struct mmlplayer_s *fmmsc)
{
  int mml_ret;
  struct mml_result_s mml_result;

  fmmsc->ltick = 0;
  do
    {
      mml_ret = parse_mml(&fmmsc->lmml, &fmmsc->lscore, &mml_result);
      switch (mml_ret)
        {
          case MML_TYPE_NOTE:
            fmmsc->ltick = mml_result.length;
            fmsynthsnd_set_soundfreq(fmmsc->lsound,
                                    musical_scale[mml_result.note_idx[0]]);
            fmsynthsnd_set_volume(fmmsc->lsound, CARRIER_LEVEL);
            print_note(0, mml_result.note_idx[0], mml_result.length);
            break;

          case MML_TYPE_REST:
            fmmsc->ltick = mml_result.length;
            fmsynthsnd_set_volume(fmmsc->lsound, 0.f);
            printf("L: Rest : %d\n", mml_result.length);
            break;

          default:

            /* Do nothing */

            break;
        }
    }
  while (!fmmsc->ltick && mml_ret != MML_TYPE_EOF);
}

/****************************************************************************
 * name: tick_callback
 ****************************************************************************/

static void tick_callback(unsigned long arg)
{
  FAR struct mmlplayer_s *fmmsc = (FAR struct mmlplayer_s *)(uintptr_t)arg;

  fmmsc->rtick--;
  fmmsc->ltick--;

  if (fmmsc->rtick <= 0)
    {
      update_righthand_note(fmmsc);
    }

  if (fmmsc->ltick <= 0)
    {
      update_lefthand_note(fmmsc);
    }
}

/****************************************************************************
 * name: app_dequeue_cb
 ****************************************************************************/

static void app_dequeue_cb(unsigned long arg,
                           FAR struct ap_buffer_s *apb)
{
  FAR struct mmlplayer_s *mmlplayer = (struct mmlplayer_s *)(uintptr_t)arg;

  apb->curbyte = 0;
  apb->flags = 0;
  apb->nbytes = fmsynth_rendering(mmlplayer->lsound,
                                  (FAR int16_t *)apb->samp,
                                  apb->nmaxbytes / sizeof(int16_t),
                                  mmlplayer->nxaudio.chnum,
                                  tick_callback,
                                  (unsigned long)(uintptr_t)mmlplayer);

  if (g_running)
    {
      nxaudio_enqbuffer(&mmlplayer->nxaudio, apb);
    }
}

/****************************************************************************
 * name: app_complete_cb
 ****************************************************************************/

static void app_complete_cb(unsigned long arg)
{
  /* Do nothing.. */

  printf("Audio loop is Done\n");
}

/****************************************************************************
 * name: app_user_cb
 ****************************************************************************/

static void app_user_cb(unsigned long arg,
                        FAR struct audio_msg_s *msg, FAR bool *running)
{
  /* Do nothing.. */
}

/****************************************************************************
 * name: autio_loop_thread
 ****************************************************************************/

static FAR void *audio_loop_thread(pthread_addr_t arg)
{
  struct mmlplayer_s *mmlplayer = (FAR struct mmlplayer_s *)arg;

  nxaudio_start(&mmlplayer->nxaudio);
  nxaudio_msgloop(&mmlplayer->nxaudio, &cbs,
                  (unsigned long)(uintptr_t)mmlplayer);

  return NULL;
}

/****************************************************************************
 * name: create_audio_thread
 ****************************************************************************/

static pthread_t create_audio_thread(FAR struct mmlplayer_s *mmlplayer)
{
  pthread_t pid;
  pthread_attr_t tattr;
  struct sched_param sparam;

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr, 4096);

  pthread_create(&pid, &tattr, audio_loop_thread,
                              (pthread_addr_t)mmlplayer);
  pthread_setname_np(pid, "mmlplayer_thread");

  return pid;
}

/****************************************************************************
 * name: delete_sounds
 ****************************************************************************/

static void delete_sounds(FAR struct mmlplayer_s *mmlplayer)
{
  if (mmlplayer->rop[0])
    {
      fmsynthutil_delete_ops(mmlplayer->rop[0]);
    }

  if (mmlplayer->rop[1])
    {
      fmsynthutil_delete_ops(mmlplayer->rop[1]);
    }

  if (mmlplayer->lop)
    {
      fmsynthutil_delete_ops(mmlplayer->lop);
    }

  if (mmlplayer->rsound[0])
    {
      fmsynthsnd_delete(mmlplayer->rsound[0]);
    }

  if (mmlplayer->rsound[1])
    {
      fmsynthsnd_delete(mmlplayer->rsound[1]);
    }

  if (mmlplayer->lsound)
    {
      fmsynthsnd_delete(mmlplayer->lsound);
    }
}

/****************************************************************************
 * name: init_fmmusi_soundsc
 ****************************************************************************/

static int init_mmlplayer_sound(FAR struct mmlplayer_s *mmlplayer, int fs,
                                int mode)
{
  CODE fmsynth_op_t *(*opfunc)(void);

  opfunc = mode == 0 ? fmsynthutil_algorithm0 :
           mode == 1 ? fmsynthutil_algorithm1 :
           mode == 2 ? fmsynthutil_algorithm2 :
           NULL;

  fmsynth_initialize(fs);

  mmlplayer->rop[0] = NULL;
  mmlplayer->rop[1] = NULL;
  mmlplayer->lop    = NULL;

  mmlplayer->rsound[0] = fmsynthsnd_create();
  mmlplayer->rsound[1] = fmsynthsnd_create();
  mmlplayer->lsound    = fmsynthsnd_create();

  if (mmlplayer->rsound[0] && mmlplayer->rsound[1] && mmlplayer->lsound)
    {
      mmlplayer->rop[0] = opfunc();
      mmlplayer->rop[1] = opfunc();
      mmlplayer->lop    = opfunc();

      if (mmlplayer->rop[0] && mmlplayer->rop[1] && mmlplayer->lop)
        {
          fmsynthsnd_set_operator(mmlplayer->rsound[0], mmlplayer->rop[0]);
          fmsynthsnd_set_operator(mmlplayer->rsound[1], mmlplayer->rop[1]);
          fmsynthsnd_set_operator(mmlplayer->lsound, mmlplayer->lop);
          fmsynthsnd_set_operator(mmlplayer->lsound, mmlplayer->lop);

          fmsynthsnd_add_subsound(mmlplayer->lsound, mmlplayer->rsound[0]);
          fmsynthsnd_add_subsound(mmlplayer->lsound, mmlplayer->rsound[1]);
        }
      else
        {
          delete_sounds(mmlplayer);
          return ERROR;
        }
    }
  else
    {
      delete_sounds(mmlplayer);
      return ERROR;
    }

  mmlplayer->rtick = 0;
  mmlplayer->ltick = 0;

  init_mml(&mmlplayer->rmml, fs, 120, 4, 4);
  init_mml(&mmlplayer->lmml, fs, 120, 4, 3);

  mmlplayer->rscore = (FAR char *)floh_walzer_right;
  mmlplayer->lscore = (FAR char *)floh_walzer_left;

  return OK;
}

/****************************************************************************
 * name: fin_mmlplayer
 ****************************************************************************/

static void fin_mmlplayer(FAR struct mmlplayer_s *mmlplayer)
{
  fin_nxaudio(&mmlplayer->nxaudio);
  delete_sounds(mmlplayer);
}

/****************************************************************************
 * name: print_help
 ****************************************************************************/

static void print_help(FAR char *name)
{
  printf("nsh> %s ([-v (volume)]) ([-m (mode)])\n", name);
}

/****************************************************************************
 * name: configure_option
 ****************************************************************************/

static int configure_option(FAR struct app_options *option,
                            int argc, char **argv)
{
  int opt;

  option->volume = APP_DEFAULT_VOL;
  option->mode = 0;
  while ((opt = getopt(argc, argv, "hv:m:")) != ERROR)
    {
      switch (opt)
        {
          case 'v':
            option->volume = atoi(optarg) * 10;
            if (option->volume < 0 || option->volume > 1000)
              {
                option->volume = 400;
              }

            break;

          case 'm':
            option->mode = atoi(optarg);
            if (option->mode < 0 || option->mode >= 3)
              {
                option->mode = 0;
              }

            break;

          case 'h':
            return ERROR;
            break;

          default:
            return ERROR;
            break;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int i;
  int ret;
  int key;
  pthread_t pid;
  struct app_options appopt;

  printf("Start %s\n", argv[0]);

  g_running = true;
  if (configure_option(&appopt, argc, argv) != OK)
    {
      print_help(argv[0]);
      return -1;
    }

  ret = init_nxaudio(&g_mmlplayer.nxaudio, APP_FS, APP_BPS, APP_CHNUM);
  if (ret < 0)
    {
      printf("init_nxaoud() returned with error!!\n");
      return -1;
    }

  nxaudio_setvolume(&g_mmlplayer.nxaudio, appopt.volume);

  ret = init_mmlplayer_sound(&g_mmlplayer, APP_FS, appopt.mode);
  if (ret != OK)
    {
      printf("init_mmlplayer_sound() returned error.\n");
      fin_nxaudio(&g_mmlplayer.nxaudio);
      return -1;
    }

  for (i = 0; i < g_mmlplayer.nxaudio.abufnum; i++)
    {
      app_dequeue_cb((unsigned long)&g_mmlplayer,
                           g_mmlplayer.nxaudio.abufs[i]);
    }

  pid = create_audio_thread(&g_mmlplayer);

  while (g_running)
    {
      key = getchar();
      if (key != EOF)
        {
          switch (key)
            {
              case 'q':
                g_running = false;
                break;
            }
        }
    }

  nxaudio_stop(&g_mmlplayer.nxaudio);
  pthread_join(pid, NULL);

  fin_mmlplayer(&g_mmlplayer);

  return ret;
}
