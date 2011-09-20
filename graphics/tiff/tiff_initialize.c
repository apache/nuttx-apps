/****************************************************************************
 * apps/graphics/tiff/tiff_initialize.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <apps/tiff.h>

#include "tiff_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* Bi-level Images
 *
 *          Offset Description                 Contents/Notes
 * Header:    0    Byte Order                  "II" or "MM"
 *            2    Magic Number                42     
 *            4    1st IFD offset              10
 *            8    [2 bytes padding]
 * IFD:      10    Number of Directory Entries 12
 *           12    NewSubfileType
 *           24    ImageWidth                  Number of columns is a user parmeter
 *           36    ImageLength                 Number of rows is a user parameter
 *           48    Compression                 Hard-coded no compression (for now)
 *           60    PhotometricInterpretation   Value is a user parameter
 *           72    StripOffsets                Offset and count determined at run time
 *           84    RowsPerStrip                Value is a user parameter
 *           96    StripByteCounts             Offset and count determined at run time
 *          108    XResolution                 Value is a user parameter
 *          120    YResolution                 Value is a user parameter
 *          132    Resolution Unit             Hard-coded to 1
 *          144    Software
 *          156    DateTime
 *          168    Next IFD offset             0
 *          170    [2 bytes padding]
 * Values:
 *          172    XResolution                 nnnn 1, nnnn is user supplied
 *          180    YResolution                 nnnn 1, nnnn is user supplied
 *          188    "NuttX"                     Length = 6 (includeing NUL terminator)
 *          194    "YYYY:MM:DD HH:MM:SS"       Length = 20 (ncluding NUL terminator)
 *          214    [2 bytes padding]
 *          216    StripOffsets                Beginning of strip offsets
 *          xxx    StripByteCounts             Beginning of strip byte counts
 *          xxx    [Probably padding]
 *          xxx    Data for strips             Beginning of strip data
 */

#define TIFF_IFD_OFFSET           (SIZEOF_TIFF_HEADER+2)

#define TIFF_BILEV_NIFDENTRIES    12
#define TIFF_BILEV_STRIPIFDOFFS   72
#define TIFF_BILEV_STRIPBCIFDOFFS 96
#define TIFF_BILEV_VALOFFSET      172
#define TIFF_BILEV_XRESOFFSET     172
#define TIFF_BILEV_YRESOFFSET     180
#define TIFF_BILEV_SWOFFSET       188
#define TIFF_BILEV_DATEOFFSET     194
#define TIFF_BILEV_STRIPOFFSET    216

/* Greyscale Images have one additional IFD entry: BitsPerSample (4 or 8)
 *
 * Header:    0    Byte Order                  "II" or "MM"
 *            2    Magic Number                42     
 *            4    1st IFD offset              10
 *            8    [2 bytes padding]
 * IFD:      10    Number of Directory Entries 13
 *           12    NewSubfileType
 *           24    ImageWidth                  Number of columns is a user parmeter
 *           36    ImageLength                 Number of rows is a user parameter
 *           48    BitsPerSample
 *           60    Compression                 Hard-coded no compression (for now)
 *           72    PhotometricInterpretation   Value is a user parameter
 *           84    StripOffsets                Offset and count determined at run time
 *           96    RowsPerStrip                Value is a user parameter
 *          108    StripByteCounts             Offset and count determined at run time
 *          120    XResolution                 Value is a user parameter
 *          132    YResolution                 Value is a user parameter
 *          144    Resolution Unit             Hard-coded to 1
 *          156    Software
 *          168    DateTime
 *          180    Next IFD offset             0
 *          182    [2 bytes padding]
 * Values:
 *          184    XResolution                 nnnn 1, nnnn is user supplied
 *          192    YResolution                 nnnn 1, nnnn is user supplied
 *          200    "NuttX"                     Length = 6 (includeing NUL terminator)
 *          206    "YYYY:MM:DD HH:MM:SS"       Length = 20 (ncluding NUL terminator)
 *          226    [2 bytes padding]
 *          228    StripOffsets                Beginning of strip offsets
 *          xxx    StripByteCounts             Beginning of strip byte counts
 *          xxx    [Probably padding]
 *          xxx    Data for strips             Beginning of strip data
 */

#define TIFF_GREY_NIFDENTRIES    13
#define TIFF_GREY_STRIPIFDOFFS   84
#define TIFF_GREY_STRIPBCIFDOFFS 108
#define TIFF_GREY_VALOFFSET      184
#define TIFF_GREY_XRESOFFSET     184
#define TIFF_GREY_YRESOFFSET     192
#define TIFF_GREY_SWOFFSET       200
#define TIFF_GREY_DATEOFFSET     206
#define TIFF_GREY_STRIPOFFSET    228

