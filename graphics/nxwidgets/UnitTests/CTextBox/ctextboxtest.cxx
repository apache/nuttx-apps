/////////////////////////////////////////////////////////////////////////////
// apps/graphics/nxwidgets/UnitTests/CTextBox/ctextboxtest.cxx
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
#include <unistd.h>
#include <debug.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxfonts.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/ctextboxtest.hxx"
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
// CTextBoxTest Method Implementations
/////////////////////////////////////////////////////////////////////////////

// CTextBoxTest Constructor

CTextBoxTest::CTextBoxTest()
{
  m_bgWindow = (CBgWindow *)NULL;
  m_nxFont   = (CNxFont *)NULL;
  m_text     = (CNxString *)NULL;
}

// CTextBoxTest Descriptor

CTextBoxTest::~CTextBoxTest()
{
  disconnect();
}

// Connect to the NX server

bool CTextBoxTest::connect(void)
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
          printf("CTextBoxTest::connect: Failed to create the default font\n");
        }

      // Set the background color

      if (!setBackgroundColor(CONFIG_CTEXTBOXTEST_BGCOLOR))
        {
          printf("CTextBoxTest::connect: setBackgroundColor failed\n");
        }
    }

  return nxConnected;
}

// Disconnect from the NX server

void CTextBoxTest::disconnect(void)
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

bool CTextBoxTest::createWindow(void)
{
  // Initialize the widget control using the default style

  m_widgetControl = new CWidgetControl((CWidgetStyle *)NULL);

  // Get an (uninitialized) instance of the background window as a class
  // that derives from INxWindow.

  m_bgWindow = getBgWindow(m_widgetControl);
  if (!m_bgWindow)
    {
      printf("CTextBoxTest::createGraphics: Failed to create CBgWindow instance\n");
      delete m_widgetControl;
      return false;
    }

  // Open (and initialize) the window

  bool success = m_bgWindow->open();
  if (!success)
    {
      printf("CTextBoxTest::createGraphics: Failed to open background window\n");
      delete m_bgWindow;
      m_bgWindow = (CBgWindow*)0;
      return false;
    }

  return true;
}

// Create a CTextBox instance

CTextBox *CTextBoxTest::createTextBox(void)
{
  // Get the width of the display

  struct nxgl_size_s windowSize;
  if (!m_bgWindow->getSize(&windowSize))
    {
      printf("CTextBoxTest::createGraphics: Failed to get window size\n");
      return (CTextBox *)NULL;
    }

  // Create an empty CNxString instance to contain the C string

  m_text = new CNxString();

  // Get the height and width of the text display area.  The width
  // is half of the display width and the height is optimized for
  // the font height

  nxgl_coord_t textBoxWidth =  windowSize.w >> 1;
  nxgl_coord_t textBoxHeight = (nxgl_coord_t)m_nxFont->getHeight();

  // The default CTextBox has borders enabled with thickness of one.
  // Add twice the thickness of the border to the height.

  textBoxHeight += 2 * 1;

  // Pick an X/Y position such that the label will be centered in the display

  nxgl_coord_t labelX = windowSize.w >> 2;
  nxgl_coord_t labelY = (windowSize.h - textBoxHeight) >> 1;

  // Now we have enough information to create the label

  return new CTextBox(m_widgetControl, labelX, labelY, textBoxWidth, textBoxHeight, *m_text);
}

// Draw the label

void CTextBoxTest::showTextBox(CTextBox *label)
{
  label->enable();
  label->enableDrawing();
  label->redraw();
}

// Inject simulated keyboard characters into NX.

void CTextBoxTest::injectChars(CTextBox *textbox, int nCh, FAR const uint8_t *string)
{
  // nx_kbdin is meant to be called by keyboard handling software.
  // Here we just inject the string under unit test control.

  // First, get the server handle.  Graphics software will never care
  // about the server handle.  Here we need it to get access to the
  // low-level keyboard interface

  NXHANDLE handle = getServer();

  // The widget must have focus to receive keyboard input

  m_widgetControl->setFocusedWidget(textbox);

  // Inject the string one character at a time to get a more realistic
  // simulation of keyboard behavior.  Note the API nx_kbdin() could
  // be used to inject the string in its entirety.

  for (int i = 0; i < nCh; i++)
    {
      // Inject the next character

      nx_kbdchin(handle, string[i]);

      // Widget events are normally handled in a modal loop.
      // However, for this case we know when there should be keyboard events pending,
      // events so we don't have to poll repeatedly.  We can just perform a one pass
      // poll

      m_widgetControl->pollEvents(textbox);

      // Sleep a bit, just for the effect (this also gives the X server loop a
      // chance to run in the simulated environment.

      usleep(500*1000);
    }
}
