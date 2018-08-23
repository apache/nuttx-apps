/****************************************************************************
 * examples/can/can_main.c
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
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
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/can/can.h>

#include "can.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_CAN_READ)
#  define CAN_OFLAGS O_RDONLY
#elif defined(CONFIG_EXAMPLES_CAN_WRITE)
#  define CAN_OFLAGS O_WRONLY
#elif defined(CONFIG_EXAMPLES_CAN_READWRITE)
#  define CAN_OFLAGS O_RDWR
#  define CONFIG_EXAMPLES_CAN_READ 1
#  define CONFIG_EXAMPLES_CAN_WRITE 1
#endif

#ifdef CONFIG_EXAMPLES_CAN_WRITE
#  ifdef CONFIG_CAN_EXTID
#    define MAX_ID CAN_MAX_EXTMSGID
#  else
#    define MAX_ID CAN_MAX_STDMSGID
#  endif
#endif

#ifdef CONFIG_NSH_BUILTIN_APPS
#  ifdef CONFIG_EXAMPLES_CAN_WRITE
#    ifdef CONFIG_CAN_EXTID
#      define OPT_STR ":n:a:b:hs"
#    else
#      define OPT_STR ":n:a:b:h"
#    endif
#  else
#    define OPT_STR ":n:h"
#  endif
#else
#  ifdef CONFIG_EXAMPLES_CAN_WRITE
#    ifdef CONFIG_CAN_EXTID
#      define OPT_STR ":a:b:hs"
#    else
#      define OPT_STR ":a:b:h"
#    endif
#  else
#    define OPT_STR ":h"
#  endif
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s"
#ifdef CONFIG_NSH_BUILTIN_APPS
          " [-n <nmsgs]"
#endif
#ifdef CONFIG_EXAMPLES_CAN_WRITE
#ifdef CONFIG_CAN_EXTID
          " [-s]"
#endif
          " [-a <min-id>] [b <max-id>]"
#endif
          "\n",
          progname);
  fprintf(stderr, "USAGE: %s -h\n",
          progname);
  fprintf(stderr, "\nWhere:\n");
#ifdef CONFIG_NSH_BUILTIN_APPS
  fprintf(stderr, "-n <nmsgs>: The number of messages to send.  Default: 32\n");
#endif
#ifdef CONFIG_EXAMPLES_CAN_WRITE
#ifdef CONFIG_CAN_EXTID
  fprintf(stderr, "-s: Use standard IDs.  Default: Extended ID\n");
#endif
  fprintf(stderr, "-a <min-id>: The start message id.  Default 1\n");
  fprintf(stderr, "-b <max-id>: The start message id.  Default %d\n", MAX_ID);
#endif
  fprintf(stderr, "-h: Show this message and exit\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_main
 ****************************************************************************/

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int can_main(int argc, FAR char *argv[])
#endif
{
  struct canioc_bittiming_s bt;

#ifdef CONFIG_EXAMPLES_CAN_WRITE
  struct can_msg_s txmsg;
#ifdef CONFIG_CAN_EXTID
  bool extended = true;
  uint32_t msgid;
#else
  uint16_t msgid;
#endif
  long minid    = 1;
  long maxid    = MAX_ID;
  uint8_t msgdata;
#endif
  int msgdlc;
  int i;

#ifdef CONFIG_EXAMPLES_CAN_READ
  struct can_msg_s rxmsg;
#endif

  size_t msgsize;
  ssize_t nbytes;
  bool badarg   = false;
  bool help     = false;
#ifdef CONFIG_NSH_BUILTIN_APPS
  long nmsgs    = CONFIG_EXAMPLES_CAN_NMSGS;
  long msgno;
#endif
  int option;
  int fd;
  int errval    = 0;
  int ret;

  /* Parse command line parameters */

  while ((option = getopt(argc, argv, OPT_STR)) != ERROR)
    {
      switch (option)
        {
#ifdef CONFIG_EXAMPLES_CAN_WRITE
#ifdef CONFIG_CAN_EXTID
          case 's':
            extended = false;
            break;
#endif

          case 'a':
            minid = strtol(optarg, NULL, 10);
            if (minid < 1 || minid > maxid)
              {
                fprintf(stderr, "<min-id> out of range\n");
                badarg = true;
              }
            break;

          case 'b':
            maxid = strtol(optarg, NULL, 10);
            if (maxid < minid || maxid > MAX_ID)
              {
                fprintf(stderr, "ERROR: <max-id> out of range\n");
                badarg = true;
              }
            break;
#endif

          case 'h':
            help = true;
            break;

#ifdef CONFIG_NSH_BUILTIN_APPS
          case 'n':
            nmsgs = strtol(optarg, NULL, 10);
            if (nmsgs < 1)
              {
                fprintf(stderr, "ERROR: <nmsgs> out of range\n");
                badarg = true;
              }
            break;
#endif

          case ':':
            fprintf(stderr, "ERROR: Bad option argument\n");
            badarg = true;
            break;

          case '?':
          default:
            fprintf(stderr, "ERROR: Unrecognized option\n");
            badarg = true;
            break;
        }
    }

  if (badarg)
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (help)
    {
      show_usage(argv[0]);
      return EXIT_SUCCESS;
    }

#if defined(CONFIG_EXAMPLES_CAN_WRITE) && defined(CONFIG_CAN_EXTID)
  if (!extended && maxid > CAN_MAX_STDMSGID)
    {
      maxid = CAN_MAX_STDMSGID;
      if (minid > maxid)
        {
          minid = maxid;
        }
    }
#endif

  if (optind != argc)
    {
      fprintf(stderr, "ERROR: Garbage on command line\n");
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

#ifdef CONFIG_NSH_BUILTIN_APPS
  printf("nmsgs: %d\n", nmsgs);
#endif
#ifdef CONFIG_EXAMPLES_CAN_WRITE
  printf("min ID: %ld max ID: %ld\n", minid, maxid);
#endif

  /* Initialization of the CAN hardware is performed by board-specific,
   * logic external prior to running this test.
   */

  /* Open the CAN device for reading */

  fd = open(CONFIG_EXAMPLES_CAN_DEVPATH, CAN_OFLAGS);
  if (fd < 0)
    {
      printf("ERROR: open %s failed: %d\n",
             CONFIG_EXAMPLES_CAN_DEVPATH, errno);
      errval = 2;
      goto errout_with_dev;
    }

  /* Show bit timing information if provided by the driver.  Not all CAN
   * drivers will support this IOCTL.
   */

  ret = ioctl(fd, CANIOC_GET_BITTIMING, (unsigned long)((uintptr_t)&bt));
  if (ret < 0)
    {
      printf("Bit timing not available: %d\n", errno);
    }
  else
    {
      printf("Bit timing:\n");
      printf("   Baud: %lu\n", (unsigned long)bt.bt_baud);
      printf("  TSEG1: %u\n", bt.bt_tseg1);
      printf("  TSEG2: %u\n", bt.bt_tseg2);
      printf("    SJW: %u\n", bt.bt_sjw);
    }

  /* Now loop the appropriate number of times, performing one loopback test
   * on each pass.
   */

#ifdef CONFIG_EXAMPLES_CAN_WRITE
  msgdlc  = 1;
  msgid   = minid;
  msgdata = 0;
#endif

#ifdef CONFIG_NSH_BUILTIN_APPS
  for (msgno = 0; msgno < nmsgs; msgno++)
#else
  for (; ; )
#endif
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

#ifdef CONFIG_EXAMPLES_CAN_WRITE

      /* Construct the next TX message */

      txmsg.cm_hdr.ch_id     = msgid;
      txmsg.cm_hdr.ch_rtr    = false;
      txmsg.cm_hdr.ch_dlc    = msgdlc;
#ifdef CONFIG_CAN_ERRORS
      txmsg.cm_hdr.ch_error  = 0;
#endif
#ifdef CONFIG_CAN_EXTID
      txmsg.cm_hdr.ch_extid  = extended;
#endif
      txmsg.cm_hdr.ch_unused = 0;

      for (i = 0; i < msgdlc; i++)
        {
          txmsg.cm_data[i] = msgdata + i;
        }

      /* Send the TX message */

      msgsize = CAN_MSGLEN(msgdlc);
      nbytes = write(fd, &txmsg, msgsize);
      if (nbytes != msgsize)
        {
          printf("ERROR: write(%ld) returned %ld\n",
                 (long)msgsize, (long)nbytes);
          errval = 3;
          goto errout_with_dev;
        }

      printf("  ID: %4u DLC: %d\n", msgid, msgdlc);

#endif

#ifdef CONFIG_EXAMPLES_CAN_READ

      /* Read the RX message */

      msgsize = sizeof(struct can_msg_s);
      nbytes = read(fd, &rxmsg, msgsize);
      if (nbytes < CAN_MSGLEN(0) || nbytes > msgsize)
        {
          printf("ERROR: read(%ld) returned %ld\n",
                 (long)msgsize, (long)nbytes);
          errval = 4;
          goto errout_with_dev;
        }

      printf("  ID: %4u DLC: %u\n",
             rxmsg.cm_hdr.ch_id, rxmsg.cm_hdr.ch_dlc);

      msgdlc = rxmsg.cm_hdr.ch_dlc;

#ifdef CONFIG_CAN_ERRORS
      /* Check for error reports */

      if (rxmsg.cm_hdr.ch_error != 0)
        {
          printf("ERROR: CAN error report: [0x%04x]\n", rxmsg.cm_hdr.ch_id);
          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TXTIMEOUT) != 0)
            {
              printf("  TX timeout\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_LOSTARB) != 0)
            {
              printf("  Lost arbitration: %02x\n", rxmsg.cm_data[0]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_CONTROLLER) != 0)
            {
              printf("  Controller error: %02x\n", rxmsg.cm_data[1]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_PROTOCOL) != 0)
            {
              printf("  Protocol error: %02x %02x\n", rxmsg.cm_data[2], rxmsg.cm_data[3]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TRANSCEIVER) != 0)
            {
              printf("  Transceiver error: %02x\n", rxmsg.cm_data[4]);
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_NOACK) != 0)
            {
              printf("  No ACK received on transmission\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_BUSOFF) != 0)
            {
              printf("  Bus off\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_BUSERROR) != 0)
            {
              printf("  Bus error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_RESTARTED) != 0)
            {
              printf("  Controller restarted\n");
            }
        }
      else
#endif
        {
#if defined(CONFIG_EXAMPLES_CAN_WRITE) && defined(CONFIG_CAN_LOOPBACK)

          /* Verify that the received messages are the same */

          if (memcmp(&txmsg.cm_hdr, &rxmsg.cm_hdr, sizeof(struct can_hdr_s)) != 0)
            {
              printf("ERROR: Sent header does not match received header:\n");
              lib_dumpbuffer("Sent header",
                             (FAR const uint8_t *)&txmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              lib_dumpbuffer("Received header",
                             (FAR const uint8_t *)&rxmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              errval = 4;
              goto errout_with_dev;
            }

          if (memcmp(txmsg.cm_data, rxmsg.cm_data, msgdlc) != 0)
            {
              printf("ERROR: Data does not match. DLC=%d\n", msgdlc);
              for (i = 0; i < msgdlc; i++)
                {
                  printf("  %d: TX 0x%02x RX 0x%02x\n",
                         i, txmsg.cm_data[i], rxmsg.cm_data[i]);
                  errval = 5;
                  goto errout_with_dev;
                }
            }

          /* Report success */

          printf("  ID: %4u DLC: %d -- OK\n", msgid, msgdlc);

#else

          /* Print the data received */

          printf("Data received:\n");
          for (i = 0; i < msgdlc; i++)
            {
              printf("  %d: 0x%02x\n", i, rxmsg.cm_data[i]);
            }
#endif
        }
#endif

#ifdef CONFIG_EXAMPLES_CAN_WRITE

      /* Set up for the next pass */

      msgdata += msgdlc;

      if (++msgid > maxid)
        {
          msgid = minid;
        }

      if (++msgdlc > CAN_MAXDATALEN)
        {
          msgdlc = 1;
        }
#endif
    }

errout_with_dev:
  close(fd);

  printf("Terminating!\n");
  fflush(stdout);
  return errval;
}