/* RGB Images have two additional IFD entries: BitsPerSample (8,8,8) and
 * SamplesPerPixel (3):
 *
 * Header:    0    Byte Order                  "II" or "MM"
 *            2    Magic Number                42     
 *            4    1st IFD offset              10
 *            8    [2 bytes padding]
 * IFD:      10    Number of Directory Entries 14
 *           12    NewSubfileType
 *           24    ImageWidth                  Number of columns is a user parmeter
 *           36    ImageLength                 Number of rows is a user parameter
 *           48    BitsPerSample               8, 8, 8
 *           60    Compression                 Hard-coded no compression (for now)
 *           72    PhotometricInterpretation   Value is a user parameter
 *           84    StripOffsets                Offset and count determined at run time
 *           96    SamplesPerPixel             Hard-coded to 3
 *          108    RowsPerStrip                Value is a user parameter
 *          120    StripByteCounts             Offset and count determined at run time
 *          132    XResolution                 Value is a user parameter
 *          144    YResolution                 Value is a user parameter
 *          156    Resolution Unit              Hard-coded to 1
 *          168    Software
 *          180    DateTime
 *          192    Next IFD offset             0
 *          194    [2 bytes padding]
 * Values:
 *          196    XResolution                 nnnn 1, nnnn is user supplied
 *          204    YResolution                 nnnn 1, nnnn is user supplied
 *          212    BitsPerSample               8, 8, 8
 *          218    [2 bytes padding]
 *          220    "NuttX"                     Length = 6 (includeing NUL terminator)
 *          226    "YYYY:MM:DD HH:MM:SS"       Length = 20 (ncluding NUL terminator)
 *          246    [2 bytes padding]
 *          248    StripOffsets                Beginning of strip offsets
 *          xxx    StripByteCounts             Beginning of strip byte counts
 *          xxx    [Probably padding]
 *          xxx    Data for strips             Beginning of strip data
 */

#define TIFF_RGB_NIFDENTRIES    13
#define TIFF_RGB_STRIPIFDOFFS   84
#define TIFF_RGB_STRIPBCIFDOFFS 120
#define TIFF_RGB_VALOFFSET      196
#define TIFF_RGB_XRESOFFSET     196
#define TIFF_RGB_YRESOFFSET     204
#define TIFF_RGB_BPSOFFSET      212
#define TIFF_RGB_SWOFFSET       220
#define TIFF_RGB_DATEOFFSET     226
#define TIFF_RGB_STRIPOFFSET    248

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_putint16
 *
 * Description:
 *   Write two bytes to the outfile.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state
 *          instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static int tiff_putint16(FAR struct tiff_info_s *info, uint16_t value)
{
  uint8_t bytes[2];
  
  /* Write the two bytes to the output file */

  tiff_put16(bytes, value);
  return tiff_write(info->outfd, bytes, 2);
}

/****************************************************************************
 * Name: tiff_putheader
 *
 * Description:
 *   Setup to create a new TIFF file.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state
 *          instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static inline int tiff_putheader(FAR struct tiff_info_s *info)
{
  struct tiff_header_s hdr;
  int ret;

  /* 0-1: Byte order */

#ifdef CONFIG_ENDIAN_BIG
  hdr.order[0] = 'M';  /* "MM"=big endian */
  hdr.order[1] = 'M';
#else
  hdr.order[0] = 'I';  /* "II"=little endian */
  hdr.order[1] = 'I';
#endif

  /* 2-3: 42 in appropriate byte order */

  tiff_put16(hdr.magic, 42);

  /* 4-7: Offset to the first IFD */

  tiff_put16(hdr.offset, TIFF_IFD_OFFSET);

  /* Write the header to the output file */

  ret = tiff_write(info->outfd, &hdr, SIZEOF_TIFF_HEADER);
  if (ret != OK)
    {
      return ret;
    }

 /* Two pad bytes following the header */

 ret = tiff_putint16(info, 0);
 return ret;
}

