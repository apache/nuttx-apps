/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CImage/cimagetest.cxx
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
#include "graphics/nxwidgets/ibitmap.hxx"
#include "graphics/nxwidgets/cbgwindow.hxx"
#include "graphics/nxwidgets/cimagetest.hxx"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Classes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Private Data
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Public Data
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Public Function Prototypes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CImageTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CImageTest Constructor

CImageTest::CImageTest()
{
  m_widgetControl = (CWidgetControl *)NULL;
  m_bgWindow      = (CBgWindow *)NULL;
}

// CImageTest Descriptor

CImageTest::~CImageTest()
{
  disconnect();
}

// Connect to the NX server

bool CImageTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_CIMAGETEST_BGCOLOR))
        {
          printf("CImageTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CImageTest::disconnect(void)
{
  // Close the window

  if (m_bgWindow)
    {
      delete m_bgWindow;
    }

  // Free the widget control instance

  if (m_widgetControl)
    {
      delete m_widgetControl;
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

bool CImageTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CImageTest::createGraphics: Failed to create CBgWindow instance\n");
      delete m_widgetControl;
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CImageTest::createGraphics: Failed to open background window\n");
      delete m_bgWindow;
      m_bgWindow = (CBgWindow*)0;
      return false;
    }

  return true;
}

// Create a CImage instance

CImage *CImageTest::createImage(IBitmap *bitmap)
{
  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CImageTest::createGraphics: Failed to get window size\n");
      return (CImage *)NULL;
    }

  // Get the height and width of the image

  nxgl_coord_t imageWidth  = bitmap->getWidth();
  nxgl_coord_t imageHeight = (nxgl_coord_t)bitmap->getHeight();

  // The default CImage has borders enabled with thickness of the border
  // width.  Add twice the thickness of the border to the width and height. (We
  // could let CImage do this for us by calling CImage::getPreferredDimensions())

  imageWidth  += 2 * 1;
  imageHeight += 2 * 1;

  // Pick an X/Y position such that the image will be centered in the display

  nxgl_coord_t imageX;
  if (imageWidth >= windowSize.w)
    {
      imageX = 0;
    }
  else
    {
      imageX = (windowSize.w - imageWidth) >> 1;
    }

  nxgl_coord_t imageY;
  if (imageHeight >= windowSize.h)
    {
      imageY = 0;
    }
  else
    {
      imageY = (windowSize.h - imageHeight) >> 1;
    }

  // Now we have enough information to create the image

  return new CImage(m_widgetControl, imageX, imageY, imageWidth, imageHeight, bitmap);
}

// Draw the image

void CImageTest::showImage(CImage *image)
{
  image->enable();
  image->enableDrawing();
  image->redraw();
  image->disableDrawing();
}
