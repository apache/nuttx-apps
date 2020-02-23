/****************************************************************************
 * system/zmodem/zm_send.c
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
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <crc16.h>
#include <crc32.h>

#include <nuttx/ascii.h>
#include "system/zmodem.h"

#include "zm.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Zmodem transmit states.
 *
 * A simple transaction, one file, no errors, no CHALLENGE, overlapped I/O.
 * These happen when zm_initialize() is called:
 *
 *   Sender               Receiver    State
 *   --------------     ------------  --------
 *   "rz\r"       ---->               N/A
 *   ZRQINIT      ---->               ZMS_START
 *                <---- ZRINIT
 *   ZSINIT       ---->               ZMS_INITACK
 *                <---- ZACK          End-of-Transfer
 *
 * These happen each time that zm_send() is called:
 *
 *   Sender               Receiver    State
 *   --------------     ------------  --------
 *   ZFILE        ---->               ZMS_FILEWAIT
 *                <---- ZRPOS
 *   ZCRC         ---->               ZMS_CRCWAIT
 *                <---- ZRPOS
 *   ZDATA        ---->
 *   Data packets ---->               ZMS_SENDING /ZMS_SENDWAIT
 *   Last packet  ---->               ZMS_SENDDONE
 *   ZEOF         ---->               ZMS_SENDEOF
 *                <---- ZRINIT
 *   ZFIN         ---->               ZMS_FINISH
 *                <---- ZFIN          End-of-Transfer
 *
 * And, finally, when zm_release() is called:
 *
 *   Sender               Receiver    State
 *   --------------     ------------  --------
 *   OO            ---->
 */

enum zmodem_state_e
{
  ZMS_START = 0,      /* ZRQINIT sent, waiting for ZRINIT from receiver */
  ZMS_INITACK,        /* Received ZRINIT, sent ZSINIT, waiting for ZACK */
  ZMS_FILEWAIT,       /* Sent file header, waiting for ZRPOS */
  ZMS_CRCWAIT,        /* Sent file CRC, waiting for ZRPOS */
  ZMS_SENDING,        /* Streaming data subpackets, ready for interrupt */
  ZMS_SENDWAIT,       /* Waiting for ZACK */
  ZMS_SENDDONE,       /* File finished, need to send ZEOF */
  ZMS_SENDEOF,        /* Sent ZEOF, waiting for ZACK */
  ZMS_FINISH,         /* Sent ZFIN, waiting for ZFIN */
  ZMS_COMMAND,        /* Waiting for command data */
  ZMS_MESSAGE,        /* Waiting for message from received */
  ZMS_DONE            /* Finished with the file transfer */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Transition actions */

static int zms_zrinit(FAR struct zm_state_s *pzm);
static int zms_attention(FAR struct zm_state_s *pzm);
static int zms_challenge(FAR struct zm_state_s *pzm);
static int zms_abort(FAR struct zm_state_s *pzm);
static int zms_ignore(FAR struct zm_state_s *pzm);
static int zms_command(FAR struct zm_state_s *pzm);
static int zms_message(FAR struct zm_state_s *pzm);
static int zms_stderrdata(FAR struct zm_state_s *pzm);
static int zms_initdone(FAR struct zm_state_s *pzm);
static int zms_sendzsinit(FAR struct zm_state_s *pzm);
static int zms_sendfilename(FAR struct zm_state_s *pzm);
static int zms_endoftransfer(FAR struct zm_state_s *pzm);
static int zms_fileskip(FAR struct zm_state_s *pzm);
static int zms_sendfiledata(FAR struct zm_state_s *pzm);
static int zms_sendpacket(FAR struct zm_state_s *pzm);
static int zms_filecrc(FAR struct zm_state_s *pzm);
static int zms_sendwaitack(FAR struct zm_state_s *pzm);
static int zms_sendnak(FAR struct zm_state_s *pzm);
static int zms_sendrpos(FAR struct zm_state_s *pzm);
static int zms_senddoneack(FAR struct zm_state_s *pzm);
static int zms_resendeof(FAR struct zm_state_s *pzm);
static int zms_xfrdone(FAR struct zm_state_s *pzm);
static int zms_finish(FAR struct zm_state_s *pzm);
static int zms_timeout(FAR struct zm_state_s *pzm);
static int zms_cmdto(FAR struct zm_state_s *pzm);
static int zms_doneto(FAR struct zm_state_s *pzm);
static int zms_error(FAR struct zm_state_s *pzm);

/* Internal helpers */

static int zms_startfiledata(FAR struct zms_state_s *pzms);
static int zms_sendfile(FAR struct zms_state_s *pzms,
                        FAR const char *filename,
                        FAR const char *rfilename, uint8_t f0, uint8_t f1);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Events handled in state ZMS_START - ZRQINIT sent, waiting for ZRINIT from
 * receiver
 */

static const struct zm_transition_s g_zms_start[] =
{
  {ZME_RINIT,     true,  ZMS_START,    zms_zrinit},
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_CHALLENGE, true,  ZMS_START,    zms_challenge},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_NAK,       false, ZMS_START,    zms_ignore},
  {ZME_COMMAND,   false, ZMS_COMMAND,  zms_command},
  {ZME_STDERR,    false, ZMS_MESSAGE,  zms_message},
  {ZME_TIMEOUT,   false, ZMS_START,    zms_timeout},
  {ZME_ERROR,     false, ZMS_START,    zms_error},
};

/* Events handled in state ZMS_INITACK - Received ZRINIT, sent (optional)
 * ZSINIT, waiting for ZACK
 */

static const struct zm_transition_s g_zmr_initack[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_ACK,       true,  ZMS_INITACK,  zms_initdone},
  {ZME_NAK,       true,  ZMS_INITACK,  zms_sendzsinit},
  {ZME_RINIT,     true,  ZMS_INITACK,  zms_zrinit},
  {ZME_CHALLENGE, true,  ZMS_INITACK,  zms_challenge},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_COMMAND,   false, ZMS_COMMAND,  zms_command},
  {ZME_STDERR,    false, ZMS_MESSAGE,  zms_message},
  {ZME_TIMEOUT,   false, ZMS_INITACK,  zms_timeout},
  {ZME_ERROR,     false, ZMS_INITACK,  zms_error},
};

/* Events handled in state ZMS_FILEWAIT- Sent file header, waiting for ZRPOS */

