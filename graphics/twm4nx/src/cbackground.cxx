/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cbackground.cxx
// Manage background image
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

#include <cfcntl>
#include <cerrno>

#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/cscaledbitmap.hxx"
#include "graphics/nxwidgets/cbgwindow.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"
#include "graphics/nxwidgets/crect.hxx"
#include "graphics/nxwidgets/cimage.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cwindowfactory.hxx"
#include "graphics/twm4nx/cmainmenu.hxx"
#include "graphics/twm4nx/cbackground.hxx"

/////////////////////////////////////////////////////////////////////////////
// CBackground Method Implementations
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CBackground Constructor
 *
 * @param hWnd - NX server handle
 */

CBackground::CBackground(FAR CTwm4Nx *twm4nx)
{
  m_twm4nx      = twm4nx;                    // Save the session instance
  m_eventq      = (mqd_t)-1;                 // No NxWidget event message queue yet
  m_backWindow  = (NXWidgets::CBgWindow *)0; // No background window yet
#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
  m_backImage   = (NXWidgets::CImage *)0;    // No background image yet
#endif
}

/**
 * CBackground Destructor
 */

CBackground::~CBackground(void)
{
  // Free resources helf by the background

  cleanup();
}

/**
 * Finish construction of the background instance.  This performs
 * That are not appropriate for the constructor because they may
 * fail.
 *
 * @param sbitmap.  Identifies the bitmap to paint on background
 * @return true on success
 */

bool CBackground::
  initialize(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap)
{
  twminfo("Create the background window\n");

  // Open a message queue to send fully digested NxWidget events.

  FAR const char *mqname = m_twm4nx->getEventQueueName();

  m_eventq = mq_open(mqname, O_WRONLY | O_NONBLOCK);
  if (m_eventq == (mqd_t)-1)
    {
      twmerr("ERROR: Failed open message queue '%s': %d\n",
             mqname, errno);
      return false;
    }

  // Create the background window (if we have not already done so)

  if (m_backWindow == (NXWidgets::CBgWindow *)0 &&
      !createBackgroundWindow())
    {
      twmerr("ERROR: Failed to create the background window\n");
      cleanup();
      return false;
    }

  twminfo("Create the background image\n");

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
  // Create the new background image

  if (!createBackgroundImage(sbitmap))
    {
      twmerr("ERROR: Failed to create the background image\n");
      cleanup();
      return false;
    }
#endif

  return true;
}

/**
 * Get the size of the physical display device which is equivalent to
 * size of the background window.
 * size of the background window.
 *
 * @return The size of the display
 */

void CBackground::getDisplaySize(FAR struct nxgl_size_s &size)
{
  // Get the widget control from the task bar window.  The physical window geometry
  // should be the same for all windows.

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Get the window bounding box from the widget control

  NXWidgets::CRect rect = control->getWindowBoundingBox();

  // And return the size of the window

  rect.getSize(size);
}

/**
 * Check if the region within 'bounds' collides with any other reserved
 * region on the desktop.  This is used for icon placement.
 *
 * @param iconBounds The candidate bounding box
 * @param collision The bounding box of the reserved region that the
 *   candidate collides with
 * @return Returns true if there is a collision
 */

bool CBackground::checkCollision(FAR const struct nxgl_rect_s &bounds,
                                 FAR struct nxgl_rect_s &collision)
{
#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
  // Is there a background image

  if (m_backImage != (NXWidgets::CImage *)0)
    {
      // Create a bounding box for the background image

      struct nxgl_size_s imageSize;
      m_backImage->getSize(imageSize);

      struct nxgl_point_s imagePos;
      m_backImage->getPos(imagePos);

      collision.pt1.x = imagePos.x;
      collision.pt1.y = imagePos.y;
      collision.pt2.x = imagePos.x + imageSize.w - 1;
      collision.pt2.y = imagePos.y + imageSize.h - 1;

      return nxgl_intersecting(&bounds, &collision);
    }
#endif

  return false;
}

/**
 * Handle the background window redraw.
 *
 * @param nxRect The region in the window that must be redrawn.
 * @param more True means that more re-draw requests will follow
 * @return true on success
 */

