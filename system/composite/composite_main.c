/****************************************************************************
 * apps/system/composite/composite_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/boardctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/composite.h>
#include <nuttx/usb/cdcacm.h>
#include <nuttx/usb/usbdev_trace.h>

#include "composite.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* A lot of effort to avoid warning about dumptrace() not being used */

#undef NEED_DUMPTRACE
#ifdef CONFIG_USBDEV_TRACE
#  if CONFIG_USBDEV_TRACE_INITIALIDSET != 0
#    define NEED_DUMPTRACE 1
#  endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* All global variables used by this add-on are packed into a structure in
 * order to avoid name collisions.
 */

struct composite_state_s g_composite;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_memory_usage
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_COMPOSITE_DEBUGMM
static void show_memory_usage(struct mallinfo *mmbefore,
                              struct mallinfo *mmafter)
{
  int diff;

  printf("              total       used       free    largest\n");
  printf("Before:%11d%11d%11d%11d\n",
         mmbefore->arena, mmbefore->uordblks, mmbefore->fordblks,
         mmbefore->mxordblk);
  printf("After: %11d%11d%11d%11d\n",
         mmafter->arena, mmafter->uordblks, mmafter->fordblks,
         mmafter->mxordblk);

  diff = mmbefore->uordblks - mmafter->uordblks;
  if (diff < 0)
    {
      printf("Change:%11d allocated\n", -diff);
    }
  else if (diff > 0)
    {
      printf("Change:%11d freed\n", diff);
    }
}
#else
# define show_memory_usage(mm1, mm2)
#endif

/****************************************************************************
 * Name: check_test_memory_usage
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_COMPOSITE_DEBUGMM
static void check_test_memory_usage(FAR const char *msg)
{
  /* Get the current memory usage */

  g_composite.mmcurrent = mallinfo();

  /* Show the change from the previous time */

  printf("\%s:\n", msg);
  show_memory_usage(&g_composite.mmprevious, &g_composite.mmcurrent);

  /* Set up for the next test */

  g_composite.mmprevious = g_composite.mmcurrent;
}
#else
#  define check_test_memory_usage(msg)
#endif

/****************************************************************************
 * Name: check_test_memory_usage
 ****************************************************************************/

#ifdef CONFIG_SYSTEM_COMPOSITE_DEBUGMM
static void final_memory_usage(FAR const char *msg)
{
  /* Get the current memory usage */

  g_composite.mmcurrent = mallinfo();

  /* Show the change from the previous time */

  printf("\n%s:\n", msg);
  show_memory_usage(&g_composite.mmstart, &g_composite.mmcurrent);
}
#else
#  define final_memory_usage(msg)
#endif

/****************************************************************************
 * Name: composite_enumerate
 ****************************************************************************/

