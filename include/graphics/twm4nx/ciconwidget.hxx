/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/ciconwidget.hxx
// Represents on desktop icon
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef ___APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIDGET_HXX
#define ___APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIDGET_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdint>
#include <cstdbool>
#include <debug.h>

#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/cnxwidget.hxx"
#include "graphics/nxwidgets/cbutton.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwidgeteventhandler.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class CNxWidget;                      // Forward reference
  class CWidgetStyle;                   // Forward reference
  class CWidgetControl;                 // Forward reference
  class CGraphicsPort;                  // Forward reference
  class CWidgetEventHandler;            // Forward reference
  class CWidgetEventArgs;               // Forward reference
  class CRlePaletteBitmap;              // Forward reference
  class CImage;                         // Forward reference
  class CLabel;                         // Forward reference
}

namespace Twm4Nx
{
  class FAR CTwm4Nx;                    // Forward reference

  /**
   * Container class that holds the Icon image and table widgets
   */

  class CIconWidget : public NXWidgets::CNxWidget,
                      public NXWidgets::CWidgetEventHandler
  {
    protected:
      FAR CTwm4Nx                   *m_twm4nx;         /**< Cached Twm4Nx session */
      FAR NXWidgets::CWidgetControl *m_widgetControl;  /**< The controlling widget */
      FAR NXWidgets::CWidgetStyle   *m_style;          /**< Widget style */

    /**
     * Draw the area of this widget that falls within the clipping region.
     * Called by the redraw() function to draw all visible regions.
     * @param port The NXWidgets::CGraphicsPort to draw to.
     * @see redraw()
     */

    virtual void drawContents(NXWidgets::CGraphicsPort* port);

    /**
     * Copy constructor is protected to prevent usage.
     */

    inline CIconWidget(const CIconWidget &radioButtonGroup) :
      CNxWidget(radioButtonGroup) { }

  public:

    /**
     * Constructor.  Note that the group determines its width and height
     * from the position and dimensions of its children.
     *
     * @param twm4nx The Twm4Nx session object
     * @param widgetControl The controlling widget for the display.
     * @param x The x coordinate of the group.
     * @param y The y coordinate of the group.
     * @param style The style that the button should use.  If this is not
     *        specified, the button will use the global default widget
     *        style.
     */

    CIconWidget(FAR CTwm4Nx *twm4nx,
                FAR NXWidgets::CWidgetControl *widgetControl,
                nxgl_coord_t x, nxgl_coord_t y,
                FAR NXWidgets::CWidgetStyle *style =
                (FAR NXWidgets::CWidgetStyle *)0);

    /**
     * Destructor.
     */

    ~CIconWidget(void)
    {
    }

    /**
     * Perform widget initialization that could fail and so it not appropriate
     * for the constructor
     *
     * @param cbitmp The bitmap image representing the icon
     * @param title The icon title string
     * @return True is returned if the widget is successfully initialized.
     */

    bool initialize(FAR NXWidgets::CRlePaletteBitmap *cbitmap,
                    FAR NXWidgets::CNxString &title);

    /**
     * Insert the dimensions that this widget wants to have into the rect
     * passed in as a parameter.  All coordinates are relative to the
     * widget's parent.  Value is based on the length of the largest string
     * in the set of options.
     *
     * @param rect Reference to a rect to populate with data.
     */

    virtual void getPreferredDimensions(NXWidgets::CRect &rect) const;

    /**
     * Handle a mouse click event.
     *
     * @param e The event data.
     */

    virtual void handleClickEvent(const NXWidgets::CWidgetEventArgs &e);

    /**
     * Handle a mouse double-click event.
     *
     * @param e The event data.
     */

    virtual void handleDoubleClickEvent(const NXWidgets::CWidgetEventArgs &e);

    /**
     * Handle a mouse button release event that occurred within the bounds of
     * the source widget.
     * @param e The event data.
     */

    virtual void handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e);

    /**
     * Handle a mouse button release event that occurred outside the bounds of
     * the source widget.
     *
     * @param e The event data.
     */

    virtual void handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e);
  };
}

#endif // ___APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIDGET_HXX
