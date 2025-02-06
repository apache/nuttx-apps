/****************************************************************************
 * apps/sensors/accel_calib/accel_calib_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <uORB/uORB.h>

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/

/* Accelerometer sampling frequency in Hertz. */

#ifndef CONFIG_ACCEL_CALIB_SAMPLING_FREQ
#define CONFIG_ACCEL_CALIB_SAMPLING_FREQ 50
#endif

/* Delimiter for output data. Default is tab */

#ifndef CONFIG_ACCEL_CALIB_DELIM
#define CONFIG_ACCEL_CALIB_DELIM "\t"
#endif

/* Shorthand for delimiter */

#define DLM CONFIG_ACCEL_CALIB_DELIM

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * hello_main
 ****************************************************************************/

int main(int argc, char **argv)
{
  int err;
  int imu;
  unsigned frequency;
  struct sensor_accel data;
  char *name;
  bool update = false;

  if (argc < 2)
    {
      fprintf(stderr, "Please provide the name of the accelerometer uORB "
                      "topic as an argument\n");
      fprintf(stderr, "Usage: accel_calib [topic]\n");
      fprintf(stderr, "Example: accel_calib sensor_accel0\n");
      return EXIT_FAILURE;
    }

  name = argv[1];

  /* Get accelerometer metadata */

  const struct orb_metadata *imu_meta = orb_get_meta(name);
  if (imu_meta == NULL)
    {
      fprintf(stderr, "Failed to get metadata for %s\n", name);
      return EXIT_FAILURE;
    }

  /* Subscribe to accelerometer */

  imu = orb_subscribe(imu_meta);
  if (imu < 0)
    {
      fprintf(stderr, "Could not subsribe to %s: %d\n", name, errno);
      return EXIT_FAILURE;
    }

  /* Set sampling rate to 50Hz */

  err = orb_set_frequency(imu, CONFIG_ACCEL_CALIB_SAMPLING_FREQ);
  if (err)
    {
      fprintf(stderr, "Wasn't able to set frequency to %uHz: %d\n",
              CONFIG_ACCEL_CALIB_SAMPLING_FREQ, err);
      return EXIT_FAILURE;
    }

  /* Show sampling rate */

  err = orb_get_frequency(imu, &frequency);
  if (err)
    {
      fprintf(stderr, "Could not verify frequency: %d\n", err);
      return EXIT_FAILURE;
    }

  printf("Sampling frequency is %uHz\n", frequency);

  /* Loop forever while printing out tab separated data as X, Y, Z */

  while (true)
    {
      err = orb_check(imu, &update);
      if (err)
        {
          fprintf(stderr, "Failed to check for new data: %d\n", err);
          continue;
        }

      /* Grab latest data */

      if (update)
        {
          err = orb_copy(imu_meta, imu, &data);
          if (err)
            {
              fprintf(stderr, "Failed to get new data: %d\n", err);
              continue;
            }

          printf("%f" DLM "%f" DLM "%f\n", data.x, data.y, data.z);
        }
    }

  orb_unsubscribe(imu);
  return EXIT_SUCCESS;
}
