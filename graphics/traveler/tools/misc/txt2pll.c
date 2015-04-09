/*******************************************************************************
 * apps/graphics/traveler/tools/misc/txt2pll.c
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
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LINE_SIZE 2048

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_line[LINE_SIZE];
static struct trv_rect_data_s g_rectdata;
static const char *g_progname;
static bool g_use_hex = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(void)
{
  fprintf(stderr,"Usage: %s [-h] [-o <outfile>] <infile>\n",
          g_progname);
  exit(EXIT_FAILURE);
}

static void parse_rect_value(int *value, char *string)
{
  long lvalue;

  if (g_use_hex)
    {
      lvalue = strtol(string, NULL, 16);
    }
  else
    {
      lvalue = strtol(string, NULL, 10);
    }

  *value = (int)lvalue;
}

static void parse_rect_data(void)
{
  char *token;
  int value;

  token = strtok(g_line, " ");
  parse_rect_value(&value, token);
  g_rectdata.plane = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.hstart = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.hend = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.vstart = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.vend = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.attribute = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.texture = value;

  token = strtok(NULL, " ");
  parse_rect_value(&value, token);
  g_rectdata.scale = value;
}

static void write_rect_data(FILE *outstream)
{
  if (fwrite((char*)&g_rectdata, RESIZEOF_TRVRECTDATA_T, 1, outstream) != 1)
    {
      fprintf(stderr, "ERROR: Failed to write rectangle data\n");
      exit(EXIT_FAILURE);
    }
}

static void parse_nrects(uint16_t *nrects, char *string)
{
  char *nbr;

  if (!string)
    {
      fprintf(stderr, "ERROR: Bad input file format\n");
      show_usage();
    }
  else
    {
      nbr = strchr(string, '=');
      if (!nbr)
        {
          fprintf(stderr, "ERROR: Bad input file format\n");
          show_usage();
        }

      *nrects = atoi(nbr + 1);
    }
}

static void read_next_line(FILE *instream)
{
  if (fgets(g_line, LINE_SIZE, instream) == NULL)
    {
      fprintf(stderr, "ERROR: Failed to read line from input file\n");
      show_usage();
    }

  printf("line: \"%s\"\n", g_line);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv, char** envp)
{
  char *infile = NULL;
  char *outfile = "transform.pll";
  struct trv_planefile_header_s header;
  char *token;
  FILE *instream = stdin;
  FILE *outstream;
  int option;
  int i;

  g_progname = argv[0];
  while ((option = getopt(argc, argv, "ho:")) != EOF)
    {
      switch (option)
        {
        case 'h' :
          g_use_hex = true;
          break;

        case 'o' :
          outfile = optarg;
          break;

        default:
          fprintf(stderr, "ERRORL Unrecognized option: %c\n", option);
          show_usage();
          break;
        }
    }

  /* We expect at least one argument after the options: The input
   * file name.
   */

  if (optind < argc - 1)
    {
      fprintf(stderr, "ERROR: Missing <infile>\n");
      show_usage();
    }
  else if (optind == argc - 1)
    {
      infile = argv[optind];
    }
  else
    {
      fprintf(stderr, "ERROR: Unexpected garbage after <infile>\n");
      show_usage();
    }

  /* Open files */

  instream = fopen(infile, "r");
  if (!instream)
    {
      fprintf(stderr, "ERROR: Unable to open %s\n", infile);
      return EXIT_FAILURE;
    }

  printf("Reading text file: %s\n", infile);

  outstream = fopen(outfile, "wb");
  if (!outstream)
    {
      fprintf(stderr, "ERROR: Unable to open %s\n", outfile);
      return EXIT_FAILURE;
    }

  printf("Writing plane file: %s\n", outfile);

  /* Read header information */

  read_next_line(instream);
  token = strtok(g_line, ", ");
  parse_nrects(&header.nxrects, token);

  token = strtok(NULL, ", ");
  parse_nrects(&header.nyrects, token);

  token = strtok(NULL, ", ");
  parse_nrects(&header.nzrects, token);

  /* Write header information */

 if (fwrite((char*)&header, SIZEOF_TRVPLANEFILEHEADER_T, 1, outstream) != 1)
   {
      fprintf(stderr, "Failed to write file header\n");
      return EXIT_FAILURE;
   }

  /* Read X Planes */

  read_next_line(instream); /* Skip over "X Planes:" line */
  printf("Reading %d X planes\n", header.nxrects);
  for (i = 0; i < header.nxrects; i++)
    {
      read_next_line(instream);
      parse_rect_data();
      write_rect_data(outstream);
    }

  /* Read Y Planes */

  read_next_line(instream); /* Skip over "Y Planes:" line */
  printf("Reading %d Y planes\n", header.nyrects);
  for (i = 0; i < header.nyrects; i++)
    {
      read_next_line(instream);
      parse_rect_data();
      write_rect_data(outstream);
    }

  /* Read Z Planes */

  read_next_line(instream); /* Skip over "Z Planes:" line */
  printf("Reading %d Z planes\n", header.nzrects);
  for (i = 0; i < header.nzrects; i++)
    {
      read_next_line(instream);
      parse_rect_data();
      write_rect_data(outstream);
    }

  fclose(instream);
  fclose(outstream);
  return EXIT_SUCCESS;
}
