/****************************************************************************
 * apps/examples/audio_rttl/audio_rttl.cxx
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
#include <math.h>
#include <string.h>
#include <arch/board/board.h>
#include <arch/chip/audio.h>

#include "audio_rttl.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Note - note freq on octave 1 */

#define C 16.35
#define CD 17.32
#define D 18.35
#define DE 19.45
#define E 20.60
#define F 21.83
#define FG 23.12
#define G 24.50
#define A 27.50
#define AB 29.14
#define B 30.84

/* Octave - base freq multiplier for the base freq */

#define OCT2 4
#define OCT3 8
#define OCT4 16
#define OCT5 32
#define OCT6 64
#define OCT7 128
#define OCT8 256

/****************************************************************************
 * Public Functions
 ****************************************************************************/

extern "C" int main(int argc, FAR char *argv[])
{
  int freq;
  float base_freq;
  char temp_note;
  int octave;
  int duration;
  int note_duration;
  int base_duration;
  int bpm;
  int cnt;

  /* Set I/O parameters for power on. */

  if (!board_audio_power_control(true))
    {
      printf("Error: board_audio_power_control() failure.\n");
      return 1;
    }

  printf("Start Audio RTTL player\n");

  bpm = 0;
  octave = 0;
  base_duration = 0;
  note_duration = 0;

  while (*g_song != ':')
    {
      g_song++;
    }

  g_song++;

  if (*g_song == 'd')
    {
      g_song = g_song + 2;
      base_duration = *g_song - '0';
      g_song++;
      if ((*g_song >= '0') && (*g_song <= '9'))
        {
          base_duration = (base_duration * 10) + (*g_song - '0');
          g_song++;
        }

      g_song++;
    }

  if (*g_song == 'o')
    {
      g_song = g_song + 2;
      octave = *g_song - '0';
      g_song = g_song + 2;
    }

  if (octave == 2)
    {
      octave = OCT2;
    }
  else if (octave == 3)
    {
      octave = OCT3;
    }
  else if (octave == 4)
    {
      octave = OCT4;
    }
  else if (octave == 5)
    {
      octave = OCT5;
    }
  else if (octave == 6)
    {
      octave = OCT6;
    }
  else if (octave == 7)
    {
      octave = OCT7;
    }
  else if (octave == 8)
    {
      octave = OCT8;
    }

  if (*g_song == 'b')
    {
      g_song = g_song + 2;
      bpm = 0;
      for (cnt = 1; cnt <= 3; cnt++)
        {
          if (*g_song != ':')
            {
              bpm = bpm * 10 + (*g_song - '0');
              g_song++;
            }

        }

      g_song++;
    }

  do
    {
      if ((*g_song >= '0') && (*g_song <= '9'))
        {
          note_duration = *g_song - '0';
          g_song++;
        }

      if ((*g_song >= '0') && (*g_song <= '9'))
        {
          note_duration = (note_duration * 10) + (*g_song - '0');
          g_song++;
        }

      if (note_duration > 0)
        {
          duration = ((60 * 1000L / bpm) * 4) / note_duration;
        }
      else
        {
          duration = ((60 * 1000L / bpm) * 4) / base_duration;
        }

      base_freq = 0;
      temp_note = ' ';
      switch (*g_song)
        {
        case 'c':
          {
            temp_note = 'c';
            base_freq = C;
            break;
          }

        case 'd':
          {
            temp_note = 'd';
            base_freq = D;
            break;
          }

        case 'e':
          {
            temp_note = 'e';
            base_freq = E;
            break;
          }

        case 'f':
          {
            temp_note = 'f';
            base_freq = F;
            break;
          }

        case 'g':
          {
            temp_note = 'g';
            base_freq = G;
            break;
          }

        case 'a':
          {
            temp_note = 'a';
            base_freq = A;
            break;
          }

        case 'b':
          {
            temp_note = 'b';
            base_freq = B;
            break;
          }

        case 'p':
          {
            temp_note = 'p';
            break;
          }

        default:
          break;
        }

      g_song++;

      if (*g_song == '.')
        {
          duration += duration / 2;
          g_song++;
        }

      if (*g_song == '#')
        {
          if (temp_note == 'c')
            {
              base_freq = CD;
            }

          if (temp_note == 'd')
            {
              base_freq = DE;
            }

          if (temp_note == 'f')
            {
              base_freq = FG;
            }

          if (temp_note == 'a')
            {
              base_freq = AB;
            }

          g_song++;
        }

      if ((*g_song - '0') == '2')
        {
          octave = OCT2;
        }

      if ((*g_song - '0') == '3')
        {
          octave = OCT3;
        }

      if ((*g_song - '0') == '4')
        {
          octave = OCT4;
        }

      if ((*g_song - '0') == '5')
        {
          octave = OCT5;
        }

      if ((*g_song - '0') == '6')
        {
          octave = OCT6;
        }

      if ((*g_song - '0') == '7')
        {
          octave = OCT7;
        }

      if ((*g_song - '0') == '8')
        {
          octave = OCT8;
        }

      g_song++;

      if (*g_song == ',')
        {
          g_song++;
        }

      /* play the sound */

      freq = ceil(base_freq * octave);

      if (!board_audio_tone_generator(1, -40, freq))
        {
          break;
        }

      usleep(duration * 1000L);
    }
  while (*g_song);

  /* Sound off. */

  if (!board_audio_tone_generator(0, 0, 0))
    {
      printf("Error: board_audio_tone_generator() failure.\n");
      return 1;
    }

  /* Set I/O parameters for power off. */

  if (!board_audio_power_control(false))
    {
      printf("Error: board_audio_power_control() failure.\n");
      return 1;
    }

  printf("Done.\n");

  return 0;
}
