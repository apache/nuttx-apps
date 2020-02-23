/****************************************************************************
 * system/zmodem/zm_state.c
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * References:
 *   "The ZMODEM Inter Application File Transfer Protocol", Chuck Forsberg,
 *    Omen Technology Inc., October 14, 1988
 *
 *    This is an original work, but I want to make sure that credit is given
 *    where due:  Parts of the state machine design were inspired by the
 *    Zmodem library of Edward A. Falk, dated January, 1995.  License
 *    unspecified.
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
#include <sys/stat.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>
#include <fcntl.h>
#include <sched.h>
#include <assert.h>
#include <errno.h>
#include <crc16.h>
#include <crc32.h>

#include <nuttx/ascii.h>

#include "zm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* State-specific data receipt handlers */

static int zm_idle(FAR struct zm_state_s *pzm, uint8_t ch);
static int zm_header(FAR struct zm_state_s *pzm, uint8_t ch);
static int zm_data(FAR struct zm_state_s *pzm, uint8_t ch);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zm_event
 *
 * Description:
 *   This is the heart of the Zmodem state machine.  Logic initiated by
 *   zm_parse() will detect events and, eventually call this function.
 *   This function will make the state transition, performing any action
 *   associated with the event.
 *
 ****************************************************************************/

static int zm_event(FAR struct zm_state_s *pzm, int event)
{
  FAR const struct zm_transition_s *ptr;

  zmdbg("ZM[R|S]_state: %d event: %d\n", pzm->state, event);

  /* Look up the entry associated with the event in the current state
   * transition table.  NOTE that each state table must be terminated with a
   * ZME_ERROR entry that provides indicates that the event was not
   * expected.  Thus, the following search will always be successful.
   */

  ptr = pzm->evtable[pzm->state];
  while (ptr->type != ZME_ERROR && ptr->type != event)
    {
      /* Skip to the next entry */

      ptr++;
    }

  zmdbg("Transition ZM[R|S]_state %d->%d discard: %d action: %p\n",
        pzm->state, ptr->next, ptr->bdiscard, ptr->action);

  /* Perform the state transition */

  pzm->state = ptr->next;

  /* Discard buffered data if so requested */

  if (ptr->bdiscard)
    {
      pzm->rcvlen = 0;
      pzm->rcvndx = 0;
    }

  /* And, finally, perform the associated action */

  return ptr->action(pzm);
}

/****************************************************************************
 * Name: zm_nakhdr
 *
 * Description:
 *   Send a NAK in response to a malformed or unsupported header.
 *
 ****************************************************************************/

static int zm_nakhdr(FAR struct zm_state_s *pzm)
{
  zmdbg("PSTATE %d:%d->%d:%d: NAKing\n",
        pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);

  /* Revert to the IDLE state */

  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* And NAK the header */

  return zm_sendhexhdr(pzm, ZNAK, g_zeroes);
}

/****************************************************************************
 * Name: zm_hdrevent
 *
 * Description:
 *   Process an event associated with a header.
 *
 ****************************************************************************/

static int zm_hdrevent(FAR struct zm_state_s *pzm)
{
  zmdbg("Received type: %d data: %02x %02x %02x %02x\n",
        pzm->hdrdata[0],
        pzm->hdrdata[1], pzm->hdrdata[2], pzm->hdrdata[3], pzm->hdrdata[4]);
  zmdbg("PSTATE %d:%d->%d:%d\n",
        pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);

  /* Revert to the IDLE state */

  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* Verify the checksum.  16- or 32-bit? */

  if (pzm->hdrfmt == ZBIN32)
    {
      uint32_t crc;

      /* Checksum is over 9 bytes:  The header type, 4 data bytes, plus 4 CRC bytes */

      crc = crc32part(pzm->hdrdata, 9, 0xffffffff);
      if (crc != 0xdebb20e3)
        {
          zmdbg("ERROR: ZBIN32 CRC32 failure: %08x vs debb20e3\n", crc);
          return zm_nakhdr(pzm);
        }
    }
  else
    {
      uint16_t crc;

      /* Checksum is over 7 bytes:  The header type, 4 data bytes, plus 2 CRC bytes */

      crc = crc16part(pzm->hdrdata, 7, 0);
      if (crc != 0)
        {
          zmdbg("ERROR: ZBIN/ZHEX CRC16 failure: %04x vs 0000\n", crc);
          return zm_nakhdr(pzm);
        }
    }

  return zm_event(pzm, pzm->hdrdata[0]);
}

