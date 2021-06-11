/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/ctwmnxevent.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NXEVENT_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NXEVENT_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>
#include <cstdbool>

/////////////////////////////////////////////////////////////////////////////
// Implementation Class Definition
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  /**
   * Twm4Nx Event Handler base class
   */

  class CTwm4NxEvent
  {
    public:

      /**
       * A virtual destructor is required in order to override the INxWindow
       * destructor.  We do this because if we delete INxWindow, we want the
       * destructor of the class that inherits from INxWindow to run, not this
       * one.
       */

      virtual ~CTwm4NxEvent(void)
      {
      }

      /**
       * Handle Twm4Nx events.
       *
       * @param eventmsg.  The received NxWidget WINDOW event message.
       * @return True if the message was properly handled.  false is
       *   return on any failure.
       */

       virtual bool event(FAR struct SEventMsg *eventmsg)
       {
         return false;
       }
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_CTWM4NXEVENT_HXX
