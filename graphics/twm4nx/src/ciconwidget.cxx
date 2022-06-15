/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ciconwidget.cxx
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
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdint>
#include <cstdbool>
#include <cerrno>

#include <sys/types.h>
#include <fcntl.h>
#include <mqueue.h>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cgraphicsport.hxx"
#include "graphics/nxwidgets/cwidgeteventargs.hxx"
#include "graphics/nxwidgets/ibitmap.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cbackground.hxx"
#include "graphics/twm4nx/ciconwidget.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor Definitions
/////////////////////////////////////////////////////////////////////////////

// No additional vertical spacing is necessary because there is plenty of
// space in the maximum font height

#define ICONWIDGET_IMAGE_VSPACING 2  // Lines between image and upper text
#define ICONWIDGET_TEXT_VSPACING  0  // Lines between upper and lower text

/////////////////////////////////////////////////////////////////////////////
// CIconWidget Method Implementations
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
  m_twm4nx           = twm4nx;           // Save the Twm4Nx session instance
  m_parent           = (FAR CWindow *)0; // No parent window yes
  m_widgetControl    = widgetControl;    // Save the widget control instance
  m_eventq           = (mqd_t)-1;        // No widget message queue yet

  // Dragging

  m_dragging         = false;            // No drag in-progress */
  m_moved            = false;            // Icon has not been moved */

  // Configure the widget

  setBorderless(true);                   // The widget is borderless (and transparent)
  setDraggable(true);                    // This widget may be dragged
  enable();                              // Enable the widget
  setRaisesEvents(true);                 // Enable event firing
  disableDrawing();                      // No drawing yet
}

/**
 * Destructor.
 */

CIconWidget::~CIconWidget(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }
}

/**
 * Perform widget initialization that could fail and so it not appropriate
 * for the constructor
 *
 * @param parent The parent window.  Needed for de-iconification.
 * @param ibitmap The bitmap image representing the icon
 * @param title The icon title string
 * @return True is returned if the widget is successfully initialized.
 */

