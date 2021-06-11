/****************************************************************************
 * apps/include/graphics/nxwm/iapplication.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATION_NXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATION_NXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include "graphics/nxwidgets/cnxstring.hxx"
#include "graphics/nxwidgets/ibitmap.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Abstract Base Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * Forward references
   */

  class IApplicationWindow;

  /**
   * IApplication provides the abstract base class for each NxWM application.
   */

  class IApplication
  {
    protected:
      /**
       * These values (and the accessors that go with them) violate the "purity"
       * of the base class.  These are really part of the task bar implementation:
       * Each application provides this state information needed by the taskbar.
       */

      bool m_minimized; /**< True if the application is minimized */
      bool m_topapp;    /**< True if this application is at the top in the hierarchy */

    public:
      /**
       * A virtual destructor is required in order to override the IApplication
       * destructor.  We do this because if we delete IApplication, we want the
       * destructor of the class that inherits from IApplication to run, not this
       * one.
       */

      virtual ~IApplication(void) { }

      /**
       * Each implementation of IApplication must provide a method to recover
       * the contained CApplicationWindow instance.
       */

      virtual IApplicationWindow *getWindow(void) const = 0;

      /**
       * Get the icon associated with the application
       *
       * @return An instance if IBitmap that may be used to rend the
       *   application's icon.  This is an new IBitmap instance that must
       *   be deleted by the caller when it is no long needed.
       */

      virtual NXWidgets::IBitmap *getIcon(void) = 0;

      /**
       * Get the name string associated with the application
       *
       * @return A copy if CNxString that contains the name of the application.
       */

      virtual NXWidgets::CNxString getName(void) = 0;

      /**
       * Start the application (perhaps in the minimized state).
       *
       * @return True if the application was successfully started.
       */

      virtual bool run(void) = 0;

      /**
       * Stop the application, put all widgets in a deactivated/disabled state
       * and wait to see what happens next.
       */

      virtual void stop(void) = 0;

      /**
       * Destroy the application and free all of its resources.  This method
       * will initiate blocking of messages from the NX server.  The server
       * will flush the window message queue and reply with the blocked
       * message.  When the block message is received by CWindowMessenger,
       * it will send the destroy message to the start window task which
       * will, finally, safely delete the application.
       */

      virtual void destroy(void) = 0;

      /**
       * The application window is hidden (either it is minimized or it is
       * maximized, but not at the top of the hierarchy
       */

      virtual void hide(void) = 0;

      /**
       * Redraw the entire window.  The application has been maximized or
       * otherwise moved to the top of the hierarchy.  This method is call from
       * CTaskbar when the application window must be displayed
       */

      virtual void redraw(void) = 0;

      /**
       * Set the application's minimized state
       *
       * @param minimized. True if the application is minimized
       */

      inline void setMinimized(bool minimized)
      {
        m_minimized = minimized;
      }

      /**
       * Set the application's top state
       *
       * @param topapp. True if the application is the new top application
       */

      inline void setTopApplication(bool topapp)
      {
        m_topapp = topapp;
      }

      /**
       * Get the application's minimized state
       *
       * @return True if the application is minimized
       */

      inline bool isMinimized(void) const
      {
        return m_minimized;
      }

      /**
       * Return true if this is the top application
       *
       * @return True if the application is the new top application
       */

      inline bool isTopApplication(void) const
      {
        return m_topapp;
      }

      /**
       * Report of this is a "normal" window or a full screen window.  The
       * primary purpose of this method is so that window manager will know
       * whether or not it show draw the task bar.
       *
       * @return True if this is a full screen window.
       */

      virtual bool isFullScreen(void) const = 0;
  };

  /**
   * IApplicationFactory provides a mechanism for creating multiple instances
   * of an application.
   */

  class IApplicationFactory
  {
  public:
    /**
     * A virtual destructor is required in order to override the IApplicationFactory
     * destructor.  We do this because if we delete IApplicationFactory, we want the
     * destructor of the class that inherits from IApplication to run, not this
     * one.
     */

    virtual ~IApplicationFactory(void) { }

    /**
     * Create a new instance of an application.
     */

    virtual IApplication *create(void) = 0;

    /**
     * Get the icon associated with the application
     *
     * @return An instance if IBitmap that may be used to rend the
     *   application's icon.  This is an new IBitmap instance that must
     *   be deleted by the caller when it is no long needed.
     */

    virtual NXWidgets::IBitmap *getIcon(void) = 0;
  };
}

#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_IAPPLICATION_NXX
