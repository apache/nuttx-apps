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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/can.h>

#include "can.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_EXAMPLES_CAN_READONLY)
#  undef CONFIG_EXAMPLES_CAN_WRITEONLY
#  undef CONFIG_EXAMPLES_CAN_READWRITE
#  define CAN_OFLAGS O_RDONLY
#elif defined(CONFIG_EXAMPLES_CAN_WRITEONLY)
#  undef CONFIG_EXAMPLES_CAN_READWRITE
#  define CAN_OFLAGS O_WRONLY
#else
#  undef CONFIG_EXAMPLES_CAN_READWRITE
#  define CONFIG_EXAMPLES_CAN_READWRITE 1
#  define CAN_OFLAGS O_RDWR
#endif

#ifndef CONFIG_EXAMPLES_CAN_NMSGS
#  define CONFIG_EXAMPLES_CAN_NMSGS 32
#endif

#define MAX_EXTID (1 << 29)
#define MAX_STDID (1 << 11)

#ifdef CONFIG_CAN_EXTID
#  define MAX_ID MAX_EXTID
#else
#  define MAX_ID MAX_STDID
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
#ifdef CONFIG_CAN_EXTID
  fprintf(stderr, "USAGE: %s [-s] [-n <nmsgs] [-a <min-id>] [b <max-id>]\n",
          progname);
#else
  fprintf(stderr, "USAGE: %s [-n <nmsgs] [-a <min-id>] [b <max-id>]\n",
          progname);
#endif
  fprintf(stderr, "USAGE: %s -h\n",
          progname);
  fprintf(stderr, "\nWhere:\n");
#ifdef CONFIG_CAN_EXTID
  fprintf(stderr, "-s: Use standard IDs.  Default: Extended ID\n");
