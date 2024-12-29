/****************************************************************************
 * apps/examples/usbserial/usbserial_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <sys/stat.h>
#include <sys/boardctl.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/usb/usbdev.h>
#include <nuttx/usb/usbdev_trace.h>

#ifdef CONFIG_CDCACM
#  include <nuttx/usb/cdcacm.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_USBSERIAL_INONLY) && defined(CONFIG_EXAMPLES_USBSERIAL_OUTONLY)
#  error "Cannot define both CONFIG_EXAMPLES_USBSERIAL_INONLY and _OUTONLY"
#endif
#if defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL) && defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG)
#  error "Cannot define both CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL and _ONLYBIG"
#endif

#if !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG) && !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL)
#  ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
#    define COUNTER_NEEDED 1
#  endif
#endif

#ifndef CONFIG_USBDEV_TRACE_INITIALIDSET
#  define CONFIG_USBDEV_TRACE_INITIALIDSET 0
#endif

#ifdef CONFIG_EXAMPLES_USBSERIAL_TRACEINIT
#  define TRACE_INIT_BITS       (TRACE_INIT_BIT)
#else
#  define TRACE_INIT_BITS       (0)
#endif

#define TRACE_ERROR_BITS        (TRACE_DEVERROR_BIT|TRACE_CLSERROR_BIT)

#ifdef CONFIG_EXAMPLES_USBSERIAL_TRACECLASS
#  define TRACE_CLASS_BITS      (TRACE_CLASS_BIT|TRACE_CLASSAPI_BIT|TRACE_CLASSSTATE_BIT)
#else
#  define TRACE_CLASS_BITS      (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSERIAL_TRACETRANSFERS
#  define TRACE_TRANSFER_BITS   (TRACE_OUTREQQUEUED_BIT|TRACE_INREQQUEUED_BIT|TRACE_READ_BIT|\
                                 TRACE_WRITE_BIT|TRACE_COMPLETE_BIT)
#else
#  define TRACE_TRANSFER_BITS   (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSERIAL_TRACECONTROLLER
#  define TRACE_CONTROLLER_BITS (TRACE_EP_BIT|TRACE_DEV_BIT)
#else
#  define TRACE_CONTROLLER_BITS (0)
#endif

#ifdef CONFIG_EXAMPLES_USBSERIAL_TRACEINTERRUPTS
#  define TRACE_INTERRUPT_BITS  (TRACE_INTENTRY_BIT|TRACE_INTDECODE_BIT|TRACE_INTEXIT_BIT)
#else
#  define TRACE_INTERRUPT_BITS  (0)
#endif

#define TRACE_BITSET            (TRACE_INIT_BITS|TRACE_ERROR_BITS|TRACE_CLASS_BITS|\
                                 TRACE_TRANSFER_BITS|TRACE_CONTROLLER_BITS|TRACE_INTERRUPT_BITS)
#ifdef CONFIG_CDCACM
#  define USBSER_DEVNAME "/dev/ttyACM0"
#else
#  define USBSER_DEVNAME "/dev/ttyUSB0"
#endif

#ifndef CONFIG_EXAMPLES_USBSERIAL_BUFSIZE
#  define CONFIG_EXAMPLES_USBSERIAL_BUFSIZE 256
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_USBSERIAL_ONLYBIG
static const char g_shortmsg[] = "Hello, World!!\n";
#endif

#ifndef CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL
static const char g_longmsg[] =
  "The Spanish Armada a Speech by Queen Elizabeth I of England\n"
  "Addressed to the English army at Tilbury Fort - 1588\n"
  "My loving people, we have been persuaded by some, that are careful of "
  "our safety, to take heed how we commit ourselves to armed multitudes, "
  "or fear of treachery; but I assure you, I do not desire to live to "
  "distrust my faithful and loving people.\n"
  "Let tyrants fear; I have always so behaved myself that, under God, I "
  "have placed my chiefest strength and safeguard in the loyal hearts and "
  "good will of my subjects. And therefore I am come amongst you at this "
  "time, not as for my recreation or sport, but being resolved, in the "
  "midst and heat of the battle, to live or die amongst you all; to lay "
  "down, for my God, and for my kingdom, and for my people, my honour and "
  "my blood, even the dust.\n"
  "I know I have but the body of a weak and feeble woman; but I have the "
  "heart of a king, and of a king of England, too; and think foul scorn "
  "hat Parma or Spain, or any prince of Europe, should dare to invade the "
  "borders of my realms: to which, rather than any dishonour should grow "
  "by me, I myself will take up arms; I myself will be your general, "
  "judge, and rewarder of every one of your virtues in the field.\n"
  "I know already, by your forwardness, that you have deserved rewards and "
  "crowns; and we do assure you, on the word of a prince, they shall be "
  "duly paid you. In the mean my lieutenant general shall be in my stead, "
  "than whom never prince commanded a more noble and worthy subject; not "
  "doubting by your obedience to my general, by your concord in the camp, "
  "and by your valour in the field, we shall shortly have a famous victory "
  "over the enemies of my God, of my kingdom, and of my people.\n";
#endif

#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
static char g_iobuffer[CONFIG_EXAMPLES_USBSERIAL_BUFSIZE];
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_USBDEV_TRACE
static int trace_callback(struct usbtrace_s *trace, void *arg)
{
  usbtrace_trprintf((trprintf_t)printf, trace->event, trace->value);
  return 0;
}

static void dumptrace(void)
{
  usbtrace_enumerate(trace_callback, NULL);
}
#else
#  define dumptrace()
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * usbserial_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_usbdev_ctrl_s ctrl;
  FAR void *handle;
#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
  int infd;
#endif
#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
  int outfd;
#endif
#ifdef COUNTER_NEEDED
  int count = 0;
#endif
  ssize_t nbytes;
#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
  int i;
  int j;
  int k;
#endif
  int ret;

  if (access(USBSER_DEVNAME, F_OK) < 0)
    {
      /* Initialize the USB serial driver */

      printf("usbserial_main: Registering USB serial driver\n");

