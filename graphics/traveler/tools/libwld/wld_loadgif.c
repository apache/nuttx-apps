/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_loadgif.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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

/*************************************************************************
 * Included files
 *************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trv_types.h"
#include "wld_mem.h"
#include "wld_bitmaps.h"
#include "wld_graphicfile.h"
#include "wld_utils.h"

/*************************************************************************
 * Pre-processor Definitions
 *************************************************************************/

#define NEXTBYTE        (*ptr++)
#define IMAGESEP        0x2c
#define GRAPHIC_EXT     0xf9
#define PLAINTEXT_EXT   0x01
#define APPLICATION_EXT 0xff
#define COMMENT_EXT     0xfe
#define START_EXTENSION 0x21
#define INTERLACEMASK   0x40
#define COLORMAPMASK    0x80

/*************************************************************************
 * Private Data
 *************************************************************************/

static int BitOffset = 0;              /* Bit Offset of next code */
static int XC = 0, YC = 0;             /* Output X and Y coords of current pixel */
static int Pass = 0;                   /* Used by output routine if interlaced pic */
static int OutCount = 0;               /* Decompressor output 'stack count' */
static int RWidth, RHeight;            /* screen dimensions */
static int Width, Height;              /* image dimensions */
static int LeftOfs, TopOfs;            /* image offset */
static int BitsPerPixel;               /* Bits per pixel, read from GIF header */
static int BytesPerScanline;           /* bytes per scanline in output rwld_er */
static int ColorMapSize;               /* number of colors */
static int Background;                 /* background color */
static int CodeSize;                   /* Code size, read from GIF header */
static int InitCodeSize;               /* Starting code size, used during Clear */
static int Code;                       /* Value returned by ReadCode */
static int MaxCode;                    /* limiting value for current code size */
static int ClearCode;                  /* GIF clear code */
static int EOFCode;                    /* GIF end-of-information code */
static int CurCode, OldCode, InCode;   /* Decompressor variables */
static int FirstFree;                  /* First free code, generated per GIF spec */
static int FreeCode;                   /* Decompressor, next free slot in hash table */
static int FinChar;                    /* Decompressor variable */
static int BitMask;                    /* AND mask for data size */
static int ReadMask;                   /* Code AND mask for current code size */

static bool Interlace, HasColormap;
static bool Verbose = true;

static uint8_t *Image;                 /* The result array */
static uint8_t *RawGIF;                /* The heap array to hold it, raw */
static uint8_t *Rwld_er;               /* The rwld_er data stream, unblocked */

/* The hash table used by the decompressor */

static int Prefix[4096];
static int Suffix[4096];

/* An output array used by the decompressor */

static int OutCode[1025];

/* The color map, read from the GIF header */

static uint8_t Red[256], Green[256], Blue[256], used[256];
static int numused;

static char *id87 = "GIF87a";
static char *id89 = "GIF89a";

/*************************************************************************
 * Private Functions
 ************************************************************************/

/*************************************************************************
 * Name: ReadCode
 * Description:
 * Fetch the next code from the data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Rwld_er array as a BIT Offset.  We compute
 * the byte Offset into the rwld_er array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it.
 ************************************************************************/

static int ReadCode(void)
{
  int RawCode;
  int ByteOffset;

  ByteOffset = BitOffset / 8;
  RawCode = Rwld_er[ByteOffset] + (0x100 * Rwld_er[ByteOffset + 1]);

  if (CodeSize >= 8)
    RawCode += (0x10000 * Rwld_er[ByteOffset + 2]);

  RawCode >>= (BitOffset % 8);
  BitOffset += CodeSize;

  return (RawCode & ReadMask);
}

static void AddToPixel(uint8_t Index)
{
  if (YC < Height)
    *(Image + YC * BytesPerScanline + XC) = Index;

  if (!used[Index])
    {
      used[Index] = 1;
      numused++;
    }

  /* Update the X-coordinate, and if it overflows, update the Y-coordinate */

  if (++XC == Width)
    {

      /* If a non-interlaced picture, just increment YC to the next scan line.
       * If it's interlaced, deal with the interlace as described in the GIF
       * spec.  Put the decoded scan line out to the screen if we haven't gone
       * pwld_ the bottom of it */

      XC = 0;
      if (!Interlace)
        {
          YC++;
        }
      else
        {
          switch (Pass)
            {
            case 0:
              YC += 8;
              if (YC >= Height)
                {
                  Pass++;
                  YC = 4;
                }
              break;
            case 1:
              YC += 8;
              if (YC >= Height)
                {
                  Pass++;
                  YC = 2;
                }
              break;
            case 2:
              YC += 4;
              if (YC >= Height)
                {
                  Pass++;
                  YC = 1;
                }
              break;
            case 3:
              YC += 2;
              break;
            default:
              break;
            }
        }
    }
}

