/****************************************************************************
 * system/zmodem/zm_receive.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "system/zmodem.h"

#include "zm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Zmodem receive states.
 *
 * A simple transaction, one file, no errors, no CHALLENGE, overlapped I/O:
 * These happen when zm_read() is called:
 *
 *   Sender               Receiver    State
 *   --------------     ------------  --------
 *   "rz\r"       ---->
 *   ZRQINIT      ---->
 *                <---- ZRINIT        ZMR_START
 *   ZSINIT       ---->
 *                <---- ZACK          ZMR_INITWAIT
 *   ZFILE        ---->
 *                <---- ZRPOS         ZMR_FILEINFO
 *   ZDATA        ---->
 *                <---- ZCRC          ZMR_CRCWAIT
 *   ZCRC         ---->               ZMR_READREADY
 *   Data packets ---->               ZMR_READING
 *   Last packet  ---->
 *   ZEOF         ---->
 *                <---- ZRINIT
 *   ZFIN         ---->
 *                <---- ZFIN          ZMR_FINISH
 *   OO           ---->               ZMR_DONE
 */


enum zmrs_e
{
  ZMR_START = 0,   /* Sent ZRINIT, waiting for ZFILE or ZSINIT */
  ZMR_INITWAIT,    /* Received ZSINIT, sent ZACK, waiting for ZFILE */
  ZMR_FILEINFO,    /* Received ZFILE, sent ZRPOS, waiting for filename in ZDATA */
  ZMR_CRCWAIT,     /* Received ZDATA filename, send ZCRC, wait for ZCRC response */
  ZMR_READREADY,   /* Received ZDATA filename and ZCRC, ready for data packets */
  ZMR_READING,     /* Reading data */
  ZMR_FINISH,      /* Received ZFIN, sent ZFIN, waiting for "OO" or ZRQINIT */
  ZMR_COMMAND,     /* Waiting for command data */
  ZMR_MESSAGE,     /* Waiting for message from receiver */
  ZMR_DONE         /* Finished with transfer */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
/* Transition actions */

static int zmr_zrinit(FAR struct zm_state_s *pzm);
static int zmr_zsinit(FAR struct zm_state_s *pzm);
static int zmr_zsrintdata(FAR struct zm_state_s *pzm);
static int zmr_startto(FAR struct zm_state_s *pzm);
static int zmr_freecnt(FAR struct zm_state_s *pzm);
static int zmr_zcrc(FAR struct zm_state_s *pzm);
static int zmr_nakcrc(FAR struct zm_state_s *pzm);
static int zmr_zfile(FAR struct zm_state_s *pzm);
static int zmr_zdata(FAR struct zm_state_s *pzm);
static int zmr_badrpos(FAR struct zm_state_s *pzm);
static int zmr_filename(FAR struct zm_state_s *pzm);
static int zmr_filedata(FAR struct zm_state_s *pzm);
static int zmr_rcvto(FAR struct zm_state_s *pzm);
static int zmr_fileto(FAR struct zm_state_s *pzm);
static int zmr_cmddata(FAR struct zm_state_s *pzm);
static int zmr_zeof(FAR struct zm_state_s *pzm);
static int zmr_zfin(FAR struct zm_state_s *pzm);
static int zmr_finto(FAR struct zm_state_s *pzm);
static int zmr_oo(FAR struct zm_state_s *pzm);
static int zmr_message(FAR struct zm_state_s *pzm);
static int zmr_zstderr(FAR struct zm_state_s *pzm);
static int zmr_cmdto(FAR struct zm_state_s *pzm);
static int zmr_doneto(FAR struct zm_state_s *pzm);
static int zmr_error(FAR struct zm_state_s *pzm);

/* Internal helpers */

static int zmr_parsefilename(FAR struct zmr_state_s *pzmr,
                             FAR const uint8_t *namptr);
static int zmr_openfile(FAR struct zmr_state_s *pzmr, uint32_t crc);
static int zmr_fileerror(FAR struct zmr_state_s *pzmr, uint8_t type,
                         uint32_t data);
static void zmr_filecleanup(FAR struct zmr_state_s *pzmr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Events handled in state ZMR_START - Sent ZRINIT, waiting for ZFILE or
 * ZSINIT
 */

static const struct zm_transition_s g_zmr_start[] =
{
  {ZME_SINIT,    false, ZMR_INITWAIT,    zmr_zsinit},
  {ZME_FILE,     false, ZMR_FILEINFO,    zmr_zfile},
  {ZME_RQINIT,   false, ZMR_START,       zmr_zrinit},
  {ZME_FIN,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_NAK,      true,  ZMR_START,       zmr_zrinit},
  {ZME_FREECNT,  false, ZMR_START,       zmr_freecnt},
  {ZME_COMMAND,  false, ZMR_COMMAND,     zmr_cmddata},
  {ZME_STDERR,   false, ZMR_MESSAGE,     zmr_message},
  {ZME_TIMEOUT,  false, ZMR_START,       zmr_startto},
  {ZME_ERROR,    false, ZMR_START,       zmr_error}
};

/* Events handled in state ZMR_INITWAIT - Received ZSINIT, sent ZACK,
 * waiting for ZFILE.
 */

static const struct zm_transition_s g_zmr_initwait[] =
{
  {ZME_DATARCVD, false, ZMR_START,       zmr_zsrintdata},
  {ZME_TIMEOUT,  false, ZMR_INITWAIT,    zmr_rcvto},
  {ZME_ERROR,    false, ZMR_INITWAIT,    zmr_error}
};

/* Events handled in state ZMR_FILEINFO - eceived ZFILE, sent ZRPOS, waiting
 * for filename in ZDATA
 */

static const struct zm_transition_s g_zmr_fileinfo[] =
{
  {ZME_DATARCVD, false, ZMR_READREADY,   zmr_filename},
  {ZME_TIMEOUT,  false, ZMR_FILEINFO,    zmr_rcvto},
  {ZME_ERROR,    false, ZMR_FILEINFO,    zmr_error}
};

/* Events handled in state ZMR_CRCWAIT - Received ZDATA filename, send ZCRC,
 * wait for ZCRC response
 */

static const struct zm_transition_s g_zmr_crcwait[] =
{
  {ZME_CRC,      false, ZMR_READREADY,   zmr_zcrc},
  {ZME_NAK,      false, ZMR_CRCWAIT,     zmr_nakcrc},
  {ZME_RQINIT,   true,  ZMR_START,       zmr_zrinit},
  {ZME_FIN,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_TIMEOUT,  false, ZMR_CRCWAIT,     zmr_fileto},
  {ZME_ERROR,    false, ZMR_CRCWAIT,     zmr_error}
};

/* Events handled in state ZMR_READREADY - Received ZDATA filename and ZCRC,
 * ready for data packets
 */

static const struct zm_transition_s g_zmr_readready[] =
{
  {ZME_DATA,     false, ZMR_READING,     zmr_zdata},
  {ZME_NAK,      false, ZMR_READREADY,   zmr_badrpos},
  {ZME_EOF,      false, ZMR_START,       zmr_zeof},
  {ZME_RQINIT,   true,  ZMR_START,       zmr_zrinit},
  {ZME_FILE,     false, ZMR_READREADY,   zmr_badrpos},
  {ZME_FIN,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_TIMEOUT,  false, ZMR_READREADY,   zmr_fileto},
  {ZME_ERROR,    false, ZMR_READREADY,   zmr_error}
};

/* Events handled in the state ZMR_READING - Reading data */

static const struct zm_transition_s g_zmr_reading[] =
{
  {ZME_RQINIT,   true,  ZMR_START,       zmr_zrinit},
  {ZME_FILE,     false, ZMR_FILEINFO,    zmr_zfile},
  {ZME_NAK,      true,  ZMR_READREADY,   zmr_badrpos},
  {ZME_FIN,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_DATA,     false, ZMR_READING,     zmr_zdata},
  {ZME_EOF,      true,  ZMR_START,       zmr_zeof},
  {ZME_DATARCVD, false, ZMR_READING,     zmr_filedata},
  {ZME_TIMEOUT,  false, ZMR_READING,     zmr_fileto},
  {ZME_ERROR,    false, ZMR_READING,     zmr_error}
};

/* Events handled in the state ZMR_FINISH - Sent ZFIN, waiting for "OO" or ZRQINIT */

static const struct zm_transition_s g_zmr_finish[] =
{
  {ZME_RQINIT,   true,  ZMR_START,       zmr_zrinit},
  {ZME_FILE,     true,  ZMR_FILEINFO,    zmr_zfile},
  {ZME_NAK,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_FIN,      true,  ZMR_FINISH,      zmr_zfin},
  {ZME_TIMEOUT,  false, ZMR_READING,     zmr_finto},
  {ZME_OO,       false, ZMR_READING,     zmr_oo},
  {ZME_ERROR,    false, ZMR_FINISH,      zmr_error}
};

/* Events handled in the state ZMR_COMMAND -  Waiting for command data */

static const struct zm_transition_s g_zmr_command[] =
{
  {ZME_DATARCVD, false, ZMR_COMMAND,     zmr_cmddata},
  {ZME_TIMEOUT,  false, ZMR_COMMAND,     zmr_cmdto},
  {ZME_ERROR,    false, ZMR_COMMAND,     zmr_error}
};

/* Events handled in ZMR_MESSAGE - Waiting for ZSTDERR data */

static struct zm_transition_s g_zmr_message[] =
{
  {ZME_DATARCVD, false, ZMR_MESSAGE,     zmr_zstderr},
  {ZME_TIMEOUT,  false, ZMR_MESSAGE,     zmr_cmdto},
  {ZME_ERROR,    false, ZMR_MESSAGE,     zmr_error}
};

/* Events handled in ZMR_DONE -- Finished with transfer.  Waiting for "OO" or  */

static struct zm_transition_s g_zmr_done[] =
{
  {ZME_TIMEOUT,  false, ZMR_DONE,        zmr_doneto},
  {ZME_ERROR,    false, ZMR_DONE,        zmr_error}
};

/* State x Event table for Zmodem receive.  The order of states must
 * exactly match the order defined in enum zmrs_e
 */

static const struct zm_transition_s *g_zmr_evtable[] =
{
  g_zmr_start,     /* ZMR_START:     Sent ZRINIT, waiting for ZFILE or ZSINIT */
  g_zmr_initwait,  /* ZMR_INITWAIT:  Received ZSINIT, sent ZACK, waiting for ZFILE */
  g_zmr_fileinfo,  /* XMRS_FILENAME: Received ZFILE, sent ZRPOS, waiting for ZDATA */
  g_zmr_crcwait,   /* ZMR_CRCWAIT:   Received ZDATA, send ZCRC, wait for ZCRC */
  g_zmr_readready, /* ZMR_READREADY: Received ZCRC, ready for data packets */
  g_zmr_reading,   /* ZMR_READING:   Reading data */
  g_zmr_finish,    /* ZMR_FINISH:    Sent ZFIN, waiting for "OO" or ZRQINIT */
  g_zmr_command,   /* ZMR_COMMAND:   Waiting for command data */
  g_zmr_message,   /* ZMR_MESSAGE:   Receiver wants to print a message */
  g_zmr_done       /* ZMR_DONE:      Transfer is complete */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zmr_zrinit
 *
 * Description:
 *   Resend ZRINIT header in response to ZRQINIT or ZNAK header
 *
 *   Paragraph 9.5 "If the receiver cannot overlap serial and disk I/O, it
 *   uses the ZRINIT frame to specify a buffer length which the sender will
 *   not overflow.  The sending program sends a ZCRCW data subpacket and
 *   waits for a ZACK header before sending the next segment of the file."
 *
 ****************************************************************************/

static int zmr_zrinit(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;
  uint8_t buf[4];

  zmdbg("ZMR_STATE %d:->%d Send ZRINIT\n", pzm->state, ZMR_START);
  pzm->state   = ZMR_START;
  pzm->flags  &= ~ZM_FLAG_OO;   /* In case we get here from ZMR_FINISH */

  /* Send ZRINIT */

  pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
  buf[0]       = CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE & 0xff;
  buf[1]       = (CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE >> 8) & 0xff;
  buf[2]       = 0;
  buf[3]       = pzmr->rcaps;
  return zm_sendhexhdr(pzm, ZRINIT, buf);
}

/****************************************************************************
 * Name: zmr_zsinit
 *
 * Description:
 *   Received a ZSINIT header in response to ZRINIT
 *
 ****************************************************************************/

static int zmr_zsinit(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  zmdbg("ZMR_STATE %d: Received ZSINIT header\n", pzm->state);

  /* Get the sender's capabilities */

  pzmr->scaps = pzm->hdrdata[4];

  /* Does the sender expect control characters to be escaped? */

  pzm->flags &= ~ZM_FLAG_ESCCTRL;
  if ((pzmr->scaps & TESCCTL) != 0)
    {
      pzm->flags |= ZM_FLAG_ESCCTRL;
    }

  /* Setup to receive a data packet.  Enter PSTATE_DATA */

  zm_readstate(pzm);
  return OK;
}

/****************************************************************************
 * Name: zmr_startto
 *
 * Description:
 *   Timed out waiting for ZSINIT or ZFILE.
 *
 ****************************************************************************/

static int zmr_startto(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  pzmr->ntimeouts++;
  zmdbg("ZMR_STATE %d: %d timeouts waiting for ZSINIT or ZFILE\n",
        pzm->state, pzmr->ntimeouts);

  if (pzmr->ntimeouts < 4)
    {
      /* Send ZRINIT again */

      return zmr_zrinit(pzm);
    }

  /* This will stop the file transfer */

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zmr_zsrintdata
 *
 * Description:
 *   Received the rest of the ZSINIT packet.
 *
 ****************************************************************************/

static int zmr_zsrintdata(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;
  uint8_t by[4];

  zmdbg("PSTATE %d:%d->%d:%d. Received the rest of the ZSINIT packet\n",
        pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);

  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* NAK if the CRC was bad */

  if ((pzm->flags & ZM_FLAG_CRKOK) == 0)
    {
      return zm_sendhexhdr(pzm, ZNAK, g_zeroes);
    }

  /* Release any previously allocated attention strings */

  if (pzmr->attn != NULL)
    {
      free(pzmr->attn);
    }

  /* Get the new attention string */

  pzmr->attn = NULL;
  if (pzm->pktbuf[0] != '\0')
    {
      pzmr->attn = strdup((char *)pzm->pktbuf);
    }

  /* And send ZACK */

  zm_be32toby(CONFIG_SYSTEM_ZMODEM_SERIALNO, by);
  return zm_sendhexhdr(pzm, ZACK, by);
}

/****************************************************************************
 * Name: zmr_freecnt
 *
 * Description:
 *   TODO: This is supposed to return the amount of free space on the media.
 *   We just return a really big number now.
 *
 ****************************************************************************/

static int zmr_freecnt(FAR struct zm_state_s *pzm)
{
  uint8_t by[4];

  zmdbg("ZMR_STATE %d\n", pzm->state);

  zm_be32toby(0xffffffff, by);
  return zm_sendhexhdr(pzm, ZACK, by);
}

/****************************************************************************
 * Name: zmr_zcrc
 *
 * Description:
 *   Received file CRC.  Need to accept or reject it.
 *
 ****************************************************************************/

static int zmr_zcrc(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  /* Get the remote file CRC */

  pzmr->crc = zm_bytobe32(pzm->hdrdata);

  /* And create the local file */

  zmdbg("ZMR_STATE %d: CRC=%08x call zmr_openfile\n", pzmr->crc, pzm->state);
  return zmr_openfile(pzmr, zm_bytobe32(pzm->hdrdata + 1));
}

/****************************************************************************
 * Name: zmr_nakcrc
 *
 * Description:
 *   The sender responded to ZCRC with NAK.  Resend the ZCRC.
 *
 ****************************************************************************/

static int zmr_nakcrc(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d: Send ZCRC\n", pzm->state);
  return zm_sendhexhdr(pzm, ZCRC, g_zeroes);
}

/****************************************************************************
 * Name: zmr_zfile
 *
 * Description:
 *   Received ZFILE.  Cache the flags and set up to receive filename in ZDATA.
 *
 ****************************************************************************/

static int zmr_zfile(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  zmdbg("ZMR_STATE %d\n", pzm->state);

  pzm->nerrors = 0;
  pzm->flags  &= ~ZM_FLAG_OO;   /* In case we get here from ZMR_FINISH */

  /* Cache flags (skipping of the initial header type byte) */

  pzmr->f0 = pzmr->cmn.hdrdata[4];
  pzmr->f1 = pzmr->cmn.hdrdata[3];
#if 0 /* Not used */
  pzmr->f2 = pzmr->cmn.hdrdata[2];
  pzmr->f3 = pzmr->cmn.hdrdata[1];
#endif

  /* Setup to receive a data packet.  Enter PSTATE_DATA */

  zm_readstate(pzm);
  return OK;
}

/****************************************************************************
 * Name: zmr_zdata
 *
 * Description:
 *   Received ZDATA header
 *
 ****************************************************************************/

static int zmr_zdata(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  zmdbg("ZMR_STATE %d\n", pzm->state);

  /* Paragraph 8.2: "The receiver compares the file position in the ZDATA
   * header with the number of characters successfully received to the file.
   * If they do not agree, a ZRPOS error response is generated to force the
   * sender to the right position within the file."
   */

  if (zm_bytobe32(pzm->hdrdata + 1) != pzmr->offset)
    {
      /* Execute the Attn sequence and then send a ZRPOS header with the
       * correct position within the file.
       */

       zmdbg("Bad position, send ZRPOS(%ld)\n",
             (unsigned long)pzmr->offset);

       return zmr_fileerror(pzmr, ZRPOS, (uint32_t)pzmr->offset);
   }

  /* Setup to receive a data packet.  Enter PSTATE_DATA */

  zm_readstate(pzm);
  return OK;
}

/****************************************************************************
 * Name: zmr_badrpos
 *
 * Description:
 *   Last ZRPOS was bad, resend it
 *
 ****************************************************************************/

static int zmr_badrpos(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;
  uint8_t by[4];

  zmdbg("ZMR_STATE %d: Send ZRPOS(%ld)\n", pzm->state, (unsigned long)pzmr->offset);

  /* Re-send ZRPOS */

  zm_be32toby(pzmr->offset, by);
  return zm_sendhexhdr(pzm, ZRPOS, by);
}

/****************************************************************************
 * Name: zmr_filename
 *
 * Description:
 *   Received file information
 *
 ****************************************************************************/

static int zmr_filename(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;
  FAR const uint8_t *pktptr;
  unsigned long filesize;
  unsigned long timestamp;
  unsigned long bremaining;
  int mode;
  int serialno;
  int fremaining;
  int filetype;
  int ret;

  zmdbg("PSTATE %d:%d->%d:%d\n",
        pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);
  zmdbg("ZMR_STATE %d\n", pzm->state);

  /* Back to the IDLE state */

  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* Verify that the CRC was correct */

  if ((pzm->flags & ZM_FLAG_CRKOK) == 0)
    {
      zmdbg("ZMR_STATE %d->%d: ERROR: Bad CRC, send ZNAK\n",
            pzm->state, ZMR_START);

      /* Send NACK if the CRC is bad */

      pzm->state = ZMR_START;
      return zm_sendhexhdr(pzm, ZNAK, g_zeroes);
    }

  /* Discard any previous local file names */

  if (pzmr->filename != NULL)
    {
      free(pzmr->filename);
      pzmr->filename = NULL;
    }

  /* Parse the new file name from the beginning of the packet and verify
   * that we can use it.
   */

  pktptr = pzmr->cmn.pktbuf;
  ret    = zmr_parsefilename(pzmr, pktptr);

  if (ret < 0)
    {
      zmdbg("ZMR_STATE %d->%d: ERROR: Failed to parse filename. Send ZSKIP: %d\n",
            pzm->state, ZMR_START, ret);

      pzmr->cmn.state = ZMR_START;
      return zm_sendhexhdr(&pzmr->cmn, ZSKIP, g_zeroes);
    }

  /* Skip over the file name (and its NUL termination) */

  pktptr += (strlen((FAR const char *)pktptr) + 1);

  /* ZFILE: Following the file name are:
   *
   *   length timestamp mode serial-number files-remaining bytes-remaining file-type
   */

  filesize   = 0;
  timestamp  = 0;
  mode       = 0;
  serialno   = 0;
  fremaining = 0;
  bremaining = 0;
  filetype   = 0;

  sscanf((FAR char *)pktptr, "%lu %lo %o %o %d %lu %d",
         &filesize, &timestamp, &mode, &serialno, &fremaining, &bremaining,
         &filetype);

  /* Only a few of these values are retained in this implementation */

  pzmr->filesize  = (off_t)filesize;
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
  pzmr->timestamp = (time_t)timestamp;
#endif

  /* Check if we need to send the CRC */

  if ((pzmr->f1 & ZMMASK) == ZMCRC)
    {
      zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_CRCWAIT);

      pzm->state = ZMR_CRCWAIT;
      return zm_sendhexhdr(pzm, ZCRC, g_zeroes);
    }

  /* We are ready to receive file data packets */

  zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_READREADY);

  pzm->state = ZMR_READREADY;
  return zmr_openfile(pzmr, 0);
}

/****************************************************************************
 * Name: zmr_filedata
 *
 * Description:
 *   Received file data
 *
 ****************************************************************************/

static int zmr_filedata(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;
  uint8_t by[4];
  int ret;

  zmdbg("ZMR_STATE %d\n", pzm->state);

  /* Check if the CRC is okay */

  if ((pzm->flags & ZM_FLAG_CRKOK) == 0)
    {
      zmdbg("ERROR: Bad crc, send ZRPOS(%ld)\n",
            (unsigned long)pzmr->offset);

      /* No.. increment the count of errors */

      pzm->nerrors++;
      zmdbg("%d data errors\n", pzm->nerrors);

      /* If the count of errors exceeds the configurable limit, then cancel
       * the transfer
       */

      if (pzm->nerrors > CONFIG_SYSTEM_ZMODEM_MAXERRORS)
        {
          zmdbg("PSTATE %d:%d->%d:%d\n",
                pzm->pstate, pzm->psubstate, PSTATE_DATA, PDATA_READ);

          /* Send the cancel string */

          zm_remwrite(pzm->remfd, g_canistr, CANISTR_SIZE);

          /* Enter PSTATE_DATA */

          zm_readstate(pzm);
          return -EIO;
        }
      else
        {
          zmdbg("PSTATE %d:%d->%d:%d\n",
                pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);
          zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_READREADY);

          /* Revert to the ready to read state and send ZRPOS to get in sync */

          pzm->state     = ZMR_READREADY;
          pzm->pstate    = PSTATE_IDLE;
          pzm->psubstate = PIDLE_ZPAD;
          return zmr_fileerror(pzmr, ZRPOS, (uint32_t)pzmr->offset);
        }
    }

  /* Write the packet of data to the file */

  ret = zm_writefile(pzmr->outfd, pzm->pktbuf, pzm->pktlen,
                     pzmr->f0 == ZCNL);
  if (ret < 0)
    {
      int errorcode = errno;

      /* Could not write to the file. */

      zmdbg("ERROR: Write to file failed: %d\n", errorcode);
      zmdbg("PSTATE %d:%d->%d:%d\n",
             pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);
      zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_FINISH);

      /* Revert to the IDLE state, send ZFERR, and terminate the transfer
       * with an error.
       */

      pzm->state     = ZMR_FINISH;
      pzm->pstate    = PSTATE_IDLE;
      pzm->psubstate = PIDLE_ZPAD;
      zmr_fileerror(pzmr, ZFERR, (uint32_t)errorcode);
      return -errorcode;
    }

