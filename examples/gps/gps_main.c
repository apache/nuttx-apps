/****************************************************************************
 * apps/examples/gps/gps_main.c
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
#include <fcntl.h>
#include <wchar.h>
#include <syslog.h>
#include <unistd.h>

#include <minmea/minmea.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MINMEA_MAX_LENGTH    256

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * gps_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int cnt;
  char ch;
  char line[MINMEA_MAX_LENGTH];
  char *port = "/dev/ttyS1";

  /* Get the GPS serial port argument. If none specified, default to ttyS1 */

  if (argc > 1)
    {
      port = argv[1];
    }

  /* Open the GPS serial port */

  fd = open(port, O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "Unable to open file %s\n", port);
      return 1;
    }

  /* Run forever */

  for (; ; )
    {
      /* Read until we complete a line */

      cnt = 0;
      do
        {
          read(fd, &ch, 1);
          if (ch != '\r' && ch != '\n')
            {
              line[cnt++] = ch;
            }
        }
      while (ch != '\r' && ch != '\n');

      line[cnt] = '\0';

      switch (minmea_sentence_id(line, false))
        {
          case MINMEA_SENTENCE_RMC:
            {
              struct minmea_sentence_rmc frame;

              if (minmea_parse_rmc(&frame, line))
                {
                  printf("Fixed-point Latitude...........: %" PRIdLEAST32
                         "\n",
                         minmea_rescale(&frame.latitude, 1000));
                  printf("Fixed-point Longitude..........: %" PRIdLEAST32
                         "\n",
                         minmea_rescale(&frame.longitude, 1000));
                  printf("Fixed-point Speed..............: %" PRIdLEAST32
                         "\n",
                         minmea_rescale(&frame.speed, 1000));
                  printf("Floating point degree latitude.: %2.6f\n",
                         minmea_tocoord(&frame.latitude));
                  printf("Floating point degree longitude: %2.6f\n",
                         minmea_tocoord(&frame.longitude));
                  printf("Floating point speed...........: %2.6f\n",
                         minmea_tocoord(&frame.speed));
                }
              else
                {
                    printf("$xxRMC sentence is not parsed\n");
                }
            }
            break;

          case MINMEA_SENTENCE_GGA:
            {
              struct minmea_sentence_gga frame;

              if (minmea_parse_gga(&frame, line))
                {
                  printf("Fix quality....................: %d\n",
                         frame.fix_quality);
                  printf("Altitude.......................: %" PRIdLEAST32
                         "\n",
                         frame.altitude.value);
                  printf("Tracked satellites.............: %d\n",
                         frame.satellites_tracked);
                }
              else
                {
                  printf("$xxGGA sentence is not parsed\n");
                }
            }
            break;

          default:
            {
            }
            break;
        }
    }

  return 0;
}
