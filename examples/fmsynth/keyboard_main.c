/****************************************************************************
 * apps/examples/fmsynth/keyboard_main.c
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

#include "operator_algorithm.h"
#include "music_scale.h"

/****************************************************************************
 * Pre-processor
 ****************************************************************************/

#define APP_FS    (48000)
#define APP_BPS   (16)
#define APP_CHNUM (2)

#define APP_DEFAULT_VOL (400)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct app_options
{
  int volume;
  int mode;
};

struct kbd_s
{
  struct nxaudio_s nxaudio;

  FAR fmsynth_sound_t *sound;
  FAR fmsynth_op_t *carrier;

  volatile int request_scale;
};

struct key_convert_s
{
  int key;
  char key_str;
  FAR const char *dispstr;
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

static struct kbd_s g_kbd;
static bool g_running = true;

static struct nxaudio_callbacks_s cbs =
{
  app_dequeue_cb,
  app_complete_cb,
  app_user_cb
};

static struct key_convert_s key_convert[] =
{
  { OCTAVE(4, MUSIC_SCALE_C),  'a', "O4C"  },
  { OCTAVE(4, MUSIC_SCALE_CS), 'w', "O4C+" },
  { OCTAVE(4, MUSIC_SCALE_D),  's', "O4D"  },
  { OCTAVE(4, MUSIC_SCALE_DS), 'e', "O4D+" },
  { OCTAVE(4, MUSIC_SCALE_E),  'd', "O4E"  },
  { OCTAVE(4, MUSIC_SCALE_F),  'f', "O4F"  },
  { OCTAVE(4, MUSIC_SCALE_FS), 't', "O4F+" },
  { OCTAVE(4, MUSIC_SCALE_G),  'g', "O4G"  },
  { OCTAVE(4, MUSIC_SCALE_GS), 'y', "O4G+" },
  { OCTAVE(4, MUSIC_SCALE_A),  'h', "O4A"  },
  { OCTAVE(4, MUSIC_SCALE_AS), 'u', "O4A+" },
  { OCTAVE(4, MUSIC_SCALE_B),  'j', "O4B"  },
  { OCTAVE(5, MUSIC_SCALE_C),  'k', "O5C"  },
  { OCTAVE(5, MUSIC_SCALE_CS), 'o', "O5C+" },
  { OCTAVE(5, MUSIC_SCALE_D),  'l', "O5D"  },
  { OCTAVE(5, MUSIC_SCALE_DS), 'p', "O5D+" },
  { OCTAVE(5, MUSIC_SCALE_E),  ';', "O5E"  },
};

#define MAX_KEYCONVERT  (sizeof(key_convert)/sizeof(key_convert[0]))

/****************************************************************************
 * Private functions
 ****************************************************************************/

/****************************************************************************
 * name: convert_key2idx
 ****************************************************************************/

static int convert_key2idx(char key)
{
  int i;

  for (i = 0; i < MAX_KEYCONVERT; i++)
    {
      if (key == key_convert[i].key_str)
        {
          return i;
        }
    }

  return -1;
}

/****************************************************************************
 * name: tick_callback
 ****************************************************************************/

static void tick_callback(unsigned long arg)
{
  FAR struct kbd_s *kbd = (FAR struct kbd_s *)(uintptr_t)arg;
  int scale = kbd->request_scale;

  if (scale != -1)
    {
      fmsynthsnd_set_soundfreq(kbd->sound,
                               musical_scale[scale]);
      kbd->request_scale = -1;
    }
}

/****************************************************************************
 * name: app_dequeue_cb
 ****************************************************************************/

static void app_dequeue_cb(unsigned long arg,
                           FAR struct ap_buffer_s *apb)
{
  FAR struct kbd_s *kbd = (FAR struct kbd_s *)(uintptr_t)arg;

  apb->curbyte = 0;
  apb->flags = 0;

  if (kbd->request_scale != -1)
    {
      apb->nbytes = fmsynth_rendering(kbd->sound,
                                      (FAR int16_t *)apb->samp,
                                      apb->nmaxbytes / sizeof(int16_t),
                                      kbd->nxaudio.chnum,
                                      tick_callback, (unsigned long)kbd);
    }
  else
    {
      apb->nbytes = fmsynth_rendering(kbd->sound,
                                      (FAR int16_t *)apb->samp,
                                      apb->nmaxbytes / sizeof(int16_t),
                                      kbd->nxaudio.chnum,
                                      NULL, 0);
    }

  if (g_running)
    {
      nxaudio_enqbuffer(&kbd->nxaudio, apb);
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
 * name: audio_loop_thread
 ****************************************************************************/

static FAR void *audio_loop_thread(pthread_addr_t arg)
{
  FAR struct kbd_s *kbd = (FAR struct kbd_s *)arg;

  nxaudio_start(&kbd->nxaudio);
  nxaudio_msgloop(&kbd->nxaudio, &cbs, (unsigned long)kbd);

  return NULL;
}

/****************************************************************************
 * name: create_audio_thread
 ****************************************************************************/

static pthread_t create_audio_thread(FAR struct kbd_s *kbd)
{
  pthread_t pid;
  pthread_attr_t tattr;
  struct sched_param sparam;

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO) - 9;
  pthread_attr_setschedparam(&tattr, &sparam);
  pthread_attr_setstacksize(&tattr, 4096);

  pthread_create(&pid, &tattr, audio_loop_thread,
                              (pthread_addr_t)kbd);
  pthread_setname_np(pid, "musickeyboard_thread");

  return pid;
}

/****************************************************************************
 * name: init_fmmusi_soundsc
 ****************************************************************************/

static int init_keyboard_sound(FAR struct kbd_s *kbd, int fs, int mode)
{
  int ret = ERROR;

  fmsynth_initialize(fs);

  kbd->sound = fmsynthsnd_create();
  if (kbd->sound)
    {
      kbd->carrier = mode == 0 ? fmsynthutil_algorithm0() :
                     mode == 1 ? fmsynthutil_algorithm1() :
                     mode == 2 ? fmsynthutil_algorithm2() :
                     NULL;
      if (!kbd->carrier)
        {
          fmsynthsnd_delete(kbd->sound);
          return ret;
        }

      fmsynthsnd_set_operator(kbd->sound, kbd->carrier);
      kbd->request_scale = -1;
      ret = OK;
    }

  return ret;
}

/****************************************************************************
 * name: fin_keyboard
 ****************************************************************************/

static void fin_keyboard(FAR struct kbd_s *kbd)
{
  fin_nxaudio(&kbd->nxaudio);
  fmsynthutil_delete_ops(kbd->carrier);
  fmsynthsnd_delete(kbd->sound);
}

/****************************************************************************
 * name: print_help
 ****************************************************************************/

static void print_help(FAR char *name)
{
  printf("nsh> %s ([-v (volume from 0 to 100)]) ([-m (mode 0, 1 or 2)])\n",
         name);
}

/****************************************************************************
 * name: configure_option
 ****************************************************************************/

static int configure_option(FAR struct app_options *option,
                            int argc, FAR char **argv)
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
            if (option->mode < 0 || option->mode > 3)
              {
                option->mode = 0;
              }
            break;

          default:
            return ERROR;
        }
    }

  return OK;
}