#ifdef CONFIG_CDCACM

      ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
      ctrl.action   = BOARDIOC_USBDEV_CONNECT;
      ctrl.instance = 0;
      ctrl.handle   = &handle;

#else

      ctrl.usbdev   = BOARDIOC_USBDEV_PL2303;
      ctrl.action   = BOARDIOC_USBDEV_CONNECT;
      ctrl.instance = 0;
      ctrl.handle   = &handle;

#endif

      ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
      if (ret < 0)
        {
          printf("usbserial_main: ERROR: Failed to create the USB serial "
                  "device: %d\n", -ret);
          return 1;
        }

      printf("usbserial_main: Successfully registered the serial driver\n");
    }

#if defined(CONFIG_USBDEV_TRACE) && CONFIG_USBDEV_TRACE_INITIALIDSET != 0
  /* If USB tracing is enabled and tracing of initial USB events is
   * specified, then dump all collected trace data to stdout
   */

  sleep(5);
  dumptrace();
#endif

  /* Then, in any event, configure trace data collection as configured */

  usbtrace_enable(TRACE_BITSET);

  /* Open the USB serial device for writing (blocking) */

#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
  do
    {
      printf("usbserial_main: Opening USB serial driver\n");
      outfd = open(USBSER_DEVNAME, O_WRONLY);
      if (outfd < 0)
        {
          int errcode = errno;
          printf("usbserial_main: ERROR: Failed to open " USBSER_DEVNAME
                 " for writing: %d\n", errcode);

          /* ENOTCONN means that the USB device is not yet connected */

          if (errcode == ENOTCONN)
            {
              printf("usbserial_main: Not connected. Wait and try again.\n");
              sleep(5);
            }
          else
            {
              /* Give up on other errors */

              printf("usbserial_main:        Aborting\n");
              return 2;
            }
        }

      /* If USB tracing is enabled, then dump all collected trace data
       * to stdout
       */

      dumptrace();
    }
  while (outfd < 0);
#endif

  /* Open the USB serial device for reading (non-blocking) */

#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
  infd = open(USBSER_DEVNAME, O_RDONLY | O_NONBLOCK);
  if (infd < 0)
    {
      printf("usbserial_main: ERROR: Failed to open " USBSER_DEVNAME
             " for reading: %d\n", errno);
      close(outfd);
      return 3;
    }
#else
  do
    {
      infd = open(USBSER_DEVNAME, O_RDONLY | O_NONBLOCK);
      if (infd < 0)
        {
          int errcode = errno;
          printf("usbserial_main: ERROR: Failed to open " USBSER_DEVNAME
                 " for reading: %d\n", errno);

          /* ENOTCONN means that the USB device is not yet connected */

          if (errcode == ENOTCONN)
            {
              printf("usbserial_main: Not connected. Wait and try again.\n");
              sleep(5);
            }
          else
            {
              /* Give up on other errors */

              printf("usbserial_main:        Aborting\n");
              return 3;
            }
        }

      /* If USB tracing is enabled, then dump all collected trace data
       * to stdout
       */

      dumptrace();
    }
  while (infd < 0);