/*************************************************************************
 * Public Functions
 ************************************************************************/

/*************************************************************************
 * Name: wld_LoadGIF
 * Description:
 ************************************************************************/

graphic_file_t *wld_LoadGIF(FILE * fp, char *fname)
{
  graphic_file_t *gfile;
  int filesize;
  register unsigned char ch;
  register unsigned char ch1;
  register uint8_t *ptr;
  register uint8_t *ptr1;
  register int i;
  short transparency = -1;

  BitOffset = 0;
  XC = YC = 0;
  Pass = 0;
  OutCount = 0;

  /* Find the size of the file */

  fseek(fp, 0L, 2);
  filesize = ftell(fp);
  fseek(fp, 0L, 0);

  if (!(ptr = RawGIF = (uint8_t *) malloc(filesize)))
    {
      wld_fatal_error("not enough memory to read gif file");
    }

  if (!(Rwld_er = (uint8_t *) malloc(filesize)))
    {
      free(ptr);
      wld_fatal_error("not enough memory to read gif file");
    }

  if (fread(ptr, filesize, 1, fp) != 1)
    wld_fatal_error("GIF data read failed");

  if (strncmp((char *)ptr, id87, 6))
    {
      if (strncmp((char *)ptr, id89, 6))
        {
          wld_fatal_error("not a GIF file");
        }
    }

  ptr += 6;

  /* Get variables from the GIF screen descriptor */

  ch = NEXTBYTE;
  RWidth = ch + 0x100 * NEXTBYTE;       /* screen dimensions... not used. */
  ch = NEXTBYTE;
  RHeight = ch + 0x100 * NEXTBYTE;

  if (Verbose)
    {
      fprintf(stderr, "screen dims: %dx%d.\n", RWidth, RHeight);
    }

  ch = NEXTBYTE;
  HasColormap = ((ch & COLORMAPMASK) ? true : false);

  BitsPerPixel = (ch & 7) + 1;
  ColorMapSize = 1 << BitsPerPixel;
  BitMask = ColorMapSize - 1;

  Background = NEXTBYTE;        /* background color... not used. */

  if (NEXTBYTE)                 /* supposed to be NULL */
    {
      wld_fatal_error("corrupt GIF file (bad screen descriptor)");
    }

  /* Read in global colormap. */

  if (HasColormap)
    {
      if (Verbose)
        {
          fprintf(stderr, "%s is %dx%d, %d bits per pixel, (%d colors).\n",
                  fname, RWidth, RHeight, BitsPerPixel, ColorMapSize);
        }

      for (i = 0; i < ColorMapSize; i++)
        {
          Red[i] = NEXTBYTE;
          Green[i] = NEXTBYTE;
          Blue[i] = NEXTBYTE;
          used[i] = 0;
        }

      numused = 0;
    }

  /* look for image separator */

  for (ch = NEXTBYTE; ch != IMAGESEP; ch = NEXTBYTE)
    {
      i = ch;
      fprintf(stderr, "EXTENSION CHARACTER: %x\n", i);
      if (ch != START_EXTENSION)
        wld_fatal_error("corrupt GIF89a file");

      /* Handle image extensions */

      switch (ch = NEXTBYTE)
        {
        case GRAPHIC_EXT:
          ch = NEXTBYTE;
          if (ptr[0] & 0x1)
            {
              transparency = ptr[3];    /* transparent color index */
              fprintf(stderr, "transparency index: %i\n", transparency);
            }

          ptr += ch;
          break;
        case PLAINTEXT_EXT:
          break;
        case APPLICATION_EXT:
          break;
        case COMMENT_EXT:
          break;
        default:
          wld_fatal_error("invalid GIF89 extension");
        }

      while ((ch = NEXTBYTE))
        {
          ptr += ch;
        }
    }

  /* Now read in values from the image descriptor */

  ch = NEXTBYTE;
  LeftOfs = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  TopOfs = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  Width = ch + 0x100 * NEXTBYTE;
  ch = NEXTBYTE;
  Height = ch + 0x100 * NEXTBYTE;
  Interlace = ((NEXTBYTE & INTERLACEMASK) ? true : false);

  if (Verbose)
    fprintf(stderr, "Reading a %d by %d %sinterlaced image...",
            Width, Height, (Interlace) ? "" : "non-");

  gfile = wld_new_graphicfile();
  gfile->palette = (color_rgb_t *) wld_malloc(sizeof(color_rgb_t) * ColorMapSize);
  for (i = 0; i < ColorMapSize; i++)
    {
      gfile->palette[i].red = Red[i];
      gfile->palette[i].green = Green[i];
      gfile->palette[i].blue = Blue[i];
    }

  gfile->bitmap = (uint8_t *) wld_malloc(Width * Height);
  gfile->type = GFILE_PALETTED;
  gfile->width = Width;
  gfile->height = Height;
  gfile->transparent_entry = transparency;

  /* Note that I ignore the possible existence of a local color map. I'm told
   * there aren't many files around that use them, and the spec says it's
   * defined for future use.  This could lead to an error reading some files.
   */

  /* Start reading the rwld_er data. First we get the intial code size and
   * compute decompressor constant values, based on this code size.
   */

  CodeSize = NEXTBYTE;
  ClearCode = (1 << CodeSize);
  EOFCode = ClearCode + 1;
  FreeCode = FirstFree = ClearCode + 2;

  /* The GIF spec has it that the code size is the code size used to compute
   * the above values is the code size given in the file, but the code size
   * used in compression/decompression is the code size given in the file plus
   * one. (thus the ++).
   */

  CodeSize++;
  InitCodeSize = CodeSize;
  MaxCode = (1 << CodeSize);
  ReadMask = MaxCode - 1;

  /* Read the rwld_er data.  Here we just transpose it from the GIF array to
   * the Rwld_er array, turning it from a series of blocks into one long data
   * stream, which makes life much easier for ReadCode().
   */

  ptr1 = Rwld_er;
  do
    {
      ch = ch1 = NEXTBYTE;
      while (ch--)
        *ptr1++ = NEXTBYTE;
      if ((ptr1 - Rwld_er) > filesize)
        wld_fatal_error("corrupt GIF file (unblock)");
    }
  while (ch1);

  free(RawGIF);                 /* We're done with the raw data now... */

  if (Verbose)
    {
      fprintf(stderr, "done.\n");
      fprintf(stderr, "Decompressing...");
    }

  Image = gfile->bitmap;
  BytesPerScanline = Width;

  /* Decompress the file, continuing until you see the GIF EOF code. One
   * obvious enhancement is to add checking for corrupt files here.
   */

  Code = ReadCode();
  while (Code != EOFCode)
    {

      /* Clear code sets everything back to its initial value, then reads the
       * immediately subsequent code as uncompressed data.
       */

      if (Code == ClearCode)
        {
          CodeSize = InitCodeSize;
          MaxCode = (1 << CodeSize);
          ReadMask = MaxCode - 1;
          FreeCode = FirstFree;
          CurCode = OldCode = Code = ReadCode();
          FinChar = CurCode & BitMask;
          AddToPixel(FinChar);
        }
      else
        {
          /* If not a clear code, then must be data: save same as CurCode and
           * InCode
           */

          CurCode = InCode = Code;

          /* If greater or equal to FreeCode, not in the hash table yet; repeat
           * the lwld_ character decoded
           */

          if (CurCode >= FreeCode)
            {
              CurCode = OldCode;
              OutCode[OutCount++] = FinChar;
            }

          /* Unless this code is raw data, pursue the chain pointed to by
           * CurCode through the hash table to its end; each code in the chain
           * puts its associated output code on the output queue.
           */

          while (CurCode > BitMask)
            {
              if (OutCount > 1024)
                {
                  fprintf(stderr, "\nCorrupt GIF file (OutCount)!\n");
                  exit(-1);
                }
              OutCode[OutCount++] = Suffix[CurCode];
              CurCode = Prefix[CurCode];
            }

          /* The lwld_ code in the chain is treated as raw data. */

          FinChar = CurCode & BitMask;
          OutCode[OutCount++] = FinChar;

          /* Now we put the data out to the Output routine. It's been stacked
           * LIFO, so deal with it that way...
           */

          for (i = OutCount - 1; i >= 0; i--)
            AddToPixel(OutCode[i]);
          OutCount = 0;

          /* Build the hash table on-the-fly. No table is stored in the file */

          Prefix[FreeCode] = OldCode;
          Suffix[FreeCode] = FinChar;
          OldCode = InCode;

          /* Point to the next slot in the table.  If we exceed the current
           * MaxCode value, increment the code size unless it's already 12. If
           * it is, do nothing: the next code decompressed better be CLEAR
           */

          FreeCode++;
          if (FreeCode >= MaxCode)
            {
              if (CodeSize < 12)
                {
                  CodeSize++;
                  MaxCode *= 2;
                  ReadMask = (1 << CodeSize) - 1;
                }
            }
        }

      Code = ReadCode();
    }

  free(Rwld_er);

  if (Verbose)
    {
      fprintf(stderr, "done.\n");
    }
  else
    {
      fprintf(stderr, "(of which %d are used)\n", numused);
    }

  return gfile;
}