static const struct zm_transition_s g_zms_filewait[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_RPOS,      true,  ZMS_SENDING,  zms_sendfiledata},
  {ZME_SKIP,      true,  ZMS_FILEWAIT, zms_fileskip},
  {ZME_CRC,       true,  ZMS_FILEWAIT, zms_filecrc},
  {ZME_NAK,       true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_CHALLENGE, true,  ZMS_FILEWAIT, zms_challenge},
  {ZME_COMMAND,   false, ZMS_COMMAND,  zms_command},
  {ZME_STDERR,    false, ZMS_MESSAGE,  zms_message},
  {ZME_TIMEOUT,   false, ZMS_FILEWAIT, zms_timeout},
  {ZME_ERROR,     false, ZMS_FILEWAIT, zms_error},
};

/* Events handled in state ZMS_CRCWAIT - Sent file CRC, waiting for ZRPOS
 * response.
 */

static const struct zm_transition_s g_zms_crcwait[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_RPOS,      true,  ZMS_SENDING,  zms_sendfiledata},
  {ZME_SKIP,      true,  ZMS_FILEWAIT, zms_fileskip},
  {ZME_NAK,       true,  ZMS_CRCWAIT,  zms_filecrc},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_CRC,       false, ZMS_CRCWAIT,  zms_filecrc},
  {ZME_CHALLENGE, false, ZMS_CRCWAIT,  zms_challenge},
  {ZME_COMMAND,   false, ZMS_COMMAND,  zms_command},
  {ZME_STDERR,    false, ZMS_MESSAGE,  zms_message},
  {ZME_TIMEOUT,   false, ZMS_CRCWAIT,  zms_timeout},
  {ZME_ERROR,     false, ZMS_CRCWAIT,  zms_error},
};

/* Events handled in state ZMS_SENDING - Sending data subpackets, ready for
 * interrupt
 */

static const struct zm_transition_s g_zmr_sending[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_attention},
  {ZME_ACK,       false, ZMS_SENDING,  zms_sendpacket},
  {ZME_RPOS,      true,  ZMS_SENDING,  zms_sendrpos},
  {ZME_SKIP,      true,  ZMS_FILEWAIT, zms_fileskip},
  {ZME_NAK,       true,  ZMS_SENDING,  zms_sendnak},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_TIMEOUT,   false, ZMS_SENDING,  zms_sendpacket},
  {ZME_ERROR,     false, ZMS_SENDING,  zms_error},
};

/* Events handled in state ZMS_SENDWAIT - Waiting for ZACK */

static const struct zm_transition_s g_zms_sendwait[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_attention},
  {ZME_ACK,       false, ZMS_SENDING,  zms_sendwaitack},
  {ZME_RPOS,      false, ZMS_SENDWAIT, zms_sendrpos},
  {ZME_SKIP,      true,  ZMS_FILEWAIT, zms_fileskip},
  {ZME_NAK,       false, ZMS_SENDING,  zms_sendnak},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_TIMEOUT,   false, ZMS_SENDWAIT, zms_timeout},
  {ZME_ERROR,     false, ZMS_SENDWAIT, zms_error},
};

/* Events handled in state ZMS_SENDDONE - File sent, need to send ZEOF */

static const struct zm_transition_s g_zms_senddone[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_ACK,       false, ZMS_SENDWAIT, zms_senddoneack},
  {ZME_RPOS,      true,  ZMS_SENDING,  zms_sendrpos},
  {ZME_SKIP,      true,  ZMS_FILEWAIT, zms_fileskip},
  {ZME_NAK,       true,  ZMS_SENDING,  zms_sendnak},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_ERROR,     false, ZMS_SENDWAIT, zms_error},
};

/* Events handled in state ZMS_SENDEOF - Sent ZEOF, waiting for ZACK or
 * ZRINIT
 *
 * Paragraph 8.2: "The sender sends a ZEOF header with the file ending
 * offset equal to the number of characters in the file.  The receiver
 * compares this number with the number of characters received.  If the
 * receiver has received all of the file, it closes the file.  If the
 * file close was satisfactory, the receiver responds with ZRINIT.  If
 * the receiver has not received all the bytes of the file, the receiver
 * ignores the ZEOF because a new ZDATA is coming.  If the receiver cannot
 * properly close the file, a ZFERR header is sent.
 */

const struct zm_transition_s g_zms_sendeof[] =
{
  {ZME_RINIT,     true,  ZMS_START,    zms_endoftransfer},
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_ACK,       false, ZMS_SENDEOF,  zms_ignore},
  {ZME_RPOS,      true,  ZMS_SENDWAIT, zms_sendrpos},
  {ZME_SKIP,      true,  ZMS_START,    zms_fileskip},
  {ZME_NAK,       true,  ZMS_SENDEOF,  zms_resendeof},
  {ZME_RINIT,     true,  ZMS_FILEWAIT, zms_sendfilename},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_TIMEOUT,   false, ZMS_SENDEOF,  zms_timeout},
  {ZME_ERROR,     false, ZMS_SENDEOF,  zms_error},
};

/* Events handled in state ZMS_FINISH - Sent ZFIN, waiting for ZFIN */

static const struct zm_transition_s g_zms_finish[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_FIN,       true,  ZMS_DONE,     zms_xfrdone},
  {ZME_NAK,       true,  ZMS_FINISH,   zms_finish},
  {ZME_RINIT,     true,  ZMS_FINISH,   zms_finish},
  {ZME_ABORT,     true,  ZMS_FINISH,   zms_abort},
  {ZME_FERR,      true,  ZMS_FINISH,   zms_abort},
  {ZME_TIMEOUT,   false, ZMS_FINISH,   zms_timeout},
  {ZME_ERROR,     false, ZMS_FINISH,   zms_error}
};

/* Events handled in state ZMS_COMMAND - Waiting for command data */

static struct zm_transition_s g_zms_command[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_DATARCVD,  false, ZMS_COMMAND,  zms_ignore},
  {ZME_TIMEOUT,   false, ZMS_COMMAND,  zms_cmdto},
  {ZME_ERROR,     false, ZMS_COMMAND,  zms_error}
};

/* Events handled in state ZMS_MESSAGE - Waiting for stderr data */

static struct zm_transition_s g_zms_message[] =
{
  {ZME_SINIT,     false, ZMS_START,    zms_ignore},
  {ZME_DATARCVD,  false, ZMS_MESSAGE,  zms_stderrdata},
  {ZME_TIMEOUT,   false, ZMS_MESSAGE,  zms_cmdto},
  {ZME_ERROR,     false, ZMS_MESSAGE,  zms_error}
};

/* Events handled in state ZMS_DONE - Finished with transfer */

static struct zm_transition_s g_zms_done[] =
{
  {ZME_TIMEOUT,   false, ZMS_DONE,     zms_doneto},
  {ZME_ERROR,     false, ZMS_DONE,     zms_error}
};

