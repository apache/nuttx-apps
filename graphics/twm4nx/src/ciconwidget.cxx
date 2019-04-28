/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ciconwidget.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <sys/types.h>
#include <cstdint>
#include <cstdbool>
#include <cfcntl>
#include <cerrno>
#include <mqueue.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"
#include "graphics/nxwidgets/ibitmap.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"

/////////////////////////////////////////////////////////////////////////////
// CIconWidget Definitions
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * Constructor.  Note that the group determines its width and height
 * from the position and dimensions of its children.
 *
 * @param widgetControl The controlling widget for the display.
 * @param x The x coordinate of the group.
 * @param y The y coordinate of the group.
 * @param style The style that the button should use.  If this is not
 *        specified, the button will use the global default widget
 *        style.
 */

CIconWidget::CIconWidget(FAR CTwm4Nx *twm4nx,
                         FAR NXWidgets::CWidgetControl *widgetControl,
                         nxgl_coord_t x, nxgl_coord_t y,
                         FAR NXWidgets::CWidgetStyle *style)
: CNxWidget(widgetControl, x, y, 0, 0, WIDGET_BORDERLESS, style)
{
  m_twm4nx         = twm4nx;        // Save the Twm4Nx session instance
  m_widgetControl  = widgetControl; // Save the widget control instance
  m_eventq         = (mqd_t)-1;     // No widget message queue yet

  // Configure the widget

  m_flags.borderless = true;        // The widget is borless (and transparent)
}

/**
 * Destructor.
 */

CIconWidget::~CIconWidget(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      (void)mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }
}

/**
 * Perform widget initialization that could fail and so it not appropriate
 * for the constructor
 *
 * @param ibitmap The bitmap image representing the icon
 * @param title The icon title string
 * @return True is returned if the widget is successfully initialized.
 */

bool CIconWidget::initialize(FAR NXWidgets::IBitmap *ibitmap,
                             FAR const NXWidgets::CNxString &title)
{
  // Open a message queue to send fully digested NxWidget events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();

  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      gerr("ERROR: Failed open message queue '%s': %d\n",
           mqname, errno);
      return false;
    }

  // Get the size of the Icon bitmap

  struct nxgl_size_s iconImageSize;
  iconImageSize.w = ibitmap->getWidth();
  iconImageSize.h = ibitmap->getHeight();

  // Get the size of the Icon name

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconFont = fonts->getIconFont();

  struct nxgl_size_s iconLabelSize;
  iconLabelSize.w  = iconFont->getStringWidth(title);
  iconLabelSize.h  = iconFont->getHeight();

  // Determine the new size of the containing widget

  struct nxgl_size_s iconWidgetSize;
  iconWidgetSize.w = ngl_max(iconImageSize.w, iconLabelSize.w);
  iconWidgetSize.h = iconImageSize.h + iconLabelSize.h + 2;

  // Update the widget size

  resize(iconWidgetSize.w, iconWidgetSize.h);

  // Get the position bitmap image, centering horizontally if the text
  // width is larger than the image width

  struct nxgl_point_s iconImagePos;
  iconImagePos.x = 0;
  iconImagePos.y = 0;

  if (iconLabelSize.w > (iconImageSize.w + 1))
    {
      iconImagePos.x = (iconLabelSize.w - iconImageSize.w) / 2;
    }

  // Create a new CImage to hold the bitmap image

  FAR NXWidgets::CImage *image =
    new NXWidgets::CImage(m_widgetControl, iconImagePos.x,
                          iconImagePos.y, iconImageSize.w, iconImageSize.h,
                          ibitmap, m_style);
  if (image == (FAR NXWidgets::CImage *)0)
    {
      gerr("ERROR: Failed to create image\n");
      return false;
    }

  image->setBorderless(true);

  // Get the position icon text, centering horizontally if the image
  // width is larger than the text width

  struct nxgl_point_s iconLabelPos;
  iconLabelPos.x = 0;
  iconLabelPos.y = iconImageSize.h + 2;

  if (iconImageSize.w > (iconLabelSize.w + 1))
    {
      iconLabelPos.x = (iconImageSize.w - iconLabelSize.w) / 2;
    }

  // Create a new CLabel to hold the icon text

  FAR NXWidgets::CLabel *label =
    new NXWidgets::CLabel(m_widgetControl, iconLabelPos.x, iconLabelPos.y,
                          iconLabelSize.w, iconLabelSize.h, title);
  if (label == (FAR NXWidgets::CLabel *)0)
    {
      gerr("ERROR: Failed to create icon label\n");
      delete image;
      return false;
    }

  label->setBorderless(true);

  // Add the CImage to to the containing widget

  image->addWidgetEventHandler(this);
  addWidget(image);

  label->addWidgetEventHandler(this);
  addWidget(label);
  return true;
}

