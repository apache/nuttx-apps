/****************************************************************************
 * apps/include/nxwidgets/include/clabelgrid.hxx
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *           Petteri Aimonen <jpa@kapsi.fi>
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
 * 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
 *    me be used to endorse or promote products derived from this software
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
 ****************************************************************************
 *
 * Portions of this package derive from Woopsi (http://woopsi.org/) and
 * portions are original efforts.  It is difficult to determine at this
 * point what parts are original efforts and which parts derive from Woopsi.
 * However, in any event, the work of  Antony Dzeryn will be acknowledged
 * in all NxWidget files.  Thanks Antony!
 *
 *   Copyright (c) 2007-2011, Antony Dzeryn
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the names "Woopsi", "Simian Zombie" nor the
 *   names of its contributors may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Antony Dzeryn ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Antony Dzeryn BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLABELGRIG_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLABELGRIG_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/tnxarray.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  /**
   * Forward references
   */

  class CWidgetControl;
  class CRect;
  class CLabel;

  /**
   * 2-dimensional grid of labels. Each row and column can have custom
   * label styles applied.
   */

  class CLabelGrid : public CNxWidget
  {
  protected:
    int m_cols;
    int m_rows;

    TNxArray<CLabel*> m_labels;
    TNxArray<int> m_colwidths;
    TNxArray<int> m_rowheights;

    /**
     * Resize the widget to the new dimensions.
     *
     * @param width The new width.
     * @param height The new height.
     */

    virtual void onResize(nxgl_coord_t width, nxgl_coord_t height);

  public:

    /**
     * Constructor for a grid of labels
     *
     * @param pWidgetControl The controlling widget for the display
     * @param x The x coordinate of the text box, relative to its parent.
     * @param y The y coordinate of the text box, relative to its parent.
     * @param width The width of the textbox.
     * @param height The height of the textbox.
     * @param cols Number of colums in the grid.
     * @param rows Number of rows in the grid.
     */

    CLabelGrid(CWidgetControl *pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
           nxgl_coord_t width, nxgl_coord_t height, int cols, int rows);

    virtual inline ~CLabelGrid() { }

    /**
     * Get reference to the label at particular position of the grid.
     *
     * @param col Zero-based index of the column.
     * @param row Zero-based index of the row.
     * @returns Reference to CLabel.
     */

    virtual CLabel &at(int col, int row);

    /**
     * Set width of a column.
     *
     * @param col Zero-based index of the column.
     * @param width Width of column in pixels, or -1 to size automatically.
     */

    void setColumnWidth(int col, int width);

    /**
     * Set height of a row.
     *
     * @param row Zero-based index of the row.
     * @param height Height of row in pixels, or -1 to size automatically.
     */

    void setRowHeight(int row, int height);

    void setBackgroundColor(nxgl_mxpixel_t color);

    void setBorderless(bool borderless);

    void useWidgetStyle(const CWidgetStyle *style);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CLABELGRIG_HXX
