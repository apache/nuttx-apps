/****************************************************************************
 * apps/graphics/traveler/tools/misc/pll2txt.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included files
 ****************************************************************************/

#define FAR

#include "trv_types.h"
#include "trv_plane.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool b_use_hex = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int dump_plane(FILE *instream, FILE *outstream, int nrects)
{
  struct trv_rect_data_s rect;
  int i;

  for (i = 1; i <= nrects; i++)
    {
      if (fread((char*)&rect, RESIZEOF_TRVRECTDATA_T, 1, instream) != 1)
        {
          fprintf(stderr, "ERROR: Read failure\n");
          return -1;
        }

      if (b_use_hex)
        {
          fprintf(outstream, "%04x %04x %04x %04x %04x %02x %02x %02x\n",
                 rect.plane, rect.hstart, rect.hend, rect.vstart, rect.vend,
                 rect.attribute, rect.texture, rect.scale);
        }
      else
        {
          fprintf(outstream, "%5d %5d %5d %5d %5d %02x %2d %d\n",
                  rect.plane, rect.hstart, rect.hend, rect.vstart, rect.vend,
                  rect.attribute, rect.texture, rect.scale);
        }
    }

  return 0;
}

static void show_usage(const char *progname)
{
  fprintf(stderr,"Usage: %s [-h] [-o outfilename] [infile]\n",
          progname);
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  char *infile = "transfrm.pll";
  char *outfile = NULL;
  struct trv_planefile_header_s header;
  int option;
  FILE *instream;
  FILE *outstream = stdout;

  while ((option = getopt(argc, argv, "ho:")) != EOF)
    {
      switch (option)
        {
        case 'h' :
          b_use_hex = true;
          break;

        case 'o' :
          outfile = optarg;
          break;

        default:
          fprintf(stderr, "ERROR: Unrecognized option: %c\n", option);
          show_usage(argv[0]);
          break;
        }
    }

  /* We expect at least one argument after the options: The input
   * file name.
   */

  if (optind == argc - 1)
    {
      infile = argv[optind];
    }
  else if (optind < argc - 1)
    {
      fprintf(stderr, "ERROR: Unexpected garbage after [infile]\n");
      show_usage(argv[0]);
    }

  instream = fopen(infile, "rb");
  if (!instream)
    {
      fprintf(stderr, "ERROR: Unable to open %s\n", infile);
      return EXIT_FAILURE;
    }

  printf("Reading plane file: %s\n", infile);

  if (outfile)
    {
      outstream = fopen(outfile, "w");
      if (!outstream)
        {
          fprintf(stderr, "ERROR: Unable to open %s\n", outfile);
          return EXIT_FAILURE;
        }

      printf("Generating text file: %s\n", outfile);
    }

  if (fread((char*)&header, SIZEOF_TRVPLANEFILEHEADER_T, 1, instream) != 1)
    {
      fprintf(stderr, "Unable to Read %s header\n", infile);
      return EXIT_FAILURE;
    }

  fprintf(outstream, "nxrects=%d, nyrects=%d, nzrects=%d\n",
         header.nxrects, header.nyrects, header.nzrects );

  fprintf(outstream, "X Planes: \n");
  if (dump_plane(instream, outstream, header.nxrects))
    {
      goto stop_dump;
    }

  fprintf(outstream, "Y Planes: \n");
  if (dump_plane(instream, outstream, header.nyrects))
    {
      goto stop_dump;
    }

  fprintf(outstream, "Z Planes: \n");
  dump_plane(instream, outstream, header.nzrects);

stop_dump:
  fclose(instream);
  if (outfile)
    {
      fclose(outstream);
    }

  return EXIT_SUCCESS;
}