/* State x Event table for Zmodem receive.  The order of states must
 * exactly match the order defined in enum zms_e
 */

static FAR const struct zm_transition_s * const g_zms_evtable[] =
{
  g_zms_start,    /* ZMS_START:    ZRQINIT sent, waiting for ZRINIT from receiver */
  g_zmr_initack,  /* ZMS_INITACK:  Received ZRINIT, sent ZSINIT, waiting for ZACK */
  g_zms_filewait, /* ZMS_FILEWAIT: Sent file header, waiting for ZRPOS */
  g_zms_crcwait,  /* ZMS_CRCWAIT:  Sent file CRC, waiting for ZRPOS response */
  g_zmr_sending,  /* ZMS_SENDING:  Sending data subpackets, ready for interrupt */
  g_zms_sendwait, /* ZMS_SENDWAIT: Waiting for ZACK */
  g_zms_senddone, /* ZMS_SENDDONE: File sent, need to send ZEOF */
  g_zms_sendeof,  /* ZMS_SENDEOF:  Sent ZEOF, waiting for ZACK */
  g_zms_finish,   /* ZMS_FINISH:   Sent ZFIN, waiting for ZFIN */
  g_zms_command,  /* ZMS_COMMAND:  Waiting for command data */
  g_zms_message,  /* ZMS_MESSAGE:  Waiting for message from receiver */
  g_zms_done      /* ZMS_DONE:     Finished with transfer */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zms_zrinit
 *
 * Description:
 *   Received ZRINIT.  Usually received while in start state, this can
 *   also be an attempt to resync after a protocol failure.
 *
 ****************************************************************************/

static int zms_zrinit(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  uint16_t rcaps;

  zmdbg("ZMS_STATE %d\n", pzm->state);

  /* hdrdata[0] is the header type; header[1-4] is payload:
   *
   *   F0 and F1 contain the  bitwise OR of the receiver capability flags
   *   P0 and ZP1 contain the size of the receiver's buffer in bytes (or 0
   *     if nonstop I/O is allowed.
   */

  pzms->rcvmax = (uint16_t)pzm->hdrdata[2] << 8 | (uint16_t)pzm->hdrdata[1];
  rcaps        = (uint16_t)pzm->hdrdata[3] << 8 | (uint16_t)pzm->hdrdata[4];

  /* Set flags associated with the capabilities */

  if ((rcaps & CANFC32) != 0)
    {
      pzm->flags |= ZM_FLAG_CRC32;
    }

  if ((rcaps & ESCCTL) != 0)
    {
      pzm->flags |= ZM_FLAG_ESCCTRL;
    }

  /* Check if the receiver supports full-duplex streaming
   *
   * ZCRCW:
   *   "If the receiver cannot overlap serial and disk I/O, it uses the
   *    ZRINIT frame to specify a buffer length which the sender will
   *    not overflow.  The sending program sends a ZCRCW data subpacket
   *    and waits for a ZACK header before sending the next segment of
   *    the file.
   *
   * ZCRCG
   *   "A data subpacket terminated by ZCRCG and CRC does not elicit a
   *    response unless an error is detected; more data subpacket(s)
   *    follow immediately."
   *
   *
   * In order to support ZCRCG, this logic must be able to sample the
   * reverse channel while streaming to determine if the receiving wants
   * interrupt the transfer (CONFIG_SYSTEM_ZMODEM_RCVSAMPLE).
   *
   * ZCRCQ
   *   "ZCRCQ data subpackets expect a ZACK response with the
   *    receiver's file offset if no error, otherwise a ZRPOS response
   *    with the last good file offset.  Another data subpacket
   *    continues immediately.  ZCRCQ subpackets are not used if the
   *    receiver does not indicate FDX ability with the CANFDX bit.
   */

#ifdef CONFIG_SYSTEM_ZMODEM_RCVSAMPLE
  /* We support CANFDX.  We can do ZCRCG if the remote sender does too */

  if ((rcaps & (CANFDX | CANOVIO)) == (CANFDX | CANOVIO) && pzms->rcvmax == 0)
    {
      pzms->dpkttype = ZCRCG;
    }
#else
  /* We don't support CANFDX.  We can do ZCRCQ if the remote sender does  */

  if ((rcaps & (CANFDX | CANOVIO)) == (CANFDX | CANOVIO) && pzms->rcvmax == 0)
    {
      /* For the local sender, this is just like ZCRCW */

      pzms->dpkttype = ZCRCQ;
    }
#endif

  /* Otherwise, we have to do ZCRCW */

  else
    {
      pzms->dpkttype = ZCRCW;
    }

#ifdef CONFIG_SYSTEM_ZMODEM_ALWAYSSINT
  return zms_sendzsinit(pzm);
#else
#  ifdef CONFIG_SYSTEM_ZMODEM_SENDATTN
  if (pzms->attn != NULL)
    {
      return zms_sendzsinit(pzm);
    }
  else
#  endif
    {
      zmdbg("ZMS_STATE %d->%d\n", pzm->state, ZMS_DONE);
      pzm->state = ZMS_DONE;
      return ZM_XFRDONE;
    }
#endif
}

/****************************************************************************
 * Name: zms_attention
 *
 * Description:
 *   Received ZSINIT while sending data.  The receiver wants something.
 *   Switch tot he ZMS_SENDWAIT state and wait.  A ZRPOS should be forth-
 *   coming.
 *
 ****************************************************************************/

static int zms_attention(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);

  /* In the case of full streaming, the presence of pending read data should
   * cause the sending logic to break out of the loop and handle the received
   * data, getting us to this point.
   */

  if (pzm->state == ZMS_SENDING || pzm->state == ZMS_SENDWAIT)
    {
      /* Enter a wait state and see what they want.  Next header *should* be
       * ZRPOS.
       */

      zmdbg("ZMS_STATE %d->%d: Interrupt\n", pzm->state, ZMS_SENDWAIT);

      pzm->state   = ZMS_SENDWAIT;
      pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
    }

  return OK;
}

/****************************************************************************
 * Name: zms_challenge
 *
 * Description:
 *   Answer challenge from receiver
 *
 ****************************************************************************/

static int zms_challenge(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);
  return zm_sendhexhdr(pzm, ZACK, pzm->hdrdata + 1);
}

/****************************************************************************
 * Name: zms_abort
 *
 * Description:
 *   Receiver has cancelled
 *
 ****************************************************************************/

static int zms_abort(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);
  return zm_sendhexhdr(pzm, ZFIN, g_zeroes);
}

/****************************************************************************
 * Name: zms_ignore
 *
 * Description:
 *   Ignore the header
 *
 ****************************************************************************/

