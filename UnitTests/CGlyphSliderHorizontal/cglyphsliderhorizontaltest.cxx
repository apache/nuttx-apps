/////////////////////////////////////////////////////////////////////////////
// NxWidgets/UnitTests/CGlyphSliderHorizontal/cglyphsliderhorizontaltest.cxx
//
//   Copyright (C) 2012-2013 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX, NxWidgets, nor the names of its contributors
//    me be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <nuttx/init.h>
#include <cstdio>
#include <cerrno>
#include <debug.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxfonts.h>

#include "nxconfig.hxx"
#include "crlepalettebitmap.hxx"
#include "cbgwindow.hxx"
#include "cglyphsliderhorizontaltest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

static const NXWidgets::nxwidget_pixel_t hilight_palette[8] =
{
  CONFIG_CGLYPHSLIDERHORIZONTALTEST_BGCOLOR, MKRGB( 61, 74,158), MKRGB(113,140,242), MKRGB(171,186,255),
  MKRGB(255,255,255), MKRGB(119,130,199), MKRGB(177,219,255), MKRGB(202,224,255),
};

static const NXWidgets::SRlePaletteBitmapEntry vol_bitmap[] =
{
  { 22,   0},                                                              /* Row 0 */
  {  9,   0}, {  5,   7}, {  8,   0},                                      /* Row 1 */
  {  6,   0}, {  2,   6}, {  6,   7}, {  2,   6}, {  1,   7}, {  5,   0},  /* Row 2 */
  {  4,   0}, {  2,   3}, {  9,   6}, {  3,   3}, {  4,   0},              /* Row 3 */
  {  3,   0}, {  5,   3}, {  6,   6}, {  5,   3}, {  3,   0},              /* Row 4 */
  {  3,   0}, { 16,   3}, {  3,   0},                                      /* Row 5 */
  {  2,   0}, {  2,   2}, { 13,   3}, {  3,   2}, {  2,   0},              /* Row 6 */
  {  1,   0}, {  1,   5}, {  4,   2}, { 10,   3}, {  4,   2}, {  1,   5},
  {  1,   0},                                                              /* Row 7 */
  {  1,   0}, {  2,   5}, { 15,   2}, {  3,   5}, {  1,   0},              /* Row 8 */
  {  1,   0}, {  5,   5}, { 10,   2}, {  5,   5}, {  1,   0},              /* Row 9 */
  {  1,   0}, {  2,   1}, { 15,   5}, {  3,   1}, {  1,   0},              /* Row 10 */
  {  1,   0}, {  5,   1}, { 10,   5}, {  5,   1}, {  1,   0},              /* Row 11 */
  {  1,   0}, { 20,   1}, {  1,   0},                                      /* Row 12 */
  {  1,   0}, { 20,   1}, {  1,   0},                                      /* Row 13 */
  {  1,   0}, { 19,   1}, {  1,   5}, {  1,   0},                          /* Row 14 */
  {  1,   0}, {  1,   5}, {  7,   1}, {  1,   5}, {  4,   2}, {  6,   1},
  {  2,   0},                                                              /* Row 15 */
  {  2,   0}, {  5,   1}, {  8,   2}, {  4,   1}, {  1,   5}, {  2,   0},  /* Row 16 */
  {  3,   0}, {  2,   1}, { 12,   2}, {  2,   1}, {  3,   0},              /* Row 17 */
  {  3,   0}, {  1,   5}, {  1,   1}, { 13,   2}, {  1,   5}, {  3,   0},  /* Row 18 */
  {  6,   0}, { 10,   2}, {  1,   5}, {  5,   0},                          /* Row 19 */
  {  6,   0}, {  1,   5}, {  8,   2}, {  7,   0},                          /* Row 20 */
  { 22,   0},                                                              /* Row 21 */
};

const struct NXWidgets::SRlePaletteBitmap g_mplayerVolBitmap =
{
  16,
  CONFIG_NXWIDGETS_FMT,
  8,
  22,
  22,
  {hilight_palette, hilight_palette},
  vol_bitmap
};

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CGlyphSliderHorizontalTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CGlyphSliderHorizontalTest Constructor

CGlyphSliderHorizontalTest::CGlyphSliderHorizontalTest()
{
  // Initialize state data

  m_widgetControl = (CWidgetControl *)NULL;
  m_bgWindow      = (CBgWindow *)NULL;
}

// CGlyphSliderHorizontalTest Descriptor

CGlyphSliderHorizontalTest::~CGlyphSliderHorizontalTest(void)
{
  disconnect();
}

// Connect to the NX server

bool CGlyphSliderHorizontalTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_CGLYPHSLIDERHORIZONTALTEST_BGCOLOR))
        {
          message("CGlyphSliderHorizontalTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CGlyphSliderHorizontalTest::disconnect(void)
{
  // Close the window

  if (m_bgWindow)
    {
      delete m_bgWindow;
      m_bgWindow = (CBgWindow *)NULL;
    }

  // Free the widget control instance

  if (m_widgetControl)
    {
      delete m_widgetControl;
      m_widgetControl = (CWidgetControl *)NULL;
    }

  // And disconnect from the server

  CNxServer::disconnect();
}

// Create the background window instance.  This function illustrates
// the basic steps to instantiate any window:
//
// 1) Create a dumb CWigetControl instance
// 2) Pass the dumb CWidgetControl instance to the window constructor
//    that inherits from INxWindow.  This will "smarten" the CWidgetControl
//    instance with some window knowlede
// 3) Call the open() method on the window to display the window.
// 4) After that, the fully smartened CWidgetControl instance can
//    be used to generate additional widgets by passing it to the
//    widget constructor

bool CGlyphSliderHorizontalTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      message("CGlyphSliderHorizontalTest::createWindow: Failed to create CBgWindow instance\n");
      disconnect();
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      message("CGlyphSliderHorizontalTest::createWindow: Failed to open background window\n");
      disconnect();
      return false;
    }

  return true;
}

// Create a slider in the center of the window

CGlyphSliderHorizontal *CGlyphSliderHorizontalTest::createSlider(void)
{
  // Get the size of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CGlyphSliderHorizontalTest::createSlider: Failed to get window size\n");
      disconnect();
      return false;
    }

  // Put the slider in the center of the display

  nxgl_coord_t sliderWidth  = windowSize.w >> 1;
  nxgl_coord_t sliderX      = windowSize.w >> 2;

  nxgl_coord_t sliderHeight = 26;
  nxgl_coord_t sliderY      = (windowSize.h - sliderHeight) >> 1;

  // Create the bitmap for the slider grip

  NXWidgets::CRlePaletteBitmap *pBitmap = new NXWidgets::
      CRlePaletteBitmap(&g_mplayerVolBitmap);

  // Create the slider

  CGlyphSliderHorizontal *slider = new CGlyphSliderHorizontal(m_widgetControl,
                                                    sliderX, sliderY,
                                                    sliderWidth, sliderHeight,
                                                    pBitmap, MKRGB(63, 90, 192));
  if (!slider)
    {
      printf("CGlyphSliderHorizontalTest::createSlider: Failed to create CGlyphSliderHorizontal\n");
      disconnect();
    }
  return slider;
}

// (Re-)draw the slider.

void CGlyphSliderHorizontalTest::showSlider(CGlyphSliderHorizontal *slider)
{
  slider->enable();        // Un-necessary, the widget is enabled by default
  slider->enableDrawing();
  slider->redraw();
}