  zmdbg("offset: %ld nchars: %d pkttype: %02x\n",
        (unsigned long)pzmr->offset, pzm->pktlen, pzm->pkttype);

  pzmr->offset += pzm->pktlen;
  zmdbg("Bytes received: %ld\n", (unsigned long)pzmr->offset);

  /* If this was the last data subpacket, leave data mode */

  if (pzm->pkttype == ZCRCE || pzm->pkttype == ZCRCW)
    {
      zmdbg("PSTATE %d:%d->%d:%d: ZCRCE|ZCRCW\n",
            pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);
      zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_READREADY);

      /* Revert to the IDLE state */

      pzm->state     = ZMR_READREADY;
      pzm->pstate    = PSTATE_IDLE;
      pzm->psubstate = PIDLE_ZPAD;
    }
  else
    {
      /* Setup to receive a data packet.  Enter PSTATE_DATA */

      zm_readstate(pzm);
    }

  /* Special handle for different packet types:
   *
   *   ZCRCW:  Non-streaming, ZACK required
   *   ZCRCG:  Streaming, no response
   *   ZCRCQ:  Streaming, ZACK required
   *   ZCRCE:  End of file, no response
   */

  if (pzm->pkttype == ZCRCQ || pzm->pkttype == ZCRCW)
    {
      zmdbg("Send ZACK\n");

      zm_be32toby(pzmr->offset, by);
      return zm_sendhexhdr(pzm, ZACK, by);
    }

  return OK;
}