bool CIconWidget::initialize(FAR CWindow *parent,
                             FAR NXWidgets::IBitmap *ibitmap,
                             FAR const NXWidgets::CNxString &title)
{
  // Open a message queue to send fully digested NxWidget events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();

  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // Get the size of the Icon bitmap

  struct nxgl_size_s iconImageSize;
  iconImageSize.w = ibitmap->getWidth();
  iconImageSize.h = ibitmap->getHeight();

  // Get the size of the Icon title

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconFont = fonts->getIconFont();

  struct nxgl_size_s titleSize;
  titleSize.w  = iconFont->getStringWidth(title);
  titleSize.h  = iconFont->getHeight();

  // Divide long Icon names into two lines

  FAR NXWidgets::CNxString topString;
  struct nxgl_size_s iconTopLabelSize;

  FAR NXWidgets::CNxString bottomString;
  struct nxgl_size_s iconBottomLabelSize;
  iconBottomLabelSize.w = 0;
  iconBottomLabelSize.h = 0;

  int sIndex = title.indexOf(' ');
  if (titleSize.w <= iconImageSize.w || sIndex < 0)
    {
      // The icon title is short or contains no simple dividing point

      topString.setText(title);
      iconTopLabelSize.w = titleSize.w;
      iconTopLabelSize.h = titleSize.h;
    }
  else
    {
      // Try dividing the string

      nxgl_coord_t halfWidth = titleSize.w / 2;

      nxgl_coord_t sWidth =
        iconFont->getStringWidth(title.subString(0, sIndex));

      nxgl_coord_t error = halfWidth - sWidth;
      if (error < 0)
        {
          error = -error;
        }

      int index;
      while ((index = title.indexOf(' ', sIndex + 1)) > 0)
        {
          // Which is the better division point?  index or SIndex?

          nxgl_coord_t width =
            iconFont->getStringWidth(title.subString(0, index));

          nxgl_coord_t tmperr = halfWidth - width;
          if (tmperr < 0)
            {
              tmperr = -tmperr;
            }

          // Break out if the errors are becoming larger

          if (tmperr >= error)
            {
              break;
            }

          error  = tmperr;
          sIndex = index;
        }

      topString.setText(title.subString(0, sIndex));
      iconTopLabelSize.w    = iconFont->getStringWidth(topString);
      iconTopLabelSize.h    = iconFont->getHeight();

      bottomString.setText(title.subString(sIndex + 1));
      iconBottomLabelSize.w = iconFont->getStringWidth(bottomString);
      iconBottomLabelSize.h = iconFont->getHeight();
    }

  // Determine the new size of the containing widget

  nxgl_coord_t maxLabelWidth = ngl_max(iconTopLabelSize.w,
                                       iconBottomLabelSize.w);

  struct nxgl_size_s iconWidgetSize;
  iconWidgetSize.w = ngl_max(iconImageSize.w, maxLabelWidth);
  iconWidgetSize.h = iconImageSize.h + iconTopLabelSize.h +
                     ICONWIDGET_IMAGE_VSPACING;

  // Check if there is a bottom label

  if (iconBottomLabelSize.h > 0)
    {
      iconWidgetSize.h += iconBottomLabelSize.h + ICONWIDGET_TEXT_VSPACING;
    }

  // Update the widget size

  resize(iconWidgetSize.w, iconWidgetSize.h);

  // Get the position bitmap image, centering horizontally if the text
  // width is larger than the image width

  struct nxgl_point_s iconImagePos;
  iconImagePos.x = 0;
  iconImagePos.y = 0;

  if (iconImageSize.w < (maxLabelWidth + 1))
    {
      iconImagePos.x = (maxLabelWidth - iconImageSize.w) / 2;
    }

  // Create a new CIconLabel to hold the bitmap image

  FAR CIconImage *image =
    new CIconImage(m_widgetControl, iconImagePos.x,
                   iconImagePos.y, iconImageSize.w, iconImageSize.h,
                   ibitmap, &m_style);
  if (image == (FAR CIconImage *)0)
    {
      twmerr("ERROR: Failed to create image\n");
      return false;
    }

  // Configure the image

  image->setBorderless(true);
  image->enable();
  image->disableDrawing();
  image->setRaisesEvents(true);
  image->setDraggable(true);

  // Add the CIconImage to the containing widget

  image->addWidgetEventHandler(this);
  addWidget(image);

  // Get the position upper icon title, centering horizontally if the image
  // width is larger than the text width

  struct nxgl_point_s iconTopLabelPos;
  iconTopLabelPos.x = 0;
  iconTopLabelPos.y = iconImageSize.h + ICONWIDGET_IMAGE_VSPACING;

  if (iconWidgetSize.w > (iconTopLabelSize.w + 1))
    {
      iconTopLabelPos.x = (iconWidgetSize.w - iconTopLabelSize.w) / 2;
    }

  // Create a new CIconLabel to hold the upper icon title

  FAR CIconLabel *topLabel =
    new CIconLabel(m_widgetControl, iconTopLabelPos.x,
                   iconTopLabelPos.y, iconTopLabelSize.w,
                   iconTopLabelSize.h, topString, &m_style);
  if (topLabel == (FAR CIconLabel *)0)
    {
      twmerr("ERROR: Failed to create icon topLabel\n");
      delete image;
      return false;
    }

  // Configure the icon topLabel

  topLabel->setFont(iconFont);
  topLabel->setBorderless(true);
  topLabel->enable();
  topLabel->disableDrawing();
  topLabel->setRaisesEvents(true);
  topLabel->setDraggable(true);

  // Add the top label to the containing widget

  topLabel->addWidgetEventHandler(this);
  addWidget(topLabel);

  // Check if there is a bottom label

  if (iconBottomLabelSize.h > 0)
    {
      // Get the position lower icon title, centering horizontally if the image
      // width is larger than the text width

      struct nxgl_point_s iconBottomLabelPos;
      iconBottomLabelPos.x = 0;
      iconBottomLabelPos.y = iconImageSize.h + iconTopLabelSize.h +
                             ICONWIDGET_IMAGE_VSPACING +
                             ICONWIDGET_TEXT_VSPACING;

      if (iconWidgetSize.w > (iconBottomLabelSize.w + 1))
        {
          iconBottomLabelPos.x = (iconWidgetSize.w - iconBottomLabelSize.w) / 2;
        }

      // Create a new CIconLabel to hold the lower icon title

      FAR CIconLabel *bottomLabel =
        new CIconLabel(m_widgetControl, iconBottomLabelPos.x,
                       iconBottomLabelPos.y, iconBottomLabelSize.w,
                       iconBottomLabelSize.h, bottomString,&m_style);
      if (bottomLabel == (FAR CIconLabel *)0)
        {
          twmerr("ERROR: Failed to create icon bottomLabel\n");
          delete topLabel;
          delete image;
          return false;
        }

      // Configure the icon bottomLabel

      bottomLabel->setFont(iconFont);
      bottomLabel->setBorderless(true);
      bottomLabel->enable();
      bottomLabel->disableDrawing();
      bottomLabel->setRaisesEvents(true);
      bottomLabel->setDraggable(true);

      // Add the top label to the containing widget

      bottomLabel->addWidgetEventHandler(this);
      addWidget(bottomLabel);
    }

  // Save the parent window

  m_parent = parent;
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
  // Generate the un-grab event

  struct SEventMsg msg;
  msg.eventID = EVENT_ICONWIDGET_UNGRAB;
  msg.obj     = (FAR void *)this;
  msg.pos.x   = e.getX();
  msg.pos.y   = e.getY();
  msg.context = EVENT_CONTEXT_ICONWIDGET;
  msg.handler = (FAR void *)0;

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
      twmerr("ERROR: mq_send failed: %d\n", errno);
    }
}