/****************************************************************************
 * Name: tiff_putifdentry
 *
 * Description:
 *   Variouis IFD entry writing routines
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state
 *          instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static int tiff_putifdentry(FAR struct tiff_info_s *info, uint16_t tag,
                           uint16_t type, uint32_t count, uint32_t offset)
{
  struct tiff_ifdentry_s ifd;
  tiff_put16(ifd.tag, tag);
  tiff_put16(ifd.type, type);
  tiff_put32(ifd.count, count);
  tiff_put32(ifd.offset, offset);
  return tiff_write(info->outfd, &ifd, SIZEOF_IFD_ENTRY);
}

static int tiff_putifdentry16(FAR struct tiff_info_s *info, uint16_t tag,
                             uint16_t type, uint32_t count, uint16_t value)
{
  union
  {
    uint8_t  b[4];
    uint32_t w;
  } u;

  u.w = 0;
  tiff_put16(u.b, value);
  return tiff_putifdentry(info, tag, count, type, u.w);
}

/****************************************************************************
 * Name: tiff_*
 *
 * Description:
 *   Variouis IFD entry writing routines
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state
 *          instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

static int tiff_newsubfiletype(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_NEWSUBFILETYPE, IFD_FIELD_LONG, 1, 0);
}
  
static int tiff_imagewidth(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_IMAGEWIDTH, IFD_FIELD_SHORT, 1, info->imgwidth);
}

static int tiff_imagelength(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_IMAGELENGTH, IFD_FIELD_SHORT, 1, info->imgheight);
}

static int tiff_greybitspersample(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_BITSPERSAMPLE, IFD_FIELD_SHORT, 1, info->bps);
}

static int tiff_rgbbitspersample(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry(info, IFD_TAG_BITSPERSAMPLE, IFD_FIELD_SHORT, 3, TIFF_RGB_BPSOFFSET);
}

static int tiff_compression(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_COMPRESSION, IFD_FIELD_SHORT, 1, TAG_COMP_NONE);
}

static int tiff_photointerp(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_PMI, IFD_FIELD_SHORT, 1, info->pmi);
}

static int tiff_stripoffsets(FAR struct tiff_info_s *info, uint32_t count, uint32_t offset)
{
  return tiff_putifdentry(info, IFD_TAG_STRIPOFFSETS, IFD_FIELD_LONG, count, offset);
}

static int tiff_samplesperpixel(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_SAMPLESPERPIXEL, IFD_FIELD_SHORT, 1, 3);
}

static int tiff_rowsperstrip(FAR struct tiff_info_s *info)
{
  return tiff_putifdentry16(info, IFD_TAG_ROWSPERSTRIP, IFD_FIELD_SHORT, 1, info->rps);
}

static int tiff_stripbytecounts(FAR struct tiff_info_s *info, uint32_t count, uint32_t offset)
{
  return tiff_putifdentry(info, IFD_TAG_STRIPCOUNTS, IFD_FIELD_LONG, count, offset);
}

static int tiff_xresolution(FAR struct tiff_info_s *info)
{
# warning "Missing logic"
  return -ENOSYS;
}

static int tiff_yresolution(FAR struct tiff_info_s *info)
{
# warning "Missing logic"
  return -ENOSYS;
}

static int tiff_resolutionunit(FAR struct tiff_info_s *info)
{
# warning "Missing logic"
  return -ENOSYS;
}

static int tiff_software(FAR struct tiff_info_s *info)
{
# warning "Missing logic"
  return -ENOSYS;
}

static int tiff_datetime(FAR struct tiff_info_s *info)
{
# warning "Missing logic"
  return -ENOSYS;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_initialize
 *
 * Description:
 *   Setup to create a new TIFF file.
 *
 * Input Parameters:
 *   info - A pointer to the caller allocated parameter passing/TIFF state instance.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_initialize(FAR struct tiff_info_s *info)
{
  int ret = -EINVAL;

  DEBUGASSERT(info && info->outfile && info->tmpfile1 && info->tmpfile2);

  /* Open all output files */

  info->outfd = open(info->outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->outfd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->outfile, errno);
      goto errout;
    }

  info->tmp1fd = open(info->tmpfile1, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->tmp1fd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->tmpfile1, errno);
      goto errout_with_outfd;
    }

  info->tmp2fd = open(info->tmpfile1, O_WRONLY|O_CREAT|O_TRUNC, 0666);
  if (info->tmp2fd < 0)
    {
      gdbg("Failed to open %s for writing: %d\n", info->tmpfile1, errno);
      goto errout_with_tmp1fd;
    }

  /* Write the TIFF header data to the outfile */

  ret = tiff_putheader(info);
  if (ret < 0)
    {
      goto errout_with_tmp2fd;
    }

  /* Write the IFD data to the outfile */
#warning "Missing Logic"
  return OK;

errout_with_tmp2fd:
  (void)close(info->tmp2fd);
  info->tmp2fd = -1;
errout_with_tmp1fd:
  (void)close(info->tmp1fd);
  info->tmp1fd = -1;
errout_with_outfd:
  (void)close(info->outfd);
  info->outfd = -1;
errout:
  return ret;
}