static int zms_ignore(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);
  return OK;
}

/****************************************************************************
 * Name: zms_command
 *
 * Description:
 *   Remote command received -- refuse it.
 *
 ****************************************************************************/

static int zms_command(FAR struct zm_state_s *pzm)
{
  uint8_t rbuf[4];

  zmdbg("ZMS_STATE %d\n", pzm->state);

  rbuf[0] = EPERM;
  rbuf[1] = 0;
  rbuf[2] = 0;
  rbuf[3] = 0;
  return zm_sendhexhdr(pzm, ZCOMPL, rbuf);
}

/****************************************************************************
 * Name: zms_message
 *
 * Description:
 *   The remote system wants to put a message on stderr
 *
 ****************************************************************************/

static int zms_message(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);

  zm_readstate(pzm);
  return OK;
}

/****************************************************************************
 * Name: zms_stderrdata
 *
 * Description:
 *   The remote system wants to put a message on stderr
 *
 ****************************************************************************/

static int zms_stderrdata(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d\n", pzm->state);

  pzm->pktbuf[pzm->pktlen] = '\0';
  fprintf(stderr, "Message: %s", (FAR char *)pzm->pktbuf);
  return OK;
}

/****************************************************************************
 * Name: zms_initdone
 *
 * Description:
 *   Received ZRINIT, sent ZRINIT, waiting for ZACK, ZACK received.  This
 *   completes the initialization sequence.  Returning ZM_XFRDONE will
 *   alloc zm_initialize() to return.
 *
 ****************************************************************************/