#endif
  fprintf(stderr, "-n <nmsgs>: The number of messages to send.  Default: 32\n");
  fprintf(stderr, "-a <min-id>: The start message id.  Default 1\n");
  fprintf(stderr, "-b <max-id>: The start message id.  Default %d\n", MAX_ID - 1);
  fprintf(stderr, "-h: Show this message and exit\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: can_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int can_main(int argc, char *argv[])
#endif
{
#ifndef CONFIG_EXAMPLES_CAN_READONLY
  struct can_msg_s txmsg;
#ifdef CONFIG_CAN_EXTID
  bool extended;
  uint32_t msgid;
#else
  uint16_t msgid;
#endif
  long minid;
  long maxid;
  int msgdlc;
  uint8_t msgdata;
#endif

#ifndef CONFIG_EXAMPLES_CAN_WRITEONLY
  struct can_msg_s rxmsg;
#endif

  size_t msgsize;
  ssize_t nbytes;
  bool badarg;
  bool help;
  long nmsgs;
  int option;
  int fd;
  int errval = 0;
  int msgno;
  int ret;
  int i;

  /* Parse command line parameters */

  nmsgs    = CONFIG_EXAMPLES_CAN_NMSGS;
  minid    = 1;
  maxid    = MAX_ID - 1;
  badarg   = false;
#ifdef CONFIG_CAN_EXTID
  extended = true;
#endif
  badarg   = false;
  help     = false;

#ifdef CONFIG_CAN_EXTID
  while ((option = getopt(argc, argv, ":n:a:b:hs")) != ERROR)
#else
  while ((option = getopt(argc, argv, ":n:a:b:h")) != ERROR)
#endif
    {
      switch (option)
        {
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
            if (maxid < minid || maxid >= MAX_ID)
              {
                fprintf(stderr, "ERROR: <max-id> out of range\n");
                badarg = true;
              }
            break;

          case 'h':
            help = true;
            break;

#ifdef CONFIG_CAN_EXTID
          case 's':
            extended = false;
            break;
#endif

          case 'n':
            nmsgs = strtol(optarg, NULL, 10);
            if (nmsgs < 1)
              {
                fprintf(stderr, "ERROR: <nmsgs> out of range\n");
                badarg = true;
              }
            break;

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

#ifdef CONFIG_CAN_EXTID
  if (!extended && maxid >= MAX_STDID)
    {
      maxid = MAX_STDID - 1;
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

  printf("can_main: nmsgs: %d min ID: %d max ID: %d\n",
         nmsgs, minid, maxid);

  /* Initialization of the CAN hardware is performed by logic external to
   * this test.
   */

  printf("can_main: Initializing external CAN device\n");
  ret = can_devinit();
  if (ret != OK)
    {
      printf("can_main: can_devinit failed: %d\n", ret);
      errval = 1;
      goto errout;
    }

  /* Open the CAN device for reading */

  printf("can_main: Hardware initialized. Opening the CAN device\n");
  fd = open(CONFIG_EXAMPLES_CAN_DEVPATH, CAN_OFLAGS);
  if (fd < 0)
    {
      printf("can_main: open %s failed: %d\n",
              CONFIG_EXAMPLES_CAN_DEVPATH, errno);
      errval = 2;
      goto errout_with_dev;
    }

  /* Now loop the appropriate number of times, performing one loopback test
   * on each pass.
   */

#ifndef CONFIG_EXAMPLES_CAN_READONLY
  msgdlc  = 1;
  msgid   = minid;
  msgdata = 0;
#endif

  for (msgno = 0; msgno < nmsgs; msgno++)
    {
      /* Flush any output before the loop entered or from the previous pass
       * through the loop.
       */

      fflush(stdout);

      /* Construct the next TX message */

#ifndef CONFIG_EXAMPLES_CAN_READONLY
      txmsg.cm_hdr.ch_id     = msgid;
      txmsg.cm_hdr.ch_rtr    = false;
      txmsg.cm_hdr.ch_dlc    = msgdlc;
      txmsg.cm_hdr.ch_error  = 0;
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
#endif

#ifdef CONFIG_EXAMPLES_CAN_WRITEONLY
      printf("  ID: %4u DLC: %d\n", msgid, msgdlc);
#endif

      /* Read the RX message */

#ifndef CONFIG_EXAMPLES_CAN_WRITEONLY
      msgsize = sizeof(struct can_msg_s);
      nbytes = read(fd, &rxmsg, msgsize);
      if (nbytes < CAN_MSGLEN(0) || nbytes > msgsize)
        {
          printf("ERROR: read(%ld) returned %ld\n",
                 (long)msgsize, (long)nbytes);
          errval = 4;
          goto errout_with_dev;
        }
#endif

#ifndef CONFIG_EXAMPLES_CAN_READONLY
      printf("  ID: %4u DLC: %u\n",
             rxmsg.cm_hdr.ch_id, rxmsg.cm_hdr.ch_dlc);
#endif

      /* Check for error reports */

#ifndef CONFIG_EXAMPLES_CAN_WRITEONLY
      if (rxmsg.cm_hdr.ch_error != 0)
        {
          printf("ERROR: CAN error report: [%04x]\n", rxmsg.cm_hdr.ch_id);
          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_SYSTEM) != 0)
            {
              printf("  Driver internal error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_RXLOST) != 0)
            {
              printf("  RX Message Lost\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TXLOST) != 0)
            {
              printf("  TX Message Lost\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_ACCESS) != 0)
            {
              printf("  RAM Access Failure\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_TIMEOUT) != 0)
            {
              printf("  Timeout Occurred\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_PASSIVE) != 0)
            {
              printf("  Error Passive\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_CRC) != 0)
            {
              printf("  RX CRC Error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_BIT) != 0)
            {
              printf("  Bit Error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_ACK) != 0)
            {
              printf("  Acknowledge Error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_FORMAT) != 0)
            {
              printf("  Format Error\n");
            }

          if ((rxmsg.cm_hdr.ch_id & CAN_ERROR_STUFF) != 0)
            {
              printf("  Stuff Error\n");
            }
        }
      else
       {
          /* Verify that the received messages are the same */

#ifdef CONFIG_EXAMPLES_CAN_READWRITE
          if (memcmp(&txmsg.cm_hdr, &rxmsg.cm_hdr, sizeof(struct can_hdr_s)) != 0)
            {
              printf("ERROR: Sent header does not match received header:\n");
              lib_dumpbuffer("Sent header", (FAR const uint8_t*)&txmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              lib_dumpbuffer("Received header", (FAR const uint8_t*)&rxmsg.cm_hdr,
                             sizeof(struct can_hdr_s));
              errval = 4;
              goto errout_with_dev;
            }

          if (memcmp(txmsg.cm_data, rxmsg.cm_data, msgdlc) != 0)
            {
              printf("ERROR: Data does not match. DLC=%d\n", msgdlc);
              for (i = 0; i < msgdlc; i++)
                {
                  printf("  %d: TX %02x RX %02x\n",
                         i, txmsg.cm_data[i], rxmsg.cm_data[i]);
                  errval = 5;
                  goto errout_with_dev;
                }
            }
        }
#endif

      /* Report success */

      printf("  ID: %4u DLC: %d -- OK\n", msgid, msgdlc);
#endif

      /* Set up for the next pass */

#ifndef CONFIG_EXAMPLES_CAN_READONLY
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

errout:
  printf("Terminating!\n");
  fflush(stdout);
  return errval;
}