/****************************************************************************
 * Name: zm_dataevent
 *
 * Description:
 *   Process an event associated with a header.
 *
 ****************************************************************************/

static int zm_dataevent(FAR struct zm_state_s *pzm)
{
  zmdbg("Received type: %d length: %d\n", pzm->pkttype, pzm->pktlen);
  zmdbg("PSTATE %d:%d->%d:%d\n",
        pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);

  /* Revert to the IDLE state */

  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* Verify the checksum. 16- or 32-bit? */

  if (pzm->hdrfmt == ZBIN32)
    {
      uint32_t crc;

      crc = crc32part(pzm->pktbuf, pzm->pktlen, 0xffffffff);
      if (crc != 0xdebb20e3)
        {
          zmdbg("ERROR: ZBIN32 CRC32 failure: %08x vs debb20e3\n", crc);
          pzm->flags &= ~ZM_FLAG_CRKOK;
        }
      else
        {
          pzm->flags |= ZM_FLAG_CRKOK;
        }

      /* Adjust the back length to exclude the packet type length of the 4-
       * byte checksum.
       */

      pzm->pktlen -= 5;
    }
  else
    {
      uint16_t crc;

      crc = crc16part(pzm->pktbuf, pzm->pktlen, 0);
      if (crc != 0)
        {
          zmdbg("ERROR: ZBIN/ZHEX CRC16 failure: %04x vs 0000\n", crc);
          pzm->flags &= ~ZM_FLAG_CRKOK;
        }
      else
        {
          pzm->flags |= ZM_FLAG_CRKOK;
        }

     /* Adjust the back length to exclude the packet type length of the 2-
      * byte checksum.
      */

      pzm->pktlen -= 3;
    }

  /* Then handle the data received event */

  return zm_event(pzm, ZME_DATARCVD);
}

/****************************************************************************
 * Name: zm_idle
 *
 * Description:
 *   Data has been received in state PSTATE_IDLE.  In this state we are
 *   looking for the beginning of a header indicated by the receipt of
 *   ZDLE.  We skip over ZPAD characters and flush the received buffer in
 *   the case where anything else is received.
 *
 ****************************************************************************/

static int zm_idle(FAR struct zm_state_s *pzm, uint8_t ch)
{
  switch (ch)
    {
    /* One or more ZPAD characters must precede the ZDLE */

    case ZPAD:
      {
        /* The ZDLE character is expected next */

        zmdbg("PSTATE %d:%d->%d:%d\n",
              pzm->pstate, pzm->psubstate, pzm->pstate, PIDLE_ZDLE);

        pzm->psubstate = PIDLE_ZDLE;
      }
      break;

    /* ZDLE indicates the beginning of a header. */

    case ZDLE:

      /* Was the ZDLE preceded by ZPAD[s]?  If not, revert to the PIDLE_ZPAD
       * substate.
       */

      if (pzm->psubstate == PIDLE_ZDLE)
        {
          zmdbg("PSTATE %d:%d->%d:%d\n",
                pzm->pstate, pzm->psubstate, PSTATE_HEADER, PHEADER_FORMAT);

          pzm->flags    &= ~ZM_FLAG_OO;
          pzm->pstate    = PSTATE_HEADER;
          pzm->psubstate = PHEADER_FORMAT;
          break;
        }
      else
        {
          zmdbg("PSTATE %d:%d->%d:%d\n",
                pzm->pstate, pzm->psubstate, pzm->pstate, PIDLE_ZPAD);

          pzm->psubstate = PIDLE_ZPAD;
        }

    /* O might be the first character of "OO".  "OO" might be part of the file
     * receiver protocol.  After receiving on e file in a group of files, the
     * receiver expected either "OO" indicating that all files have been sent,
     * or a ZRQINIT header indicating the start of the next file.
     */

    case 'O':
      /* Is "OO" a possibility in this context?  Fall through to the default
       * case if not.
       */

      if ((pzm->flags & ZM_FLAG_OO) != 0)
        {
          /* Yes... did we receive an 'O' before this one? */

          if (pzm->psubstate == PIDLE_OO)
            {
              /* This is the second 'O' of "OO". the receiver operation is
               * finished.
               */

              zmdbg("PSTATE %d:%d->%d:%d\n",
                    pzm->pstate, pzm->psubstate, pzm->pstate, PIDLE_ZPAD);

              pzm->flags    &= ~ZM_FLAG_OO;
              pzm->psubstate = PIDLE_ZPAD;
              return zm_event(pzm, ZME_OO);
            }
          else
            {
              /* No... then this is the first 'O' that we have seen */

              zmdbg("PSTATE %d:%d->%d:%d\n",
                    pzm->pstate, pzm->psubstate, pzm->pstate, PIDLE_OO);

              pzm->psubstate = PIDLE_OO;
            }
          break;
        }

    /* Unexpected character.  Wait for the next ZPAD to get us back in sync. */

    default:
      if (pzm->psubstate != PIDLE_ZPAD)
        {
          zmdbg("PSTATE %d:%d->%d:%d\n",
                pzm->pstate, pzm->psubstate, pzm->pstate, PIDLE_ZPAD);

          pzm->psubstate = PIDLE_ZPAD;
        }
      break;
    }

  return OK;
}