static int zms_initdone(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d->%d\n", pzm->state, ZMS_DONE);

  pzm->state = ZMS_DONE;
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zms_sendzsinit
 *
 * Description:
 *   The remote system wants to put a message on stderr
 *
 ****************************************************************************/

static int zms_sendzsinit(FAR struct zm_state_s *pzm)
{
#ifdef CONFIG_SYSTEM_ZMODEM_SENDATTN
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  FAR char *at = (pzms->attn != NULL) ? pzms->attn : "";
#endif
  int ret;

  /* Change to ZMS_INITACK state */

  zmdbg("ZMS_STATE %d->%d\n", pzm->state, ZMS_INITACK);
  pzm->state = ZMS_INITACK;

  /* Send the ZSINIT header (optional)
   *
   * Paragraph 11.3  ZSINIT.  "The Sender sends flags followed by a binary
   * data subpacket terminated with ZCRCW."
   */

  ret = zm_sendbinhdr(pzm, ZSINIT, g_zeroes);
  if (ret >= 0)
    {
      /* Paragraph 11.3 "The data subpacket contains the null terminated
       * Attn sequence, maximum length 32 bytes including the terminating
       * null."
       *
       * We expect a ZACK next.
       */

#ifdef CONFIG_SYSTEM_ZMODEM_SENDATTN
      /* Send the NUL-terminated attention string */

      ret = zm_senddata(pzm, (FAR uint8_t *)at, strlen(at) + 1);
#else
      /* Send a null string */

      ret = zm_senddata(pzm, g_zeroes, 1);
#endif
    }

  return ret;
}

/****************************************************************************
 * Name: zms_sendfilename
 *
 * Description:
 *   Send ZFILE header and filename.  Wait for a response from receiver.
 *
 ****************************************************************************/

static int zms_sendfilename(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  FAR uint8_t *ptr = pzm->scratch;
  int len;
  int ret;

  zmdbg("ZMS_STATE %d->%d\n", pzm->state, ZMS_FILEWAIT);

  pzm->state = ZMS_FILEWAIT;
  ret = zm_sendbinhdr(pzm, ZFILE, pzms->fflags);
  if (ret < 0)
    {
      zmdbg("ERROR: zm_sendbinhdr failed: %d\n", ret);
      return ret;
    }

  /* Paragraph 13:
   *   Pathname
   *     The pathname (conventionally, the file name) is sent as a null
   *     terminated ASCII string.
   */

  len = strlen(pzms->rfilename);
  memcpy(ptr, pzms->rfilename, len + 1);
  ptr += len + 1;

  /* Paragraph 13:
   *
   *   Length
   *     The file length ... is stored as a decimal string counting  the
   *     number of data bytes in the file.
   *   Modification Date
   *     A single space separates the modification date from the file
   *     length. ... The mod date is sent as an octal number giving ...
   *     A date of 0 implies the modification date is unknown and should be
   *     left as the date the file is received.
   *   File Mode
   *     A single space separates the file mode from the modification date.
   *     The file mode is stored as an octal string. Unless the file
   *     originated from a Unix system, the file mode is set to 0.
   *   Serial Number
   *     A single space separates the serial number from the file mode.
   *     The serial number of the transmitting program is stored as an
   *     octal string.  Programs which do not have a serial number should
   *     omit this field, or set it to 0.
   *   Number of Files Remaining
   *     If the number of files remaining is sent, a single space separates
   *     this field from the previous field.  This field is coded as a
   *     decimal number, and includes the current file.
   *   Number of Bytes Remaining
   *     If the number of bytes remaining is sent, a single space
   *     separates this field from the previous field. This field is coded
   *     as a decimal number, and includes the current file
   *   File Type
   *     If the file type is sent, a single space separates this field from
   *     the previous field.  This field is coded as a decimal number.
   *     Currently defined values are:
   *
   *       0  Sequential file - no special type
   *       1  Other types to be defined.
   *
   *   The file information is terminated by a null.
   */

#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
  sprintf((FAR char *)ptr, "%ld %lo 0 %d 1 %ld 0",
          (unsigned long)pzms->filesize, (unsigned long)pzms->timestamp,
          CONFIG_SYSTEM_ZMODEM_SERIALNO, (unsigned long)pzms->filesize);
#else
  sprintf((FAR char *)ptr, "%ld 0 0 %d 1 %ld 0",
          (unsigned long)pzms->filesize, CONFIG_SYSTEM_ZMODEM_SERIALNO,
          (unsigned long)pzms->filesize);
#endif

  ptr += strlen((char *)ptr);
  *ptr++ = '\0';

  len =  ptr - pzm->scratch;
  DEBUGASSERT(len < CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE);
  return zm_senddata(pzm, pzm->scratch, len);
}

/****************************************************************************
 * Name: zms_fileskip
 *
 * Description:
 *   The entire file has been transferred, ZEOF has been sent to the remote
 *   receiver, and the receiver has returned ZRINIT.  Time to send ZFIN.
 *
 ****************************************************************************/

static int zms_endoftransfer(FAR struct zm_state_s *pzm)
{
  zmdbg("ZMS_STATE %d send ZFIN\n", pzm->state);
  pzm->state = ZMS_FINISH;

  return zm_sendhexhdr(pzm, ZFIN, g_zeroes);
}

/****************************************************************************
 * Name: zms_fileskip
 *
 * Description:
 *   Received ZSKIP, receiver doesn't want this file.
 *
 ****************************************************************************/

static int zms_fileskip(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;

  zmdbg("ZMS_STATE %d\n", pzm->state);
  close(pzms->infd);
  pzms->infd = -1;
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zms_sendfiledata
 *
 * Description:
 *   Send a chunk of file data in response to a ZRPOS.  This may be followed
 *   followed by a ZCRCE, ZCRCG, ZCRCQ or ZCRCW dependinig on protocol flags.
 *
 ****************************************************************************/

static int zms_sendfiledata(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;

  pzm->flags &= ~ZM_FLAG_WAIT;
  return zms_startfiledata(pzms);
}

/****************************************************************************
 * Name: zms_sendpacket
 *
 * Description:
 *   Send a chunk of file data in response to a ZRPOS.  This may be followed
 *   followed by a ZCRCE, ZCRCG, ZCRCQ or ZCRCW dependinig on protocol flags.
 *
 *   This function is called after ZDATA is send, after previous data was
 *   ACKed, and on certain error conditions where it is necessary to re-send
 *   the file data.
 *
 ****************************************************************************/

static int zms_sendpacket(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  ssize_t nwritten;
  int32_t unacked;
  bool bcrc32;
  uint32_t crc;
  uint8_t by[4];
  uint8_t *ptr;
  uint8_t type;
  bool wait = false;
  int sndsize;
  int pktsize;
  int i;

  /* Loop, sending packets while we can if the receiver supports streaming
   * data.
   */

  do
    {
      /* This is the number of byte left in the file to be sent */

      sndsize = pzms->filesize - pzms->offset;

      /* This is the number of bytes that have been sent but not yet acknowledged. */

      unacked = pzms->offset - pzms->lastoffs;

      /* Can we still send?  If so, how much?   If rcvmax is zero, then the
       * remote can handle full streaming and we never have to wait.
       * Otherwise, we have to restrict the total number of unacknowledged
       * bytes to rcvmax.
       */

      zmdbg("sndsize: %d unacked: %d rcvmax: %d\n",
            sndsize, unacked, pzms->rcvmax);

      if (pzms->rcvmax != 0)
        {
          /* If we were to send 'sndsize' more bytes, would that exceed recvmax? */

          if (sndsize + unacked > pzms->rcvmax)
            {
              /* Yes... clip the maximum so that we stay within that limit */

              int maximum = pzms->rcvmax - unacked;
              if (sndsize < maximum)
                {
                  sndsize = maximum;
                }

              wait = true;
              zmdbg("Clipped sndsize: %d\n", sndsize);
            }
        }

      /* Can we send anything? */

      if (sndsize <= 0)
        {
          /* No, not now. Keep waiting */

          zmdbg("ZMS_STATE %d->%d\n", pzm->state, ZMS_SENDWAIT);

          pzm->state   = ZMS_SENDWAIT;
          pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
          return OK;
        }

      /* Determine what kind of packet to send
       *
       * ZCRCW:
       *   "If the receiver cannot overlap serial and disk I/O, it uses the
       *    ZRINIT frame to specify a buffer length which the sender will
       *    not overflow.  The sending program sends a ZCRCW data subpacket
       *    and waits for a ZACK header before sending the next segment of
       *    the file.
       *
       * ZCRCG
       *   "A data subpacket terminated by ZCRCG and CRC does not elicit a
       *    response unless an error is detected; more data subpacket(s)
       *    follow immediately."
       *
       * ZCRCQ
       *   "ZCRCQ data subpackets expect a ZACK response with the
       *    receiver's file offset if no error, otherwise a ZRPOS response
       *    with the last good file offset.  Another data subpacket
       *    continues immediately.  ZCRCQ subpackets are not used if the
       *    receiver does not indicate FDX ability with the CANFDX bit.
       */

      if ((pzm->flags & ZM_FLAG_WAIT) != 0)
        {
          type = ZCRCW;
          pzm->flags &= ~ZM_FLAG_WAIT;
        }
      else if (wait)
        {
          type = ZCRCW;
        }
      else
        {
          type = pzms->dpkttype;
        }

      /* Read characters from file and put into buffer until buffer is full
       * or file is exhausted
       */

      bcrc32      = ((pzm->flags & ZM_FLAG_CRC32) != 0);
      crc         = bcrc32 ? 0xffffffff : 0;
      pzm->flags &= ~ZM_FLAG_ATSIGN;

      ptr         = pzm->scratch;
      pktsize     = 0;

#ifdef CONFIG_SYSTEM_ZMODEM_SNDFILEBUF
      /* Read multiple bytes of file and store into the temporal buffer */

      zm_read(pzms->infd, pzm->filebuf, CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE);

      i = 0;
#endif

      while (pktsize <= (CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE - 10) &&
             (pzms->offset < pzms->filesize))
        {
          /* Add the new value to the accumulated CRC */

#ifdef CONFIG_SYSTEM_ZMODEM_SNDFILEBUF
          uint8_t ch = pzm->filebuf[i++];
#else
          uint8_t ch = zm_getc(pzms->infd);
#endif
          if (!bcrc32)
            {
              crc = (uint32_t)crc16part(&ch, 1, (uint16_t)crc);
            }
          else
            {
              crc = crc32part(&ch, 1, crc);
            }

          /* Put the character into the buffer, escaping as necessary */

          ptr = zm_putzdle(pzm, ptr, ch);

          /* Recalculate the accumulated packet size to handle expansion due
           * to escaping.
           */

          pktsize = (int32_t)(ptr - pzm->scratch);

          /* And increment the file offset */

          pzms->offset++;
        }

#ifdef CONFIG_SYSTEM_ZMODEM_SNDFILEBUF
      /* Restore file position to be read next time */

      lseek(pzms->infd, pzms->offset, SEEK_SET);
#endif

      /* If we've reached file end, a ZEOF header will follow.  If there's
       * room in the outgoing buffer for it, end the packet with ZCRCE and
       * append the ZEOF header.  If there isn't room, we'll have to do a
       * ZCRCW
       */

      pzm->flags &= ~ZM_FLAG_EOF;
      if (pzms->offset == pzms->filesize)
        {
          pzm->flags |= ZM_FLAG_EOF;
          if (wait || (pzms->rcvmax != 0 && pktsize < 24))
            {
              type = ZCRCW;
            }
          else
            {
              type = ZCRCE;
            }
        }

      /* Save the ZDLE in the transmit buffer */

      *ptr++ = ZDLE;

      /* Save the type */

      if (!bcrc32)
        {
          crc = (uint32_t)crc16part(&type, 1, (uint16_t)crc);
        }
      else
        {
          crc = crc32part(&type, 1, crc);
        }

      *ptr++ = type;

      /* Update the CRC and put the CRC in the transmit buffer */

      if (!bcrc32)
        {
          ptr = zm_putzdle(pzm, ptr, (crc >> 8) & 0xff);
          ptr = zm_putzdle(pzm, ptr, crc & 0xff);
        }
      else
        {
          /* REVISIT: Why complemented? */

          crc = ~crc;
          for (i = 0; i < 4; i++, crc >>= 8)
            {
              ptr = zm_putzdle(pzm, ptr, crc & 0xff);
            }
        }

      /* Get the final packet size */

      pktsize = ptr - pzm->scratch;
      DEBUGASSERT(pktsize < CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE);

      /* And send the packet */

      zmdbg("Sending %d bytes. New offset: %ld\n",
            pktsize, (unsigned long)pzms->offset);

      nwritten = zm_remwrite(pzm->remfd, pzm->scratch, pktsize);
      if (nwritten < 0)
        {
          zmdbg("ERROR: zm_remwrite failed: %d\n", (int)nwritten);
          return (int)nwritten;
        }

      /* Then do what?   That depends on the type of the transfer */

      switch (type)
        {
          /* That was the last packet.  Send ZCRCE to indicate the end of
           * file.
           */

        case ZCRCE:  /* CRC next, transfer ends, ZEOF follows */
          zmdbg("ZMS_STATE %d->%d: ZCRCE\n", pzm->state, ZMS_SENDEOF);

          pzm->state   = ZMS_SENDEOF;
          pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
          zm_be32toby(pzms->offset, by);
          return zm_sendhexhdr(pzm, ZEOF, by);

        /* We need to want for ZACK */

        case ZCRCW:  /* CRC next, send ZACK, transfer ends */
          if ((pzm->flags & ZM_FLAG_EOF) != 0)
            {
              zmdbg("ZMS_STATE %d->%d: EOF\n", pzm->state, ZMS_SENDDONE);
              pzm->state = ZMS_SENDDONE;
            }
          else
            {
              zmdbg("ZMS_STATE %d->%d: Not EOF\n", pzm->state, ZMS_SENDWAIT);
              pzm->state = ZMS_SENDWAIT;
            }

          pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
          break;

       /* No response is expected -- we are streaming */

        case ZCRCG:  /* Transfer continues non-stop */
        case ZCRCQ:  /* Expect ZACK, transfer may continues non-stop */
        default:
          zmdbg("ZMS_STATE %d->%d: Default\n", pzm->state, ZMS_SENDING);

          pzm->state = ZMS_SENDING;
          break;
        }
    }
#ifdef CONFIG_SYSTEM_ZMODEM_RCVSAMPLE
  while (pzm->state == ZMS_SENDING && !zm_rcvpending(pzm));
#else
  while (0);
#endif

  return OK;
}

/****************************************************************************
 * Name: zms_filecrc
 *
 * Description:
 *   ZFILE has been sent and waiting for ZCRC.  ZRPOS received.
 *
 ****************************************************************************/

static int zms_filecrc(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  uint8_t by[4];
  uint32_t crc;

  crc = zm_filecrc(pzm, pzms->filename);
  zmdbg("ZMS_STATE %d: CRC %08x\n", pzm->state, crc);

  zm_be32toby(crc, by);
  return zm_sendhexhdr(pzm, ZCRC, by);
}

/****************************************************************************
 * Name: zms_sendwaitack
 *
 * Description:
 *   An ACK arrived while waiting to transmit data.  Update last known
 *   receiver offset, and try to send more data.
 *
 ****************************************************************************/

static int zms_sendwaitack(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  uint8_t by[4];
  off_t offset;
  int ret;

  /* Paragraph 11.4  ZACK.  ?Acknowledgment to a ZSINIT frame, ..., ZCRCQ or
   * ZCRCW data subpacket.  ZP0 to ZP3 contain file offset."
   */

  offset = zm_bytobe32(pzm->hdrdata + 1);

  if (offset > pzms->lastoffs)
    {
      pzms->lastoffs = offset;
    }

  zmdbg("ZMS_STATE %d: offset: %ld\n", pzm->state, (unsigned long)offset);

  /* Now send the next data packet */

  zm_be32toby(pzms->offset, by);
  ret = zm_sendbinhdr(pzm, ZDATA, by);
  if (ret != OK)
    {
      return ret;
    }

  return zms_sendpacket(pzm);
}

/****************************************************************************
 * Name: zms_sendnak
 *
 * Description:
 *   ZDATA header was corrupt.  Start from beginning
 *
 ****************************************************************************/

static int zms_sendnak(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  off_t offset;

  /* Save the ZRPOS file offset */

  pzms->offset = pzms->zrpos;

  /* TODO: What is the correct thing to do if lseek fails? Send ZEOF? */

  offset = lseek(pzms->infd, pzms->offset, SEEK_SET);
  if (offset == (off_t)-1)
    {
      int errorcode = errno;

      zmdbg("ERROR: Failed to seek to %ld: %d\n",
            (unsigned long)pzms->offset, errorcode);
      DEBUGASSERT(errorcode > 0);
      return -errorcode;
    }

  zmdbg("ZMS_STATE %d: offset: %ld\n",
        pzm->state, (unsigned long)pzms->offset);

  return zms_sendpacket(pzm);
}

/****************************************************************************
 * Name: zms_sendrpos
 *
 * Description:
 *   Received ZRPOS while sending a file.  Set the new offset and try again.
 *
 ****************************************************************************/

static int zms_sendrpos(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;

  pzm->nerrors++;
  pzm->flags |= ZM_FLAG_WAIT;
  return zms_startfiledata(pzms);
}

/****************************************************************************
 * Name: zms_senddoneack
 *
 * Description:
 *   ACK arrived after last file packet sent.  Send the ZEOF
 *
 ****************************************************************************/

static int zms_senddoneack(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  uint8_t by[4];
  off_t offset;

  /* Paragraph 11.4  ZACK.  Acknowledgment to a ZSINIT frame, ..., ZCRCQ or
   * ZCRCW data subpacket.  ZP0 to ZP3 contain file offset.
   */

  offset = zm_bytobe32(pzm->hdrdata + 1);

  if (offset > pzms->lastoffs)
    {
      pzms->lastoffs = offset;
    }

  zmdbg("ZMS_STATE %d->%d: offset: %ld\n",
        pzm->state, ZMS_SENDEOF, (unsigned long)pzms->offset);

  /* Now send the ZEOF */

  pzm->state   = ZMS_SENDEOF;
  pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
  zm_be32toby(pzms->offset, by);
  return zm_sendhexhdr(pzm, ZEOF, by);
}

/****************************************************************************
 * Name: zms_resendeof
 *
 * Description:
 *   ACK arrived after last file packet sent.  Send the ZEOF
 *
 ****************************************************************************/

static int zms_resendeof(FAR struct zm_state_s *pzm)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)pzm;
  uint8_t by[4];

  zm_be32toby(pzms->offset, by);
  return zm_sendhexhdr(pzm, ZEOF, by);
}

