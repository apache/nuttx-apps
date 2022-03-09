/****************************************************************************
 * apps/graphics/tiff/tiff_addstrip.c
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

#include <assert.h>
#include <errno.h>
#include <debug.h>

#include "graphics/tiff.h"

#include "tiff_internal.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

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
 * Name: tiff_addstrip
 *
 * Description:
 *   Convert an RGB565 strip to an RGB888 strip and write it to tmpfile2.
 *
 *   Add an image data strip.  The size of the strip in pixels must be equal
 *   to the RowsPerStrip x ImageWidth values that were provided to
 *   tiff_initialize().
 *
 * Input Parameters:
 *   info    - A pointer to the caller allocated parameter passing/TIFF state instance.
 *   buffer  - A buffer containing a single row of data.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/
int tiff_convstrip(FAR struct tiff_info_s *info, FAR const uint8_t *strip)
{
#ifdef CONFIG_DEBUG_GRAPHICS
  size_t ntotal;
#endif
  size_t nbytes;
  FAR uint16_t *src;
  FAR uint8_t *dest;
  uint16_t rgb565;
  int ret;
  int i;

  DEBUGASSERT(info->iobuffer != NULL);

  /* Convert each RGB565 pixel to RGB888 */

  src    = (FAR uint16_t *)strip;
  dest   = info->iobuffer;
  nbytes = 0;
#ifdef CONFIG_DEBUG_GRAPHICS
  ntotal = 0;
#endif

  for (i = 0; i < info->pps; i++)
    {
      /* Convert RGB565 to RGB888 */

      rgb565  = *src++;
      *dest++ = (rgb565 >> (11-3)) & 0xf8; /* Move bits 11-15 to 3-7 */
      *dest++ = (rgb565 >> ( 5-2)) & 0xfc; /* Move bits  5-10 to 2-7 */
      *dest++ = (rgb565 << (   3)) & 0xf8; /* Move bits  0- 4 to 3-7 */

      /* Update the byte count */

      nbytes += 3;
#ifdef CONFIG_DEBUG_GRAPHICS
      ntotal += 3;
#endif

      /* Flush the conversion buffer to tmpfile2 when it becomes full */

      if (nbytes > (info->iosize-3))
        {
          ret = tiff_write(info->tmp2fd, info->iobuffer, nbytes);
          if (ret < 0)
            {
              return ret;
            }

          /* Reset to refill the conversion buffer */

          dest   = info->iobuffer;
          nbytes = 0;
        }
    }

  /* Flush any buffer data to tmpfile2 */

  ret = tiff_write(info->tmp2fd, info->iobuffer, nbytes);
#ifdef CONFIG_DEBUG_GRAPHICS
  DEBUGASSERT(ntotal == info->bps);
#endif
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tiff_addstrip
 *
 * Description:
 *   Add an image data strip.  The size of the strip in pixels must be equal
 *   to the RowsPerStrip x ImageWidth values that were provided to
 *   tiff_initialize().
 *
 * Input Parameters:
 *   info    - A pointer to the caller allocated parameter passing/TIFF state instance.
 *   buffer  - A buffer containing a single row of data.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int tiff_addstrip(FAR struct tiff_info_s *info, FAR const uint8_t *strip)
{
  ssize_t newsize;
  int ret;

  /* Add the new strip based on the color format.  For FB_FMT_RGB16_565,
   * will have to perform a conversion to RGB888.
   */

  if (info->colorfmt == FB_FMT_RGB16_565)
    {
      ret = tiff_convstrip(info, strip);
    }

  /* For other formats, it is a simple write using the number of bytes per strip */

  else
    {
      ret = tiff_write(info->tmp2fd, strip, info->bps);
    }

  if (ret < 0)
    {
      goto errout;
    }

  /* Write the byte count to the outfile and the offset to tmpfile1 */

  ret = tiff_putint32(info->outfd, info->bps);
  if (ret < 0)
    {
      goto errout;
    }
  info->outsize += 4;

  ret = tiff_putint32(info->tmp1fd, info->tmp2size);
  if (ret < 0)
    {
      goto errout;
    }
  info->tmp1size += 4;

  /* Increment the size of tmp2file. */

  info->tmp2size += info->bps;

  /* Pad tmpfile2 as necessary achieve word alignment */

  newsize = tiff_wordalign(info->tmp2fd, info->tmp2size);
  if (newsize < 0)
    {
      ret = (int)newsize;
      goto errout;
    }
  info->tmp2size = (size_t)newsize;

  /* Increment the number of strips in the TIFF file */

  info->nstrips++;
  return OK;

errout:
  tiff_abort(info);
  return ret;
}