/**
 * Insert the dimensions that this widget wants to have into the rect
 * passed in as a parameter.  All coordinates are relative to the
 * widget's parent.  Value is based on the length of the largest string
 * in the set of options.
 *
 * @param rect Reference to a rect to populate with data.
 */

void CIconWidget::getPreferredDimensions(NXWidgets::CRect &rect) const
{
  struct nxgl_size_s widgetSize;
  getSize(widgetSize);

  struct nxgl_point_s widgetPos;
  getPos(widgetPos);

  rect.setX(widgetPos.x);
  rect.setY(widgetPos.y);
  rect.setWidth(widgetSize.w);
  rect.setHeight(widgetSize.h);
}

/**
 * Handle ICONWIDGET events.
 *
 * @param eventmsg.  The received NxWidget ICON event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWidget::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_ICONWIDGET_GRAB:   /* Left click on icon.  Start drag */
        success = iconGrab(eventmsg);
        break;

      case EVENT_ICONWIDGET_DRAG:   /* Mouse movement while clicked */
        success = iconDrag(eventmsg);
        break;

      case EVENT_ICONWIDGET_UNGRAB: /* Left click release while dragging. */
        success = iconUngrab(eventmsg);
        break;

      default:
        success = false;
        break;
    }

  return success;
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

void CIconWidget::handleUngrabEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Exit the dragging state

  m_drag = false;

  // Generate the un-grab event

  struct SEventMsg msg;
  msg.eventID = EVENT_ICONWIDGET_UNGRAB;
  msg.pos.x   = e.getX();
  msg.pos.y   = e.getY();
  msg.delta.x = 0;
  msg.delta.y = 0;
  msg.context = EVENT_CONTEXT_ICON;
  msg.handler = (FAR CTwm4NxEvent *)0;
  msg.obj     = (FAR void *)this;

  // NOTE that we cannot block because we are on the same thread
  // as the message reader.  If the event queue becomes full then
  // we have no other option but to lose events.
  //
  // I suppose we could recurse and call Twm4Nx::dispatchEvent at
  // the risk of runaway stack usage.

  int ret = mq_send(m_eventq, (FAR const char *)&msg,
                    sizeof(struct SEventMsg), 100);
  if (ret < 0)
    {
      gerr("ERROR: mq_send failed: %d\n", ret);
    }
}

/**
 * Override the mouse button drag event.
 *
 * @param e The event data.
 */

void CIconWidget::handleDragEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We don't care which widget is being dragged, only that we are in the
  // dragging state.

  if (m_drag)
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_ICONWIDGET_DRAG;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.delta.x = e.getVX();
      msg.delta.y = e.getVY();
      msg.context = EVENT_CONTEXT_ICON;
      msg.handler = (FAR CTwm4NxEvent *)0;
      msg.obj     = (FAR void *)this;

      // NOTE that we cannot block because we are on the same thread
      // as the message reader.  If the event queue becomes full then
      // we have no other option but to lose events.
      //
      // I suppose we could recurse and call Twm4Nx::dispatchEvent at
      // the risk of runaway stack usage.

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          gerr("ERROR: mq_send failed: %d\n", ret);
        }
    }
}

/**
 * Override a drop event, triggered when the widget has been dragged-
 * and-dropped.
 *
 * @param e The event data.
 */

void CIconWidget::handleDropEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // When the Drop Event is received, both isClicked and isBeingDragged()
  // will return false.  No checks are performed.

  if (m_drag)
    {
      // Yes.. handle the drop event

      handleUngrabEvent(e);
    }
}

/**
 * Handle a mouse click event.
 *
 * @param e The event data.
 */

