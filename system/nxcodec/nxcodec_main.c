/****************************************************************************
 * apps/system/nxcodec/nxcodec_main.c
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <getopt.h>
#include <errno.h>

#include "nxcodec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NXCODEC_VER    "1.00"

#define NXCODEC_WIDTH  640
#define NXCODEC_HEIGHT 480

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_short_options[] = "d:s:hf:i:o:";

static const struct option g_long_options[] =
{
  { "device",  required_argument, NULL, 'd' },
  { "size",    required_argument, NULL, 's' },
  { "help",    no_argument,       NULL, 'h' },
  { "format",  required_argument, NULL, 'f' },
  { "infile",  required_argument, NULL, 'i' },
  { "outfile", required_argument, NULL, 'o' },
  { NULL,      0,                 NULL, 0   }
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void usage(FAR const char *progname)
{
  printf("NxCodec Version: "NXCODEC_VER"\n"
         "Usage: %s -d devname -s [wxh] -f [informt] "
         "-i infile -f [outformat] -o outfile\n"
         "Default settings for decoder parameters\n\n"
         "Options:\n"
         "-d | --device  Video device name\n"
         "-s | --size    Size of stream\n"
         "-h | --help    Print this message\n"
         "-f | --format  Format of stream\n"
         "-i | --infile  Input filename for M2M devices\n"
         "-o | --outfile Outputs stream to filename\n",
         progname);

  exit(EXIT_SUCCESS);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * nxcodec_main
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  nxcodec_t codec;
  int ret;
  char cc[5] =
    {
      0
    };

  /* Default settings for decoder parameters */

  codec.output.format.fmt.pix.width =
  codec.capture.format.fmt.pix.width = NXCODEC_WIDTH;

  codec.output.format.fmt.pix.height =
  codec.capture.format.fmt.pix.height = NXCODEC_HEIGHT;

  codec.output.format.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
  codec.capture.format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;

  while (1)
    {
      int idx;
      int c;

      c = getopt_long(argc, argv, g_short_options, g_long_options, &idx);
      if (-1 == c)
        {
          break;
        }

      switch (c)
        {
          case 0: /* getopt_long() flag */
            break;

          case 'd':
            memset(codec.devname, 0, sizeof(codec.devname));
            snprintf(codec.devname, sizeof(codec.devname), "%s", optarg);

            printf("nxcodec device name: %s\n", codec.devname);

            break;

          case 's' :
            sscanf(optarg, "%"SCNu32"x%"SCNu32"",
                   &codec.capture.format.fmt.pix.width,
                   &codec.capture.format.fmt.pix.height);

            codec.output.format.fmt.pix.width =
                                codec.capture.format.fmt.pix.width;
            codec.output.format.fmt.pix.height =
                                codec.capture.format.fmt.pix.height;
            printf("nxcodec size: %"PRIu32"x%"PRIu32"\n",
                   codec.output.format.fmt.pix.width,
                   codec.output.format.fmt.pix.height);

            break;

          case 'h':
            usage(argv[0]);

          case 'f':
            memset(cc, 0, 5);
            snprintf(cc, sizeof(cc), "%s", optarg);
            break;

          case 'i':
            memset(codec.output.filename, 0, sizeof(codec.output.filename));
            snprintf(codec.output.filename,
                     sizeof(codec.output.filename), "%s", optarg);

            if (cc[0])
              {
                codec.output.format.fmt.pix.pixelformat =
                                    v4l2_fourcc(cc[0], cc[1], cc[2], cc[3]);
              }

            printf("nxcodec input  file: %s, format %c%c%c%c - %"PRIu32"\n",
                   codec.output.filename,
                   cc[0], cc[1], cc[2], cc[3],
                   codec.output.format.fmt.pix.pixelformat);

            break;

          case 'o':
            memset(codec.capture.filename, 0,
                   sizeof(codec.capture.filename));
            snprintf(codec.capture.filename,
                     sizeof(codec.capture.filename), "%s", optarg);

            if (cc[0])
              {
                codec.capture.format.fmt.pix.pixelformat =
                                     v4l2_fourcc(cc[0], cc[1], cc[2], cc[3]);
              }

            printf("nxcodec output file: %s, format %c%c%c%c - %"PRIu32"\n",
                   codec.capture.filename,
                   cc[0], cc[1], cc[2], cc[3],
                   codec.capture.format.fmt.pix.pixelformat);

            break;

          default:
            usage(argv[0]);
            break;
        }
    }

  ret = nxcodec_init(&codec);
  if (ret < 0)
    {
      printf("nxcodec init failed: %d\n", ret);
      return ret;
    }

  printf("nxcodec init DONE.\n");

  ret = nxcodec_start(&codec);
  if (ret < 0)
    {
      printf("nxcodec start failed: %d\n", ret);
      goto end0;
    }

  printf("nxcodec started.\n");

  while (1)
    {
      struct pollfd pfd =
      {
        .events =  POLLIN | POLLOUT,
        .fd = codec.fd,
      };

      poll(&pfd, 1, -1);

      if (pfd.revents & POLLIN)
        {
          if (nxcodec_context_dequeue_frame(&codec.capture) < 0)
            {
              printf("nxcodec dequeue frame failed: %s\n", strerror(errno));
              break;
            }
        }

      if (pfd.revents & POLLOUT)
        {
          if (nxcodec_context_enqueue_frame(&codec.output) < 0)
            {
              printf("nxcodec enqueue frame failed: %s\n", strerror(errno));
              break;
            }
        }
    }

  nxcodec_stop(&codec);
  printf("nxcodec stop DONE.\n");

end0:
  nxcodec_uninit(&codec);
  return ret;
}
