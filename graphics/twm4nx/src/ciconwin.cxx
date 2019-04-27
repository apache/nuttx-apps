/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/ciconwin.cxx
// Icon Windows
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
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

#include <stdio.h>

#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/crlepalettebitmap.hxx"

#include "graphics/twm4nx/twm4nx_config.hxx"
#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cfonts.hxx"
#include "graphics/twm4nx/cmenus.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cicon.hxx"
#include "graphics/twm4nx/ciconwin.hxx"
#include "graphics/twm4nx/ctwm4nxevent.hxx"
#include "graphics/twm4nx/twm4nx_widgetevents.hxx"
#include "graphics/twm4nx/twm4nx_cursor.hxx"

/////////////////////////////////////////////////////////////////////////////
// CTwm4Nx Implementation
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CIconWin Constructor
 */

CIconWin::CIconWin(CTwm4Nx *twm4nx)
{
  m_twm4nx       = twm4nx;                        // Cached the Twm4Nx session
  m_nxwin        = (FAR NXWidgets::CNxWindow *)0; // The cursor "raw" window

  // Dragging

  m_drag         = false;                         // No drag in progress
  m_dragOffset.x = 0;                             // Offset from mouse to window origin
  m_dragOffset.y = 0;
  m_dragCSize.w  = 0;                             // The grab cursor size
  m_dragCSize.h  = 0;
}

/**
 * CIconWin Destructor
 */

CIconWin::~CIconWin(void)
{
  cleanup();
}

/**
 * Create the icon window
 *
 * @param parent  The parent window
 * @param sbitmap The Icon image
 * @param pos     The default position
 */

bool CIconWin::initialize(FAR CWindow *parent,
                          FAR const NXWidgets::SRlePaletteBitmap *sbitmap,
                          FAR const struct nxgl_point_s *pos)
{
  struct nxgl_point_s final;

  // Git the size of the Icon Image

  struct nxgl_size_s iconImageSize;
  iconImageSize.h = sbitmap->height;
  iconImageSize.w = sbitmap->height;

  // Git the size of the Icon name

  FAR CFonts *fonts = m_twm4nx->getFonts();
  FAR NXWidgets::CNxFont *iconFont = fonts->getIconFont();
  FAR const char *iconName = parent->getWindowName();

  struct nxgl_size_s iconWindowSize;
  iconWindowSize.w  = iconFont->getStringWidth(iconName);
  iconWindowSize.w += 6;

  // Handle the case where the name string is wider than the icon

  struct nxgl_point_s iconWindowPos;
  if (iconWindowSize.w < iconImageSize.w)
    {
      // Center

      iconWindowPos.x  = (iconImageSize.w - iconWindowSize.w) / 2;
      iconWindowPos.x += 3;
      iconWindowSize.w = iconImageSize.w;
    }
  else
    {
      iconWindowPos.x = 3;
    }

  iconWindowPos.y  = iconImageSize.h + iconFont->getHeight();
  iconWindowSize.h = iconImageSize.h + iconFont->getHeight() + 4;

  // Create the icon window
  // 1. Get the server instance.  m_twm4nx inherits from NXWidgets::CNXServer
  //    so we all ready have the server instance.
  // 2. Create the style, using the selected colors (REVISIT)

  // 3. Create a Widget control instance for the window using the default
  //    style for now.  CWindowEvent derives from CWidgetControl.

  FAR CWindowEvent *control = new CWindowEvent(m_twm4nx);

  // 4. Create the icon window

  m_nxwin = m_twm4nx->createRawWindow(control);
  if (m_nxwin == (FAR NXWidgets::CNxWindow *)0)
    {
      delete control;
      return false;
    }

  // 5. Open and initialize the icon window

  bool success = m_nxwin->open();
  if (!success)
    {
      delete m_nxwin;
      m_nxwin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

  // 6. Set the initial window size

  if (!m_nxwin->setSize(&iconWindowSize))
    {
      delete m_nxwin;
      m_nxwin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

  // 7. Set the initial window position

  if (!m_nxwin->setPosition(&iconWindowPos))
    {
      delete m_nxwin;
      m_nxwin = (FAR NXWidgets::CNxWindow *)0;
      return false;
    }

  // We need to figure out where to put the icon window now, because getting
  // here means that we am going to make the icon visible.

  FAR CIcon *cicon = m_twm4nx->getIcon();
  cicon->place(parent, pos, &final);

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  if (final.x > displaySize.w)
    {
      final.x = displaySize.w - iconWindowSize.w;
    }

  if (final.y > displaySize.h)
    {
      final.y = displaySize.h - iconImageSize.h - iconFont->getHeight() - 4;
    }

  (void)m_nxwin->setPosition(&final);
  return true;
}

/**
 * Handle ICON events.
 *
 * @param eventmsg.  The received NxWidget ICON event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWin::event(FAR struct SEventMsg *eventmsg)
{
  bool success = true;

  switch (eventmsg->eventID)
    {
      case EVENT_ICONWIN_GRAB:   /* Left click on icon.  Start drag */
        success = iconGrab(eventmsg);
        break;

      case EVENT_ICONWIN_DRAG:   /* Mouse movement while clicked */
        success = iconDrag(eventmsg);
        break;

      case EVENT_ICONWIN_UNGRAB: /* Left click release while dragging. */
        success = iconUngrab(eventmsg);
        break;

      default:
        success = false;
        break;
    }

  return success;
}

/**
 * Handle the ICON_GRAB event.  That corresponds to a left
 * mouse click on the icon
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWin::iconGrab(FAR struct SEventMsg *eventmsg)
{
  // Promote the icon to a modal window

  m_nxwin->modal(true);

  // Indicate that dragging has started.

  m_drag = false;

  // Get the icon position.

  struct nxgl_point_s framePos;
  m_nxwin->getPosition(&framePos);

  // Determine the relative position of the icon and the mouse

  m_dragOffset.x = framePos.x - eventmsg->pos.x;
  m_dragOffset.y = framePos.y - eventmsg->pos.y;

  // Select the grab cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_GBCURSOR_IMAGE);

  // Remember the grab cursor size

  m_dragCSize.w = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.w;
  m_dragCSize.h = CONFIG_TWM4NX_GBCURSOR_IMAGE.size.h;
  return true;
}

/**
 * Handle the ICON_DRAG event.  That corresponds to a mouse
 * movement when the icon is in a grabbed state.
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWin::iconDrag(FAR struct SEventMsg *eventmsg)
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

      return m_nxwin->setPosition(&newpos);
    }

  return false;
}

/**
 * Handle the ICON_UNGRAB event.  The corresponds to a mouse
 * left button release while in the grabbed
 *
 * @param eventmsg.  The received NxWidget event message.
 * @return True if the message was properly handled.  false is
 *   return on any failure.
 */

bool CIconWin::iconUngrab(FAR struct SEventMsg *eventmsg)
{
  // One last position update

  if (!iconDrag(eventmsg))
    {
      return false;
    }

  // Indicate no longer dragging

  m_drag = false;

  // No long modal

  m_nxwin->modal(false);

  // Restore the normal cursor image

  m_twm4nx->setCursorImage(&CONFIG_TWM4NX_CURSOR_IMAGE);
  return false;
}

/**
 * Cleanup on failure or as part of the destructor
 */

void CIconWin::cleanup(void)
{
  // Close windows

  if (m_nxwin != (FAR NXWidgets::CNxWindow *)0)
    {
      delete m_nxwin;
      m_nxwin  = (FAR NXWidgets::CNxWindow *)0;
    }
}