void CIconWidget::handleClickEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We don't care which component of the icon widget was clicked only that
  // we are not currently being dragged

  if (!m_drag)
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_ICONWIDGET_GRAB;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.delta.x = 0;
      msg.delta.y = 0;
      msg.context = EVENT_CONTEXT_ICON;
      msg.handler = (FAR CTwm4NxEvent *)0;
      msg.obj     = (FAR void *)this;

      // NOTE that we cannot block because we are on the same thread
      // as the message reader.  If the event queue becomes full then
      // we have no other option but to lose events.
      //
      // I suppose we could recurse and call Twm4Nx::dispatchEvent at
      // the risk of runaway stack usage.

      int ret = mq_send(m_eventq, (FAR const char *)&msg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          gerr("ERROR: mq_send failed: %d\n", ret);
        }
    }
}

/**
 * Override the virtual CWidgetEventHandler::handleReleaseEvent.  This
 * event will fire when the widget is released.  isClicked() will
 * return false for the widget.
 *
 * @param e The event data.
 */

void CIconWidget::handleReleaseEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Handle the case where a release event was received, but the
  // window was not dragged.

  if (m_drag)
    {
      // Handle the non-drag drop event

      handleUngrabEvent(e);
    }
}

/**
 * Handle a mouse button release event that occurred outside the bounds of
 * the source widget.
 *
 * @param e The event data.
 */

void CIconWidget::handleReleaseOutsideEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Handle the case where a release event was received, but the
  // window was not dragged.

  if (m_drag)
    {
      // Handle the non-drag drop event

      handleUngrabEvent(e);
    }
}

/**
 * Handle the EVENT_ICONWIDGET_GRAB event.  That corresponds to a left
 * mouse click on the icon widtet
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWidget::iconGrab(FAR struct SEventMsg *eventmsg)
{
  // Indicate that dragging has started.

  m_drag = false;

  // Get the icon position.

  struct nxgl_point_s widgetPos;
  getPos(widgetPos);

  // Determine the relative position of the icon and the mouse

  m_dragOffset.x = widgetPos.x - eventmsg->pos.x;
  m_dragOffset.y = widgetPos.y - eventmsg->pos.y;

  // Select the grab cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_GBCURSOR_IMAGE);

  // Remember the grab cursor size

  m_dragCSize.w = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.w;
  m_dragCSize.h = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.h;
  return true;
}

/**
 * Handle the EVENT_ICONWIDGET_DRAG event.  That corresponds to a mouse
 * movement when the icon is in a grabbed state.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWidget::iconDrag(FAR struct SEventMsg *eventmsg)
{
  if (m_drag)
    {
      // Calculate the new icon position

      struct nxgl_point_s newpos;
      newpos.x = eventmsg->pos.x + m_dragOffset.x;
      newpos.y = eventmsg->pos.y + m_dragOffset.y;

      // Keep the icon on the display (at least enough of it so that we
      // can still grab it)

      struct nxgl_size_s displaySize;
      m_twm4nx->getDisplaySize(&displaySize);

      if (newpos.x < 0)
        {
          newpos.x = 0;
        }
      else if (newpos.x + m_dragCSize.w > displaySize.w)
        {
          newpos.x = displaySize.w - m_dragCSize.w;
        }

      if (newpos.y < 0)
        {
          newpos.y = 0;
        }
      else if (newpos.y + m_dragCSize.h > displaySize.h)
        {
          newpos.y = displaySize.h - m_dragCSize.h;
        }

      // Set the new window position

      return moveTo(newpos.x, newpos.y);
    }

  return false;
}

/**
 * Handle the EVENT_ICONWIDGET_UNGRAB event.  The corresponds to a mouse
 * left button release while in the grabbed state.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWidget::iconUngrab(FAR struct SEventMsg *eventmsg)
{
  // One last position update

  if (!iconDrag(eventmsg))
    {
      return false;
    }

  // Indicate no longer dragging

  m_drag = false;

  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
  return false;
}

/**
 * Draw the area of this widget that falls within the clipping region.
 * Called by the redraw() function to draw all visible regions.
 * @param port The NXWidgets::CGraphicsPort to draw to.
 * @see redraw()
 */

void CIconWidget::drawContents(NXWidgets::CGraphicsPort *port)
{
  port->drawFilledRect(getX(), getY(), getWidth(), getHeight(),
                       getBackgroundColor());
}
