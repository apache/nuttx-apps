/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/ctwm4nx.hxx
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

// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//   Copyright 1988 by Evans & Sutherland Computer Corporation,
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
// Although not listed as a copyright holder, thanks and recognition need
// to go to Tom LaStrange, the original author of TWM.

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NX_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NX_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdlib>
#include <semaphore.h>
#include <mqueue.h>

#include <nuttx/nx/nx.h>
#include <nuttx/nx/nxglib.h>
#include <nuttx/nx/nxfonts.h>

#include "graphics/nxwidgets/nxconfig.hxx"
#include "graphics/nxwidgets/cnxserver.hxx"
#include "graphics/nxwidgets/cnxwindow.hxx"
#include "graphics/nxwidgets/cimage.hxx"

#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/twm4nx_events.hxx"

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class  CInput;         // Forward reference
  class  CBackground;    // Forward reference
  class  CWidgetEvent;   // Forward reference
  class  CIconMgr;       // Forward reference
  class  CFonts;         // Forward reference
  class  CWindow;        // Forward reference
  class  CMainMenu;      // Forward reference
  class  CResize;        // Forward reference
  class  CWindowFactory; // Forward reference
  class  CResize;        // Forward reference
  struct SWindow;        // Forward reference

  /**
   * Public Constant Data
   */

  extern const char GNoName[];                   /**< Name to use when there is no name */

  /**
   * This class provides the overall state of the window manager. It is also
   * the heart of the window manager:  It inherits for CNxServer and, hence,
   * represents the NX server itself.
   */

  class CTwm4Nx : public NXWidgets::CNxServer
  {
    private:
      int                          m_display;     /**< Display that we are using */
      FAR char                    *m_queueName;   /**< NxWidget event queue name */
      mqd_t                        m_eventq;      /**< NxWidget event message queue */
      FAR CBackground             *m_background;  /**< Background window management */
      FAR CIconMgr                *m_iconmgr;     /**< The Default icon manager */
      FAR CWindowFactory          *m_factory;     /**< The cached CWindowFactory instance */
      FAR CFonts                  *m_fonts;       /**< The cached Cfonts instance */
      FAR CMainMenu               *m_mainMenu;    /**< The cached CMainMenu instance */
      FAR CResize                 *m_resize;      /**< The cached CResize instance */

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) || !defined(CONFIG_TWM4NX_NOMOUSE)
      FAR CInput                  *m_input;       /**< Keyboard/mouse input injector */
#endif
      /* Display properties */

      FAR struct nxgl_size_s       m_displaySize; /**< Size of the display */
      FAR struct nxgl_size_s       m_maxWindow;   /**< Maximum size of a window */

      /**
       * Connect to the NX server
       *
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      bool connect(void);

      /**
       * Generate a random message queue name.  Different message queue
       * names are required for each instance of Twm4Nx that is started.
       */

      inline void genMqName(void);

      /**
       * Handle SYSTEM events.
       *
       * @param eventmsg.  The received NxWidget SYSTEM event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

      inline bool systemEvent(FAR struct SEventMsg *eventmsg);

      /**
       * Cleanup in preparation for termination.
       */

      void cleanup(void);

    public:

      /**
       * CTwm4Nx Constructor
       *
       * @param display.  Indicates which display will be used.  Usually zero
       *   except in the case wehre there of multiple displays.
       */

       CTwm4Nx(int display);

       /**
        * CTwm4Nx Destructor
        */

       ~CTwm4Nx(void);

       /**
        * Perform initialization additional, post-construction initialization
        * that may fail.  This initialization logic fully initialized the
        * Twm4Nx session.  Upon return, the session is ready for use.
        *
        * After Twm4Nx is initialized, external applications should register
        * themselves into the Main Menu in order to be a part of the desktop.
        *
        * @return True if the Twm4Nx was properly initialized.  false is
        * returned on any failure.
        */

       bool initialize(void);

       /**
        * This is the main, event loop of the Twm4Nx session.
        *
        * @return True if the Twm4Nxr was terminated noramly.  false is returned
        * on any failure.
        */

       bool eventLoop(void);

       /**
        * Return a reference to the randomly generated event messageq queue
        * name.  Different message queue names are required for each instance
        * of Twm4Nx that is started.
        */

        inline FAR const char *getEventQueueName(void)
        {
          return m_queueName;
        }

      /**
       * Return the size of the physical display (whichi is equivalent to the
       * size of the contained background window).
       *
       * @return The size of the display.
       */

       inline void getDisplaySize(FAR struct nxgl_size_s *size)
       {
         size->w = m_displaySize.w;
         size->h = m_displaySize.h;
       }

      /**
       * Return the pixel depth.
       *
       * REVISIT:  Currently only the pixel depth configured for NxWidgets is
       * supported.  That is probably compatible with support for multiple
       * displays of differing resolutions.
       *
       * @return The number of bits-per-pixel.
       */

       inline uint8_t getPixelDepth(void)
       {
         return CONFIG_NXWIDGETS_BPP;
       }

      /**
       * Return the maximum size of a window.
       *
       * @return The maximum size of a window.
       */

       inline void maxWindowSize(FAR struct nxgl_size_s *size)
       {
         size->w = m_maxWindow.w;
         size->h = m_maxWindow.h;
       }

      /**
       * Return the session's CBackground instance.
       *
       * @return The contained instance of the CBackground class for this
       *   session.
       */

       inline FAR CBackground *getBackground(void)
       {
         return m_background;
       }

      /**
       * Return the session's Icon Manager instance.
       *
       * @return The contained instance of the Icon Manager for this session.
       */

       inline FAR CIconMgr *getIconMgr(void)
       {
         return m_iconmgr;
       }

      /**
       * Return the session's CWindowFactory instance.
       *
       * @return The contained instance of the CWindow instance this
       *   session.
       */

       inline FAR CWindowFactory *getWindowFactory(void)
       {
         return m_factory;
       }

      /**
       * Return the session's CFonts instance.
       *
       * @return The contained instance of the CFonts instance for this
       *   session.
       */

       inline FAR CFonts *getFonts(void)
       {
         return m_fonts;
       }

      /**
       * Return the session's CMainMenu instance.
       *
       * @return The contained instance of the CMainMenu instance for this
       *   session.
       */

       inline FAR CMainMenu *getMainMenu(void)
       {
         return m_mainMenu;
       }

      /**
       * Return the session's CResize instance.
       *
       * @return The contained instance of the CResize instance for this
       *   session.
       */

       inline FAR CResize *getResize(void)
       {
         return m_resize;
       }

#if !defined(CONFIG_TWM4NX_NOKEYBOARD) || !defined(CONFIG_TWM4NX_NOMOUSE)
      /**
       * Return the session's CInput instance.
       *
       * @return The contained instance of the CInput instance for this
       *   session.
       */

       inline FAR CInput *getInput(void)
       {
         return m_input;
       }
#endif

      /**
       * Dispatch NxWidget-related events.  Normally used only internally
       * but there is one use case where messages are injected here from
       * CMenus.
       *
       * @param eventmsg.  The received NxWidget event message.
       * @return True if the message was properly dispatched.  false is
        *   return on any failure.
       */

      bool dispatchEvent(FAR struct SEventMsg *eventmsg);

      /**
       * Cleanup and exit Twm4Nx abnormally.
       */

      void abort(void);
    };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NX_HXX
