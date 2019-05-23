/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/twm4nx_events.hxx
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_EVENTS_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_EVENTS_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <nuttx/config.h>

#include <cstdint>
#include <cstdbool>

#include <nuttx/nx/nxglib.h>

/////////////////////////////////////////////////////////////////////////////
// Preprocessor Definitions
/////////////////////////////////////////////////////////////////////////////

// struct SRedrawEventMsg is the largest message as of this writing

#define MAX_EVENT_MSGSIZE    sizeof(struct SRedrawEventMsg)
#define MAX_EVENT_PAYLOAD    (MAX_EVENT_MSGSIZE - sizeof(uint16_t))

#define EVENT_CRITICAL       0x0800
#define EVENT_RECIPIENT(id)  ((id) & EVENT_RECIPIENT_MASK)
#define EVENT_ISCRITICAL(id) (((id) & EVENT_CRITICAL) != 0)

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
   * Event ID format:
   *
   * rrrr nooo oooo oooo
   *
   *   Bits 0-10:  Opcode
   *   Bit 11:     Critical event (cannot be discarded*)
   *   Bits 12-15: Recipient of the event
   *
   * * During a window resize operation, all events are ignored except for
   *   those that are marked critical.
   */

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
    EVENT_SYSTEM_ERROR         = 0x0801,  /**< Report system error */
    EVENT_SYSTEM_EXIT          = 0x0802,  /**< Terminate the Twm4Nx session */
    EVENT_SYSTEM_STARTUP       = 0x0003,  /**< Start an application */

    // Recipient == BACKGOUND

    EVENT_BACKGROUND_XYINPUT   = 0x1000,  /**< Poll for widget mouse/touch events */
    EVENT_BACKGROUND_REDRAW    = 0x1801,  /**< Redraw the background */

    // Recipient == ICONWIDGET

    EVENT_ICONWIDGET_GRAB      = 0x2000,  /**< Click on toolbar title */
    EVENT_ICONWIDGET_DRAG      = 0x2001,  /**< Drag window */
    EVENT_ICONWIDGET_UNGRAB    = 0x2002,  /**< Release click on toolbar */

    // Recipient == ICONMGR

    EVENT_ICONMGR_XYINPUT      = 0x3000,  /**< Poll for widget mouse/touch events */
    EVENT_ICONMGR_DEICONIFY    = 0x3001,  /**< De-iconify or raise the Icon Manager */

    // Recipient == MENU

    EVENT_MENU_XYINPUT         = 0x4000,  /**< Poll for widget mouse/touch events */
    EVENT_MENU_COMPLETE        = 0x4001,  /**< Menu selection complete */
    EVENT_MENU_SUBMENU         = 0x4002,  /**< Sub-menu selected */

    // Recipient == MAINMENU

    EVENT_MAINMENU_SELECT      = 0x5000,  /**< Main menu item selection */

    // Recipient == WINDOW

    EVENT_WINDOW_RAISE         = 0x6000,  /**< Raise window to the top of the hierarchy */
    EVENT_WINDOW_LOWER         = 0x6001,  /**< Lower window to the bottom of the hierarchy */
    EVENT_WINDOW_DEICONIFY     = 0x6002,  /**< De-iconify and raise window  */
    EVENT_WINDOW_DRAG          = 0x6003,  /**< Drag window */
    EVENT_WINDOW_DELETE        = 0x6804,  /**< Delete window */
    EVENT_WINDOW_DESKTOP       = 0x6005,  /**< Show the desktop */

    // Recipient == TOOLBAR

    EVENT_TOOLBAR_XYINPUT      = 0x7800,  /**< Poll for widget mouse/touch events */
    EVENT_TOOLBAR_GRAB         = 0x7001,  /**< Click on title widget */
    EVENT_TOOLBAR_UNGRAB       = 0x7002,  /**< Release click on title widget */
    EVENT_TOOLBAR_MENU         = 0x7003,  /**< Toolbar menu button released */
    EVENT_TOOLBAR_MINIMIZE     = 0x7004,  /**< Toolbar minimize button released */
    EVENT_TOOLBAR_TERMINATE    = 0x7005,  /**< Toolbar delete button released */

    // Recipient == BORDER

    // Recipient == RESIZE

    EVENT_RESIZE_XYINPUT       = 0x9800,  /**< Poll for widget mouse/touch events */
    EVENT_RESIZE_BUTTON        = 0x9801,  /**< Start or stop a resize sequence */
    EVENT_RESIZE_MOVE          = 0x9802,  /**< Mouse movement during a resize sequence */
    EVENT_RESIZE_PAUSE         = 0x9803,  /**< Pause resize operation when unclicked */
    EVENT_RESIZE_RESUME        = 0x9804,  /**< Resume resize operation when re-clicked */
    EVENT_RESIZE_STOP          = 0x9805,  /**< End a resize sequence on second press  */

    // Recipient == APP
    // All application defined events must (1) use recipient == EVENT_RECIPIENT_APP,
    // and (2) provide an instance of CTwm4NxEvent in the SEventMsg structure.

  };

  // Contexts for events.  These basically identify the source of the event
  // message.

  enum EEventContext
  {
    EVENT_CONTEXT_WINDOW = 0,
    EVENT_CONTEXT_TOOLBAR,
    EVENT_CONTEXT_BACKGROUND,
    EVENT_CONTEXT_ICONWIDGET,
    EVENT_CONTEXT_ICONMGR,
    EVENT_CONTEXT_MENU,
    EVENT_CONTEXT_RESIZE,
    NUM_CONTEXTS
  };

  /**
   * This type represents a generic messages, particularly button press
   * or release events.
   */

  struct SEventMsg
  {
    // Common fields

    uint16_t eventID;                   /**< Encoded event ID */
    FAR void *obj;                      /**< Context specific reference */
    FAR void *handler;                  /**< Context specific handler */

    // Event-specific fields

    struct nxgl_point_s pos;            /**< X/Y position */
    uint8_t context;                    /**< Button press context */
  };

  /**
   * This message form is used with CWindowEvent redraw commands
   */

  struct SRedrawEventMsg
  {
    // Common fields

    uint16_t eventID;                   /**< Encoded event ID */
    FAR void *obj;                      /**< Context specific reference */
    FAR void *handler;                  /**< Context specific handler */

    // Event-specific fields

    struct nxgl_rect_s rect;            /**< Region to be redrawn */
    bool more;                          /**< True: More redraw requests will follow */
  };

  /**
   * This message form is used with CWindowEVent mouse/touchscreen
   * input events
   */

  struct SXyInputEventMsg
  {
    // Common fields

    uint16_t eventID;                   /**< Encoded event ID */
    FAR void *obj;                      /**< Context specific reference */
    FAR void *handler;                  /**< Context specific handler */

    // Event-specific fields

    struct nxgl_point_s pos;            /**< X/Y position */
    uint8_t buttons;                    /**< Bit set of button presses */
  };

  /**
   * This message form of the message used by CWindowEvent for blocked and
   * keyboard input messages
   */

  struct SNxEventMsg
  {
    // Common fields

    uint16_t eventID;                   /**< Encoded event ID */
    FAR void *obj;                      /**< Context specific reference */
    FAR void *handler;                  /**< Context specific handler */

    // Event-specific fields

    FAR CWindowEvent *instance;         /**< X/Y position */
  };
}

#endif // __APPS_INCLUDE_GRAPHICS_TWM4NX_TWM4NX_EVENTS_HXX
