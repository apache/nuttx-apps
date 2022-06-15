/********************************************************************************************
 * apps/graphics/nxwm/src/cstartwindow.cxx
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
 ********************************************************************************************/

/********************************************************************************************
 * Included Files
 ********************************************************************************************/

#include <nuttx/config.h>

#include <cstdlib>
#include <cerrno>

#include <fcntl.h>
#include <sched.h>

#include "graphics/nxwidgets/cwidgetcontrol.hxx"

#include "graphics/nxwm/nxwmconfig.hxx"
#include "graphics/nxglyphs.hxx"
#include "graphics/nxwm/ctaskbar.hxx"
#include "graphics/nxwm/cstartwindow.hxx"

/********************************************************************************************
 * Pre-Processor Definitions
 ********************************************************************************************/

/********************************************************************************************
 * Global Data
 ********************************************************************************************/

/**
 * The well-known name for the Start Window's message queue.
 */

FAR const char *NxWM::g_startWindowMqName = CONFIG_NXWM_STARTWINDOW_MQNAME;

/********************************************************************************************
 * CStartWindow Method Implementations
 ********************************************************************************************/

extern const struct NXWidgets::SRlePaletteBitmap CONFIG_NXWM_STARTWINDOW_ICON;

using namespace NxWM;

/**
 * CStartWindow Constructor
 *
 * @param taskbar.  A pointer to the parent task bar instance
 * @param window.  The window to be used by this application.
 */

CStartWindow::CStartWindow(CTaskbar *taskbar, CApplicationWindow *window)
{
  // Save the constructor data

  m_taskbar = taskbar;
  m_window  = window;

  // Add our personalized window label

  NXWidgets::CNxString myName = getName();
  window->setWindowLabel(myName);

  // Add our callbacks to the application window

  window->registerCallbacks(static_cast<IApplicationCallback *>(this));
}

/**
 * CStartWindow Constructor
 */

CStartWindow::~CStartWindow(void)
{
  // There would be a problem if we were stopped with the start window task
  // running... that should never happen but we'll check anyway:

  stop();

  // Although we didn't create it, we are responsible for deleting the
  // application window

  delete m_window;

  // Then stop and delete all applications

  removeAllApplications();
}

/**
 * Each implementation of IApplication must provide a method to recover
 * the contained IApplicationWindow instance.
 */

IApplicationWindow *CStartWindow::getWindow(void) const
{
  return static_cast<IApplicationWindow*>(m_window);
}

/**
 * Get the icon associated with the application
 *
 * @return An instance if IBitmap that may be used to rend the
 *   application's icon.  This is an new IBitmap instance that must
 *   be deleted by the caller when it is no long needed.
 */

NXWidgets::IBitmap *CStartWindow::getIcon(void)
{
  NXWidgets::CRlePaletteBitmap *bitmap =
    new NXWidgets::CRlePaletteBitmap(&CONFIG_NXWM_STARTWINDOW_ICON);

  return bitmap;
}

/**
 * Get the name string associated with the application
 *
 * @return A copy if CNxString that contains the name of the application.
 */

NXWidgets::CNxString CStartWindow::getName(void)
{
  return NXWidgets::CNxString("Start Window");
}

/**
 * Start the application.
 *
 * @return True if the application was successfully started.
 */

bool CStartWindow::run(void)
{
  return true;
}

/**
 * Stop the application.
 */

void CStartWindow::stop(void)
{
}

/**
 * Destroy the application and free all of its resources.  This method
 * will initiate blocking of messages from the NX server.  The server
 * will flush the window message queue and reply with the blocked
 * message.  When the block message is received by CWindowMessenger,
 * it will send the destroy message to the start window task which
 * will, finally, safely delete the application.
 */

void CStartWindow::destroy(void)
{
  // What are we doing?  This should never happen because the start
  // window task is persistent!

  // Block any further window messages

  m_window->block(this);

  // Make sure that the application is stopped

  stop();
}

/**
 * The application window is hidden (either it is minimized or it is
 * maximized, but not at the top of the hierarchy
 */

void CStartWindow::hide(void)
{
  // Disable drawing and events on all icons

  for (int i = 0; i < m_slots.size(); i++)
    {
      NXWidgets::CImage *image = m_slots.at(i).image;
      image->disableDrawing();
      image->setRaisesEvents(false);
    }
}

/**
 * Redraw the entire window.  The application has been maximized or
 * otherwise moved to the top of the hierarchy
 */