/****************************************************************************
 * name: print_keyusage()
 ****************************************************************************/

static void print_keyusage(void)
{
  int i;

  printf("press any key below to make a sound.\n");
  for (i = 0; i < MAX_KEYCONVERT; i++)
    {
      printf("   [KEY]: %c, [CODE]: %s\n",
             key_convert[i].key_str,
             key_convert[i].dispstr);
    }
  printf("\n");
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
  int key_idx;

  g_running = true;
  if (configure_option(&appopt, argc, argv) != OK)
    {
      print_help(argv[0]);
      return -1;
    }

  ret = init_nxaudio(&g_kbd.nxaudio, APP_FS, APP_BPS, APP_CHNUM);
  if (ret < 0)
    {
      printf("init_nxaoud() returned with error!!\n");
      return -1;
    }

  nxaudio_setvolume(&g_kbd.nxaudio, appopt.volume);

  ret = init_keyboard_sound(&g_kbd, APP_FS, appopt.mode);
  if (ret != OK)
    {
      fin_nxaudio(&g_kbd.nxaudio);
      printf("init_keyboard_sound() error!!\n");
      return -1;
    }

  /* Render audio samples in audio buffers */

  for (i = 0; i < g_kbd.nxaudio.abufnum; i++)
    {
      app_dequeue_cb((unsigned long)&g_kbd,
                           g_kbd.nxaudio.abufs[i]);
    }

  pid = create_audio_thread(&g_kbd);

  printf("Start %s\n", argv[0]);
  print_keyusage();

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

              default:
                key_idx = convert_key2idx(key);
                if (key_idx >= 0)
                  {
                    g_kbd.request_scale = key_convert[key_idx].key;
                    printf("%s \n", key_convert[key_idx].dispstr);
                    fflush(stdout);
                  }
                break;
            }
        }
    }

  nxaudio_stop(&g_kbd.nxaudio);
  pthread_join(pid, NULL);

  fin_keyboard(&g_kbd);

  return ret;
}
