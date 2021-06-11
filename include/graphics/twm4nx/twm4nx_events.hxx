/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/twm4nx/twm4nx_events.hxx
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

    // Recipient == BACKGROUND

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