/****************************************************************************
 * Name: zmr_rcvto
 *
 * Description:
 *   Timed out waiting:
 *
 *   1) In state ZMR_INITWAIT - Received ZSINIT, waiting for data, or
 *   2) In state XMRS_FILENAME - Received ZFILE, waiting for file _info
 *
 ****************************************************************************/

static int zmr_rcvto(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  /* Increment the count of timeouts */

  pzmr->ntimeouts++;
  zmdbg("ZMR_STATE %d: Send timeouts: %d\n", pzm->state, pzmr->ntimeouts);

  /* If the number of timeouts exceeds a limit, then about the transfer */

  if (pzmr->ntimeouts > 4)
    {
      return -ETIMEDOUT;
    }

  /* Re-send the ZRINIT header */

  return zmr_zrinit(pzm);
}

/****************************************************************************
 * Name: zmr_fileto
 *
 * Description:
 *   Timed out waiting
 *   1) In state ZMR_CRCWAIT - Received filename, waiting for CRC,
 *   2) In state ZMR_READREADY - Received filename, ready to read, or
 *   3) IN state ZMR_READING - Reading data
 *
 ****************************************************************************/

static int zmr_fileto(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  /* Increment the count of timeouts */

  pzmr->ntimeouts++;
  zmdbg("ZMR_STATE %d: %d send timeouts\n", pzm->state, pzmr->ntimeouts);

  /* If the number of timeouts exceeds a limit, then restart the transfer */

  if (pzmr->ntimeouts > 2)
    {
      /* Re-send the ZRINIT header */

      pzmr->ntimeouts = 0;
      return zmr_zrinit(pzm);
    }

  return pzm->state == ZMR_CRCWAIT ? zmr_nakcrc(pzm) : zmr_badrpos(pzm);
}

