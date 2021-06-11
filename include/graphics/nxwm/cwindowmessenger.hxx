/****************************************************************************
 * apps/include/graphics/nxwm/cwindowmessenger.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_NXWM_CWINDOWMESSENGER_HXX
#define __APPS_INCLUDE_GRAPHICS_NXWM_CWINDOWMESSENGER_HXX

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>

#include <nuttx/wqueue.h>
#include <nuttx/nx/nxtk.h>
#include <nuttx/nx/nxterm.h>

#include "graphics/nxwidgets/cwindoweventhandler.hxx"
#include "graphics/nxwidgets/cwidgetstyle.hxx"
#include "graphics/nxwidgets/cwidgetcontrol.hxx"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Implementation Classes
 ****************************************************************************/

#if defined(__cplusplus)

namespace NxWM
{
  /**
   * Forward references.
   */

  class IApplication;

  /**
   * The class CWindowMessenger integrates the widget control with some special
   * handling of mouse and keyboard inputs needs by NxWM.  It use used
   * in place of CWidgetControl whenever an NxWM window is created.
   *
   * CWindowMessenger cohabitates with CWidgetControl only because it needs the
   * CWidgetControl as an argument in its messaging.
   */

  class CWindowMessenger : public NXWidgets::CWindowEventHandler,
                           public NXWidgets::CWidgetControl
  {
  private:
    /** Structure that stores data for the work queue callback. */

    struct work_state_t
      {
        work_s work;
        CWindowMessenger *windowMessenger;
        void *instance;

        work_state_t() : work() {}
      };

    /** Work queue callback functions */

    static void inputWorkCallback(FAR void *arg);
    static void destroyWorkCallback(FAR void *arg);

#ifdef CONFIG_NX_XYINPUT
    /**
     * Handle an NX window mouse input event.
     */

    void handleMouseEvent(FAR const struct nxgl_point_s *pos, uint8_t buttons);
#endif

#ifdef CONFIG_NX_KBD
    /**
     * Handle a NX window keyboard input event.
     */

    void handleKeyboardEvent(void);
#endif

    /**
     * Handle a NX window blocked event
     *
     * @param arg - User provided argument (see nx_block or nxtk_block)
     */

    void handleBlockedEvent(FAR void *arg);

  public:

    /**
     * CWindowMessenger Constructor
     *
     * @param style The default style that all widgets on this display
     *   should use.  If this is not specified, the widget will use the
     *   values stored in the defaultCWidgetStyle object.
     */

     CWindowMessenger(FAR const NXWidgets::CWidgetStyle *style =
       (const NXWidgets::CWidgetStyle *)NULL);

    /**
     * CWindowMessenger Destructor.
     */

    ~CWindowMessenger(void);
  };
}
#endif // __cplusplus

#endif // __APPS_INCLUDE_GRAPHICS_NXWM_CWINDOWMESSENGER_HXX