/****************************************************************************
 * Name: zms_xfrdone
 *
 * Description:
 *   Sent ZFIN, received ZFIN in response.  This transfer is complete.
 *   We will not send the "OO" until the last file has been sent.
 *
 ****************************************************************************/

static int zms_xfrdone(FAR struct zm_state_s *pzm)
{
  zmdbg("Transfer complete\n");
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zms_finish
 *
 * Description:
 *   Sent ZFIN, received ZNAK or ZRINIT
 *
 ****************************************************************************/

static int zms_finish(FAR struct zm_state_s *pzm)
{
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zms_timeout
 *
 * Description:
 *   An timeout occurred in this state
 *
 ****************************************************************************/

static int zms_timeout(FAR struct zm_state_s *pzm)
{
  zmdbg("ERROR: Receiver did not respond\n");
  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zms_cmdto
 *
 * Description:
 *   Timed out waiting for command or stderr data
 *
 ****************************************************************************/

static int zms_cmdto(FAR struct zm_state_s *pzm)
{
  zmdbg("ERROR: No command received\n");
  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: zms_doneto
 *
 * Description:
 *   Timed out in ZMS_DONE state
 *
 ****************************************************************************/

static int zms_doneto(FAR struct zm_state_s *pzm)
{
  zmdbg("Timeout if ZMS_DONE\n");
  return ZM_XFRDONE;
}

/****************************************************************************
 * Name: zms_error
 *
 * Description:
 *   An unexpected event occurred in this state
 *
 ****************************************************************************/

static int zms_error(FAR struct zm_state_s *pzm)
{
  zmdbg("Unhandled event, header=%d\n", pzm->hdrdata[0]);
  pzm->flags |= ZM_FLAG_WAIT;
  return OK;
}

/****************************************************************************
 * Name: zms_sendfiledata
 *
 * Description:
 *   Send a chunk of file data in response to a ZRPOS.  This may be followed
 *   followed by a ZCRCE, ZCRCG, ZCRCQ or ZCRCW dependinig on protocol flags.
 *
 ****************************************************************************/

static int zms_startfiledata(FAR struct zms_state_s *pzms)
{
  off_t offset;
  int ret;

  zmdbg("ZMS_STATE %d: offset %ld nerrors %d\n",
        pzms->cmn.state, (unsigned long)pzms->offset, pzms->cmn.nerrors);

  /* Paragraph 8.2: "A ZRPOS header from the receiver initiates transmission
   * of the file data starting at the offset in the file specified in the
   * ZRPOS header."
   */

  pzms->zrpos      = zm_bytobe32(pzms->cmn.hdrdata + 1);
  pzms->offset     = pzms->zrpos;
  pzms->lastoffs   = pzms->zrpos;

  /* See to the requested file position */

  offset = lseek(pzms->infd, pzms->offset, SEEK_SET);
  if (offset == (off_t)-1)
    {
      int errorcode = errno;

      zmdbg("ERROR: Failed to seek to %ld: %d\n",
            (unsigned long)pzms->offset, errorcode);
      DEBUGASSERT(errorcode > 0);
      return -errorcode;
    }

  /* Paragraph 8.2: "The sender sends a ZDATA binary header (with file
   * position) followed by one or more data subpackets."
   */

  zmdbg("ZMS_STATE %d: Send ZDATA offset %ld\n",
        pzms->cmn.state, (unsigned long)pzms->offset);

  ret = zm_sendbinhdr(&pzms->cmn, ZDATA, pzms->cmn.hdrdata + 1);
  if (ret != OK)
    {
      return ret;
    }

  return zms_sendpacket(&pzms->cmn);
}

/****************************************************************************
 * Name: zms_sendfile
 *
 * Description:
 *   Begin transmission of a file
 *
 ****************************************************************************/

/* Called by user to begin transmission of a file */

static int zms_sendfile(FAR struct zms_state_s *pzms,
                        FAR const char *filename,
                        FAR const char *rfilename, uint8_t f0, uint8_t f1)
{
  struct stat buf;
  int ret;

  DEBUGASSERT(pzms && filename && rfilename);
  zmdbg("filename: %s rfilename: %s f0: %02x f1: %02x\n",
        filename, rfilename, f0, f1);

  /* TODO: The local file name *must* be an absolute patch for now.  This if
   * environment variables are supported, then any relative paths could be
   * extended using the contents of the current working directory CWD.
   */

  if (filename[0] != '/')
    {
      zmdbg("ERROR: filename must be an absolute path: %s\n", filename);
      return -ENOSYS;
    }

  /* Get information about the local file */

  ret = stat(filename, &buf);
  if (ret < 0)
    {
      int errorcode = errno;
      DEBUGASSERT(errorcode > 0);
      zmdbg("Failed to stat %s: %d\n", filename, errorcode);
      return -errorcode;
    }

  /* Open the local file for reading */

  pzms->infd = open(filename, O_RDONLY);
  if (pzms->infd < 0)
    {
      int errorcode = errno;
      DEBUGASSERT(errorcode > 0);
      zmdbg("Failed to open %s: %d\n", filename, errorcode);
      return -errorcode;
    }

  /* Initialize for the transfer */

  pzms->cmn.flags &= ~ZM_FLAG_EOF;
  pzms->filename   = filename;
  pzms->rfilename  = rfilename;
  DEBUGASSERT(pzms->filename && pzms->rfilename);

  pzms->fflags[3]  = f0;
  pzms->fflags[2]  = f1;
  pzms->fflags[1]  = 0;
  pzms->fflags[0]  = 0;
  pzms->offset     = 0;
  pzms->lastoffs   = 0;

  pzms->filesize   = buf.st_size;
#ifdef CONFIG_SYSTEM_ZMODEM_TIMESTAMPS
  pzms->timestamp  = buf.st_mtime;
#endif

  zmdbg("ZMS_STATE %d->%d\n", pzms->cmn.state, ZMS_FILEWAIT);

  pzms->cmn.state  = ZMS_FILEWAIT;

  zmdbg("ZMS_STATE %d: Send ZFILE(%s)\n", pzms->cmn.state, pzms->rfilename);
  return zms_sendfilename(&pzms->cmn);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: zms_initialize
 *
 * Description:
 *   Initialize for Zmodem send operation.  The overall usage is as follows:
 *
 *   1) Call zms_initialize() to create the partially initialized struct
 *      zms_state_s instance.
 *   2) Make any custom settings in the struct zms_state_s instance.
 *   3) Create a stream instance to get the "file" to transmit and add
 *      the filename to pzms
 *   4) Call zms_send() to transfer the file.
 *   5) Repeat 3) and 4) to transfer as many files as desired.
 *   6) Call zms_release() when the final file has been transferred.
 *
 * Input Parameters:
 *   remfd - The R/W file/socket descriptor to use for communication with the
 *      remote peer.
 *
 ****************************************************************************/