/****************************************************************************
 * Name: zmr_zeof
 *
 * Description:
 *   Received ZEOF packet. File is now complete
 *
 ****************************************************************************/

static int zmr_zeof(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  zmdbg("ZMR_STATE %d: offset=%ld\n", pzm->state, (unsigned long)pzmr->offset);

  /* Verify the file length */

  if (zm_bytobe32(pzm->hdrdata + 1) != pzmr->offset)
    {
      zmdbg("ERROR: Bad length\n");
      zmdbg("ZMR_STATE %d->%d\n",  pzm->state, ZMR_READREADY);

      pzm->state = ZMR_READREADY;
      return OK;         /* it was probably spurious */
    }

  /* Close the output file.
   * TODO: if we can't close the file, send a ZFERR.
   */

  close(pzmr->outfd);
  pzmr->outfd = -1;

  /* TODO:  Set the file timestamp and access privileges */

  /* Re-send the ZRINIT header so that we are ready for the next file */

  return zmr_zrinit(pzm);
}

/****************************************************************************
 * Name: zmr_cmddata
 *
 * Description:
 *   Received command data (not implemented)
 *
 ****************************************************************************/

static int zmr_cmddata(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d\n", pzm->state);
  return OK;
}

/****************************************************************************
 * Name: zmr_zfin
 *
 * Description:
 *   Received ZFIN, respond with ZFIN.  Wait for ZRQINIT or "OO"
 *
 ****************************************************************************/

