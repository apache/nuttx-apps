/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/twm4nx_widgetevents.hxx
// Twm4Nx Widget Event Handling
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_WIDGETEVENTS_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_WIDGETEVENTS_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdint>
#include <cstdbool>

#include <nuttx/nx/nxglib.h>
#include "graphics/nxwidgets/cwindoweventhandler.hxx"

/////////////////////////////////////////////////////////////////////////////
// Preprocessor Definitions
/////////////////////////////////////////////////////////////////////////////

#define MAX_EVENT_MSGSIZE sizeof(struct SEventMsg)
#define MAX_EVENT_PAYLOAD (MAX_EVENT_MSGSIZE - sizeof(uint16_t))

/////////////////////////////////////////////////////////////////////////////
// WidgetEvent
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class CNxTkWindow;  // Forward reference
}

namespace Twm4Nx
{
  class CWindow;      // Forward reference
  class CWindowEvent; // Forward reference
  class CTwm4NxEvent; // Forward reference
  class CTwm4Nx;      // Forward reference

  ///////////////////////////////////////////////////////////////////////////
  // Public Types
  ///////////////////////////////////////////////////////////////////////////

  /**
   * This enumeration identifies the recipient of the event
   */

  enum EEventRecipient
  {
    EVENT_RECIPIENT_SYSTEM     = 0x0000,  /**< Twm4Nx system event */
    EVENT_RECIPIENT_BACKGROUND = 0x1000,  /**< Background window event */
    EVENT_RECIPIENT_ICONWIDGET = 0x2000,  /**< Icon Widget event */
    EVENT_RECIPIENT_ICONMGR    = 0x3000,  /**< Icon Manager event */
    EVENT_RECIPIENT_MENU       = 0x4000,  /**< Menu related event */
    EVENT_RECIPIENT_MAINMENU   = 0x5000,  /**< Menu related event */
    EVENT_RECIPIENT_WINDOW     = 0x6000,  /**< Window related event */
    EVENT_RECIPIENT_TOOLBAR    = 0x7000,  /**< Toolbar related event */
    EVENT_RECIPIENT_BORDER     = 0x8000,  /**< Window border related event */
    EVENT_RECIPIENT_RESIZE     = 0x9000,  /**< Window resize event */
    EVENT_RECIPIENT_APP        = 0xa000,  /**< App received event via CTwn4NxEvent */
    EVENT_RECIPIENT_MASK       = 0xf000,  /**< Used to isolate recipient */
  };

 /**
  * Specific events include the recipient as part of the event ID encoding.
  */

  enum EEventID
  {
    // Recipient == SYSTEM

    EVENT_SYSTEM_NOP           = 0x0000,  /**< Null event */
    EVENT_SYSTEM_ERROR         = 0x0001,  /**< Report system error */
    EVENT_SYSTEM_EXIT          = 0x0002,  /**< Terminate the Twm4Nx session */

    // Recipient == BACKGOUND

    EVENT_BACKGROUND_XYINPUT   = 0x1000,  /**< Poll window for widget mouse/touch events */
    EVENT_BACKGROUND_KBDINPUT  = 0x1001,  /**< Poll window for widget keyboard events */
    EVENT_BACKGROUND_REDRAW    = 0x1002,  /**< Redraw the background */

    // Recipient == ICONWIDGET

    EVENT_ICONWIDGET_GRAB      = 0x2000,  /**< Click on toolbar title */
    EVENT_ICONWIDGET_DRAG      = 0x2001,  /**< Drag window */
    EVENT_ICONWIDGET_UNGRAB    = 0x2002,  /**< Release click on toolbar */

    // Recipient == ICONMGR

    // Recipient == MENU

    EVENT_MENU_IDENTIFY        = 0x4000,  /**< Describe the window */
    EVENT_MENU_VERSION         = 0x4001,  /**< Show the Twm4Nx version */
    EVENT_MENU_ICONIFY         = 0x4002,  /**< Tool bar minimize button pressed */
    EVENT_MENU_DEICONIFY       = 0x4003,  /**< Window icon pressed */
    EVENT_MENU_FUNCTION        = 0x4004,  /**< Perform function on unknown menu */
    EVENT_MENU_TITLE           = 0x4005,  /**< REVISIT: Really an action not an event */
    EVENT_MENU_ROOT            = 0x4006,  /**< REVISIT: Popup root menu */

    // Recipient == MAINMENU

    EVENT_MAINMENU_SELECT      = 0x5000,  /**< Main menu item selection */

    // Recipient == WINDOW