bool CBackground::redrawBackgroundWindow(FAR const struct nxgl_rect_s *rect,
                                         bool more)
{
  twminfo("Redrawing..\n");

  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Get the graphics port for drawing on the background window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the region to redraw

  struct nxgl_size_s redrawSize;
  redrawSize.w = rect->pt2.x - rect->pt1.x + 1;
  redrawSize.h = rect->pt2.y - rect->pt1.y + 1;

  // Fill the redraw region with the background color

  port->drawFilledRect(rect->pt1.x, rect->pt1.y,
                       redrawSize.w, redrawSize.h,
                       CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR);

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
  if (m_backImage != (NXWidgets::CImage *)0)
    {
      // Does any part of the image need to be redrawn?

      FAR NXWidgets::CRect cimageRect = m_backImage->getBoundingBox();

      struct nxgl_rect_s imageRect;
      cimageRect.getNxRect(&imageRect);

      struct nxgl_rect_s intersection;
      nxgl_rectintersect(&intersection, rect, &imageRect);

      if (!nxgl_nullrect(&intersection))
        {
          // Then re-draw the background image on the window

          m_backImage->enableDrawing();
          m_backImage->redraw();
        }
    }
#endif

  // Now redraw any background icons that need to be redrawn

  FAR CWindowFactory *factory = m_twm4nx->getWindowFactory();
  factory->redrawIcons(rect);

  return true;
}

/**
 * Handle EVENT_BACKGROUND events.
 *
 * @param eventmsg.  The received NxWidget WINDOW event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CBackground::event(FAR struct SEventMsg *eventmsg)
{
  twminfo("eventID: %u\n", eventmsg->eventID);

  // Handle the event

  bool success = true;
  switch (eventmsg->eventID)
    {
     case EVENT_BACKGROUND_XYINPUT:    // Poll for icon mouse/touch events
        {
          // This event message is sent from CWindowEvent whenever mouse,
          // touchscreen, or keyboard entry events are received in the
          // background window.

          NXWidgets::CWidgetControl *control =
            m_backWindow->getWidgetControl();

          FAR struct SXyInputEventMsg *xymsg =
            (FAR struct SXyInputEventMsg *)eventmsg;

          // pollEvents() returns true if any interesting event occurred
          // within a widget that is associated with the background window.
          // false is not a failure.

          if (!control->pollEvents())
            {
              // If there is no interesting widget event, then this might be
              // a background click.  In that case, we should bring up the
              // main menu (if it is not already up).

              showMainMenu(xymsg->pos, xymsg->buttons);
            }

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
          // If there is a background image, then clicking the background
          // image is equivalent to clicking the background.  We need to
          // handle this case because the image widget occludes the
          // background

          else if (m_backImage != (NXWidgets::CImage *)0 &&
                   m_backImage->isClicked())
            {
              // Treat the image click like background click:  Bring up the
              // main menu (if it is not already up).

              showMainMenu(xymsg->pos, xymsg->buttons);
            }
#endif
        }
        break;

      case EVENT_BACKGROUND_REDRAW:    // Redraw the background
        {
          FAR struct SRedrawEventMsg *redrawmsg =
            (FAR struct SRedrawEventMsg *)eventmsg;

          success = redrawBackgroundWindow(&redrawmsg->rect,
                                            redrawmsg->more);
        }
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Create the background window.
 *
 * @return true on success
 */

bool CBackground::createBackgroundWindow(void)
{
  // Create an instance of the background window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  struct SAppEvents events;
  events.eventObj    = (FAR void *)this;
  events.redrawEvent = EVENT_BACKGROUND_REDRAW;
  events.resizeEvent = EVENT_SYSTEM_NOP;
  events.mouseEvent  = EVENT_BACKGROUND_XYINPUT;
  events.kbdEvent    = EVENT_SYSTEM_NOP;
  events.closeEvent  = EVENT_SYSTEM_NOP;
  events.deleteEvent = EVENT_WINDOW_DELETE;

  FAR CWindowEvent *control =
    new CWindowEvent(m_twm4nx, (FAR void *)0, events);

  // Create the background window (CTwm4Nx inherits from CNxServer)

  m_backWindow = m_twm4nx->getBgWindow(control);
  if (m_backWindow == (FAR NXWidgets::CBgWindow *)0)
    {
      twmerr("ERROR:  Failed to create BG window\n");
      return false;
    }

  // Open the background window

  if (!m_backWindow->open())
    {
      twmerr("ERROR:  Failed to open the BG window\n");
      delete m_backWindow;
      m_backWindow = (FAR NXWidgets::CBgWindow *)0;
      return false;
    }

  // Get the graphics port for drawing on the background window

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the region to redraw (the whole display)

  struct nxgl_size_s windowSize;
  if (!m_backWindow->getSize(&windowSize))
    {
      twmerr("ERROR: getSize failed\n");
      delete m_backWindow;
      m_backWindow = (FAR NXWidgets::CBgWindow *)0;
      return false;
    }

  // Fill the display with the background color

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       CONFIG_TWM4NX_DEFAULT_BACKGROUNDCOLOR);
  return true;
}

