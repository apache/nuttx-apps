/****************************************************************************
 * apps/graphics/nxwidgets/src/cstickyimage.cxx
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cstickyimage.hxx"
#include "graphics/nxwidgets/cgraphicsport.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * CImage Method Implementations
 ****************************************************************************/

using namespace NXWidgets;

/**
 * Constructor for an image.
 *
 * @param pWidgetControl The controlling widget for the display
 * @param x The x coordinate of the image box, relative to its parent.
 * @param y The y coordinate of the image box, relative to its parent.
 * @param width The width of the image box.
 * @param height The height of the image box.
 * @param bitmap The source bitmap image.
 * @param style The style that the widget should use.  If this is not
 *        specified, the image will use the global default widget
 *        style.
 */

CStickyImage::CStickyImage(CWidgetControl *pWidgetControl,
                           nxgl_coord_t x, nxgl_coord_t y,
                           nxgl_coord_t width, nxgl_coord_t height,
                           FAR IBitmap *bitmap, CWidgetStyle *style)
: CImage(pWidgetControl, x, y, width, height, bitmap, style)
{
  m_stuckSelection = false;
}

/**
 * Sets the image's stuck selection state.
 *
 * @param selection The new stuck selection state.
 */

void CStickyImage::setStuckSelection(bool selection)
{
  m_stuckSelection = selection;
  redraw();
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CStickyImage::drawContents(CGraphicsPort *port)
{
  CImage::drawContents(port, m_stuckSelection);
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 *
 * @param port The CGraphicsPort to draw to.
 * @see redraw()
 */

void CStickyImage::drawBorder(CGraphicsPort *port)
{
  CImage::drawBorder(port, m_stuckSelection);
}