static int zmr_zfin(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  /* We are finished and will send ZFIN.  Transition to the ZMR_FINISH state
   * and wait for either ZRQINIT meaning that another file follows or "OO"
   * meaning that we are all done.
   */

  zmdbg("PSTATE %d:%d->%d:%d:  Send ZFIN\n",
         pzm->pstate, pzm->psubstate, PSTATE_IDLE, PIDLE_ZPAD);
  zmdbg("ZMR_STATE %d\n", pzm->state);

  pzm->pstate    = ZMR_FINISH;
  pzm->pstate    = PSTATE_IDLE;
  pzm->psubstate = PIDLE_ZPAD;

  /* Release any resource still held from the last file transfer */

  zmr_filecleanup(pzmr);

  /* Let the parser no that "OO" is a possibility */

  pzm->flags    |= ZM_FLAG_OO;

  /* Now send the ZFIN response */

  return zm_sendhexhdr(pzm, ZFIN, g_zeroes);
}

/****************************************************************************
 * Name: zmr_finto
 *
 * Description:
 *   Timedout in state ZMR_FINISH - Sent ZFIN, waiting for "OO"
 *
 ****************************************************************************/

static int zmr_finto(FAR struct zm_state_s *pzm)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s *)pzm;

  /* Increment the count of timeouts (not really necessary because we are
   * done).
   */

  pzmr->ntimeouts++;
  pzm->flags  &= ~ZM_FLAG_OO; /* No longer expect "OO" */

  zmdbg("ZMR_STATE %d: %d send timeouts\n", pzm->state, pzmr->ntimeouts);

  /* And terminate the reception with a timeout error */

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zmr_oo
 *
 * Description:
 *   Received "OO" in the ZMR_FINISH state.  We are finished!
 *
 ****************************************************************************/