/****************************************************************************
 * Name: zm_header
 *
 * Description:
 *   Data has been received in state PSTATE_HEADER (i.e., ZDLE was received
 *   in PSTAT_IDLE).
 *
 *   The following headers are supported:
 *
 *   16-bit Binary:
 *     ZPAD ZDLE ZBIN type f3/p0 f2/p1 f1/p2 f0/p3 crc-1 crc-2
 *     Payload length: 7 (type, 4 bytes data, 2 byte CRC)
 *   32-bit Binary:
 *     ZPAD ZDLE ZBIN32 type f3/p0 f2/p1 f1/p2 f0/p3 crc-1 crc-2 crc-3 crc-4
 *     Payload length: 9 (type, 4 bytes data, 4 byte CRC)
 *   Hex:
 *     ZPAD ZPAD ZDLE ZHEX type f3/p0 f2/p1 f1/p2 f0/p3 crc-1 crc-2 CR LF [XON]
 *     Payload length: 16 (14 hex digits, cr, lf, ignoring optional XON)
 *
 ****************************************************************************/

static int zm_header(FAR struct zm_state_s *pzm, uint8_t ch)
{
  /* ZDLE encountered in this state means that the following character is
   * escaped.
   */

  if (ch == ZDLE && (pzm->flags & ZM_FLAG_ESC) == 0)
    {
      /* Indicate that we are beginning the escape sequence and return */

      pzm->flags |= ZM_FLAG_ESC;
      return OK;
    }

  /* Handle the escaped character in an escape sequence */

  if ((pzm->flags & ZM_FLAG_ESC) != 0)
    {
      switch (ch)
        {
        /* Two special cases */

        case ZRUB0:
          ch = ASCII_DEL;
          break;

        case ZRUB1:
          ch = 0xff;
          break;

        /* The typical case:  Toggle bit 6 */

        default:
          ch ^= 0x40;
          break;
        }

      /* We are no longer in an escape sequence */

      pzm->flags &= ~ZM_FLAG_ESC;
    }

  /* Now handle the next character, escaped or not, according to the current
   * PSTATE_HEADER substate.
   */

  switch (pzm->psubstate)
    {
    /* Waiting for the header format {ZBIN, ZBIN32, ZHEX} */

    case PHEADER_FORMAT:
      {
        switch (ch)
          {
          /* Supported header formats */

          case ZHEX:
          case ZBIN:
          case ZBIN32:
            {
              /* Save the header format character. Next we expect the header
               * data payload beginning with the header type.
               */

              pzm->hdrfmt    = ch;
              pzm->psubstate = PHEADER_PAYLOAD;
              pzm->hdrndx    = 0;
            }
            break;

          default:
            {
              /* Unrecognized header format. */

              return zm_nakhdr(pzm);
            }
        }
      }
      break;

    /* Waiting for header payload */

    case PHEADER_PAYLOAD:
      {
        int ndx = pzm->hdrndx;

        switch (pzm->hdrfmt)
          {
          /* Supported header formats */

          case ZHEX:
            {
              if (!isxdigit(ch))
                {
                  return zm_nakhdr(pzm);
                }

              /* Save the MS nibble; setup to receive the LS nibble.  Index
               * is not incremented.
               */

              pzm->hdrdata[ndx] = zm_decnibble(ch) << 4;
              pzm->psubstate    = PHEADER_LSPAYLOAD;
            }
            break;

          case ZBIN:
          case ZBIN32:
            {
              /* Save the payload byte and increment the index. */

              pzm->hdrdata[ndx] = ch;
              ndx++;

              /* Check if the full header payload has bee buffered.
               *
               * The ZBIN format uses 16-bit CRC so the binary length of the
               * full payload is 1+4+2 = 7 bytes; the ZBIN32 uses a 32-bit CRC
               * so the binary length of the payload is 1+4+4 = 9 bytes;
               */

              if (ndx >= 9 || (pzm->hdrfmt == ZBIN && ndx >= 7))
                {
                  return zm_hdrevent(pzm);
                }
              else
                {
                  /* Setup to receive the next byte */

                  pzm->psubstate = PHEADER_PAYLOAD;
                  pzm->hdrndx    = ndx;
                }
            }
            break;

          default: /* Should not happen */
            break;
          }
      }
      break;

    /* Waiting for LS nibble header type (ZHEX only) */

    case PHEADER_LSPAYLOAD:
      {
        int ndx = pzm->hdrndx;

        if (pzm->hdrfmt == ZHEX && isxdigit(ch))
          {
            /* Save the LS nibble and increment the index. */

            pzm->hdrdata[ndx] |= zm_decnibble(ch);
            ndx++;

            /* The ZHEX format uses 16-bit CRC.  So the binary length
             * of the sequence is 1+4+2 = 7 bytes.
             */

            if (ndx >= 7)
              {
                return zm_hdrevent(pzm);
              }
            else
              {
                /* Setup to receive the next MS nibble */

                pzm->psubstate = PHEADER_PAYLOAD;
                pzm->hdrndx    = ndx;
              }
          }
        else
          {
            return zm_nakhdr(pzm);
          }
      }
      break;
    }

  return OK;
}