    EVENT_WINDOW_XYINPUT       = 0x6000,  /**< Poll window for widget mouse/touch events */
    EVENT_WINDOW_KBDINPUT      = 0x6001,  /**< Poll window for widget keyboard events */
    EVENT_WINDOW_FOCUS         = 0x6002,  /**< Enter modal state */
    EVENT_WINDOW_UNFOCUS       = 0x6003,  /**< Exit modal state */
    EVENT_WINDOW_RAISE         = 0x6004,  /**< Raise window to the top of the heirarchy */
    EVENT_WINDOW_LOWER         = 0x6005,  /**< Lower window to the bottom of the heirarchy */
    EVENT_WINDOW_DEICONIFY     = 0x6006,  /**< De-iconify and raise window  */
    EVENT_WINDOW_DRAG          = 0x6007,  /**< Drag window */
    EVENT_WINDOW_DELETE        = 0x6008,  /**< Delete window */

    // Recipient == TOOLBAR

    EVENT_TOOLBAR_GRAB         = 0x7000,  /**< Click on title widget */
    EVENT_TOOLBAR_UNGRAB       = 0x7001,  /**< Release click on title widget */
    EVENT_TOOLBAR_MENU         = 0x7002,  /**< Toolbar menu button released */
    EVENT_TOOLBAR_MINIMIZE     = 0x7003,  /**< Toolbar minimize button released */
    EVENT_TOOLBAR_RESIZE       = 0x7004,  /**< Toolbar resize button released */
    EVENT_TOOLBAR_TERMINATE    = 0x7005,  /**< Toolbar delete button released */

    // Recipient == BORDER

    // Recipient == RESIZE

    EVENT_RESIZE_START         = 0x9000,  /**< Start window resize */
    EVENT_RESIZE_VERTZOOM      = 0x9001,  /**< Zoom vertically only */
    EVENT_RESIZE_HORIZOOM      = 0x9002,  /**< Zoom horizontally only */
    EVENT_RESIZE_FULLZOOM      = 0x9003,  /**< Zoom both vertically and horizontally */
    EVENT_RESIZE_LEFTZOOM      = 0x9004,  /**< Zoom left only */
    EVENT_RESIZE_RIGHTZOOM     = 0x9005,  /**< Zoom right only */
    EVENT_RESIZE_TOPZOOM       = 0x9006,  /**< Zoom top only */
    EVENT_RESIZE_BOTTOMZOOM    = 0x9007,  /**< Zoom bottom only */

    // Recipient == APP
    // All application defined events must (1) use recepient == EVENT_RECIPIENT_APP,
    // and (2) provide an instance of CTwm4NxEvent in the SEventMsg structure.

  };

  // Contexts for button press events.  These basically identify the sender
  // of the event message.

  enum EButtonContext
  {
    EVENT_CONTEXT_WINDOW = 0,
    EVENT_CONTEXT_TOOLBAR,
    EVENT_CONTEXT_BACKGROUND,
    EVENT_CONTEXT_ICON,
    EVENT_CONTEXT_FRAME,
    EVENT_CONTEXT_ICONMGR,
    EVENT_CONTEXT_NAME,
    EVENT_CONTEXT_IDENTIFY,
    NUM_CONTEXTS
  };

  /**
   * This type represents a generic messages, particularly button press
   * or released events.
   */

  struct SEventMsg
  {
    uint16_t eventID;                   /**< Encoded event ID */
    struct nxgl_point_s pos;            /**< X/Y position */
    uint8_t context;                    /**< Button press context */
    FAR CTwm4NxEvent *handler;          /**< App event handler (APP recipient only) */
    FAR void *obj;                      /**< Window object (CWindow or CIconWidget) */
  };

  /**
   * This message form is used with CWindowEvent redraw commands
   */

  struct SRedrawEventMsg
  {
    uint16_t eventID;                   /**< Encoded event ID */
    struct nxgl_rect_s rect;            /**< Region to be redrawn */
    bool more;                          /**< True: More redraw requests will follow */
  };

  /**
   * This message for is used with CWindowEVent mouse/keyboard input events
   */

  struct SXyInputEventMsg
  {
    uint16_t eventID;                   /**< Encoded event ID */
    struct nxgl_point_s pos;            /**< X/Y position */
    uint8_t buttons;                    /**< Bit set of button presses */
    FAR void *obj;                      /**< Context specific reference */
  };

  /**
   * This is the alternative form of the message used by
   * CWindowEvent for blocked and keyboard input messages
   */

  struct SNxEventMsg
  {
    uint16_t eventID;                   /**< Encoded event ID */
    FAR CWindowEvent *instance;         /**< X/Y position */
    FAR void *obj;                      /**< Context specific reference */
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_WIDGETEVENTS_HXX