/**
 * Override the mouse button drag event.
 *
 * @param e The event data.
 */

void CIconWidget::handleDragEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // We don't care which component of the icon widget was clicked only that
  // we are not currently being dragged

  if (m_dragging)
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_ICONWIDGET_DRAG;
      msg.obj     = (FAR void *)this;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.context = EVENT_CONTEXT_ICONWIDGET;
      msg.handler = (FAR void *)0;

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
          twmerr("ERROR: mq_send failed: %d\n", errno);
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

  if (m_dragging)
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

  if (!m_dragging)
    {
      // Generate the event

      struct SEventMsg msg;
      msg.eventID = EVENT_ICONWIDGET_GRAB;
      msg.obj     = (FAR void *)this;
      msg.pos.x   = e.getX();
      msg.pos.y   = e.getY();
      msg.context = EVENT_CONTEXT_ICONWIDGET;
      msg.handler = (FAR void *)0;

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
          twmerr("ERROR: mq_send failed: %d\n", errno);
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

  if (m_dragging)
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

  if (m_dragging)
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
  // Indicate that dragging has started but the icon has not
  // yet been moved.

  m_dragging  = true;
  m_moved     = false;
  m_collision = false;

  // Get the icon position.

  getPos(m_dragPos);

  // Determine the relative position of the icon and the mouse

  m_dragOffset.x = m_dragPos.x - eventmsg->pos.x;
  m_dragOffset.y = m_dragPos.y - eventmsg->pos.y;

#ifdef CONFIG_TWM4NX_MOUSE
  // Select the grab cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_GBCURSOR_IMAGE);

  // Remember the grab cursor size.

  m_dragCSize.w = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.w;
  m_dragCSize.h = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.h;
#else
  // Fudge a value for the case where we are using a touchscreen.

  m_dragCSize.w = 16;
  m_dragCSize.h = 16;
#endif

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
 if (m_dragging)
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

      // Check if the icon has moved

      struct nxgl_point_s oldpos;
      getPos(oldpos);

      if (oldpos.x != newpos.x || oldpos.y != newpos.y)
        {
          // Re-draw the widget at the new background window position.
          // NOTE that this is done before moving erasing the old position which
          // probably overlaps the new position.

          disableDrawing();
          if (!moveTo(newpos.x, newpos.y))
            {
              twmerr("ERROR: moveTo() failed\n");
              return false;
            }

          // Redraw the background window in the rectangle previously occupied by
          // the widget.

          struct nxgl_size_s widgetSize;
          getSize(widgetSize);

          struct nxgl_rect_s bounds;
          bounds.pt1.x = oldpos.x;
          bounds.pt1.y = oldpos.y;
          bounds.pt2.x = oldpos.x + widgetSize.w - 1;
          bounds.pt2.y = oldpos.y + widgetSize.h - 1;

          FAR CBackground *backgd = m_twm4nx->getBackground();
          if (!backgd->redrawBackgroundWindow(&bounds, false))
            {
              twmerr("ERROR: redrawBackgroundWindow() failed\n");
              return false;
            }

          // Now redraw the icon in its new position

          enableDrawing();
          redraw();

          // Check if the icon at this position intersects any reserved
          // region on the background.  If not, check if some other icon is
          // already occupying this position


          bounds.pt1.x = newpos.x;
          bounds.pt1.y = newpos.y;
          bounds.pt2.x = newpos.x + widgetSize.w - 1;
          bounds.pt2.y = newpos.y + widgetSize.h - 1;

          struct nxgl_rect_s collision;
          if (backgd->checkCollision(bounds, collision))
            {
              // Yes.. Remember that we are at a colliding position when
              // the if the icon is ungrabbed.

              m_collision = true;
            }

          // No.. Check if some other icon is already occupying this
          // position

           else
            {
              FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
              if (factory->checkCollision(m_parent, bounds, collision))
                {
                  // Yes.. Remember that we are at a colliding position when
                  // the if the icon is ungrabbed.

                  m_collision = true;
                }
              else
                {
                  // No collision.. Remember the last good position

                  m_dragPos.x = newpos.x;
                  m_dragPos.y = newpos.y;
                  m_collision = false;
                }
            }

          // The icon was moved... we are really dragging!

          m_moved = true;
        }

      return true;
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
  // One last position update (but only if was previously moved)

  if (m_moved && !iconDrag(eventmsg))
    {
      return false;
    }

#ifdef CONFIG_TWM4NX_MOUSE
  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
#endif

  bool success = true;

  // There are two possibilities:  (1) The icon was moved.  In this case, we
  // leave the icon up and in its new position.  Or (2) the icon was simply
  // clicked in which case we need to de-iconify the window.  We also check
  // that we still have m_dragging == true to handle multiple UNGRAB events.
  // That happens when we get both the DROP and RELEASE events.

  if (m_dragging && !m_moved)
    {
      m_parent->deIconify();
    }

  // Another possibility is that the icon was drug into a position that
  // collides with a reserved area on the background or that overlaps another
  // icon.  In that case, we need to revert to last known good position.

  else if (m_dragging && m_collision)
    {
      // Get the current, colliding position.

      struct nxgl_point_s oldpos;
      getPos(oldpos);

      // Move the widget to the last known good position.
      // NOTE that this is done before moving erasing the old position which
      // probably overlaps the new position.

      disableDrawing();
      if (!moveTo(m_dragPos.x, m_dragPos.y))
        {
          twmerr("ERROR: moveTo() failed\n");
          success = false;
        }
      else
        {
          // Redraw the background window in the rectangle previously occupied
          // by the widget.

          struct nxgl_size_s widgetSize;
          getSize(widgetSize);

          struct nxgl_rect_s bounds;
          bounds.pt1.x = oldpos.x;
          bounds.pt1.y = oldpos.y;
          bounds.pt2.x = oldpos.x + widgetSize.w - 1;
          bounds.pt2.y = oldpos.y + widgetSize.h - 1;

          FAR CBackground *backgd = m_twm4nx->getBackground();
          if (!backgd->redrawBackgroundWindow(&bounds, false))
            {
              twmerr("ERROR: redrawBackgroundWindow() failed\n");
              return false;
            }

          // Now redraw the icon in its new position

          enableDrawing();
          redraw();
        }
    }

  // Indicate no longer dragging

  m_dragging  = false;
  m_collision = false;
  m_moved     = false;

  return success;
}
