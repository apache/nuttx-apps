/****************************************************************************
 * apps/graphics/tiff/tiff_internal.h
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

#ifndef __APPS_GRAPHICS_TIFF_TIFF_INTERNAL_H
#define __APPS_GRAPHICS_TIFF_TIFF_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>

#include <nuttx/nx/nxglib.h>
#include "graphics/tiff.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Image Type ***************************************************************/

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
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_read
 *
 * Description:
 *   Read TIFF data from the specified file
 *
 * Input Parameters:
 *   fd - Open file descriptor to read from
 *   buffer - Read-only buffer containing the data to be written
 *   count - The number of bytes to write
 *
 * Returned Value:
 *   On success, then number of bytes read; Zero is returned on EOF.
 *   Otherwise, a negated errno value on failure.
 *
 ****************************************************************************/

ssize_t tiff_read(int fd, FAR void *buffer, size_t count);

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

int tiff_write(int fd, FAR const void *buffer, size_t count);

/****************************************************************************
 * Name: tiff_putint16
 *
 * Description:
 *   Write two bytes to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   value - The 2-byte, uint16_t value to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putint16(int fd, uint16_t value);

/****************************************************************************
 * Name: tiff_putint32
 *
 * Description:
 *   Write four bytes to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   value - The 4-byte, uint32_t value to write
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putint32(int fd, uint32_t value);

/****************************************************************************
 * Name: tiff_putstring
 *
 * Description:
 *  Write a string of fixed length to the outfile.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   string - A pointer to the memory containing the string
 *   len - The length of the string (including the NUL terminator)
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_putstring(int fd, FAR const char *string, int len);

/****************************************************************************
 * Name: tiff_wordalign
 *
 * Description:
 *  Pad a file with zeros as necessary to achieve word alignament.
 *
 * Input Parameters:
 *   fd - File descriptor to be used.
 *   size - The current size of the file
 *
 * Returned Value:
 *   The new size of the file on success.  A negated errno value on failure.
 *
 ****************************************************************************/

ssize_t tiff_wordalign(int fd, size_t size);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_GRAPHICS_TIFF_TIFF_INTERNAL_H */
