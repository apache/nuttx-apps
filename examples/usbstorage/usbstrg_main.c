/****************************************************************************
 * examples/usbstorage/usbstrg_main.c
 *
 *   Copyright (C) 2008-2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <debug.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/usbdev_trace.h>

#include "usbstrg.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_USBSTRG_TRACEINIT
#  define TRACE_INIT_BITS       (TRACE_INIT_BIT)
#else
#  define TRACE_INIT_BITS       (0)
#endif

#define TRACE_ERROR_BITS        (TRACE_DEVERROR_BIT|TRACE_CLSERROR_BIT)

#ifdef CONFIG_EXAMPLES_USBSTRG_TRACECLASS
#  define TRACE_CLASS_BITS      (TRACE_CLASS_BIT|TRACE_CLASSAPI_BIT|TRACE_CLASSSTATE_BIT)
#else
#  define TRACE_CLASS_BITS      (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSTRG_TRACETRANSFERS
#  define TRACE_TRANSFER_BITS   (TRACE_OUTREQQUEUED_BIT|TRACE_INREQQUEUED_BIT|TRACE_READ_BIT|\
                                 TRACE_WRITE_BIT|TRACE_COMPLETE_BIT)
#else
#  define TRACE_TRANSFER_BITS   (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSTRG_TRACECONTROLLER
#  define TRACE_CONTROLLER_BITS (TRACE_EP_BIT|TRACE_DEV_BIT)
#else
#  define TRACE_CONTROLLER_BITS (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSTRG_TRACEINTERRUPTS
#  define TRACE_INTERRUPT_BITS  (TRACE_INTENTRY_BIT|TRACE_INTDECODE_BIT|TRACE_INTEXIT_BIT)
#else
#  define TRACE_INTERRUPT_BITS  (0)
#endif

#define TRACE_BITSET            (TRACE_INIT_BITS|TRACE_ERROR_BITS|TRACE_CLASS_BITS|\
                                 TRACE_TRANSFER_BITS|TRACE_CONTROLLER_BITS|TRACE_INTERRUPT_BITS)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This is the handle that references to this particular USB storage driver
 * instance.  It is only needed if the USB mass storage device example is
 * built using CONFIG_EXAMPLES_USBSTRG_BUILTIN.  In this case, the value
 * of the driver handle must be remembered between the 'msconn' and 'msdis'
 * commands.
 */

#ifdef CONFIG_EXAMPLES_USBSTRG_BUILTIN
static FAR void *g_mshandle;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: usbstrg_enumerate
 ****************************************************************************/

