/****************************************************************************
 * apps/include/graphics/nxwidgets/cnxfont.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CNXFONT_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CNXFONT_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "graphics/nxwidgets/nxconfig.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  class CNxString;
  struct SBitmap;

  /**
   * Class defining the properties of one font.
   */

  class CNxFont
  {
  private:
    enum nx_fontid_e m_fontId;              /**< The font ID. */
    NXHANDLE m_fontHandle;                  /**< The font handle */
    FAR const struct nx_font_s *m_pFontSet; /** < The font set metrics */
    nxgl_mxpixel_t m_fontColor;             /**< Color to draw the font with when rendering. */
    nxgl_mxpixel_t m_transparentColor;      /**< Background color that should not be rendered. */

  public:

    /**
     * CNxFont Constructor.
     *
     * @param fontid The font ID to use.
     * @param fontColor The font color to use.
     * @param transparentColor The color in the font bitmap used as the
     *    background color.
     */

    CNxFont(enum nx_fontid_e fontid, nxgl_mxpixel_t fontColor,
            nxgl_mxpixel_t transparentColor);

    /**
     * CNxFont Destructor.
     */

    ~CNxFont() { }

    /**
     * Checks if supplied character is blank in the current font.
     *
     * @param letter The character to check.
     * @return True if the glyph contains any pixels to be drawn.  False if
     * the glyph is blank.
     */

    const bool isCharBlank(const nxwidget_char_t letter) const;

    /**
     * Gets the color currently being used as the drawing color.
     *
     * @return The current drawing color.
     */

    inline const nxgl_mxpixel_t getColor() const
    {
      return m_fontColor;
    }

    /**
     * Sets the color to use as the drawing color.  If set, this overrides
     * the colors present in a non-monochrome font.
     * @param color The new drawing color.
     */

    inline void setColor(const nxgl_mxpixel_t color)
    {
      m_fontColor = color;
    }

    /**
     * Get the color currently being used as the transparent background
     * color.
     * @return The transparent background color.
     */

    inline const nxgl_mxpixel_t getTransparentColor() const
    {
      return m_transparentColor;
    }

    /**
     * Sets the transparent background color to a new value.
     * @param color The new background color.
     */

    inline void setTransparentColor(const nxgl_mxpixel_t color)
    {
      m_transparentColor = color;
    }

    /**
     * Draw an individual character of the font to the specified bitmap.
     *
     * @param bitmap The bitmap to draw to.
     * @param letter The character to output.
     */

    void drawChar(FAR SBitmap *bitmap, nxwidget_char_t letter);

    /**
     * Get the width of a string in pixels when drawn with this font.
     *
     * @param text The string to check.
     * @return The width of the string in pixels.
     */

    nxgl_coord_t getStringWidth(const CNxString &text) const;

    inline nxgl_coord_t getStringWidth(FAR const CNxString *text) const
    {
      return getStringWidth(*text);
    }

    /**
     * Get the width of a portion of a string in pixels when drawn with this
     * font.
     *
     * @param text The string to check.
     * @param startIndex The start point of the substring within the string.
     * @param length The length of the substring in chars.
     * @return The width of the substring in pixels.
     */

    nxgl_coord_t getStringWidth(const CNxString &text,
                                int startIndex, int length) const;

    inline nxgl_coord_t getStringWidth(FAR const CNxString *text,
                                      int startIndex, int length) const
    {
      return getStringWidth(*text, startIndex, length);
    }

    /**
     * Gets font metrics for a particular character
     *
     *
     * @param letter The character to get the width of.
     * @param metrics The location to return the font metrics
     */

    void getCharMetrics(nxwidget_char_t letter,
                        FAR struct nx_fontmetric_s *metrics) const;

    /**
     * Get the width of an individual character.
     *
     * @param letter The character to get the width of.
     * @return The width of the character in pixels.
     */

    nxgl_coord_t getCharWidth(nxwidget_char_t letter) const;

    /**
     * Get the height of an individual character.
     *
     * @param letter The letter to get the height of.
     * @return The height of the character in pixels.
     */

    inline nxgl_coord_t getCharHeight(nxwidget_char_t letter) const;

    /**
     * Gets the maximum width of the font.
     *
     * @return The height of the font.
     */

    inline const uint8_t getMaxWidth(void) const
    {
      return m_pFontSet->mxwidth;
    }

    /**
     * Gets the height of the font.
     *
     * @return The height of the font.
     */

    inline const uint8_t getHeight(void) const
    {
      return m_pFontSet->mxheight;
    }
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CNXFONT_HXX