/****************************************************************************
 * Name: zm_data
 *
 * Description:
 *   Data has been received in state PSTATE_DATA.  PSTATE_DATA is set by
 *   Zmodem transfer logic when it expects to received data from the
 *   remote peer.
 *
 *   FORMAT:
 *     xx xx xx xx ... xx ZDLE <type> crc-1 crc-2 [crc-3 crc-4]
 *
 *   Where xx is binary data (that may be escaped).  The 16- or 32-bit CRC
 *   is selected based on a preceding header.  ZHEX data packets are not
 *   supported.
 *
 *   When setting pstate to PSTATE_DATA, it is also expected that the
 *   following initialization is performed:
 *
 *   - The crc value is initialized appropriately
 *   - ncrc is set to zero.
 *   - pktlen is set to zero
 *
 ****************************************************************************/

static int zm_data(FAR struct zm_state_s *pzm, uint8_t ch)
{
  int ret;

  /* ZDLE encountered in this state means that the following character is
   * escaped.  Escaped characters may appear anywhere within the data packet.
   */

  if (ch == ZDLE && (pzm->flags & ZM_FLAG_ESC) == 0)
    {
      /* Indicate that we are beginning the escape sequence and return */

      pzm->flags |= ZM_FLAG_ESC;
      return OK;
    }

  /* Make sure that there is space for another byte in the packet buffer */

  if (pzm->pktlen >= ZM_PKTBUFSIZE)
    {
      zmdbg("ERROR:  The packet buffer is full\n");
      zmdbg("        ch=%c[%02x] pktlen=%d pkttype=%02x ncrc=%d\n",
            isprint(ch) ? ch : '.', ch, pzm->pktlen, pzm->pkttype, pzm->ncrc);
      zmdbg("        rcvlen=%d rcvndx=%d\n",
            pzm->rcvlen, pzm->rcvndx);
      return -ENOSPC;
    }

  /* Handle the escaped character in an escape sequence */

  if ((pzm->flags & ZM_FLAG_ESC) != 0)
    {
      switch (ch)
        {
        /* The data packet type may immediately follow the ZDLE in PDATA_READ
         * substate.
         */

        case ZCRCW: /* Data packet (Non-streaming, ZACK response expected) */
        case ZCRCE: /* Data packet (End-of-file, no response unless an error occurs) */
        case ZCRCG: /* Data packet (Full streaming, no response) */
        case ZCRCQ: /* Data packet (ZACK response expected) */
          {
            /* Save the packet type, change substates, and set of count that
             * indicates the number of bytes still to be added to the packet
             * buffer:
             *
             *   ZBIN:   1+2 = 3
             *   ZBIN32: 1+4 = 5
             */

            pzm->pkttype   = ch;
            pzm->psubstate = PDATA_CRC;
            pzm->ncrc      = (pzm->hdrfmt == ZBIN32) ? 5 : 3;
          }
          break;

        /* Some special cases */

        case ZRUB0:
          ch = ASCII_DEL;
          break;

        case ZRUB1:
          ch = 0xff;
          break;

        /* The typical case:  Toggle bit 6 */

        default:
          ch ^= 0x40;
          break;
        }

      /* We are no longer in an escape sequence */

      pzm->flags &= ~ZM_FLAG_ESC;
    }

  /* Transfer received data from the I/O buffer to the packet buffer.
   * Accumulate the CRC for the received data.  This includes the data
   * payload plus the packet type code plus the CRC itself.
   */

   pzm->pktbuf[pzm->pktlen++] = ch;
   if (pzm->ncrc == 1)
     {
       /* We are at the end of the packet.  Check the CRC and post the event */

       ret = zm_dataevent(pzm);

       /* The packet data has been processed.  Discard the old buffered
        * packet data.
        */

       pzm->pktlen = 0;
       pzm->ncrc   = 0;
       return ret;
     }
   else if (pzm->ncrc > 1)
     {
       /* We are still parsing the CRC.  Decrement the count of CRC bytes
        * remaining.
        */

       pzm->ncrc--;
     }

  return OK;
}

