/****************************************************************************
 * apps/testing/drivers/fftest/fftest.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/bits.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/input/ff.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define N_EFFECTS 6

/****************************************************************************
 * Private Types
 ****************************************************************************/

FAR static const char *g_effect_names[] =
{
  "Sine vibration",
  "Constant Force",
  "Spring Condition",
  "Damping Condition",
  "Strong Rumble",
  "Weak Rumble"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  unsigned char fffeatures[1 + FF_MAX / 8 / sizeof(unsigned char)];
  FAR const char *device_file_name = "/dev/input_ff0";
  struct ff_effect effects[N_EFFECTS];
  struct ff_event_s play;
  struct ff_event_s stop;
  struct ff_event_s gain;
  int neffects; /* Number of effects the device can play at the same time */
  int fd;
  int i;

  printf("Force feedback test program.\n");
  for (i = 1; i < argc; i++)
    {
      if (strncmp(argv[i], "--help", 8) == 0)
        {
          printf("Usage: %s /dev/input_ffX\n", argv[0]);
          printf("Tests the force feedback driver\n");
          return -EINVAL;
        }
      else
        {
          device_file_name = argv[i];
        }
    }

  fd = open(device_file_name, O_WRONLY);
  if (fd < 0)
    {
      perror("Open device file");
      return -errno;
    }

  printf("Device %s opened\n", device_file_name);

  /* Query device */

  printf("features:\n");

  /* Force feedback effects */

  memset(fffeatures, 0, sizeof(fffeatures));
  if (ioctl(fd, EVIOCGBIT, fffeatures) < 0)
    {
      perror("Ioctl force feedback features query");
      goto out;
    }

  printf("  * Force feedback effects types: ");
  if (test_bit(FF_CONSTANT, fffeatures))
    {
      printf("Constant, ");
    }

  if (test_bit(FF_PERIODIC, fffeatures))
    {
      printf("Periodic, ");
    }

  if (test_bit(FF_RAMP, fffeatures))
    {
      printf("Ramp, ");
    }

  if (test_bit(FF_SPRING, fffeatures))
    {
      printf("Spring, ");
    }

  if (test_bit(FF_FRICTION, fffeatures))
    {
      printf("Friction, ");
    }

  if (test_bit(FF_DAMPER, fffeatures))
    {
      printf("Damper, ");
    }

  if (test_bit(FF_RUMBLE, fffeatures))
    {
      printf("Rumble, ");
    }

  if (test_bit(FF_INERTIA, fffeatures))
    {
      printf("Inertia, ");
    }

  if (test_bit(FF_GAIN, fffeatures))
    {
      printf("Gain, ");
    }

  if (test_bit(FF_AUTOCENTER, fffeatures))
    {
      printf("Autocenter, ");
    }

  printf("\n    Force feedback periodic effects: ");
  if (test_bit(FF_SQUARE, fffeatures))
    {
      printf("Square, ");
    }

  if (test_bit(FF_TRIANGLE, fffeatures))
    {
      printf("Triangle, ");
    }

  if (test_bit(FF_SINE, fffeatures))
    {
      printf("Sine, ");
    }

  if (test_bit(FF_SAW_UP, fffeatures))
    {
      printf("Saw up, ");
    }

  if (test_bit(FF_SAW_DOWN, fffeatures))
    {
      printf("Saw down, ");
    }

  if (test_bit(FF_CUSTOM, fffeatures))
    {
      printf("Custom, ");
    }

  printf("\n    [");
  for (i = 0; i < sizeof(fffeatures) / sizeof(unsigned char); i++)
    {
      printf("%02X ", fffeatures[i]);
    }

  printf("]\n");
  printf("  * Number of simultaneous effects: ");
  if (ioctl(fd, EVIOCGEFFECTS, &neffects) < 0)
    {
      perror("Ioctl number of effects");
      goto out;
    }

  printf("%d\n\n", neffects);

  /* Set master gain to 75% if supported */

  if (test_bit(FF_GAIN, fffeatures))
    {
      memset(&gain, 0, sizeof(gain));
      gain.code = FF_GAIN;
      gain.value = 0xc000; /* [0, 0xFFFF] */

      printf("Setting master gain to 75%% ... ");
      fflush(stdout);

      if (write(fd, &gain, sizeof(gain)) != sizeof(gain))
        {
          perror("Error: write fd failed");
          goto out;
        }
      else
        {
          printf("OK\n");
        }
    }

  /* Download a periodic sinusoidal effect */

  memset(&effects[0], 0, sizeof(effects[0]));
  effects[0].type = FF_PERIODIC;
  effects[0].id = -1;
  effects[0].u.periodic.waveform = FF_SINE;
  effects[0].u.periodic.period = 100; /* 0.1 second */
  effects[0].u.periodic.magnitude = 0x7fff;
  effects[0].u.periodic.offset = 0;
  effects[0].u.periodic.phase = 0;
  effects[0].direction = 0x4000; /* Along X axis */
  effects[0].u.periodic.envelope.attack_length = 1000;
  effects[0].u.periodic.envelope.attack_level = 0x7fff;
  effects[0].u.periodic.envelope.fade_length = 1000;
  effects[0].u.periodic.envelope.fade_level = 0x7fff;
  effects[0].trigger.button = 0;
  effects[0].trigger.interval = 0;
  effects[0].replay.length = 20000; /* 20 seconds */
  effects[0].replay.delay = 1000;

  printf("Uploading effect #0 (Periodic sinusoidal) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[0]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[0].id);
    }

  /* Download a constant effect */

  effects[1].type = FF_CONSTANT;
  effects[1].id = -1;
  effects[1].u.constant.level = 0x2000; /* Strength : 25 % */
  effects[1].direction = 0x6000;
  effects[1].u.constant.envelope.attack_length = 1000;
  effects[1].u.constant.envelope.attack_level = 0x1000;
  effects[1].u.constant.envelope.fade_length = 1000;
  effects[1].u.constant.envelope.fade_level = 0x1000;
  effects[1].trigger.button = 0;
  effects[1].trigger.interval = 0;
  effects[1].replay.length = 20000; /* 20 seconds */
  effects[1].replay.delay = 0;

  printf("Uploading effect #1 (Constant) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[1]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[1].id);
    }

  /* Download a condition spring effect */

  effects[2].type = FF_SPRING;
  effects[2].id = -1;
  effects[2].u.condition[0].right_saturation = 0x7fff;
  effects[2].u.condition[0].left_saturation = 0x7fff;
  effects[2].u.condition[0].right_coeff = 0x2000;
  effects[2].u.condition[0].left_coeff = 0x2000;
  effects[2].u.condition[0].deadband = 0x0;
  effects[2].u.condition[0].center = 0x0;
  effects[2].u.condition[1] = effects[2].u.condition[0];
  effects[2].trigger.button = 0;
  effects[2].trigger.interval = 0;
  effects[2].replay.length = 20000; /* 20 seconds */
  effects[2].replay.delay = 0;

  printf("Uploading effect #2 (Spring) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[2]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[2].id);
    }

  /* Download a condition damper effect */

  effects[3].type = FF_DAMPER;
  effects[3].id = -1;
  effects[3].u.condition[0].right_saturation = 0x7fff;
  effects[3].u.condition[0].left_saturation = 0x7fff;
  effects[3].u.condition[0].right_coeff = 0x2000;
  effects[3].u.condition[0].left_coeff = 0x2000;
  effects[3].u.condition[0].deadband = 0x0;
  effects[3].u.condition[0].center = 0x0;
  effects[3].u.condition[1] = effects[3].u.condition[0];
  effects[3].trigger.button = 0;
  effects[3].trigger.interval = 0;
  effects[3].replay.length = 20000; /* 20 seconds */
  effects[3].replay.delay = 0;

  printf("Uploading effect #3 (Damper) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[3]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[3].id);
    }

  /* A strong rumbling effect */

  effects[4].type = FF_RUMBLE;
  effects[4].id = -1;
  effects[4].u.rumble.strong_magnitude = 0x8000;
  effects[4].u.rumble.weak_magnitude = 0;
  effects[4].replay.length = 5000;
  effects[4].replay.delay = 1000;

  printf("Uploading effect #4 (Strong rumble, with heavy motor) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[4]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[4].id);
    }

  /* A weak rumbling effect */

  effects[5].type = FF_RUMBLE;
  effects[5].id = -1;
  effects[5].u.rumble.strong_magnitude = 0;
  effects[5].u.rumble.weak_magnitude = 0xc000;
  effects[5].replay.length = 5000;
  effects[5].replay.delay = 0;

  printf("Uploading effect #5 (Weak rumble, with light motor) ... ");
  fflush(stdout);

  if (ioctl(fd, EVIOCSFF, &effects[5]) < 0)
    {
      perror("Error: ioctl failed");
      goto out;
    }
  else
    {
      printf("OK (id %d)\n", effects[5].id);
    }

  /* Ask user what effects to play */

  do
    {
      printf("Enter effect number, -1 to exit\n");
      i = -1;
      if (scanf("%d", &i) == EOF)
        {
          printf("Read error\n");
        }
      else if (i >= 0 && i < N_EFFECTS)
        {
          memset(&play, 0, sizeof(play));
          play.code = effects[i].id;
          play.value = 1;
          if (write(fd, &play, sizeof(play)) < 0)
            {
              perror("Play effect failed");
              goto out;
            }

          printf("Now Playing: %s\n", g_effect_names[i]);
        }
      else if (i == -2)
        {
          /* Crash test */

          i = *((FAR int *)0);
          printf("Crash test: %d\n", i);
        }
      else if (i != -1)
        {
          printf("No such effect\n");
        }
    }
  while (i >= 0);

  /* Stop the effects */

  printf("Stopping effects\n");
  for (i = 0; i < N_EFFECTS; i++)
    {
      memset(&stop, 0, sizeof(stop));
      stop.code = effects[i].id;
      stop.value = 0;

      if (write(fd, &stop, sizeof(stop)) < 0)
        {
          perror("Error write fd failed");
          goto out;
        }
    }

out:
  close(fd);
  return errno > 0 ? -errno : 0;
}
