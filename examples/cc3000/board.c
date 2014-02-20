/****************************************************************************
 * apps/examples/cc3000/board.c
 *
 * This code is based on the TI sample code "Basic WiFi Application"
 * and has the callback routines that TI expects for their library.
 *
 * TI uses callback routines to make their library portable: these routines,
 * and the routines in the SPI files, will be different for an Arduino,
 * a TI MSP430, a PIC, etc. but the core library shouldn't have to be
 * changed.
 *
 * Derives from an application to demo an Arduino connected to the TI CC3000
 *
 *   Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Don't sue me if my code blows up your board and burns down your house
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "board.h"
#include <stdbool.h>
#include <nuttx/wireless/cc3000/wlan.h>
#include <nuttx/wireless/cc3000/hci.h>
#include <nuttx/wireless/cc3000.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NETAPP_IPCONFIG_MAC_OFFSET     (20)
#define CC3000_APP_BUFFER_SIZE         (5)
#define CC3000_RX_BUFFER_OVERHEAD_SIZE (20)

/****************************************************************************
 * Public Data
 ****************************************************************************/

volatile unsigned long ulSmartConfigFinished,
  ulCC3000Connected,
  ulCC3000DHCP,
  OkToDoShutDown,
  ulCC3000DHCP_configured;

volatile uint8_t ucStopSmartConfig;

uint8_t asyncNotificationWaiting = false;
long lastAsyncEvent;
uint8_t dhcpIPAddress[4];

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/* The TI library calls this routine when asynchronous events happen.
 *
 *   For example you tell the CC3000 to turn itself on and connect
 *   to an access point then your code can go on to do its own thing.
 *   When the CC3000 is done configuring itself (e.g. it gets an IP
 *   address from the DHCP server) it will call this routine so you
 *   can take appropriate action.
 */

void CC3000_AsyncCallback(long lEventType, char * data, uint8_t length)
{
  lastAsyncEvent = lEventType;

  switch (lEventType)
    {
    case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
      ulSmartConfigFinished = 1;
      ucStopSmartConfig     = 1;
      asyncNotificationWaiting=true;
      break;

    case HCI_EVNT_WLAN_UNSOL_CONNECT:
      ulCC3000Connected = 1;
      asyncNotificationWaiting=true;
      break;

    case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
      ulCC3000Connected = 0;
      ulCC3000DHCP      = 0;
      ulCC3000DHCP_configured = 0;
      asyncNotificationWaiting=true;
      break;

    case HCI_EVNT_WLAN_UNSOL_DHCP:
      /* Notes:
       * 1) IP config parameters are received swapped
       * 2) IP config parameters are valid only if status is OK, i.e. ulCC3000DHCP becomes 1
       * only if status is OK, the flag is set to 1 and the addresses are valid
       */

      if (*(data + NETAPP_IPCONFIG_MAC_OFFSET) == 0)
        {
          ulCC3000DHCP = 1;
          dhcpIPAddress[0] = data[3];
          dhcpIPAddress[1] = data[2];
          dhcpIPAddress[2] = data[1];
          dhcpIPAddress[3] = data[0];
        }
      else
        {
          ulCC3000DHCP = 0;
          dhcpIPAddress[0] = 0;
          dhcpIPAddress[1] = 0;
          dhcpIPAddress[2] = 0;
          dhcpIPAddress[3] = 0;
        }
      asyncNotificationWaiting=true;
      break;

    case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
      OkToDoShutDown = 1;
      asyncNotificationWaiting=true;
      break;

    default:
      asyncNotificationWaiting=true;
      break;
    }
}

/* The TI library calls these routines on CC3000 startup.
 *
 *  This library does not send firmware, driver, or bootloader patches
 *  so we do nothing and we return NULL.
 */

char *SendFirmwarePatch(unsigned long *Length)
{
  *Length = 0;
  return NULL;
}

char *SendDriverPatch(unsigned long *Length)
{
  *Length = 0;
  return NULL;
}

char *SendBootloaderPatch(unsigned long *Length)
{
  *Length = 0;
  return NULL;
}

/* This is my routine to simplify CC3000 startup.
 *
 *   It sets the Arduino pins then calls the normal CC3000 routines
 *   wlan_init() with all the callbacks and wlan_start() with 0
 *   to indicate we're not sending any patches.
 */

void CC3000_Init(void)
{
  static bool once = false;

  if (!once)
     {
       wireless_archinitialize(132);
       once = true;
     }

  cc3000_wlan_init(132, CC3000_AsyncCallback,
    SendFirmwarePatch,
    SendDriverPatch,
    SendBootloaderPatch);

  wlan_start(0);
}