/****************************************************************************
 * Name: zm_parse
 *
 * Description:
 *   New data from the remote peer is available in pzm->rcvbuf.  The number
 *   number of bytes of new data is given by rcvlen.
 *
 *   This function will parse the data in the buffer and, based on the
 *   current state and the contents of the buffer, will drive the Zmodem
 *   state machine.
 *
 ****************************************************************************/

static int zm_parse(FAR struct zm_state_s *pzm, size_t rcvlen)
{
  uint8_t ch;
  int ret;

  DEBUGASSERT(pzm && rcvlen <= CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE);
  zm_dumpbuffer("Received", pzm->rcvbuf, rcvlen);

  /* We keep a copy of the length and buffer index in the state structure.
   * This is only so that deeply nested logic can use these values.
   */

  pzm->rcvlen = rcvlen;
  pzm->rcvndx = 0;

  /* Process each byte until we reach the end of the buffer (or until the
   * data is discarded.
   */

  while (pzm->rcvndx < pzm->rcvlen)
    {
      /* Get the next byte from the buffer */

      ch = pzm->rcvbuf[pzm->rcvndx];
      pzm->rcvndx++;

      /* Handle sequences of CAN characters.  When we encounter 5 in a row,
       * then we consider this a request to cancel the file transfer.
       */

      if (ch == ASCII_CAN)
        {
          if (++pzm->ncan >= 5)
            {
              zmdbg("Remote end has canceled\n");
              pzm->rcvlen = 0;
              pzm->rcvndx = 0;
              return zm_event(pzm, ZME_CANCEL);
            }
        }
      else
        {
          /* Not CAN... reset the sequence count */

          pzm->ncan = 0;
        }

      /* Skip over XON and XOFF */

      if (ch != ASCII_XON && ch != ASCII_XOFF)
        {
          /* And process what follows based on the current parsing state */

          switch (pzm->pstate)
            {
            case PSTATE_IDLE:
              ret = zm_idle(pzm, ch);
              break;

            case PSTATE_HEADER:
              ret = zm_header(pzm, ch);
              break;

            case PSTATE_DATA:
              ret = zm_data(pzm, ch);
              break;

            /* This should not happen */

            default:
              zmdbg("ERROR: Invalid state: %d\n", pzm->pstate);
              ret = -EINVAL;
              break;
            }

          /* Handle end-of-transfer and irrecoverable errors by breaking out
           * of the loop and return a non-zero return value to indicate that
           * transfer is complete.
           */

          if (ret != OK)
            {
              zmdbg("%s: %d\n", ret < 0 ? "Aborting" : "Done", ret);
              return ret;
            }
        }
    }

  /* If we made it through the entire buffer with no errors detected, then
   * return OK == 0 meaning that everything is okay, but we are not finished
   * with the transfer.
   */

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zm_datapump
 *
 * Description:
 *   Drive the Zmodem state machine by reading data from the remote peer and
 *   providing that data to the parser.  This loop runs until a fatal error
 *   is detected or until the state machine reports that the transfer has
 *   completed successfully.
 *
 ****************************************************************************/

int zm_datapump(FAR struct zm_state_s *pzm)
{
  int ret = OK;
  ssize_t nread;

  /* Loop until either a read error occurs or until a non-zero value is
   * returned by the parser.
   */

  do
    {
      /* Start/restart the timer.  Whenever we read data from the peer we
       * must anticipate a timeout because we can never be sure that the peer
       * is still responding.
       */

      sched_lock();
      zm_timerstart(pzm, pzm->timeout);

      /* Read a block of data.  read() will return: (1) nread > 0 and nread
       * <= CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE on success, (2) nread == 0 on end
       * of file, or (3) nread < 0 on a read error or interruption by a
       * signal.
       */

      nread = read(pzm->remfd, pzm->rcvbuf, CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE);

      /* Stop the timer */

      zm_timerstop(pzm);
      sched_unlock();

      /* EOF from the remote peer can only mean that we lost the connection
       * somehow.
       */

      if (nread == 0)
        {
          zmdbg("ERROR: Unexpected end-of-file\n");
          return -ENOTCONN;
        }

      /* Did some error occur? */

      else if (nread < 0)
        {
          int errorcode = errno;

          /* EINTR is not an error... it simply means that this read was
           * interrupted by an signal before it obtained in data.  However,
           * the signal may be SIGALRM indicating an timeout condition.
           * We will know in this case because the signal handler will set
           * ZM_FLAG_TIMEOUT.
           */

          if (errorcode == EINTR)
            {
              /* Check for a timeout */

              if ((pzm->flags & ZM_FLAG_TIMEOUT) != 0)
                {
                  /* Yes... a timeout occurred */

                  ret = zm_timeout(pzm);
                }

              /* No.. then just ignore the EINTR. */
            }
          else
            {
              /* But anything else is bad and we will return the failure
               * in those cases.
               */

              zmdbg("ERROR: read failed: %d\n", errorcode);
              return -errorcode;
            }
        }

      /* Then provide that data to the state machine via zm_parse().
       * zm_parse() will return a non-zero value if we need to terminate
       * the loop (with a negative value indicating a failure).
       */

      else /* nread > 0 */
        {
          ret = zm_parse(pzm, nread);
          if (ret < 0)
            {
              zmdbg("ERROR: zm_parse failed: %d\n", ret);
            }
        }
    }
  while (ret == OK);

  return ret;
}


/****************************************************************************
 * Name: zm_readstate
 *
 * Description:
 *   Enter PSTATE_DATA.
 *
 ****************************************************************************/

void zm_readstate(FAR struct zm_state_s *pzm)
{
  zmdbg("PSTATE %d:%d->%d:%d\n",
        pzm->pstate, pzm->psubstate, PSTATE_DATA, PDATA_READ);

  pzm->pstate    = PSTATE_DATA;
  pzm->psubstate = PDATA_READ;
  pzm->pktlen    = 0;
  pzm->ncrc      = 0;
}

/****************************************************************************
 * Name: zm_timeout
 *
 * Description:
 *   Called by the watchdog logic if/when a timeout is detected.
 *
 ****************************************************************************/

int zm_timeout(FAR struct zm_state_s *pzm)
{
  return zm_event(pzm, ZME_TIMEOUT);
}
