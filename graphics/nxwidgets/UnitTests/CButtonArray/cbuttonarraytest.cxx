/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CButtonArray/cbuttonarraytest.cxx
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
#include "graphics/nxwidgets/cbuttonarraytest.hxx"
#include "graphics/nxwidgets/cbgwindow.hxx"

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
// CButtonArrayTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CButtonArrayTest Constructor

CButtonArrayTest::CButtonArrayTest()
{
  m_widgetControl = (CWidgetControl *)NULL;
  m_bgWindow      = (CBgWindow *)NULL;
}

// CButtonArrayTest Descriptor

CButtonArrayTest::~CButtonArrayTest()
{
  disconnect();
}

// Connect to the NX server

bool CButtonArrayTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_CBUTTONARRAYTEST_BGCOLOR))
        {
          printf("CButtonArrayTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CButtonArrayTest::disconnect(void)
{
  // Close the window

  if (m_bgWindow)
    {
      delete m_bgWindow;
    }

  // Destroy the widget control

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

bool CButtonArrayTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CButtonArrayTest::createGraphics: Failed to create CBgWindow instance\n");
      delete m_widgetControl;
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CButtonArrayTest::createGraphics: Failed to open background window\n");
      delete m_bgWindow;
      m_bgWindow = (CBgWindow*)0;
      return false;
    }

  return true;
}

// Create a CButtonArray instance

CButtonArray *CButtonArrayTest::createButtonArray(void)
{
  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CButtonArrayTest::createGraphics: Failed to get window size\n");
      return (CButtonArray *)NULL;
    }

  // Pick an X/Y position such that the button array will be centered in the display

  nxgl_coord_t buttonArrayX;
  if (BUTTONARRAY_WIDTH >= windowSize.w)
    {
      buttonArrayX = 0;
    }
  else
    {
      buttonArrayX = (windowSize.w - BUTTONARRAY_WIDTH) >> 1;
    }

  nxgl_coord_t buttonArrayY;
  if (BUTTONARRAY_HEIGHT >= windowSize.h)
    {
      buttonArrayY = 0;
    }
  else
    {
      buttonArrayY = (windowSize.h - BUTTONARRAY_HEIGHT) >> 1;
    }

  // Now we have enough information to create the button array

  return new CButtonArray(m_widgetControl,
                          buttonArrayX, buttonArrayY,
                          BUTTONARRAY_NCOLUMNS, BUTTONARRAY_NROWS,
                          BUTTONARRAY_BUTTONWIDTH, BUTTONARRAY_BUTTONHEIGHT);
}

// Draw the button array

void CButtonArrayTest::showButton(CButtonArray *buttonArray)
{
  buttonArray->enable();        // Un-necessary, the widget is enabled by default
  buttonArray->enableDrawing();
  buttonArray->redraw();
}

// Perform a simulated mouse click on a button in the array.  This method injects
// the mouse click through the NX hierarchy just as would real mouse
// hardware.

void CButtonArrayTest::click(CButtonArray *buttonArray, int column, int row)
{
  // nx_mousein is meant to be called by mouse handling software.
  // Here we just inject a left-button click directly in the center of
  // the selected button.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level mouse interface

  NXHANDLE handle = getServer();

  // The the coordinates of the center of the button

  nxgl_coord_t buttonX = buttonArray->getX() +
                         column * BUTTONARRAY_BUTTONWIDTH +
                         BUTTONARRAY_BUTTONWIDTH/2;
  nxgl_coord_t buttonY = buttonArray->getY() +
                         row * BUTTONARRAY_BUTTONHEIGHT +
                         BUTTONARRAY_BUTTONHEIGHT/2;

  // Then inject the mouse click

  nx_mousein(handle, buttonX, buttonY, NX_MOUSE_LEFTBUTTON);
}

// The counterpart to click.  This simulates a button release through
// the same mechanism.

void CButtonArrayTest::release(CButtonArray *buttonArray, int column, int row)
{
  // nx_mousein is meant to be called by mouse handling software.
  // Here we just inject a left-button click directly in the center of
  // the button.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level mouse interface

  NXHANDLE handle = getServer();

  // The the coordinates of the center of the button

  nxgl_coord_t buttonX = buttonArray->getX() +
                         column * BUTTONARRAY_BUTTONWIDTH +
                         BUTTONARRAY_BUTTONWIDTH/2;
  nxgl_coord_t buttonY = buttonArray->getY() +
                         row * BUTTONARRAY_BUTTONHEIGHT +
                         BUTTONARRAY_BUTTONHEIGHT/2;

  // Then inject the mouse release

  nx_mousein(handle, buttonX, buttonY, NX_MOUSE_NOBUTTONS);
}

// Widget events are normally handled in a modal loop.
// However, for this case we know when there should be press and release
// events so we don't have to poll.  We can just perform a one pass poll
// then check if the event was processed corredly.

void CButtonArrayTest::poll(CButtonArray *button)
{
  // Poll for mouse events

  m_widgetControl->pollEvents(button);
}