static int zmr_oo(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d: Done\n", pzm->state);
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zmr_message
 *
 * Description:
 *   The remote system wants to put a message on stderr
 *
 ****************************************************************************/

int zmr_message(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d\n", pzm->state);

  /* Setup to receive a data packet.  Enter PSTATE_DATA */

  zm_readstate(pzm);
  return OK;
}

/****************************************************************************
 * Name: zmr_zstderr
 *
 * Description:
 *   The remote system wants to put a message on stderr
 *
 ****************************************************************************/

static int zmr_zstderr(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d\n", pzm->state);

  pzm->pktbuf[pzm->pktlen] = '\0';
  fprintf(stderr, "Message: %s", (char*)pzm->pktbuf);
  return OK;
}

/****************************************************************************
 * Name: zmr_cmdto
 *
 * Description:
 *   Timed out waiting for command or stderr data
 *
 ****************************************************************************/

static int zmr_cmdto(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d: Timed out:  No command received\n", pzm->state);

  /* Terminate the reception with a timeout error */

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zmr_doneto
 *
 * Description:
 *   Timed out in ZMR_DONE state
 *
 ****************************************************************************/

static int zmr_doneto(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d: Timeout if ZMR_DONE\n", pzm->state);

  /* Terminate the reception with a timeout error */

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zmr_error
 *
 * Description:
 *   An unexpected event occurred in this state
 *
 ****************************************************************************/

static int zmr_error(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMR_STATE %d: Protocol error, header=%d\n",
        pzm->state, pzm->hdrdata[0]);

  pzm->flags |= ZM_FLAG_WAIT;
  pzm->flags &= ~ZM_FLAG_OO;   /* In case we get here from ZMR_FINISH */
  return OK;
}

/****************************************************************************
 * Name: zmr_parsefilename
 *
 * Description:
 *   Get an appropriate path to open.  The path is returned in pzmr->filename.
 *
 ****************************************************************************/

static int zmr_parsefilename(FAR struct zmr_state_s *pzmr,
                             FAR const uint8_t *namptr)
{
  static uint32_t uniqno = 0;
  struct stat buf;
  uint8_t f0;
  uint8_t f1;
  bool exists;
  int ret;

  DEBUGASSERT(pzmr && !pzmr->filename);

  /* Don't allow absolute paths */

  if (*namptr == '/')
    {
      return -EINVAL;
    }

  /* Extend the relative path to the file storage directory */

  asprintf(&pzmr->filename, "%s/%s", pzmr->pathname, namptr);
  if (!pzmr->filename)
    {
      zmdbg("ERROR: Failed to allocate full path %s/%s\n",
            CONFIG_SYSTEM_ZMODEM_MOUNTPOINT, namptr);
      return -ENOMEM;
    }

  /* Check if the file already exists at this path */

  ret = stat(pzmr->filename, &buf);
  if (ret == OK)
    {
      exists = true;
    }
  else
    {
      int errcode = errno;
      if (errcode == ENOENT)
        {
          exists = false;
        }
      else
        {
          zmdbg("ERROR: stat of %s failed: %d\n", pzmr->filename, errcode);
          ret = -errcode;
          goto errout_with_filename;
        }
    }

  /* If remote end has not specified transfer flags (f0), we can use the
   * local definitions.  We will stick with f0=f1=0 which will be a binary
   * file.
   */

  f0 = pzmr->f0;
  f1 = pzmr->f1;

  zmdbg("File %s f0=%02x, f1=%-2x, exists=%d, size=%ld/%ld\n",
        pzmr->filename, f0, f1, exists, (long)buf.st_size,
        (long)pzmr->filesize);

  /* Are we appending to an existing file? */

  if (f0 == ZCRESUM)
    {
      /* One corner case:  The file exists and is already the requested
       * size.  We already have the file!
       */

       if (exists && buf.st_size == pzmr->filesize)
         {
           zmdbg("ZCRESUM: Rejected\n");
           ret = -EEXIST;
           goto errout_with_filename;
         }

      /* Otherwise, indicate that we will append to the file (whether it
       * exists yet or not.
       */

      pzmr->cmn.flags |= ZM_FLAG_APPEND;
    }

  /* Check if the first is required to be there and fail if it is not */

  if (!exists && (f1 & ZMSKNOLOC) != 0)
    {
      zmdbg("ZMSKNOLOC: Rejected\n");
      ret = -ENOENT;
      goto errout_with_filename;
    }

  /* Now perform whatever actions are required for this file */

  switch (f1 & ZMMASK)
    {
    case ZMNEWL:              /* Transfer if source newer or longer */
      {
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
        zmdbg("ZMNEWL: timestamp=%08lx st_mtime=%08lx\n",
               (unsigned long)pzmr->timestamp, (unsigned long)buf.st_mtime);
#endif
        zmdbg("ZMNEWL: filesize=%08lx st_size=%08lx\n",
               (unsigned long)pzmr->filesize, (unsigned long)buf.st_size);

        if (exists &&
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
            pzmr->timestamp <= buf.st_mtime &&
#endif
            pzmr->filesize <= buf.st_size)
          {
            zmdbg("ZMNEWL: Rejected\n");
            ret = -EPERM;
            goto errout_with_filename;
          }
      }
      break;

    case ZMCRC:               /* Transfer if different CRC or length */
      {
        uint32_t crc = zm_filecrc(&pzmr->cmn, pzmr->filename);
        zmdbg("ZMCRC: crc=%08x vs. %08x\n", pzmr->crc, crc);
        zmdbg("       filesize=%08lx st_size=%08lx\n",
               (unsigned long)pzmr->filesize, (unsigned long)buf.st_size);

        if (exists &&
            pzmr->filesize == buf.st_size &&
            pzmr->crc == crc)
          {
            zmdbg("ZMCRC: Rejected\n");
            ret = -EPERM;
            goto errout_with_filename;
          }
      }
      break;

    case ZMAPND:              /* Append to existing file, if any */
      pzmr->cmn.flags |= ZM_FLAG_APPEND;

    case 0:                   /* Implementation dependent */
    case ZMCLOB:              /* Replace existing file */
      break;

    case ZMNEW:               /* Transfer if source is newer */
      {
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
        zmdbg("ZMNEWL: timestamp=%08lx st_mtime=%08lx\n",
              (unsigned long)pzmr->timestamp, (unsigned long)buf.st_mtime);

        if (exists &&
            pzmr->timestamp <= buf.st_mtime)
          {
            zmdbg("ZMNEW: Rejected\n");
            ret = -EPERM;
            goto errout_with_filename;
          }
#endif
      }
      break;

    case ZMDIFF:              /* Transfer if dates or lengths different */
      {
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
        zmdbg("ZMDIFF: timestamp=%08lx st_mtime=%0l8x\n",
               (unsigned long)pzmr->timestamp, (unsigned long)buf.st_mtime);
#endif
        zmdbg("ZMDIFF: filesize=%08lx st_size=%08lx\n",
               (unsigned long)pzmr->filesize, (unsigned long)buf.st_size);

        if (exists &&
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
            pzmr->timestamp == buf.st_mtime &&
#endif
            pzmr->filesize == buf.st_size)
          {
            zmdbg("ZMDIFF: Rejected\n");
            ret = -EPERM;
            goto errout_with_filename;
          }
      }
      break;

    case ZMPROT:              /* Protect: transfer only if dest doesn't exist */
      {
        if (exists)
          {
            zmdbg("ZMPROT: Rejected\n");
            ret = -EPERM;
            goto errout_with_filename;
          }
      }
      break;

    case ZMCHNG:              /* Change filename if destination exists */
      {
        FAR char *candidate;

        /* Does the file exist? */

        if (exists)
          {
            /* Yes.. loop until we create a unique file name */

            while (exists)
              {
                /* Create a candidate file name */

                asprintf(&candidate, "%s_%d", pzmr->filename, ++uniqno);
                if (!candidate)
                  {
                    zmdbg("ERROR:  Failed to allocate candidate %s_%d\n",
                          pzmr->filename, uniqno);
                    ret = -ENOMEM;
                    goto errout_with_filename;
                  }

                /* Does the candidate also exist? */

                ret = stat(candidate, &buf);
                if (ret < 0)
                  {
                    int errcode = errno;
                    if (errcode != ENOENT)
                      {
                        zmdbg("ERROR: stat of %s failed: %d\n", candidate, errcode);
                      }

                    /* Free the old filename and replace it with the candidate */

                    free(pzmr->filename);
                    pzmr->filename = candidate;
                    exists = false;
                  }
              }
          }
        }
        break;

      default:
        ret = -EINVAL;
        goto errout_with_filename;
    }

  /* We have accepted pzmr->filename.  If the file exists and we are not
   * appending to it, then unlink the old file now.
   */

  if (exists && (pzmr->cmn.flags & ZM_FLAG_APPEND) == 0)
    {
      ret = unlink(pzmr->filename);
      if (ret != OK)
        {
          int errorcode = errno;

          zmdbg("ERROR: unlink of %s failed: %d\n", pzmr->filename, errorcode);
          ret = -errorcode;
          goto errout_with_filename;
        }
    }

  zmdbg("Accepted filename: %s\n", pzmr->filename);
  return OK;

errout_with_filename:
  free(pzmr->filename);
  pzmr->filename = NULL;
  return ret;
}

/****************************************************************************
 * Name: zmr_openfile
 *
 * Description:
 *   If no output file has been opened to receive the data, then open the
 *   file for output whose name is in pzm->pktbuf.
 *
 ****************************************************************************/

static int zmr_openfile(FAR struct zmr_state_s *pzmr, uint32_t crc)
{
  off_t offset;
  uint8_t by[4];

  /* Has an output file already been opened?  Do we have a file name? */

  if (pzmr->outfd < 0)
    {
      /* No.. We should have a filename from the ZFILE packet? */

      if (!pzmr->filename)
        {
          zmdbg("No filename!\n");
          goto skip;
        }

      /* Yes.. then open this file for output */

      pzmr->outfd = open((FAR char *)pzmr->filename,
                         O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (pzmr->outfd < 0)
        {
          zmdbg("ERROR: Failed to open %s: %d\n", pzmr->filename, errno);
          goto skip;
        }
    }

  /* Are we appending/resuming a transfer? */

  offset = 0;
  if (pzmr->f0 == ZCRESUM)
    {
      /* Yes... get the current file position */

      offset = lseek(pzmr->outfd, 0, SEEK_CUR);
      if (offset == (off_t)-1)
        {
          zmdbg("ERROR: lseek failed: %d\n", errno);
          goto skip;
        }
    }

  zmdbg("ZMR_STATE %d->%d: Send ZRPOS(%ld)\n",
        pzmr->cmn.state, ZMR_READREADY, (unsigned long)offset);

  pzmr->offset = offset;
  pzmr->cmn.state = ZMR_READREADY;
  zm_be32toby(pzmr->offset, by);
  return zm_sendhexhdr(&pzmr->cmn, ZRPOS, by);

  /* We get here on any failures.  This file will be skipped. */

skip:
  zmdbg("ZMR_STATE %d->%d: Send ZSKIP\n", pzmr->cmn.state, ZMR_START);

  pzmr->cmn.state = ZMR_START;
  return zm_sendhexhdr(&pzmr->cmn, ZSKIP, g_zeroes);
}

/****************************************************************************
 * Name: zmr_fileerror
 *
 * Description:
 *   A receiver-detected file error has occurred.  Send Attn followed by
 *   the specified header (ZRPOS or XFERR).
 *
 ****************************************************************************/

static int zmr_fileerror(FAR struct zmr_state_s *pzmr, uint8_t type,
                         uint32_t data)
{
  FAR uint8_t *src;
  FAR uint8_t *dest;
  uint8_t by[4];

  /* Set the state back to IDLE to abort the transfer */

  zmdbg("PSTATE %d:%d->%d:%d\n",
        pzmr->cmn.pstate, pzmr->cmn.psubstate, PSTATE_IDLE, PIDLE_ZPAD);

  pzmr->cmn.pstate    = PSTATE_IDLE;
  pzmr->cmn.psubstate = PIDLE_ZPAD;

  /* Send Attn */

  if (pzmr->attn != NULL)
    {
      ssize_t nwritten;
      int len;

      /* Copy the attention string to the I/O buffer (pausing is ATTNPSE
       * is encountered.
       */

      dest = pzmr->cmn.pktbuf;
      for (src = (FAR void *)pzmr->attn; *src != '\0'; src++)
        {
          if (*src == ATTNBRK )
            {
#ifdef CONFIG_SYSTEM_ZMODEM_SENDBREAK
              /* Send a break
               * TODO: tcsendbreak() does not yet exist.
               */

              tcsendbreak(pzm->remfd, 0);
#endif
            }
          else if (*src == ATTNPSE)
            {
              /* Pause for 1 second */

              sleep(1);
            }
          else
            {
              /* Transfer the character */

              *dest++ = *src;
            }
        }

      /* Null-terminate and send */

      *dest++ = '\0';

      len = strlen((FAR char *)pzmr->cmn.pktbuf);
      nwritten = zm_remwrite(pzmr->cmn.remfd, pzmr->cmn.pktbuf, len);
      if (nwritten < 0)
        {
          zmdbg("ERROR: zm_remwrite failed: %d\n", (int)nwritten);
          return (int)nwritten;
        }
    }

  /* Send the specified header */

  zm_be32toby(data, by);
  return zm_sendhexhdr(&pzmr->cmn, type, by);
}

/****************************************************************************
 * Name: zmr_filecleanup
 *
 * Description:
 *   Release resources tied up by the last file transfer
 *
 ****************************************************************************/

static void zmr_filecleanup(FAR struct zmr_state_s *pzmr)
{
  /* Make sure that the file is closed */

  if (pzmr->outfd >= 0)
    {
      close(pzmr->outfd);
      pzmr->outfd = -1;
    }

  /* Deallocate the file name and attention strings */

  if (pzmr->filename)
    {
      free(pzmr->filename);
      pzmr->filename = NULL;
    }

  if (pzmr->attn)
    {
      free(pzmr->attn);
      pzmr->attn = NULL;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zmr_initialize
 *
 * Description:
 *   Initialize for Zmodem receive operation
 *
 * Input Parameters:
 *   remfd - The R/W file/socket descriptor to use for communication with the
 *      remote peer.
 *
 * Returned Value:
 *   An opaque handle that can be use with zmr_receive() and zmr_release().
 *
 ****************************************************************************/

ZMRHANDLE zmr_initialize(int remfd)
{
  FAR struct zmr_state_s *pzmr;
  FAR struct zm_state_s *pzm;
  int ret;

  /* Allocate a new Zmodem receive state structure */

  pzmr = (FAR struct zmr_state_s*)zalloc(sizeof(struct zmr_state_s));
  if (pzmr)
    {
      /* Initialize the state structure */

      pzm            = &pzmr->cmn;
      pzm->evtable   = g_zmr_evtable;
      pzm->state     = ZMR_START;
      pzm->pstate    = PSTATE_IDLE;
      pzm->psubstate = PIDLE_ZPAD;
      pzm->remfd     = remfd;
      pzmr->rcaps    = CANFC32 | CANFDX;
      pzmr->outfd    = -1;

      /* Create a timer to handle timeout events */

      ret = zm_timerinit(pzm);
      if (ret < 0)
        {
          zmdbg("ERROR: zm_timerinit failed: %d\n", ret);
          free(pzmr);
          return (ZMRHANDLE)NULL;
        }

      /* Note that no action is taken now... a timeout of zero is set (because
       * of the memset).  If there is nothing pending, ZRINIT will be sent.
       */

      zmdbg("Initial state: %d\n", pzm->state);
    }

  return (ZMRHANDLE)pzmr;
}

/****************************************************************************
 * Name: zmr_receive
 *
 * Description:
 *   Receive file(s) sent from the remote peer.
 *
 * Input Parameters:
 *   handle    - The handler created by zmr_initialize().
 *   pathname  - The name of the local path to hold the file
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zmr_receive(ZMRHANDLE handle, FAR const char *pathname)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s*)handle;

  pzmr->pathname = pathname;

  /* The first thing that should happen is to receive ZRQINIT from the
   * remote sender.  This could take while so use a long timeout.
   */

  pzmr->cmn.timeout = CONFIG_SYSTEM_ZMODEM_CONNTIME;

  /* Then the state machine data pump will do the rest of the job */

  return zm_datapump(&pzmr->cmn);
}

/****************************************************************************
 * Name: zmr_release
 *
 * Description:
 *   Called by the user when there are no more files to receive.
 *
 * Input Parameters:
 *   handle - The handler created by zmr_initialize().
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int zmr_release(ZMRHANDLE handle)
{
  FAR struct zmr_state_s *pzmr = (FAR struct zmr_state_s*)handle;
  int ret;

  /* Send ZFIN */

  pzmr->cmn.state = ZMR_FINISH;
  ret = zm_sendhexhdr(&pzmr->cmn, ZFIN, g_zeroes);
  zmdbg("ZMR_STATE %d: Send ZFIN\n", pzmr->cmn.state);

  /* Release the timer resources */

  zm_timerrelease(&pzmr->cmn);

  /* Clean up any resources that may be held from the last file transfer */

  zmr_filecleanup(pzmr);

  /* Then release the receive state structure itself */

  free(pzmr);
  return ret;
}