#ifdef CONFIG_USBDEV_TRACE
static int usbstrg_enumerate(struct usbtrace_s *trace, void *arg)
{
  switch (trace->event)
    {
    case TRACE_DEVINIT:
      message("USB controller initialization: %04x\n", trace->value);
      break;

    case TRACE_DEVUNINIT:
      message("USB controller un-initialization: %04x\n", trace->value);
      break;

    case TRACE_DEVREGISTER:
      message("usbdev_register(): %04x\n", trace->value);
      break;

    case TRACE_DEVUNREGISTER:
      message("usbdev_unregister(): %04x\n", trace->value);
      break;

    case TRACE_EPCONFIGURE:
      message("Endpoint configure(): %04x\n", trace->value);
      break;

    case TRACE_EPDISABLE:
      message("Endpoint disable(): %04x\n", trace->value);
      break;

    case TRACE_EPALLOCREQ:
      message("Endpoint allocreq(): %04x\n", trace->value);
      break;

    case TRACE_EPFREEREQ:
      message("Endpoint freereq(): %04x\n", trace->value);
      break;

    case TRACE_EPALLOCBUFFER:
      message("Endpoint allocbuffer(): %04x\n", trace->value);
      break;

    case TRACE_EPFREEBUFFER:
      message("Endpoint freebuffer(): %04x\n", trace->value);
      break;

    case TRACE_EPSUBMIT:
      message("Endpoint submit(): %04x\n", trace->value);
      break;

    case TRACE_EPCANCEL:
      message("Endpoint cancel(): %04x\n", trace->value);
      break;

    case TRACE_EPSTALL:
      message("Endpoint stall(true): %04x\n", trace->value);
      break;

    case TRACE_EPRESUME:
      message("Endpoint stall(false): %04x\n", trace->value);
      break;

    case TRACE_DEVALLOCEP:
      message("Device allocep(): %04x\n", trace->value);
      break;

    case TRACE_DEVFREEEP:
      message("Device freeep(): %04x\n", trace->value);
      break;

    case TRACE_DEVGETFRAME:
      message("Device getframe(): %04x\n", trace->value);
      break;

    case TRACE_DEVWAKEUP:
      message("Device wakeup(): %04x\n", trace->value);
      break;

    case TRACE_DEVSELFPOWERED:
      message("Device selfpowered(): %04x\n", trace->value);
      break;

    case TRACE_DEVPULLUP:
      message("Device pullup(): %04x\n", trace->value);
      break;

    case TRACE_CLASSBIND:
      message("Class bind(): %04x\n", trace->value);
      break;

    case TRACE_CLASSUNBIND:
      message("Class unbind(): %04x\n", trace->value);
      break;

    case TRACE_CLASSDISCONNECT:
      message("Class disconnect(): %04x\n", trace->value);
      break;

    case TRACE_CLASSSETUP:
      message("Class setup(): %04x\n", trace->value);
      break;

    case TRACE_CLASSSUSPEND:
      message("Class suspend(): %04x\n", trace->value);
      break;

    case TRACE_CLASSRESUME:
      message("Class resume(): %04x\n", trace->value);
      break;

    case TRACE_CLASSRDCOMPLETE:
      message("Class RD request complete: %04x\n", trace->value);
      break;

    case TRACE_CLASSWRCOMPLETE:
      message("Class WR request complete: %04x\n", trace->value);
      break;

    default:
      switch (TRACE_ID(trace->event))
        {
        case TRACE_CLASSAPI_ID:        /* Other class driver system API calls */
          message("Class API call %d: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_CLASSSTATE_ID:      /* Track class driver state changes */
          message("Class state %d: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTENTRY_ID:        /* Interrupt handler entry */
          message("Interrrupt %d entry: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTDECODE_ID:       /* Decoded interrupt trace->event */
          message("Interrrupt decode %d: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTEXIT_ID:         /* Interrupt handler exit */
          message("Interrrupt %d exit: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_OUTREQQUEUED_ID:    /* Request queued for OUT endpoint */
          message("EP%d OUT request queued: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INREQQUEUED_ID:     /* Request queued for IN endpoint */
          message("EP%d IN request queued: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_READ_ID:            /* Read (OUT) action */
          message("EP%d OUT read: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_WRITE_ID:           /* Write (IN) action */
          message("EP%d IN write: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_COMPLETE_ID:        /* Request completed */
          message("EP%d request complete: %04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_DEVERROR_ID:        /* USB controller driver error event */
          message("Controller error: %02x:%04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_CLSERROR_ID:        /* USB class driver error event */
          message("Class error: %02x:%04x\n", TRACE_DATA(trace->event), trace->value);
          break;

        default:
          message("Unrecognized event: %02x:%02x:%04x\n",
                  TRACE_ID(trace->event) >> 8, TRACE_DATA(trace->event), trace->value);
          break;
        }
    }
  return OK;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * user_start/msconn_main
 *
 * Description:
 *   This is the main program that configures the USB mass storage device
 *   and exports the LUN(s).  If CONFIG_EXAMPLES_USBSTRG_BUILTIN is defined
 *   in the NuttX configuration, then this program can be executed by
 *   entering the "msconn" command at the NSH console.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_USBSTRG_BUILTIN
#  define MAIN_NAME msconn_main
#  define MAIN_NAME_STRING "msconn"
#else
#  define MAIN_NAME user_start
#  define MAIN_NAME_STRING "user_start"
#endif

int MAIN_NAME(int argc, char *argv[])
{
  FAR void *handle;
  int ret;

  /* If this program is implemented as the NSH 'msconn' command, then we need to
   * do a little error checking to assure that we are not being called re-entrantly.
   */

#ifdef CONFIG_EXAMPLES_USBSTRG_BUILTIN

   /* Check if there is a non-NULL USB mass storage device handle (meaning that the
    * USB mass storage device is already configured).
    */

   if (g_mshandle)
     {
       message(MAIN_NAME_STRING ": ERROR: Already connected\n");
       return 1;
     }
#endif

  /* Initialize USB trace output IDs */

  usbtrace_enable(TRACE_BITSET);

  /* Register block drivers (architecture-specific) */

  message(MAIN_NAME_STRING ": Creating block drivers\n");
  ret = usbstrg_archinitialize();
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_archinitialize failed: %d\n", -ret);
      return 2;
    }

  /* Then exports the LUN(s) */

  message(MAIN_NAME_STRING ": Configuring with NLUNS=%d\n", CONFIG_EXAMPLES_USBSTRG_NLUNS);
  ret = usbstrg_configure(CONFIG_EXAMPLES_USBSTRG_NLUNS, &handle);
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_configure failed: %d\n", -ret);
      usbstrg_uninitialize(handle);
      return 3;
    }
  message(MAIN_NAME_STRING ": handle=%p\n", handle);

  message(MAIN_NAME_STRING ": Bind LUN=0 to %s\n", CONFIG_EXAMPLES_USBSTRG_DEVPATH1);
  ret = usbstrg_bindlun(handle, CONFIG_EXAMPLES_USBSTRG_DEVPATH1, 0, 0, 0, false);
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_bindlun failed for LUN 1 using %s: %d\n",
               CONFIG_EXAMPLES_USBSTRG_DEVPATH1, -ret);
      usbstrg_uninitialize(handle);
      return 4;
    }

#if CONFIG_EXAMPLES_USBSTRG_NLUNS > 1

  message(MAIN_NAME_STRING ": Bind LUN=1 to %s\n", CONFIG_EXAMPLES_USBSTRG_DEVPATH2);
  ret = usbstrg_bindlun(handle, CONFIG_EXAMPLES_USBSTRG_DEVPATH2, 1, 0, 0, false);
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_bindlun failed for LUN 2 using %s: %d\n",
               CONFIG_EXAMPLES_USBSTRG_DEVPATH2, -ret);
      usbstrg_uninitialize(handle);
      return 5;
    }

#if CONFIG_EXAMPLES_USBSTRG_NLUNS > 2

  message(MAIN_NAME_STRING ": Bind LUN=2 to %s\n", CONFIG_EXAMPLES_USBSTRG_DEVPATH3);
  ret = usbstrg_bindlun(handle, CONFIG_EXAMPLES_USBSTRG_DEVPATH3, 2, 0, 0, false);
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_bindlun failed for LUN 3 using %s: %d\n",
               CONFIG_EXAMPLES_USBSTRG_DEVPATH3, -ret);
      usbstrg_uninitialize(handle);
      return 6;
    }

#endif
#endif

  ret = usbstrg_exportluns(handle);
  if (ret < 0)
    {
      message(MAIN_NAME_STRING ": usbstrg_exportluns failed: %d\n", -ret);
      usbstrg_uninitialize(handle);
      return 7;
    }

  /* It this program was configued as an NSH command, then just exit now.
   * Also, if signals are not enabled (and, hence, sleep() is not supported.
   * then we have not real option but to exit now.
   */

#if !defined(CONFIG_EXAMPLES_USBSTRG_BUILTIN) && !defined(CONFIG_DISABLE_SIGNALS)

  /* Otherwise, this thread will hang around and monitor the USB storage activity */

  for (;;)
    {
      msgflush();
      sleep(5);

#  ifdef CONFIG_USBDEV_TRACE
      message("\nuser_start: USB TRACE DATA:\n");
      ret =  usbtrace_enumerate(usbstrg_enumerate, NULL);
      if (ret < 0)
        {
          message(MAIN_NAME_STRING ": usbtrace_enumerate failed: %d\n", -ret);
          usbstrg_uninitialize(handle);
          return 8;
        }
#  else
      message(MAIN_NAME_STRING ": Still alive\n");
#  endif
    }
#elif defined(CONFIG_EXAMPLES_USBSTRG_BUILTIN)

   /* Return the USB mass storage device handle so it can be used by the 'misconn'
    * command.
    */

   message(MAIN_NAME_STRING ": Connected\n");
   g_mshandle = handle;

#else /* defined(CONFIG_DISABLE_SIGNALS) */

  /* Just exit */
 
   message(MAIN_NAME_STRING ": Exiting\n");

#endif
   return 0;
}

/****************************************************************************
 * msdis_main
 *
 * Description:
 *   This is a program entry point that will disconnet the USB mass storage
 *   device.  This program is only available if CONFIG_EXAMPLES_USBSTRG_BUILTIN
 *   is defined in the NuttX configuration.  In that case, this program can
 *   be executed by entering the "msdis" command at the NSH console.
 *
 ****************************************************************************/

#ifdef CONFIG_EXAMPLES_USBSTRG_BUILTIN
int msdis_main(int argc, char *argv[])
{
  /* First check if the USB mass storage device is already connected */

  if (!g_mshandle)
    {
      message("msdis: ERROR: Not connected\n");
      return 1;
    }

  /* Then disconnect the device and uninitialize the USB mass storage driver */

   usbstrg_uninitialize(g_mshandle);
   g_mshandle = NULL;
   message("msdis: Disconnected\n");
   return 0;
}
#endif
