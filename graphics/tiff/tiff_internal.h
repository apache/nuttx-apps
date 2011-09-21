/****************************************************************************
 * apps/graphics/tiff/tiff_internal.h
 *
 *   Copyright (C) 2010 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRPHICS_TIFF_TIFF_INTERNAL_H
#define __APPS_GRPHICS_TIFF_TIFF_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

#include <nuttx/nx/nxglib.h>
#include <apps/tiff.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* Image Type **************************************************************/

#define IMGFLAGS_BILEV_BIT     (1 << 0)
#define IMGFLAGS_GREY_BIT      (1 << 1)
#define IMGFLAGS_GREY8_BIT     (1 << 2)
#define IMGFLAGS_RGB_BIT       (1 << 3)
#define IMGFLAGS_RGB565_BIT    (1 << 4)

#define IMGFLAGS_FMT_Y1        (IMGFLAGS_BILEV_BIT)
#define IMGFLAGS_FMT_Y4        (IMGFLAGS_GREY_BIT)
#define IMGFLAGS_FMT_Y8        (IMGFLAGS_GREY_BIT|IMGFLAGS_GREY8_BIT)
#define IMGFLAGS_FMT_RGB16_565 (IMGFLAGS_RGB_BIT)
#define IMGFLAGS_FMT_RGB24     (IMGFLAGS_RGB_BIT|IMGFLAGS_RGB565_BIT)

#define IMGFLAGS_ISBILEV(f) \
  (((f) & IMGFLAGS_BILEV_BIT) != 0)
#define IMGFLAGS_ISGREY(f) \
  (((f) & IMGFLAGS_GREY_BIT) != 0)
#define IMGFLAGS_ISGREY4(f) \
  (((f) & (IMGFLAGS_GREY_BIT|IMGFLAGS_GREY8_BIT)) == IMGFLAGS_GREY_BIT)
#define IMGFLAGS_ISGREY8(f) \
  (((f) & (IMGFLAGS_GREY_BIT|IMGFLAGS_GREY8_BIT)) == (IMGFLAGS_GREY_BIT|IMGFLAGS_GREY8_BIT))
#define IMGFLAGS_ISRGB(f) \
  (((f) & IMGFLAGS_FMT_RGB24) != 0)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_write
 *
 * Description:
 *   Write TIFF data to the specified file
 *
 * Input Parameters:
 *   fd - Open file descriptor to write to
 *   buffer - Read-only buffer containing the data to be written
 *   count - The number of bytes to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

EXTERN int tiff_write(int fd, FAR const void *buffer, size_t count);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif  /* __APPS_GRPHICS_TIFF_TIFF_INTERNAL_H */

