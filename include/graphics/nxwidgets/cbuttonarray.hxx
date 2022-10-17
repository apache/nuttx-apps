/****************************************************************************
 * apps/include/graphics/nxwidgets/cbuttonarray.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBUTTONARRAY_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBUTTONARRAY_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/clabel.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"

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
  class CNxString;

  /**
   * Manages a two-dimensional array of buttons as one widget.  A two-
   * dimensional array of buttons might also be called a "keyboard".
   */

  class CButtonArray : public CNxWidget
  {
  protected:
    uint8_t       m_buttonColumns; /**< The number of columns in one row */
    uint8_t       m_buttonRows;    /**< The number buttons in one column */
    bool          m_redrawButton;  /**< True: Redraw button; False: redraw all */
    bool          m_cursorOn;      /**< Cursor on; highlighted button displayed */
    bool          m_cursorChange;  /**< True: Redraw cursor button only */
    nxgl_coord_t  m_buttonWidth;   /**< The width of one button in pixels */
    nxgl_coord_t  m_buttonHeight;  /**< The height of one button in rows */
    nxgl_coord_t  m_clickX;        /**< The X position of the last clicked button */
    nxgl_coord_t  m_clickY;        /**< The Y position of the last clicked button */
    uint8_t       m_cursorColumn;  /**< The column index of the highlighted button */
    uint8_t       m_cursorRow;     /**< The row index of the highlighted button */
    CNxString    *m_buttonText;    /**< Text for each button */

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawContents(CGraphicsPort *port);

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     *
     * @param port The CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawBorder(CGraphicsPort *port);

    /**
     * Redraw only one button
     *
     * @param port The CGraphicsPort to draw to.
     * @param column The button column index
     * @param row The button row index
     * @param useClicked Draw the button using the 'clicked' button style,
     *        regardless of the actual button state.
     * @see onClick() and onRelease()
     */

    virtual void drawButton(CGraphicsPort *port, int column, int row, bool useClicked);

    /**
     * Redraws the button.
     *
     * @param x The x coordinate of the click.
     * @param y The y coordinate of the click.
     */

    virtual void onClick(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Raises an action.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onPreRelease(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Redraws the button.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onRelease(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Redraws the button.
     *
     * @param x The x coordinate of the mouse.
     * @param y The y coordinate of the mouse.
     */

    virtual void onReleaseOutside(nxgl_coord_t x, nxgl_coord_t y);

    /**
     * Convert an X/Y position to a button column/row index
     *
     * @param x The x position
     * @param y The y position
     * @param column The location to return the column index of the button
     *    of interest
     * @param row The location to return the row index of the button of
     *    interest
     * @return false is the position is invalid
     */

    virtual bool posToButton(nxgl_coord_t x, nxgl_coord_t y, int &column, int &row);

    /**
     * Updates the GUI after the text has changed.
     */

    virtual void onTextChange(void);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CButtonArray(const CButtonArray &button) : CNxWidget(button) { }

  public:

    /**
     * Constructor for an array of buttons.
     *
     * @param pWidgetControl The widget control for the display.
     * @param x The x coordinate of the button array, relative to its parent.
     * @param y The y coordinate of the button array, relative to its parent.
     * @param buttonColumns The number of buttons in one row of the button array
     * @param buttonRows The number of buttons in one column of the button array
     * @param buttonWidth The width of one button
     * @param buttonHeight The height of one button
     * @param style The style that the button should use.  If this is not
     *        specified, the button will use the global default widget
     *        style.
     */

    CButtonArray(CWidgetControl *pWidgetControl,
                 nxgl_coord_t x, nxgl_coord_t y,
                 uint8_t buttonColumns, uint8_t buttonRows,
                 nxgl_coord_t buttonWidth, nxgl_coord_t buttonHeight,
                 CWidgetStyle *style = NULL);

    /**
     * CButtonArray Destructor.
     */

    ~CButtonArray(void);

    /**
     * Resize the array of buttons.  Button labels will be removed and will
     * have to be reapplied in the new geometry.
     *
     * @param buttonColumns The number of buttons in one row of the button array
     * @param buttonRows The number of buttons in one column of the button array
     * @param buttonWidth The width of one button
     * @param buttonHeight The height of one button
     */

    bool resizeArray(uint8_t buttonColumns, uint8_t buttonRows,
                     nxgl_coord_t buttonWidth, nxgl_coord_t buttonHeight);

    /**
     * Returns the string shown in the label.
     *
     * @param column The column index of the button of interest
     * @param row The row index of the button of interest
     * @return The label's text.
     */

    virtual const CNxString &getText(int column, int row) const;

    /**
     * Set the text displayed in the label.
     *
     * @param column The column index of the button to set
     * @param row The row index of the button to set
     * @param text String to display.
     */

    virtual void setText(int column, int row, const CNxString &text);

    /**
     * Return the position of the last clicked button (0,0 will be returned
     * the no button has every been clicked).  The button at this position
     * is currently clicked then, in addition, return true.
     *
     * @param column The location to return the column index of the button
     *    of interest
     * @param row The location to return the row index of the button of
     *    interest
     * @return True if any button in the array is clicked
     */

    virtual bool isButtonClicked(int &column, int &row) const;

    /**
     * Check if this specific button in the array is clicked
     *
     * @param column The column of the button to check.
     * @param row The row of the button to check.
     * @return True if this button is clicked
     */

    virtual bool isThisButtonClicked(int column, int row) const;

    /**
     * Control the cursor state.
     *
     * @param cursorOn True(1), the current cursor position will be highlighted
     */

    virtual void cursor(bool cursorOn);

    /**
     * Return the current cursor position (button indices) and an indication
     * if the button at the cursor is currently highlighted.
     *
     * @param column The location to return the column index of the button
     *    of interest
     * @param row The location to return the row index of the button of
     *    interest
     * @return True if the cursor is enabled and the button is highlighted
     */

    virtual bool getCursorPosition(int &column, int &row) const;

    /**
     * Set the cursor position (button indices).  Note that the cursor
     * does not have to be enabled to set the position.
     *
     * @param column The column index of the button of interest
     * @param row The row index of the button of interest
     * @return True if the cursor position is valid
     */

    virtual bool setCursorPosition(int column, int row);

    /**
     * Check if this specific button in the array is at the cursor position
     * and highlighted.
     *
     * @param column The column of the button to check.
     * @param row The row of the button to check.
     * @return True if this button is at the cursor position and highlighted.
     */

    virtual bool isCursorPosition(int column, int row) const;

    /**
     * Insert the dimensions that this widget wants to have into the rect
     * passed in as a parameter.  All coordinates are relative to the
     * widget's parent.
     *
     * @param rect Reference to a rect to populate with data.
     */

    virtual void getPreferredDimensions(CRect &rect) const;

    /**
     * Sets the font.
     *
     * @param font A pointer to the font to use.
     */

    virtual void setFont(CNxFont *font);
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWIDGETS_CBUTTONARRAY_HXX