ZMSHANDLE zms_initialize(int remfd)
{
  FAR struct zms_state_s *pzms;
  FAR struct zm_state_s *pzm;
  ssize_t nwritten;
  int ret;

  DEBUGASSERT(remfd >= 0);

  /* Allocate the instance */

  pzms = (FAR struct zms_state_s *)zalloc(sizeof(struct zms_state_s));
  if (pzms)
    {
      pzm            = &pzms->cmn;
      pzm->evtable   = g_zms_evtable;
      pzm->state     = ZMS_START;
      pzm->pstate    = PSTATE_IDLE;
      pzm->psubstate = PIDLE_ZPAD;
      pzm->remfd     = remfd;

      /* Create a timer to handle timeout events */

      ret = zm_timerinit(pzm);
      if (ret < 0)
        {
          zmdbg("ERROR: zm_timerinit failed: %d\n", ret);
          goto errout;
        }

      /* Send "rz\r" to the remote end.
       *
       * Paragraph 8.1: "The sending program may send the string "rz\r" to
       * invoke the receiving program from a possible command mode.  The
       * "rz" followed by carriage return activates a ZMODEM receive program
       * or command if it were not already active.
       */

      nwritten = zm_remwrite(pzm->remfd, (uint8_t *) "rz\r", 3);
      if (nwritten < 0)
        {
          zmdbg("ERROR: zm_remwrite failed: %d\n", (int)nwritten);
          goto errout_with_timer;
        }

      /* Send ZRQINIT
       *
       * Paragraph 8.1: "Then the sender may send a ZRQINIT header.  The
       * ZRQINIT header causes a previously started receive program to send
       * its ZRINIT header without delay."
       */

      ret = zm_sendhexhdr(&pzms->cmn, ZRQINIT, g_zeroes);
      if (ret < 0)
        {
          zmdbg("ERROR: zm_sendhexhdr failed: %d\n", ret);
          goto errout_with_timer;
        }

      /* Set up a timeout for the response */

      pzm->timeout = CONFIG_SYSTEM_ZMODEM_RESPTIME;
      zmdbg("ZMS_STATE %d sent ZRQINIT\n", pzm->state);

      /* Now drive the state machine by reading data from the remote peer
       * and providing that data to the parser.  zm_datapump runs until an
       * irrecoverable error is detected or until the ZRQINIT is ACK-ed by
       * the remote receiver.
       */

      ret = zm_datapump(&pzms->cmn);
      if (ret < 0)
        {
          zmdbg("ERROR: zm_datapump failed: %d\n", ret);
          goto errout_with_timer;
        }
    }

  return (ZMSHANDLE)pzms;

errout_with_timer:
  zm_timerrelease(&pzms->cmn);
errout:
  free(pzms);
  return (ZMSHANDLE)NULL;
}