#endif
#endif

  printf("usbserial_main: Successfully opened the serial driver\n");

  /* Send messages and get responses -- forever */

  for (; ; )
    {
      /* Test IN (device-to-host) messages */

#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
#if !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG) && !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL)
      if (count < 8)
        {
          printf("usbserial_main: Saying hello\n");
          nbytes = write(outfd, g_shortmsg, sizeof(g_shortmsg));
          count++;
        }
      else
        {
          printf("usbserial_main: Reciting QEI's speech of 1588\n");
          nbytes = write(outfd, g_longmsg, sizeof(g_longmsg));
          count = 0;
        }

#elif !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL)
      printf("usbserial_main: Reciting QEI's speech of 1588\n");
      nbytes = write(outfd, g_longmsg, sizeof(g_longmsg));

#else /* !defined(CONFIG_EXAMPLES_USBSERIAL_ONLYBIG) */
      printf("usbserial_main: Saying hello\n");
      nbytes = write(outfd, g_shortmsg, sizeof(g_shortmsg));
#endif

      /* Test if the write was successful */

      if (nbytes < 0)
        {
          printf("usbserial_main: ERROR: write failed: %d\n", errno);
#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
          close(infd);
#endif
          close(outfd);
          return 4;
        }

      printf("usbserial_main: %ld bytes sent\n", (long)nbytes);
#endif /* CONFIG_EXAMPLES_USBSERIAL_OUTONLY */

      /* Test OUT (host-to-device) messages */

#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
      /* Poll for incoming messages */

      printf("usbserial_main: Polling for OUT messages\n");
      for (i = 0; i < 5; i++)
        {
          memset(g_iobuffer, 'X', CONFIG_EXAMPLES_USBSERIAL_BUFSIZE);
          nbytes = read(infd, g_iobuffer, CONFIG_EXAMPLES_USBSERIAL_BUFSIZE);
          if (nbytes < 0)
            {
              int errorcode = errno;
              if (errorcode != EAGAIN)
                {
                  printf("usbserial_main: ERROR: read failed: %d\n", errno);
                  close(infd);
#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
                  close(outfd);
#endif
                  return 6;
                }
            }
          else
            {
              printf("usbserial_main: Received %ld bytes:\n", (long)nbytes);
              if (nbytes > 0)
                {
                  for (j = 0; j < nbytes; j += 16)
                    {
                      printf("usbserial_main: %03x: ", j);
                      for (k = 0; k < 16; k++)
                        {
                          if (k == 8)
                            {
                              printf(" ");
                            }

                          if (j + k < nbytes)
                            {
                              printf("%02x", g_iobuffer[j + k]);
                            }
                          else
                            {
                              printf("  ");
                            }
                        }

                      printf(" ");
                      for (k = 0; k < 16; k++)
                        {
                          if (k == 8)
                            {
                              printf(" ");
                            }

                          if (j + k < nbytes)
                            {
                              if (g_iobuffer[j + k] >= 0x20 &&
                                  g_iobuffer[j + k] < 0x7f)
                                {
                                  printf("%c", g_iobuffer[j + k]);
                                }
                              else
                                {
                                  printf(".");
                                }
                            }
                           else
                            {
                              printf(" ");
                            }
                        }

                      printf("\n");
                    }
                }
            }

#ifdef CONFIG_EXAMPLES_USBSERIAL_CONFIG_WAIT
          usleep(CONFIG_EXAMPLES_USBSERIAL_OUT_WAITING_TIME * 1000);
#else
          sleep(1);
#endif /* CONFIG_EXAMPLES_USBSERIAL_CONFIG_WAIT */
        }

#else /* CONFIG_EXAMPLES_USBSERIAL_INONLY */
      printf("usbserial_main: Waiting\n");
#ifdef CONFIG_EXAMPLES_USBSERIAL_CONFIG_WAIT
      usleep(CONFIG_EXAMPLES_USBSERIAL_IN_WAITING_TIME * 1000);
#else
      sleep(5);
#endif /* CONFIG_EXAMPLES_USBSERIAL_CONFIG_WAIT */
#endif /* CONFIG_EXAMPLES_USBSERIAL_INONLY */

      /* If USB tracing is enabled, then dump all collected trace data
       * to stdout
       */

      dumptrace();
    }

  /* Won't get here, but if we did this what we would have to do */

#ifndef CONFIG_EXAMPLES_USBSERIAL_INONLY
  close(infd);
#endif
#ifndef CONFIG_EXAMPLES_USBSERIAL_OUTONLY
  close(outfd);
#endif
  return 0;
}
