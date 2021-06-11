/****************************************************************************
 * apps/include/graphics/nxwidgets/ckeypad.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CKEYPAD_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CKEYPAD_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cbuttonarray.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NXWidgets
{
  /**
   * Forward references
   */

  class CWidgetControl;
  class CWidgetStyle;

  /**
   * Extends the CButtonArray class to support a alphanumeric keypad.
   */

  class CKeypad : public CButtonArray
  {
  protected:
    NXHANDLE m_hNxServer;  /**< NX server handle */
    bool     m_numeric;    /**< True: Numeric keypad, False: Alpha */

    /**
     * Configure the keypad for the currently selected display mode.
     */

    void configureKeypadMode(void);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CKeypad(const CKeypad &keypad) : CButtonArray(keypad) { }

  public:

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

    CKeypad(CWidgetControl *pWidgetControl, NXHANDLE hNxServer,
              nxgl_coord_t x, nxgl_coord_t y,
              nxgl_coord_t width, nxgl_coord_t height,
              CWidgetStyle *style = (CWidgetStyle *)NULL);

    /**
     * CKeypad Destructor.
     */

    inline ~CKeypad(void) {}

    /**
     * Returns the current keypad display mode
     *
     * @return True: keypad is in numeric mode.  False: alphanumeric.
     */

    inline const bool isNumericKeypad(void) const
    {
      return m_numeric;
    }

    /**
     * Returns the current keypad display mode
     *
     * @param mode True: put keypad in numeric mode.  False: in alphanumeric.
     */

    inline void setKeypadMode(bool numeric)
    {
      m_numeric = numeric;
      configureKeypadMode();
    }

    /**
     * Catch button clicks.
     *
     * @param x The x coordinate of the click.
     * @param y The y coordinate of the click.
     */

    virtual void onClick(nxgl_coord_t x, nxgl_coord_t y);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CKEYPAD_HXX
