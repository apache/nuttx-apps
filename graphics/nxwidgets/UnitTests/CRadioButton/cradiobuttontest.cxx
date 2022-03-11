/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CRadioButton/cradiobuttontest.cxx
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
#include "graphics/nxwidgets/cradiobuttontest.hxx"
#include "graphics/nxwidgets/cbitmap.hxx"
#include "graphics/nxglyphs.hxx"

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
// CRadioButtonTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CRadioButtonTest Constructor

CRadioButtonTest::CRadioButtonTest()
{
  // Initialize state data

  m_widgetControl = (CWidgetControl *)NULL;
  m_bgWindow      = (CBgWindow *)NULL;

  // Peek at the radio button glyph to get a good estimate of the size

  m_size.w        = g_radioButtonOn.width;
  m_size.h        = g_radioButtonOn.height;
}

// CRadioButtonTest Descriptor

CRadioButtonTest::~CRadioButtonTest(void)
{
  disconnect();
}

// Connect to the NX server

bool CRadioButtonTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Set the background color

      if (!setBackgroundColor(CONFIG_CRADIOBUTTONTEST_BGCOLOR))
        {
          printf("CRadioButtonTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CRadioButtonTest::disconnect(void)
{
  // Free the radiobutton group

  if (m_radioButtonGroup)
    {
      delete m_radioButtonGroup;
      m_radioButtonGroup = (CRadioButtonGroup *)NULL;
    }

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

bool CRadioButtonTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CRadioButtonTest::createGraphics: Failed to create CBgWindow instance\n");
      delete m_widgetControl;
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CRadioButtonTest::createGraphics: Failed to open background window\n");
      delete m_bgWindow;
      m_bgWindow = (CBgWindow*)0;
      return false;
    }

  return true;
}

// Create a CRadioButton instance

CRadioButton *CRadioButtonTest::newRadioButton(void)
{
  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CRadioButtonTest::newRadioButton: Failed to get window size\n");
      return (CRadioButton *)NULL;
    }

  // Create the radio button group, if we have not already done so

  if (!m_radioButtonGroup)
    {
      // Start the radio button group in the center of the upper, left
      // quadrant.  The initial size is 0,0 and the default style is used.

      nxgl_coord_t groupX = windowSize.w >> 2;
      nxgl_coord_t groupY = windowSize.h >> 2;

      m_radioButtonGroup = new CRadioButtonGroup(m_widgetControl, groupX, groupY);
      if (!m_radioButtonGroup)
        {
          printf("CRadioButtonTest::newRadioButton: Failed to create the radio button group\n");
          return (CRadioButton *)NULL;
        }
    }

  // Create the new radio button at the bottom of the radio button group and
  // with the size of the ON radio button glyph (they all need to be the same
  // size!).  Note that coordinates are relative to the parent bounding box.

  nxgl_coord_t buttonX = 0;
  nxgl_coord_t buttonY = m_radioButtonGroup->getHeight();

  return m_radioButtonGroup->newRadioButton(buttonX, buttonY, m_size.w, m_size.h);
}

// (Re-)draw the buttons.

void CRadioButtonTest::showButtons(void)
{
  m_radioButtonGroup->enable();        // Un-necessary, the widget is enabled by default
  m_radioButtonGroup->enableDrawing();
  m_radioButtonGroup->redraw();
}

// Push the radio button

void CRadioButtonTest::pushButton(CRadioButton *button)
{
  // Get the button center coordinates

  nxgl_coord_t buttonX = button->getX() + (button->getWidth() >> 1);
  nxgl_coord_t buttonY = button->getY() + (button->getHeight() >> 1);

  // Then push the button by calling nx_mousein.  nx_mousein is meant to be
  // called by mouse handling software. Here we just inject a left-button click
  // directly in the center of the radio button.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level mouse interface

  NXHANDLE handle = getServer();

  // Then inject the mouse click

  nx_mousein(handle, buttonX, buttonY, NX_MOUSE_LEFTBUTTON);

  // Poll for mouse events
  //
  // Widget events are normally handled in a modal loop.
  // However, for this case we know that we just pressed the mouse button
  // so we don't have to poll.  We can just perform a one pass poll then
  // then check if the mouse event was processed corredly.

  m_widgetControl->pollEvents(m_radioButtonGroup);

  // Then inject the mouse release

  nx_mousein(handle, buttonX, buttonY, 0);

  // And poll for more mouse events

  m_widgetControl->pollEvents(m_radioButtonGroup);

  // And re-draw the buttons (the mouse click event should have automatically
  // triggered the re-draw)
  //
  // showButtons();
}

// Show the state of the radio button group

void CRadioButtonTest::showButtonState(void)
{
  int index = m_radioButtonGroup->getSelectedIndex();
  if (index < 0)
    {
      printf("CRadioButtonTest::showButtonState No button is pressed\n");
    }
  else
    {
      printf("CRadioButtonTest::showButtonState button%d is selected\n", index+1);
    }
}