/**
 * Create the background image.
 *
 * @return true on success
 */

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
bool CBackground::
  createBackgroundImage(FAR const struct NXWidgets::SRlePaletteBitmap *sbitmap)
{
 // Get the size of the display

  struct nxgl_size_s windowSize;
  if (!m_backWindow->getSize(&windowSize))
    {
      twmerr("ERROR: getSize failed\n");
      return false;
    }

  // Get the widget control from the background window

  NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();

  // Create the sbitmap object

  NXWidgets::CRlePaletteBitmap *cbitmap =
    new NXWidgets::CRlePaletteBitmap(sbitmap);

  if (cbitmap == (NXWidgets::CRlePaletteBitmap *)0)
    {
      twmerr("ERROR: Failed to create bitmap\n");
      return false;
    }

  // Get the size of the bitmap image

  struct nxgl_size_s imageSize;
  imageSize.w = cbitmap->getWidth();
  imageSize.h = (nxgl_coord_t)cbitmap->getHeight();

  // Pick an X/Y position such that the image will be centered in the display

  struct nxgl_point_s imagePos;
  if (imageSize.w >= windowSize.w)
    {
      imagePos.x = 0;
    }
  else
    {
      imagePos.x = (windowSize.w - imageSize.w) >> 1;
    }

  if (imageSize.h >= windowSize.h)
    {
      imagePos.y = 0;
    }
  else
    {
      imagePos.y = (windowSize.h - imageSize.h) >> 1;
    }

  // Now we have enough information to create the image

  m_backImage = new NXWidgets::CImage(control, imagePos.x, imagePos.y,
                                      imageSize.w, imageSize.h, cbitmap);
  if (m_backImage == (NXWidgets::CImage *)0)
    {
      twmerr("ERROR: Failed to create CImage\n");
      delete cbitmap;
      return false;
    }

  // Configure and draw the background image.
  // NOTE that we need to get events from the background image.  That is
  // because the image occludes the background and we have to treat image
  // clicks just as background clicks.

  m_backImage->setBorderless(true);
  m_backImage->setRaisesEvents(true);

  m_backImage->enable();
  m_backImage->enableDrawing();
  m_backImage->redraw();
  return true;
}
#endif

/**
 * Bring up the main menu (if it is not already up).
 *
 * @param pos The window click position.
 * @param buttons The set of mouse button presses.
 */

void CBackground::showMainMenu(FAR struct nxgl_point_s &pos,
                               uint8_t buttons)
{
  // Is the main menu already up?  Was the mouse left button pressed?

  FAR CMainMenu *cmain = m_twm4nx->getMainMenu();
  if (!cmain->isVisible() && (buttons & MOUSE_BUTTON_1) != 0)
    {
      // Bring up the main menu

      struct SEventMsg outmsg;
      outmsg.eventID = EVENT_MAINMENU_SELECT;
      outmsg.pos.x   = pos.x;
      outmsg.pos.y   = pos.y;
      outmsg.context = EVENT_CONTEXT_BACKGROUND;
      outmsg.handler = (FAR void *)0;
      outmsg.obj     = (FAR void *)this;

      int ret = mq_send(m_eventq, (FAR const char *)&outmsg,
                        sizeof(struct SEventMsg), 100);
      if (ret < 0)
        {
          twmerr("ERROR: mq_send failed: %d\n", errno);
        }
   }
}

/**
 * Release resources held by the background.
 */

void CBackground::cleanup(void)
{
  // Close the NxWidget event message queue

  if (m_eventq != (mqd_t)-1)
    {
      mq_close(m_eventq);
      m_eventq = (mqd_t)-1;
    }

#ifdef CONFIG_TWM4NX_BACKGROUND_HASIMAGE
  // Delete the background image

  if (m_backImage != (NXWidgets::CImage *)0)
    {
      delete m_backImage;
      m_backImage = (NXWidgets::CImage *)0;
    }
#endif

  // Delete the background

  if (m_backWindow != (NXWidgets::CBgWindow *)0)
    {
      // Delete the contained widget control.  We are responsible for it
      // because we created it

      NXWidgets::CWidgetControl *control = m_backWindow->getWidgetControl();
      if (control != (NXWidgets::CWidgetControl *)0)
        {
          delete control;
        }

      // Then delete the background

      delete m_backWindow;
      m_backWindow = (NXWidgets::CBgWindow *)0;
    }
}
