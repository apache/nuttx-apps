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
#include <mqueue.h>
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
  class IBitmap;                        // Forward reference
  class CNxString;                      // Forward reference
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
  class CTwm4Nx;                        // Forward reference
  class CWindow;                        // Forward reference

  /**
   * Self-dragging versions of CLabel and CImage
   */

  class CIconLabel : public NXWidgets::CLabel
  {
    public:

      /**
       * Constructor for a label containing a string.
       *
       * @param pWidgetControl The controlling widget for the display
       * @param x The x coordinate of the text box, relative to its parent.
       * @param y The y coordinate of the text box, relative to its parent.
       * @param width The width of the textbox.
       * @param height The height of the textbox.
       * @param text Pointer to a string to display in the textbox.
       * @param style The style that the button should use.  If this is not
       *        specified, the button will use the global default widget
       *        style.
       */

      inline CIconLabel(NXWidgets::CWidgetControl *pWidgetControl,
                        nxgl_coord_t x, nxgl_coord_t y,
                        nxgl_coord_t width, nxgl_coord_t height,
                        const NXWidgets::CNxString &text,
                        NXWidgets::CWidgetStyle *style = (NXWidgets::CWidgetStyle *)0) :
      NXWidgets::CLabel(pWidgetControl, x, y, width, height, text, style)
      {
      }

      /**
       * Called when the widget is clicked.
       *
       * @param x The x coordinate of the click.
       * @param y The y coordinate of the click.
       */

      inline void onClick(nxgl_coord_t x, nxgl_coord_t y)
      {
        startDragging(x, y);
      }
  };

  class CIconImage : public NXWidgets::CImage
  {
    public:

      /**
       * Constructor for an image.
       *
       * @param pWidgetControl The controlling widget for the display
       * @param x The x coordinate of the image box, relative to its parent.
       * @param y The y coordinate of the image box, relative to its parent.
       * @param width The width of the textbox.
       * @param height The height of the textbox.
       * @param bitmap The source bitmap image.
       * @param style The style that the widget should use.  If this is not
       *        specified, the button will use the global default widget
       *        style.
       */

      inline CIconImage(NXWidgets::CWidgetControl *pWidgetControl,
                        nxgl_coord_t x, nxgl_coord_t y,
                        nxgl_coord_t width, nxgl_coord_t height,
                        FAR NXWidgets::IBitmap *bitmap,
                        NXWidgets::CWidgetStyle *style = (NXWidgets::CWidgetStyle *)0) :
      NXWidgets::CImage(pWidgetControl, x, y, width, height, bitmap, style)
      {
      }

      /**
       * Called when the widget is clicked.
       *
       * @param x The x coordinate of the click.
       * @param y The y coordinate of the click.
       */

      inline void onClick(nxgl_coord_t x, nxgl_coord_t y)
      {
        startDragging(x, y);
      }
  };

  /**
   * Container class that holds the Icon image and table widgets
   */

  class CIconWidget : public NXWidgets::CNxWidget,
                      public NXWidgets::CWidgetEventHandler
  {
    protected:
      FAR CTwm4Nx                   *m_twm4nx;         /**< Cached Twm4Nx session */
      FAR CWindow                   *m_parent;         /**< The parent window (for de-iconify) */
      mqd_t                          m_eventq;         /**< NxWidget event message queue */
      FAR NXWidgets::CWidgetControl *m_widgetControl;  /**< The controlling widget */

      // Dragging

      struct nxgl_point_s            m_dragPos;        /**< Last good icon position */
      struct nxgl_point_s            m_dragOffset;     /**< Offset from mouse to window origin */
      struct nxgl_size_s             m_dragCSize;      /**< The grab cursor size */
      bool                           m_dragging;       /**< Drag in-progress */
      bool                           m_collision;      /**< Current position collides */
      bool                           m_moved;          /**< Icon has been moved */

      /**
       * Called when the widget is clicked.
       *
       * @param x The x coordinate of the click.
       * @param y The y coordinate of the click.
       */

      inline void onClick(nxgl_coord_t x, nxgl_coord_t y)
      {
        startDragging(x, y);
      }

      /**
       * After the widget has been grabbed, it may be dragged then dropped,
       * or it may be simply "un-grabbed".  Both cases are handled here.
       *
       * NOTE: Unlike the other event handlers, this does NOT override any
       * virtual event handling methods.  It just combines some common event-
       * handling logic.
       *
       * @param e The event data.
       */

      void handleUngrabEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override the mouse button drag event.
       *
       * @param e The event data.
       */

      void handleDragEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override a drop event, triggered when the widget has been dragged-
       * and-dropped.
       *
       * @param e The event data.
       */

      void handleDropEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Handle a mouse click event.
       *
       * @param e The event data.
       */

      void handleClickEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Override the virtual CWidgetEventHandler::handleReleaseEvent.  This
       * event will fire when the widget is released.  isClicked() will
       * return false for the widget.
       *
       * @param e The event data.
       */

      void handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Handle a mouse button release event that occurred outside the bounds of
       * the source widget.
       *
       * @param e The event data.
       */

      void handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e);

      /**
       * Handle the EVENT_ICONWIDGET_GRAB event.  That corresponds to a left
       * mouse click on the icon widget.
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconGrab(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the EVENT_ICONWIDGET_DRAG event.  That corresponds to a mouse
       * movement when the icon is in a grabbed state.
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconDrag(FAR struct SEventMsg *eventmsg);

      /**
       * Handle the EVENT_ICONWIDGET_UNGRAB event.  The corresponds to a mouse
       * left button release while in the grabbed state
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool iconUngrab(FAR struct SEventMsg *eventmsg);

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

      ~CIconWidget(void);

      /**
       * Perform widget initialization that could fail and so it not appropriate
       * for the constructor
       *
       * @param parent The parent window.  Needed for de-iconification.
       * @param ibitmap The bitmap image representing the icon
       * @param title The icon title string
       * @return True is returned if the widget is successfully initialized.
       */

      bool initialize(FAR CWindow *parent, FAR NXWidgets::IBitmap *ibitmap,
                      FAR const NXWidgets::CNxString &title);

      /**
       * Insert the dimensions that this widget wants to have into the rect
       * passed in as a parameter.  All coordinates are relative to the
       * widget's parent.  Value is based on the length of the largest string
       * in the set of options.
       *
       * @param rect Reference to a rect to populate with data.
       */

      void getPreferredDimensions(NXWidgets::CRect &rect) const;

      /**
       * Handle ICON WIDGET events.
       *
       * @param eventmsg.  The received NxWidget ICON event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool event(FAR struct SEventMsg *eventmsg);
  };
}

#endif // ___APPS_INCLUDE_GRAPHICS_TWM4NX_CICONWIDGET_HXX
