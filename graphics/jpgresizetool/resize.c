/****************************************************************************
 * apps/graphics/jpgresizetool/resize.c
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

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* Every N line, report resizing progress */
#define JPEGRESIZE_PROGRESS_INTERVAL  40

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Usage: jpgresize input.jpg output.jpg scale_denom quality%
 * (scale_denom = 1, 2, 4, 8)
 * thumbnail example: jpgresize /sd/PROC/100.jpg /sd/THUMBS/100.jpg 8 20
 */

int main(int argc, char *argv[])
{
  /* Take arguments */

  if (argc != 5)
    {
      fprintf(stderr,
              "Usage: %s input.jpg output.jpg scale_denom quality\n",
              argv[0]);
      return -1;
    }

  const char *input_filename  = argv[1];
  const char *output_filename = argv[2];
  int scale_denom = atoi(argv[3]);
  if (scale_denom != 1 && scale_denom != 2 && scale_denom != 4 &&
      scale_denom != 8 && scale_denom != 16)
    {
      fprintf(stderr, "scale_denom must be 1, 2, 4, 8, or 16\n");
      return -1;
    }

  int quality = atoi(argv[4]);

  /* Opening input */

  FILE *infile = fopen(input_filename, "rb");
  if (!infile)
    {
      perror("fopen input");
      return -1;
    }

  /* Configuring decompressor */

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);

  cinfo.scale_num = 1;
  cinfo.scale_denom = scale_denom;
  jpeg_start_decompress(&cinfo);

  int width = cinfo.output_width;
  int height = cinfo.output_height;
  int pixel_size = cinfo.output_components;

  /* Opening output */

  FILE *outfile = fopen(output_filename, "wb");
  if (!outfile)
    {
      perror("fopen output");
      jpeg_destroy_decompress(&cinfo);
      fclose(infile);
      return -1;
    }

  /* Configuring compressor */

  struct jpeg_compress_struct cinfo_out;
  struct jpeg_error_mgr jerr_out;
  cinfo_out.err = jpeg_std_error(&jerr_out);
  jpeg_create_compress(&cinfo_out);
  jpeg_stdio_dest(&cinfo_out, outfile);

  cinfo_out.image_width = width;
  cinfo_out.image_height = height;
  cinfo_out.input_components = pixel_size;
  cinfo_out.in_color_space = cinfo.out_color_space;
  jpeg_set_defaults(&cinfo_out);
  jpeg_set_quality(&cinfo_out, quality, TRUE);
  jpeg_start_compress(&cinfo_out, TRUE);

  /* Allocating memory by using JPEG their own allocator. Do not use malloc */

  printf("Allocating... \r");
  JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)
      ((j_common_ptr)&cinfo, JPOOL_IMAGE, width * pixel_size, 1);

  /* Start the job */

  printf("Resizing...   \n\r");
  uint8_t togglechar = 0;
  while (cinfo.output_scanline < cinfo.output_height)
    {
      /* Pretty loading bar... */

      if (cinfo.output_scanline % JPEGRESIZE_PROGRESS_INTERVAL == 1)
        {
          printf("\r[%u%%]",
                 (unsigned)(((float)cinfo.output_scanline
                 / cinfo.output_height)
                 * 100));
          togglechar = !togglechar;
        }

      if (togglechar)
        {
          printf("-");
        }
      else
        {
          printf("#");
        }

      fflush(stdout);

      /* Actual work */

      jpeg_read_scanlines(&cinfo, buffer, 1);
      jpeg_write_scanlines(&cinfo_out, buffer, 1);
    }

  /* Erase progress bar */

  printf("\r      ");
  for (size_t i = 0; i < JPEGRESIZE_PROGRESS_INTERVAL; i++)
    {
      printf(" ");
    }

  /* All done */

  printf("\rDone\n\r");

  jpeg_finish_compress(&cinfo_out);
  jpeg_destroy_compress(&cinfo_out);
  fclose(outfile);

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

  printf("Resized JPEG written to %s\n", output_filename);
  return 0;
}