/****************************************************************************
 * Name: zms_send
 *
 * Description:
 *   Send a file.
 *
 * Input Parameters:
 *   handle - Handle previoulsy returned by xms_initialize()
 *   filename - The name of the local file to transfer
 *   rfilename - The name of the remote file name to create
 *   option - Describes optional transfer behavior
 *   f1 - The F1 transfer flags
 *   skip - True:  Skip if file not present at receiving end.
 *
 * Assumptions:
 *   The filename and rfilename pointers refer to memory that will persist
 *   at least until the transfer completes.
 *
 ****************************************************************************/

int zms_send(ZMSHANDLE handle, FAR const char *filename,
             FAR const char *rfilename, enum zm_xfertype_e xfertype,
             enum zm_option_e option,  bool skip)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)handle;
  uint8_t f1;
  int ret;

  /* At this point either (1) zms_intiialize() has just been called or
   * (2) the previous file has been sent.
   */

  DEBUGASSERT(pzms && filename && rfilename && pzms->cmn.state == ZMS_DONE);

  /* Set the ZMSKNOLOC option is so requested */

  f1 = option;
  if (skip)
    {
      f1 |= ZMSKNOLOC;
    }

  /* Initiate sending of the file.  This will open the source file,
   * initialize data structures and set the ZMSS_FILEWAIT state.
   */

  ret = zms_sendfile(pzms, filename, rfilename, (uint8_t)xfertype, f1);
  if (ret < 0)
    {
      zmdbg("ERROR: zms_sendfile failed: %d\n", ret);
      return ret;
    }

  /* Now drive the state machine by reading data from the remote peer
   * and providing that data to the parser.  zm_datapump runs until an
   * irrecoverable error is detected or until the file is sent correctly.
   */

  return zm_datapump(&pzms->cmn);
}

/****************************************************************************
 * Name: zms_release
 *
 * Description:
 *   Called by the user when there are no more files to send.
 *
 ****************************************************************************/

int zms_release(ZMSHANDLE handle)
{
  FAR struct zms_state_s *pzms = (FAR struct zms_state_s *)handle;
  ssize_t nwritten;
  int ret = OK;

  /* Send "OO" */

  nwritten = zm_remwrite(pzms->cmn.remfd, (FAR const uint8_t *)"OO", 2);
  if (nwritten < 0)
    {
      zmdbg("ERROR: zm_remwrite failed: %d\n", (int)nwritten);
      ret = (int)nwritten;
    }

  /* Release the timer resources */

  zm_timerrelease(&pzms->cmn);

  /* Make sure that the file is closed */

  if (pzms->infd)
    {
      close(pzms->infd);
    }

  /* And free the Zmodem state structure */

  free(pzms);
  return ret;
}
