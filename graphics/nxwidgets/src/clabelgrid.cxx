/****************************************************************************
 * apps/graphics/NxWkidgets/nwidgets/src/clabelgrid.cxx
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <debug.h>

#include "graphics/nxwidgets/clabelgrid.hxx"
#include "graphics/nxwidgets/clabel.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * CLabelGrid Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

CLabelGrid::CLabelGrid(CWidgetControl* pWidgetControl, nxgl_coord_t x, nxgl_coord_t y,
                       nxgl_coord_t width, nxgl_coord_t height, int cols, int rows):
                       CNxWidget(pWidgetControl, x, y, width, height, 0, 0),
                       m_cols(cols), m_rows(rows)
{
  setPermeable(true); // To allow easier relayouting

  int cell_width = width / m_cols;
  int cell_height = height / m_cols;

  for (int row = 0; row < m_rows; row++)
    {
      m_rowheights.push_back(-1); // -1 signifies automatic sizing
    }

  for (int col = 0; col < m_cols; col++)
    {
      m_colwidths.push_back(-1);
    }

  for (int row = 0; row < m_rows; row++)
    {
      for (int col = 0; col < m_cols; col++)
        {
          CLabel *label = new CLabel(pWidgetControl, col * cell_width, row * cell_height,
                                     cell_width, cell_height, "");
          this->addWidget(label);
          m_labels.push_back(label);
        }
    }
}

CLabel& CLabelGrid::at(int col, int row)
{
  assert(col >= 0 && col < m_cols);
  assert(row >= 0 && row < m_rows);
  return *m_labels.at(row * m_cols + col);
}

void CLabelGrid::onResize(nxgl_coord_t width, nxgl_coord_t height)
{
  this->disableDrawing();

  // Count the number of automatically sized columns and rows and
  // space available to them.

  int autocols = 0;
  int fixedwidth = 0;

  for (int i = 0; i < m_cols; i++)
    {
      if (m_colwidths.at(i) < 0)
        {
          autocols++;
        }
      else
        {
          fixedwidth += m_colwidths.at(i);
        }
    }

  int autorows = 0;
  int fixedheight = 0;

  for (int i = 0; i < m_rows; i++)
    {
      if (m_rowheights.at(i) < 0)
        {
          autorows++;
        }
      else
        {
          fixedheight += m_rowheights.at(i);
        }
    }

  // Avoid divide by zero

  if (autocols == 0)
    {
      autocols = 1;
    }

  if (autorows == 0)
    {
      autorows = 1;
    }

  // Divide the space among the rows and columns

  int auto_width = (width - fixedwidth) / autocols;
  int auto_height = (height - fixedheight) / autorows;
  int y = 0;

  for (int row = 0; row < m_rows; row++)
    {
      int h = m_rowheights.at(row);
      if (h < 0)
        {
          h = auto_height;
        }

      int x = 0;
      for (int col = 0; col < m_cols; col++)
        {
          int w = m_colwidths.at(col);
          if (w < 0)
            {
              w = auto_width;
            }

          this->at(col, row).changeDimensions(x, y, w, h);

          ginfo("G %d %d: %d %d %d %d\n", col, row, x, y, w, h);
          x += w;
        }

      y += h;
    }

  this->enableDrawing();
  redraw();
}

void CLabelGrid::setColumnWidth(int col, int width)
{
  m_colwidths.at(col) = width;
  onResize(getWidth(), getHeight());
}

void CLabelGrid::setRowHeight(int row, int height)
{
  m_rowheights.at(row) = height;
  onResize(getWidth(), getHeight());
}

void CLabelGrid::setBackgroundColor(nxgl_mxpixel_t color)
{
  CNxWidget::setBackgroundColor(color);

  for (int row = 0; row < m_rows; row++)
    {
      for (int col = 0; col < m_cols; col++)
      {
        this->at(col, row).setBackgroundColor(color);
      }
    }
}

void CLabelGrid::setBorderless(bool borderless)
{
  CNxWidget::setBorderless(borderless);

  for (int row = 0; row < m_rows; row++)
    {
      for (int col = 0; col < m_cols; col++)
        {
          this->at(col, row).setBorderless(borderless);
        }
    }
}

void CLabelGrid::useWidgetStyle(const CWidgetStyle* style)
{
  CNxWidget::useWidgetStyle(style);

  for (int row = 0; row < m_rows; row++)
    {
      for (int col = 0; col < m_cols; col++)
        {
          this->at(col, row).useWidgetStyle(style);
        }
    }
}