void CStartWindow::redraw(void)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the widget control associated with the NXTK window

  NXWidgets::CWidgetControl *control =  window->getWidgetControl();

  // Get the graphics port for drawing on the widget control

  NXWidgets::CGraphicsPort *port = control->getGraphicsPort();

  // Get the size of the application window

  struct nxgl_size_s windowSize;
  if (!window->getSize(&windowSize))
    {
      return;
    }

  // Fill the entire window with the background color

  port->drawFilledRect(0, 0, windowSize.w, windowSize.h,
                       CONFIG_NXWM_DEFAULT_BACKGROUNDCOLOR);

  // Begin adding icons in the upper left corner

  struct nxgl_point_s iconPos;
  iconPos.x = CONFIG_NXWM_STARTWINDOW_HSPACING;
  iconPos.y = CONFIG_NXWM_STARTWINDOW_VSPACING;

  // Add each icon in the list of applications

  for (int i = 0; i < m_slots.size(); i++)
    {
      // Get the icon associated with this application

      NXWidgets::CImage *image = m_slots.at(i).image;

      // Disable drawing of the icon image; disable events from the icon

      image->disableDrawing();
      image->setRaisesEvents(false);

      // Get the size of the icon image

      NXWidgets::CRect rect;
      image->getPreferredDimensions(rect);

      // Center the image in the region defined by the maximum icon size

      struct nxgl_point_s imagePos;
      imagePos.x = iconPos.x;
      imagePos.y = iconPos.y;

      if (rect.getWidth() < m_iconSize.w)
        {
          imagePos.x += ((m_iconSize.w - rect.getWidth()) >> 1);
        }

      if (rect.getHeight() < m_iconSize.h)
        {
          imagePos.y += ((m_iconSize.h - rect.getHeight()) >> 1);
        }

      // Set the position of the icon bitmap

      image->moveTo(imagePos.x, imagePos.y);

      // Then re-draw the icon at the new position

      image->enableDrawing();
      image->redraw();
      image->setRaisesEvents(true);

      // Advance the next icon position to the left

      iconPos.x +=  m_iconSize.w + CONFIG_NXWM_TASKBAR_HSPACING;

      // If there is insufficient space on on this row for the next
      // max-sized icon, then advance to the next row

      if (iconPos.x + m_iconSize.w >= windowSize.w)
        {
          iconPos.x  = CONFIG_NXWM_STARTWINDOW_HSPACING;
          iconPos.y += m_iconSize.h + CONFIG_NXWM_TASKBAR_VSPACING;

          // Don't try drawing past the bottom of the window

          if (iconPos.y >= windowSize.w)
            {
              break;
            }
        }
    }
}

/**
 * Report of this is a "normal" window or a full screen window.  The
 * primary purpose of this method is so that window manager will know
 * whether or not it show draw the task bar.
 *
 * @return True if this is a full screen window.
 */

bool CStartWindow::isFullScreen(void) const
{
  return m_window->isFullScreen();
}

/**
 * Add the application to the start window.  The general sequence is:
 *
 * 1. Call IApplicationFactory::create to a new instance of the application
 * 2. Call CStartWindow::addApplication to add the application to the
 *    start window.
 *
 * @param app.  The new application to add to the start window
 * @return true on success
 */

bool CStartWindow::addApplication(IApplicationFactory *app)
{
  // Recover the NXTK window instance contained in the application window

  NXWidgets::INxWindow *window = m_window->getWindow();

  // Get the widget control associated with the NXTK window

  NXWidgets::CWidgetControl *control =  window->getWidgetControl();

  // Get the bitmap icon that goes with this application

  NXWidgets::IBitmap *bitmap = app->getIcon();

  // Create a CImage instance to manage the application icon

  NXWidgets::CImage *image =
    new NXWidgets::CImage(control, 0, 0, bitmap->getWidth(),
                          bitmap->getHeight(), bitmap, 0);
  if (!image)
    {
      return false;
    }

  // Configure the image, disabling drawing for now

  image->setBorderless(true);
  image->disableDrawing();

  // Register to get events from the mouse clicks on the image

  image->addWidgetEventHandler(this);

  // Add the application to end of the list of applications managed by
  // the start window

  struct SStartWindowSlot slot;
  slot.app   = app;
  slot.image = image;
  m_slots.push_back(slot);

  // Re-calculate the icon bounding box

  getIconBounds();
  return true;
}

/**
 * Called when the window minimize button is pressed.
 */

void CStartWindow::minimize(void)
{
  m_taskbar->minimizeApplication(static_cast<IApplication*>(this));
}

/**
 * Called when the window minimize close is pressed.
 */

void CStartWindow::close(void)
{
  // Do nothing... you can't close the start window!!!
}

/**
 * Calculate the icon bounding box
 */

void CStartWindow::getIconBounds(void)
{
  // Visit each icon

  struct nxgl_size_s maxSize;
  maxSize.w = 0;
  maxSize.h = 0;

  for (int i = 0; i < m_slots.size(); i++)
    {
      // Get the icon associated with this application

      NXWidgets::CImage *image = m_slots.at(i).image;

      // Get the size of the icon image

      NXWidgets::CRect rect;
      image->getPreferredDimensions(rect);

      // Keep the maximum height and width

      if (rect.getWidth() > maxSize.h)
        {
          maxSize.w = rect.getWidth() ;
        }

      if (rect.getHeight() > maxSize.h)
        {
          maxSize.h = rect.getHeight();
        }
    }

  // And save the new maximum size

  m_iconSize.w = maxSize.w;
  m_iconSize.h = maxSize.h;
}

/**
 * Stop all applications
 */

void CStartWindow::removeAllApplications(void)
{
  // Stop all applications and remove them from the start window.  Clearly, there
  // are some ordering issues here... On an orderly system shutdown, disconnection
  // should really occur priority to deleting instances

  while (!m_slots.empty())
    {
      // Remove the application factory from the start menu

      IApplicationFactory *app = m_slots.at(0).app;

      // Now, delete the image and the application

      delete app;
      delete m_slots.at(0).image;

      // And discard the data in this slot

      m_slots.erase(0);
    }
}

/**
 * Handle a widget action event.  For CImage, this is a mouse button pre-release event.
 *
 * @param e The event data.
 */

void CStartWindow::handleActionEvent(const NXWidgets::CWidgetEventArgs &e)
{
  // Was an ICON clicked?

  for (int i = 0; i < m_slots.size(); i++)
    {
      // Is this the clicked icon?

      NXWidgets::CImage *image = m_slots.at(i).image;
      if (image->isClicked())
        {
          // Create a new copy of the application

          IApplication *app = m_slots.at(i).app->create();
          if (app)
            {
              // Start the new copy of the application

              if (m_taskbar->startApplication(app, false))
                {
                  // Then break out of the loop

                  break;
                }
              else
                {
                  // If we cannot start the app.  Destroy the instance we
                  // created and see what happens next.  Could be interesting.

                  app->stop();
                  app->destroy();
                }
            }
        }
    }
}
