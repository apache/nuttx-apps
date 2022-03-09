/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CGlyphSliderHorizontal/cglyphsliderhorizontaltest.cxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
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

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxwidgets/cbgwindow.hxx"
#include "graphics/nxwidgets/cglyphsliderhorizontaltest.hxx"

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
          printf("CGlyphSliderHorizontalTest::connect: setBackgroundColor failed\n");
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
      printf("CGlyphSliderHorizontalTest::createWindow: Failed to create CBgWindow instance\n");
      disconnect();
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CGlyphSliderHorizontalTest::createWindow: Failed to open background window\n");
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
