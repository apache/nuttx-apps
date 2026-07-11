/****************************************************************************
 * apps/examples/audio_rttl/audio_rttl_main.c
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

#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>

#ifdef CONFIG_PWM
#include <nuttx/timers/pwm.h>
#endif

#include <audioutils/rtttl.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct player_s
{
  void (*play)(struct rtttl_tone); /* Play RTTTL sound function */
  void (*teardown)(void);          /* Clean-up function */
  void *arg;                       /* Private data for functions */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Default song if none was passed. Otherwise, used as buffer for reading
 * from a file.
 */

static char g_default_song[CONFIG_EXAMPLES_AUDIO_SOUND_MAXLEN] =
    "Test:d=4,o=6,b=90:c,c#,d,d#,e,f,f#,g,a,a#,b,p,b,a#,a,g,"
    "f#,f,e,d#,d,c#,c";

/* The selected player is a global variable so it can be accessed from within
 * the player functions properly.
 */

static struct player_s g_player;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: play_default
 *
 * Description:
 *   The default player choice; prints an error and exits.
 *
 * Input Parameters:
 *   sound - The sound to play
 *
 ****************************************************************************/

static void play_default(struct rtttl_tone sound)
{
  fprintf(stderr, "No player selected!\n");
  return;
}

/****************************************************************************
 * Name: teardown_default
 *
 * Description:
 *   Default tear-down function. Does nothing.
 *
 ****************************************************************************/

static void teardown_default(void)
{
  /* No-op */

  return;
}

#ifdef CONFIG_PWM

/****************************************************************************
 * Name: play_pwm
 *
 * Description:
 *   RTTTL player for audio sinks using PWM devices
 *
 * Input Parameters:
 *   sound - The sound to play
 *
 ****************************************************************************/

static void play_pwm(struct rtttl_tone sound)
{
  int err;
  int i;
  struct pwm_info_s info;
  int fd = *((int *)g_player.arg);

  info.channels[0].channel = 1;
  info.channels[0].duty = b16divi(uitoub16(50) - 1, 100); /* 50% duty cycle */
  info.channels[0].cpol = PWM_CPOL_NDEF;                  /* Default */
  info.channels[0].dcpol = PWM_DCPOL_NDEF;                /* Default */
  info.frequency = sound.frequency_100hz / 100;

  /* Copy settings to all channels */

  for (i = 1; i < CONFIG_PWM_NCHANNELS; i++)
    {
      info.channels[i] = info.channels[0];
      info.channels[i].channel = i + 1;
    }

  err = ioctl(fd, PWMIOC_SETCHARACTERISTICS, &info);
  if (err < 0)
    {
      fprintf(stderr, "Failed to set PWM characteristics: %d\n", errno);
      return;
    }

  err = ioctl(fd, PWMIOC_START, 0);
  if (err < 0)
    {
      fprintf(stderr, "Failed to start PWM: %d\n", errno);
      return;
    }

  usleep(sound.duration_us); /* Wait */

  err = ioctl(fd, PWMIOC_STOP, 0);
  if (err < 0)
    {
      fprintf(stderr, "Failed to stop PWM: %d\n", errno);
    }
}

/****************************************************************************
 * Name: teardown_pwm
 *
 * Description:
 *   Teardown function for PWM players
 *
 ****************************************************************************/

static void teardown_pwm(void)
{
  int fd = *((int *)g_player.arg);
  close(fd);
}
#endif /* CONFIG_PWM */

/****************************************************************************
 * Name: print_usage
 *
 * Description:
 *   Prints out the usage information for the program.
 *
 * Input Parameters:
 *   sink - The stream to output the usage information
 *
 ****************************************************************************/

static void print_usage(FILE *sink)
{
  fprintf(sink, "USAGE: audio_rttl <device path> [-s string] [-f file]\n");
  fprintf(sink, "Ex: audio_rttl /dev/tone0\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int err;
  int fd;
  int c;
  FILE *fptr;
  size_t bread;
  const char *song = g_default_song;
  const char *device = NULL;
  const char *file = NULL;

  g_player.play = play_default;
  g_player.teardown = teardown_default;
  g_player.arg = NULL;

  while ((c = getopt(argc, argv, ":hs:f:")) != -1)
    {
      switch (c)
        {
        case 'h':
          print_usage(stdout);
          break;
        case 's':
          song = optarg;
          break;
        case 'f':
          file = optarg;
          break;
        case '?':
          fprintf(stderr, "Unknown option -%c\n", optopt);
          print_usage(stderr);
          return EXIT_FAILURE;
        }
    }

  if (argc <= optind)
    {
      fprintf(stderr, "Missing argument for device path.\n");
      return EXIT_FAILURE;
    }

  device = argv[optind++];

  /* Based on the device path, we choose which player should be used and
   * perform the setup.
   */

#ifdef CONFIG_PWM
  if (strstr(device, "pwm"))
    {
      fd = open(device, O_RDWR);
      if (fd < 0)
        {
          fprintf(stderr, "Failed to open %s: %d\n", device, errno);
          return EXIT_FAILURE;
        }
      else
        {
          g_player.arg = &fd;
        }

      g_player.play = play_pwm;
      g_player.teardown = teardown_pwm;
    }
#endif /* CONFIG_PWM */

  /* If the user passed a file and a song string, use the string. Otherwise,
   * read from the file
   */

  if (file != NULL && song == g_default_song)
    {
      fptr = fopen(file, "r");
      if (fptr == NULL)
        {
          fprintf(stderr, "Couldn't open file '%s': %d", file, errno);
          g_player.teardown();
          return EXIT_FAILURE;
        }

      bread =
          fread(g_default_song, sizeof(char), sizeof(g_default_song), fptr);

      /* Ensure null termination of the string. */

      if (bread <= sizeof(g_default_song))
        {
          g_default_song[bread - 1] = '\0';
        }

      fclose(fptr);
    }

  /* Play the song */

  err = rtttl_play(song, g_player.play);
  if (err != 0)
    {
      fprintf(stderr, "Failed to play song: %d\n", err);
      return err;
    }

  /* Tear down the player and exit */

  g_player.teardown();
  return EXIT_SUCCESS;
}
