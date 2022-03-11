/****************************************************************************
 * apps/graphics/nxwidgets/src/ckeypad.cxx
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

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/ckeypad.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

// The geometry of the button array

#define BUTTONARRAY_NCOLUMNS      4
#define BUTTONARRAY_NROWS         7

/****************************************************************************
 * Private Data
 ****************************************************************************/

using namespace NXWidgets;

static FAR const char *g_alphaLabels[BUTTONARRAY_NCOLUMNS*BUTTONARRAY_NROWS] = {
 "=>", "A", "B", "<DEL",
 "C", "D", "E", "F",
 "G", "H", "I", "J",
 "K", "L", "M", "N",
 "O", "P", "Q", "R",
 "S", "T", "U", "V",
 "W", "X", "Y", "Z"
};

static FAR const char *g_numLabels[BUTTONARRAY_NCOLUMNS*BUTTONARRAY_NROWS] = {
 "=>", "0", "1", "<DEL",
 "2", "3", "4", "5",
 "6", "7", "8", "9",
 "-", "#", "*", "&",
 "?", "!", "@", "%",
 ":", ",", ".", "/",
 "\"", "'", "(", ")"
};

/****************************************************************************
 * Method Implementations
 ****************************************************************************/

/**
 * Constructor for buttons that display a string.
 *
 * @param pWidgetControl The widget control for the display.
 * @param hNxServer The NX server that will receive the keyboard input
 * @param x The x coordinate of the keypad, relative to its parent.
 * @param y The y coordinate of the keypad, relative to its parent.
 * @param width The width of the keypad
 * @param height The height of the keypad
 * @param style The style that the button should use.  If this is not
 *        specified, the button will use the global default widget
 *        style.
 */

CKeypad::CKeypad(CWidgetControl *pWidgetControl, NXHANDLE hNxServer,
                 nxgl_coord_t x, nxgl_coord_t y,
                 nxgl_coord_t width, nxgl_coord_t height,
                  CWidgetStyle *style)
: CButtonArray(pWidgetControl, x, y, 4, 7, width / BUTTONARRAY_NCOLUMNS,
               height / BUTTONARRAY_NROWS, style)
{
  m_hNxServer = hNxServer;

  // Force the height and width to be an exact multiple of the button height and width

  resize(m_buttonWidth * m_buttonColumns, m_buttonHeight * m_buttonRows);

  // Start in alphabetic mode

  setKeypadMode(false);
}

/**
 * Catch button clicks.
 *
 * @param x The x coordinate of the click.
 * @param y The y coordinate of the click.
 */

void  CKeypad::onClick(nxgl_coord_t x, nxgl_coord_t y)
{
  // Perform the redraw

  CButtonArray::onClick(x, y);

  // The button should now be clicked

  int column;
  int row;

  if (isButtonClicked(column, row))
    {
      // The left arrow at (0,0) is a special case

      char ch;
      if (column == 0 && row == 0)
        {
           m_widgetControl->newCursorControlEvent(CURSOR_RIGHT);
           return;
        }

      // The backspace at (3,0) is a special case

      else if (column == 3 && row == 0)
        {
          ch = KEY_CODE_BACKSPACE;
        }

      // Otherwise, just grab the character for the arrays

      else
        {
          int index = row * BUTTONARRAY_NCOLUMNS + column;
          FAR const char *ptr;
          if (m_numeric)
            {
              ptr = g_numLabels[index];
            }
          else
            {
              ptr = g_alphaLabels[index];
            }

          ch = ptr[0];
        }

      // Then inject the keyboard input into the NX server.  The NX
      // server will determine which window receives the input.
      // The ultimate recipient will be the widget in the top window
      // that has focus.

      nx_kbdchin(m_hNxServer, (uint8_t)ch);
    }
}

/**
 * Configure the keypad for the currently selected display mode.
 */

void CKeypad::configureKeypadMode(void)
{
  // Add the labels to each button.  Each call to setText would
  // cause a text change event and, hence, redrawing of the

  FAR const char **ptr = m_numeric ? g_numLabels : g_alphaLabels;
  for (int j = 0; j < BUTTONARRAY_NROWS; j++)
    {
      for (int i = 0; i < BUTTONARRAY_NCOLUMNS; i++)
        {
          // setText does this.  But each call to setText would
          // cause a text change even and, hence, redrawing of
          // the whole keypad.  So we access the protected data
          // directly and redraw only once.

          CNxString string = *ptr++;
          m_buttonText[j * m_buttonColumns + i] = string;
        }
    }

  // Then redraw the display

  onTextChange();
}
