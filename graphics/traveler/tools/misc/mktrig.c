
/****************************************************************************
 * apps/graphics/traveler/tools/misc/mktrig.c
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/****************************************************************************
 * Included files
 ****************************************************************************/

#define PI 3.1415926

/****************************************************************************
 * Included files
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  FILE *outfile;
  double angle;
  double valuef;
  uint32_t value32;
  uint16_t value16;
  int i;
  int j;

  outfile = fopen("trigtbl.tmp", "w");
  if (!outfile)
    {
      fprintf(stderr, "Unable to open trigtbl.tmp\n");
      exit(1);
    }

  fprintf(outfile, "#define TWOPI            1920\n");
  fprintf(outfile, "#define PI                960\n");
  fprintf(outfile, "#define HALFPI            480\n");
  fprintf(outfile, "#define QTRPI             240\n\n");

  fprintf(outfile, "const int32_t g_tan_table[PI + HALFPI + 1] =\n");
  fprintf(outfile, "{\n");

  for (i = 0; i < 1440;)
    {
      fprintf(outfile, "  ");
      for (j = 0; ((j < 5) && (i < 1440));)
        {
          angle = ((double)i) * (2.0 * PI / 1920.0);
          valuef = (4096.0 * tan(angle));
          if (valuef >= 0.0)
            {
              valuef += 0.5;
            }
          else
            {
              valuef -= 0.5;
            }

          value32 = (uint32_t)((long)(valuef));
          fprintf(outfile, "0x%08lx", (unsigned long)value32);

          i++;
          j++;
          if ((j < 5) && (i < 1440))
            {
              fprintf(outfile, ", ");
            }
        }

      fprintf(outfile, ",\n");
    }

  fprintf(outfile, "  0x00000000\n");
  fprintf(outfile, "};\n\n");

  fprintf(outfile, "const int16_t g_sin_table[TWOPI + HALFPI + 1] =\n");
  fprintf(outfile, "{\n");

  for (i = 0; i < 2400;)
    {
      fprintf(outfile, "  ");
      for (j = 0; ((j < 8) && (i < 2400));)
        {
          angle = ((double)i) * (2.0 * PI / 1920.0);
          valuef = (4096.0 * sin(angle));
          if (valuef >= 0.0)
            {
              valuef += 0.5;
            }
          else
            {
              valuef -= 0.5;
            }

          value16 = (uint16_t)((long)(valuef));
          fprintf(outfile, "0x%04x", (unsigned int)value16);

          i++;
          j++;
          if ((j < 8) && (i < 2400))
            {
              fprintf(outfile, ", ");
            }
        }

      fprintf(outfile, ",\n");
    }

  fprintf(outfile, "  0x1000\n");
  fprintf(outfile, "};\n\n");

  fprintf(outfile, "const int32_t cscTable[TWOPI + HALFPI + 1] =\n");
  fprintf(outfile, "{\n");

  for (i = 0; i < 2400;)
    {
      fprintf(outfile, "  ");
      for (j = 0; ((j < 5) && (i < 2400));)
        {
          angle = ((double)i) * (2.0 * PI / 1920.0);
          valuef = (4096.0 / sin(angle));
          if (valuef >= 0.0)
            {
              valuef += 0.5;
            }
          else
            {
              valuef -= 0.5;
            }

          value32 = (uint32_t)((long)(valuef));
          fprintf(outfile, "0x%08lx", (unsigned long)value32);

          i++;
          j++;
          if ((j < 5) && (i < 2400))
            {
              fprintf(outfile, ", ");
            }
        }

      fprintf(outfile, ",\n");
    }

  fprintf(outfile, "  0x00001000\n");
  fprintf(outfile, "};\n\n");

  fclose(outfile);
  return 0;
}
