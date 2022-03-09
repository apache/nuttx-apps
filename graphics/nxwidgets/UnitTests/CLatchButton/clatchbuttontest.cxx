/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CLatchButton/clatchbuttontest.cxx
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
#include "graphics/nxwidgets/clatchbuttontest.hxx"
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
// CLatchButtonTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CLatchButtonTest Constructor

CLatchButtonTest::CLatchButtonTest()
{
  m_bgWindow = (CBgWindow *)NULL;
  m_nxFont   = (CNxFont *)NULL;
  m_text     = (CNxString *)NULL;
}

// CLatchButtonTest Descriptor

CLatchButtonTest::~CLatchButtonTest()
{
  disconnect();
}

// Connect to the NX server

bool CLatchButtonTest::connect(void)
{
  // Connect to the server

  bool nxConnected = CNxServer::connect();
  if (nxConnected)
    {
      // Create the default font instance

      m_nxFont = new CNxFont(NXFONT_DEFAULT,
                             CONFIG_NXWIDGETS_DEFAULT_FONTCOLOR,
                             CONFIG_NXWIDGETS_TRANSPARENT_COLOR);
      if (!m_nxFont)
        {
          printf("CLatchButtonTest::connect: Failed to create the default font\n");
        }

      // Set the background color

      if (!setBackgroundColor(CONFIG_CLATCHBUTTONTEST_BGCOLOR))
        {
          printf("CLatchButtonTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CLatchButtonTest::disconnect(void)
{
  // Close the window

  if (m_bgWindow)
    {
      delete m_bgWindow;
    }

  // Free the display string

  if (m_text)
    {
      delete m_text;
      m_text = (CNxString *)NULL;
    }

  // Free the default font

  if (m_nxFont)
    {
      delete m_nxFont;
      m_nxFont = (CNxFont *)NULL;
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

bool CLatchButtonTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CLatchButtonTest::createGraphics: Failed to create CBgWindow instance\n");
      delete m_widgetControl;
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CLatchButtonTest::createGraphics: Failed to open background window\n");
      delete m_bgWindow;
      m_bgWindow = (CBgWindow*)0;
      return false;
    }

  return true;
}

// Create a CLatchButton instance

CLatchButton *CLatchButtonTest::createButton(FAR const char *text)
{
  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CLatchButtonTest::createGraphics: Failed to get window size\n");
      return (CLatchButton *)NULL;
    }

  // Create a CNxString instance to contain the C string

  m_text = new CNxString(text);

  // Get the height and width of the text display area

  nxgl_coord_t stringWidth  = m_nxFont->getStringWidth(*m_text);
  nxgl_coord_t stringHeight = (nxgl_coord_t)m_nxFont->getHeight();

  // The default CLatchButton has borders enabled with thickness of the border
  // width.  Add twice the thickness of border the to the width and height. (We
  // could let CLatchButton do this for us by calling
  // CLatchButton::getPreferredDimensions())

  stringWidth  += 2 * 1;
  stringHeight += 2 * 1;

  // Pick an X/Y position such that the button will be centered in the display

  nxgl_coord_t buttonX;
  if (stringWidth >= windowSize.w)
    {
      buttonX = 0;
    }
  else
    {
      buttonX = (windowSize.w - stringWidth) >> 1;
    }

  nxgl_coord_t buttonY;
  if (stringHeight >= windowSize.h)
    {
      buttonY = 0;
    }
  else
    {
      buttonY = (windowSize.h - stringHeight) >> 1;
    }

  // Save the center position of the button for use by click and release

  m_center.x = buttonX + (stringWidth >> 1);
  m_center.y = buttonY + (stringHeight >> 1);

  // Now we have enough information to create the button

  return new CLatchButton(m_widgetControl, buttonX, buttonY, stringWidth, stringHeight, *m_text);
}

// Draw the button

void CLatchButtonTest::showButton(CLatchButton *button)
{
  button->enable();        // Un-necessary, the widget is enabled by default
  button->enableDrawing();
  button->redraw();
}

// Perform a simulated mouse click on the button.  This method injects
// the mouse click through the NX hierarchy just as would real mouse
// hardware.

void CLatchButtonTest::click(void)
{
  // nx_mousein is meant to be called by mouse handling software.
  // Here we just inject a left-button click directly in the center of
  // the button.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level mouse interface

  NXHANDLE handle = getServer();

  // Then inject the mouse click

  nx_mousein(handle, m_center.x, m_center.y, NX_MOUSE_LEFTBUTTON);
}

// The counterpart to click.  This simulates a button release through
// the same mechanism.

void CLatchButtonTest::release(void)
{
  // nx_mousein is meant to be called by mouse handling software.
  // Here we just inject a left-button click directly in the center of
  // the button.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level mouse interface

  NXHANDLE handle = getServer();

  // Then inject the mouse click

  nx_mousein(handle, m_center.x, m_center.y, NX_MOUSE_NOBUTTONS);
}

// Widget events are normally handled in a modal loop.
// However, for this case we know when there should be press and release
// events so we don't have to poll.  We can just perform a one pass poll.

void CLatchButtonTest::poll(CLatchButton *button)
{
  // Poll for mouse events

  m_widgetControl->pollEvents(button);
}