#ifdef NEED_DUMPTRACE
static int composite_enumerate(struct usbtrace_s *trace, void *arg)
{
  switch (trace->event)
    {
    case TRACE_DEVINIT:
      printf("USB controller initialization: %04x\n", trace->value);
      break;

    case TRACE_DEVUNINIT:
      printf("USB controller un-initialization: %04x\n", trace->value);
      break;

    case TRACE_DEVREGISTER:
      printf("usbdev_register(): %04x\n", trace->value);
      break;

    case TRACE_DEVUNREGISTER:
      printf("usbdev_unregister(): %04x\n", trace->value);
      break;

    case TRACE_EPCONFIGURE:
      printf("Endpoint configure(): %04x\n", trace->value);
      break;

    case TRACE_EPDISABLE:
      printf("Endpoint disable(): %04x\n", trace->value);
      break;

    case TRACE_EPALLOCREQ:
      printf("Endpoint allocreq(): %04x\n", trace->value);
      break;

    case TRACE_EPFREEREQ:
      printf("Endpoint freereq(): %04x\n", trace->value);
      break;

    case TRACE_EPALLOCBUFFER:
      printf("Endpoint allocbuffer(): %04x\n", trace->value);
      break;

    case TRACE_EPFREEBUFFER:
      printf("Endpoint freebuffer(): %04x\n", trace->value);
      break;

    case TRACE_EPSUBMIT:
      printf("Endpoint submit(): %04x\n", trace->value);
      break;

    case TRACE_EPCANCEL:
      printf("Endpoint cancel(): %04x\n", trace->value);
      break;

    case TRACE_EPSTALL:
      printf("Endpoint stall(true): %04x\n", trace->value);
      break;

    case TRACE_EPRESUME:
      printf("Endpoint stall(false): %04x\n", trace->value);
      break;

    case TRACE_DEVALLOCEP:
      printf("Device allocep(): %04x\n", trace->value);
      break;

    case TRACE_DEVFREEEP:
      printf("Device freeep(): %04x\n", trace->value);
      break;

    case TRACE_DEVGETFRAME:
      printf("Device getframe(): %04x\n", trace->value);
      break;

    case TRACE_DEVWAKEUP:
      printf("Device wakeup(): %04x\n", trace->value);
      break;

    case TRACE_DEVSELFPOWERED:
      printf("Device selfpowered(): %04x\n", trace->value);
      break;

    case TRACE_DEVPULLUP:
      printf("Device pullup(): %04x\n", trace->value);
      break;

    case TRACE_CLASSBIND:
      printf("Class bind(): %04x\n", trace->value);
      break;

    case TRACE_CLASSUNBIND:
      printf("Class unbind(): %04x\n", trace->value);
      break;

    case TRACE_CLASSDISCONNECT:
      printf("Class disconnect(): %04x\n", trace->value);
      break;

    case TRACE_CLASSSETUP:
      printf("Class setup(): %04x\n", trace->value);
      break;

    case TRACE_CLASSSUSPEND:
      printf("Class suspend(): %04x\n", trace->value);
      break;

    case TRACE_CLASSRESUME:
      printf("Class resume(): %04x\n", trace->value);
      break;

    case TRACE_CLASSRDCOMPLETE:
      printf("Class RD request complete: %04x\n", trace->value);
      break;

    case TRACE_CLASSWRCOMPLETE:
      printf("Class WR request complete: %04x\n", trace->value);
      break;

    default:
      switch (TRACE_ID(trace->event))
        {
        case TRACE_CLASSAPI_ID:        /* Other class driver system API calls */
          printf("Class API call %d: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_CLASSSTATE_ID:      /* Track class driver state changes */
          printf("Class state %d: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTENTRY_ID:        /* Interrupt handler entry */
          printf("Interrupt %d entry: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTDECODE_ID:       /* Decoded interrupt trace->event */
          printf("Interrupt decode %d: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INTEXIT_ID:         /* Interrupt handler exit */
          printf("Interrupt %d exit: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_OUTREQQUEUED_ID:    /* Request queued for OUT endpoint */
          printf("EP%d OUT request queued: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_INREQQUEUED_ID:     /* Request queued for IN endpoint */
          printf("EP%d IN request queued: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_READ_ID:            /* Read (OUT) action */
          printf("EP%d OUT read: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_WRITE_ID:           /* Write (IN) action */
          printf("EP%d IN write: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_COMPLETE_ID:        /* Request completed */
          printf("EP%d request complete: %04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_DEVERROR_ID:        /* USB controller driver error event */
          printf("Controller error: %02x:%04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        case TRACE_CLSERROR_ID:        /* USB class driver error event */
          printf("Class error: %02x:%04x\n",
                 TRACE_DATA(trace->event), trace->value);
          break;

        default:
          printf("Unrecognized event: %02x:%02x:%04x\n",
                  TRACE_ID(trace->event) >> 8,
                  TRACE_DATA(trace->event), trace->value);
          break;
        }
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: dumptrace
 *
 * Description:
 *   Dump collected trace data.
 *
 ****************************************************************************/

#ifdef NEED_DUMPTRACE
static int dumptrace(void)
{
  int ret;

  ret =  usbtrace_enumerate(composite_enumerate, NULL);
  if (ret < 0)
    {
      printf("dumptrace: usbtrace_enumerate failed: %d\n", -ret);
    }

  return ret;
}
#else
#  define dumptrace() (OK)
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * conn_main
 *
 * Description:
 *   This is the main program that configures the USB mass storage device
 *   and exports the LUN(s).  If CONFIG_NSH_BUILTIN_APPS is defined
 *   in the NuttX configuration, then this program can be executed by
 *   entering the "conn" command at the NSH console.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  int config = CONFIG_SYSTEM_COMPOSITE_DEFCONFIG;
  int ret;

  /* If this program is implemented as the NSH 'conn' command, then we need
   * to do a little error checking to assure that we are not being called
   * re-entrantly.
   */

  /* Check if there is a non-NULL USB mass storage device handle (meaning
   * that the composite device is already configured).
   */

  if (g_composite.cmphandle)
    {
      fprintf(stderr, "conn_main: ERROR: Already connected\n");
      return 1;
    }

  /* There is one optional argument.. the interface configuration ID */

  if (argc == 2)
    {
      config = atoi(argv[1]);
    }
  else if (argc > 2)
    {
      fprintf(stderr, "conn_main: ERROR: Too many arguments: %d\n", argc);
      return EXIT_FAILURE;
    }

  /* Initialize USB trace output IDs */

  usbtrace_enable(TRACE_BITSET);

#ifdef CONFIG_SYSTEM_COMPOSITE_DEBUGMM
  g_composite.mmstart    = mallinfo();
  g_composite.mmprevious = g_composite.mmstart;
#endif

  /* Perform architecture-specific initialization */

  printf("conn_main: Performing architecture-specific initialization\n");

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_INITIALIZE;
  ctrl.instance = 0;
  ctrl.config   = config;
  ctrl.handle   = NULL;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("conn_main: boardctl(BOARDIOC_USBDEV_CONTROL) failed: %d\n",
             -ret);
      return 1;
    }

  check_test_memory_usage("After boardctl(BOARDIOC_USBDEV_CONTROL)");

  /* Initialize the USB composite device device */

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_CONNECT;
  ctrl.instance = 0;
  ctrl.config   = config;
  ctrl.handle   = &g_composite.cmphandle;

  ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
  if (ret < 0)
    {
      printf("conn_main: boardctl(BOARDIOC_USBDEV_CONTROL) failed: %d\n",
             -ret);
      return 1;
    }

  check_test_memory_usage("After boardctl(BOARDIOC_USBDEV_CONTROL)");

#ifdef NEED_DUMPTRACE
  /* This thread will hang around and monitor the USB activity */

  /* Now looping */

  for (; ; )
    {
      /* Sleep for a bit */

      fflush(stdout);
      sleep(5);

      /* Dump trace data */

      printf("\n" "conn_main: USB TRACE DATA:\n");
      ret = dumptrace();
      if (ret < 0)
        {
          break;
        }

      check_test_memory_usage("After usbtrace_enumerate()");
    }
#endif

  /* Dump debug memory usage */

  printf("conn_main: Exiting\n");
  final_memory_usage("Final memory usage");
  return 0;
}

/****************************************************************************
 * disconn_main
 *
 * Description:
 *   This is a program entry point that will disconnect the USB mass storage
 *   device.  This program is only available if CONFIG_SYSTEM_COMPOSITE = y
 *   is defined in the NuttX configuration.  In that case, this program can
 *   be executed by entering the "msdis" command at the NSH console.
 *
 ****************************************************************************/

int disconn_main(int argc, char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  int config = CONFIG_SYSTEM_COMPOSITE_DEFCONFIG;

  /* First check if the USB mass storage device is already connected */

  if (!g_composite.cmphandle)
    {
      fprintf(stderr, "disconn_main: ERROR: Not connected\n");
      return 1;
    }

  check_test_memory_usage("Since MS connection");

  /* There is one optional argument.. the interface configuration ID */

  if (argc == 2)
    {
      config = atoi(argv[1]);
    }
  else if (argc > 2)
    {
      fprintf(stderr, "conn_main: ERROR: Too many arguments: %d\n", argc);
      return EXIT_FAILURE;
    }

  /* Then disconnect the device and uninitialize the composite driver */

  ctrl.usbdev   = BOARDIOC_USBDEV_COMPOSITE;
  ctrl.action   = BOARDIOC_USBDEV_DISCONNECT;
  ctrl.instance = 0;
  ctrl.config   = config;
  ctrl.handle   = &g_composite.cmphandle;

  boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);

  g_composite.cmphandle = NULL;
  printf("disconn_main: Disconnected\n");
  check_test_memory_usage("After boardctl(BOARDIOC_USBDEV_CONTROL)");

  /* Dump debug memory usage */

  final_memory_usage("Final memory usage");
  return 0;
}
