/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CScrollbarHorizontal/cscrollbarhorizontaltest.cxx
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
#include "graphics/nxwidgets/cbgwindow.hxx"
#include "graphics/nxwidgets/cscrollbarhorizontaltest.hxx"

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
// CScrollbarHorizontalTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CScrollbarHorizontalTest Constructor

CScrollbarHorizontalTest::CScrollbarHorizontalTest()
{
  // Initialize state data

  m_widgetControl = NULL;
  m_bgWindow      = NULL;
}

// CScrollbarHorizontalTest Descriptor

CScrollbarHorizontalTest::~CScrollbarHorizontalTest(void)
{
  disconnect();
}

// Connect to the NX server

bool CScrollbarHorizontalTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_CSCROLLBARHORIZONTALTEST_BGCOLOR))
        {
          printf("CScrollbarHorizontalTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CScrollbarHorizontalTest::disconnect(void)
{
  // Close the window

  if (m_bgWindow)
    {
      delete m_bgWindow;
      m_bgWindow = NULL;
    }

  // Free the widget control instance

  if (m_widgetControl)
    {
      delete m_widgetControl;
      m_widgetControl = NULL;
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

bool CScrollbarHorizontalTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl(NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CScrollbarHorizontalTest::createWindow: Failed to create CBgWindow instance\n");
      disconnect();
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CScrollbarHorizontalTest::createWindow: Failed to open background window\n");
      disconnect();
      return false;
    }

  return true;
}

// Create a scrollbar in the center of the window

CScrollbarHorizontal *CScrollbarHorizontalTest::createScrollbar(void)
{
  // Get the size of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CScrollbarHorizontalTest::createScrollbar: Failed to get window size\n");
      disconnect();
      return false;
    }

  // Put the scrollbar in the center of the display

  nxgl_coord_t scrollbarWidth  = windowSize.w >> 1;
  nxgl_coord_t scrollbarX      = windowSize.w >> 2;

  nxgl_coord_t scrollbarHeight = 10;
  nxgl_coord_t scrollbarY      = (windowSize.h - scrollbarHeight) >> 1;

  // Create the scrollbar

  CScrollbarHorizontal *scrollbar =
    new CScrollbarHorizontal(m_widgetControl,
                             scrollbarX, scrollbarY,
                             scrollbarWidth, scrollbarHeight);
  if (!scrollbar)
    {
      printf("CScrollbarHorizontalTest::createScrollbar: Failed to create CScrollbarHorizontal\n");
      disconnect();
    }
  return scrollbar;
}

// (Re-)draw the scrollbar.

void CScrollbarHorizontalTest::showScrollbar(CScrollbarHorizontal *scrollbar)
{
  scrollbar->enable();        // Un-necessary, the widget is enabled by default
  scrollbar->enableDrawing();
  scrollbar->redraw();
}
