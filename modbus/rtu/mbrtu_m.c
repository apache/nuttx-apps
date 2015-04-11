/****************************************************************************
 * apps/modbus/rtu/mbrtu_m.c
 *
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2013 China Beijing Armink <armink.ztl@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdlib.h>
#include <string.h>

#include "port.h"

#include <apps/modbus/mb.h>
#include <apps/modbus/mb_m.h>
#include "mbrtu.h"
#include <apps/modbus/mbframe.h>

#include "mbcrc.h"

#include <apps/modbus/mbport.h>

#if defined(CONFIG_RTU_ASCII_MASTER)

/****************************************************************************
 * Included Files
 ****************************************************************************/

#define MB_SER_PDU_SIZE_MIN     4     /* Minimum size of a Modbus RTU
                                       * frame. */
#define MB_SER_PDU_SIZE_MAX     256   /* Maximum size of a Modbus RTU
                                       * frame. */
#define MB_SER_PDU_SIZE_CRC     2     /* Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0     /* Offset of slave address in
                                       * Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1     /* Offset of Modbus-PDU in Ser-PDU. */

/****************************************************************************
 * Private Type Definitions
 ****************************************************************************/

typedef enum
  {
    STATE_M_RX_INIT,            /* Receiver is in initial state. */
    STATE_M_RX_IDLE,            /* Receiver is in idle state. */
    STATE_M_RX_RCV,             /* Frame is being received. */
    STATE_M_RX_ERROR,           /* If the frame is invalid. */
  } eMBMasterRcvState;

typedef enum
  {
    STATE_M_TX_IDLE,            /* Transmitter is in idle state. */
    STATE_M_TX_XMIT,            /* Transmitter is in transfer state. */
    STATE_M_TX_XFWR,            /* Transmitter is in transfer finish and
                                 * wait receive state. */
  } eMBMasterSndState;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static volatile eMBMasterSndState eSndState;
static volatile eMBMasterRcvState eRcvState;

static volatile uint8_t ucMasterRTUSndBuf[MB_PDU_SIZE_MAX];
static volatile uint8_t ucMasterRTURcvBuf[MB_SER_PDU_SIZE_MAX];
static volatile uint16_t usMasterSendPDULength;

static volatile uint8_t *pucMasterSndBufferCur;
static volatile uint16_t usMasterSndBufferCount;

static volatile uint16_t usMasterRcvBufferPos;
static volatile bool xFrameIsBroadcast = false;

static volatile eMBMasterTimerMode eMasterCurTimerMode;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

eMBErrorCode eMBMasterRTUInit(uint8_t ucPort, speed_t ulBaudRate,
                              eMBParity eParity)
{
  eMBErrorCode eStatus = MB_ENOERR;
  speed_t usTimerT35_50us;

  ENTER_CRITICAL_SECTION();

  /* Modbus RTU uses 8 Databits. */

  if (xMBMasterPortSerialInit(ucPort, ulBaudRate, 8, eParity) != true)
    {
      eStatus = MB_EPORTERR;
    }
  else
    {
      /* If baudrate > 19200 then we should use the fixed timer values t35 =
       * 1750us. Otherwise t35 must be 3.5 times the character time.
       */

      if (ulBaudRate > 19200)
        {
          usTimerT35_50us = 35; /* 1800us. */
        }
      else
        {
          /* The timer reload value for a character is given by: ChTimeValue =
           * Ticks_per_1s / ( Baudrate / 11 ) = 11 * Ticks_per_1s / Baudrate =
           * 220000 / Baudrate The reload for t3.5 is 1.5 times this value and
           * similary for t3.5.
           */

          usTimerT35_50us = (7UL * 220000UL) / (2UL * ulBaudRate);
        }

      if (xMBMasterPortTimersInit((uint16_t) usTimerT35_50us) != true)
        {
          eStatus = MB_EPORTERR;
        }
    }

  EXIT_CRITICAL_SECTION();
  return eStatus;
}

void eMBMasterRTUStart(void)
{
  ENTER_CRITICAL_SECTION();

  /* Initially the receiver is in the state STATE_M_RX_INIT. we start the timer
   * and if no character is received within t3.5 we change to STATE_M_RX_IDLE.
   * This makes sure that we delay startup of the modbus protocol stack until
   * the bus is free.
   */

  eRcvState = STATE_M_RX_INIT;
  vMBMasterPortSerialEnable(true, false);
  vMBMasterPortTimersT35Enable();

  EXIT_CRITICAL_SECTION();
}

void eMBMasterRTUStop(void)
{
  ENTER_CRITICAL_SECTION();
  vMBMasterPortSerialEnable(false, false);
  vMBMasterPortTimersDisable();
  EXIT_CRITICAL_SECTION();
}

eMBErrorCode eMBMasterRTUReceive(uint8_t *pucRcvAddress, uint8_t **pucFrame,
                                 uint16_t *pusLength)
{
  eMBErrorCode eStatus = MB_ENOERR;

  ENTER_CRITICAL_SECTION();
  assert_param(usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX);

  /* Length and CRC check */

  if ((usMasterRcvBufferPos >= MB_SER_PDU_SIZE_MIN)
      && (usMBCRC16((uint8_t *) ucMasterRTURcvBuf, usMasterRcvBufferPos) == 0))
    {
      /* Save the address field. All frames are passed to the upper layed and
       * the decision if a frame is used is done there.
       */

      *pucRcvAddress = ucMasterRTURcvBuf[MB_SER_PDU_ADDR_OFF];

      /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus size of
       * address field and CRC checksum.
       */

      *pusLength =
        (uint16_t) (usMasterRcvBufferPos - MB_SER_PDU_PDU_OFF -
                    MB_SER_PDU_SIZE_CRC);

      /* Return the start of the Modbus PDU to the caller. */

      *pucFrame = (uint8_t *) & ucMasterRTURcvBuf[MB_SER_PDU_PDU_OFF];
    }
  else
    {
      eStatus = MB_EIO;
    }

  EXIT_CRITICAL_SECTION();
  return eStatus;
}

eMBErrorCode eMBMasterRTUSend(uint8_t ucSlaveAddress, const uint8_t *pucFrame,
                              uint16_t usLength)
{
  eMBErrorCode eStatus = MB_ENOERR;
  uint16_t usCRC16;

  if (ucSlaveAddress > CONFIG_MB_MASTER_TOTAL_SLAVE_NUM)
    {
      return MB_EINVAL;
    }

  ENTER_CRITICAL_SECTION();

  /* Check if the receiver is still in idle state. If not we where to slow with
   * processing the received frame and the master sent another frame on the
   * network. We have to abort sending the frame.
   */

  if (eRcvState == STATE_M_RX_IDLE)
    {
      /* First byte before the Modbus-PDU is the slave address. */

      pucMasterSndBufferCur = (uint8_t *) pucFrame - 1;
      usMasterSndBufferCount = 1;

      /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */

      pucMasterSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
      usMasterSndBufferCount += usLength;

      /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */

      usCRC16 =
        usMBCRC16((uint8_t *) pucMasterSndBufferCur, usMasterSndBufferCount);
      ucMasterRTUSndBuf[usMasterSndBufferCount++] = (uint8_t) (usCRC16 & 0xFF);
      ucMasterRTUSndBuf[usMasterSndBufferCount++] = (uint8_t) (usCRC16 >> 8);

      /* Activate the transmitter. */

      eSndState = STATE_M_TX_XMIT;
      vMBMasterPortSerialEnable(false, true);
    }
  else
    {
      eStatus = MB_EIO;
    }

  EXIT_CRITICAL_SECTION();
  return eStatus;
}

bool xMBMasterRTUReceiveFSM(void)
{
  bool xTaskNeedSwitch = false;
  uint8_t ucByte;

  assert_param((eSndState == STATE_M_TX_IDLE) ||
               (eSndState == STATE_M_TX_XFWR));

  /* Always read the character. */

  (void)xMBMasterPortSerialGetByte((CHAR *) & ucByte);

  switch (eRcvState)
    {
      /* If we have received a character in the init state we have to wait
       * until the frame is finished.
       */

    case STATE_M_RX_INIT:
      vMBMasterPortTimersT35Enable();
      break;

      /* In the error state we wait until all characters in the damaged frame
       * are transmitted.
       */

    case STATE_M_RX_ERROR:
      vMBMasterPortTimersT35Enable();
      break;

      /* In the idle state we wait for a new character. If a character is
       * received the t1.5 and t3.5 timers are started and the receiver is in
       * the state STATE_RX_RECEIVCE and disable early the timer of respond
       * timeout.
       */

    case STATE_M_RX_IDLE:
      /* In time of respond timeout,the receiver receive a frame. Disable timer
       * of respond timeout and change the transmitter state to idle.
       */

      vMBMasterPortTimersDisable();
      eSndState = STATE_M_TX_IDLE;

      usMasterRcvBufferPos = 0;
      ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;
      eRcvState = STATE_M_RX_RCV;

      /* Enable t3.5 timers. */

      vMBMasterPortTimersT35Enable();
      break;

      /* We are currently receiving a frame. Reset the timer after every
       * character received. If more than the maximum possible number of bytes
       * in a modbus frame is received the frame is ignored.
       */

    case STATE_M_RX_RCV:
      if (usMasterRcvBufferPos < MB_SER_PDU_SIZE_MAX)
        {
          ucMasterRTURcvBuf[usMasterRcvBufferPos++] = ucByte;
        }
      else
        {
          eRcvState = STATE_M_RX_ERROR;
        }

      vMBMasterPortTimersT35Enable();
      break;
    }

  return xTaskNeedSwitch;
}

bool xMBMasterRTUTransmitFSM(void)
{
  bool xNeedPoll = false;

  assert_param(eRcvState == STATE_M_RX_IDLE);

  switch (eSndState)
    {
      /* We should not get a transmitter event if the transmitter is in idle
       * state.
       */

    case STATE_M_TX_IDLE:
      /* enable receiver/disable transmitter. */

      vMBMasterPortSerialEnable(true, false);
      break;

    case STATE_M_TX_XMIT:
      /* check if we are finished. */

      if (usMasterSndBufferCount != 0)
        {
          xMBMasterPortSerialPutByte((CHAR) * pucMasterSndBufferCur);
          pucMasterSndBufferCur++;      /* next byte in sendbuffer. */
          usMasterSndBufferCount--;
        }
      else
        {
          xFrameIsBroadcast =
            (ucMasterRTUSndBuf[MB_SER_PDU_ADDR_OFF] ==
             MB_ADDRESS_BROADCAST) ? true : false;

          /* Disable transmitter. This prevents another transmit buffer empty
           * interrupt.
           */

          vMBMasterPortSerialEnable(true, false);
          eSndState = STATE_M_TX_XFWR;

          /* If the frame is broadcast ,master will enable timer of convert
           * delay, else master will enable timer of respond timeout.
           */

          if (xFrameIsBroadcast == true)
            {
              vMBMasterPortTimersConvertDelayEnable();
            }
          else
            {
              vMBMasterPortTimersRespondTimeoutEnable();
            }
        }
      break;
    }

  return xNeedPoll;
}

bool xMBMasterRTUTimerExpired(void)
{
  bool xNeedPoll = false;

  switch (eRcvState)
    {
      /* Timer t35 expired. Startup phase is finished. */

    case STATE_M_RX_INIT:
      xNeedPoll = xMBMasterPortEventPost(EV_MASTER_READY);
      break;

      /* A frame was received and t35 expired. Notify the listener that a new
       * frame was received.
       */

    case STATE_M_RX_RCV:
      xNeedPoll = xMBMasterPortEventPost(EV_MASTER_FRAME_RECEIVED);
      break;

      /* An error occured while receiving the frame. */

    case STATE_M_RX_ERROR:
      vMBMasterSetErrorType(EV_ERROR_RECEIVE_DATA);
      xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
      break;

      /* Function called in an illegal state. */

    default:
      assert_param((eRcvState == STATE_M_RX_INIT) ||
                   (eRcvState == STATE_M_RX_RCV) ||
                   (eRcvState == STATE_M_RX_ERROR) ||
                   (eRcvState == STATE_M_RX_IDLE));
      break;
    }

  eRcvState = STATE_M_RX_IDLE;

  switch (eSndState)
    {
      /* A frame was send finish and convert delay or respond timeout expired.
       * If the frame is broadcast,The master will idle,and if the frame is not
       * broadcast.Notify the listener process error.
       */

    case STATE_M_TX_XFWR:
      if (xFrameIsBroadcast == false)
        {
          vMBMasterSetErrorType(EV_ERROR_RESPOND_TIMEOUT);
          xNeedPoll = xMBMasterPortEventPost(EV_MASTER_ERROR_PROCESS);
        }
      break;

      /* Function called in an illegal state. */

    default:
      assert_param((eSndState == STATE_M_TX_XFWR) ||
                   (eSndState == STATE_M_TX_IDLE));
      break;
    }

  eSndState = STATE_M_TX_IDLE;

  vMBMasterPortTimersDisable();

  /* If timer mode is convert delay, the master event then turns
   * EV_MASTER_EXECUTE status.
   */

  if (eMasterCurTimerMode == MB_TMODE_CONVERT_DELAY)
    {
      xNeedPoll = xMBMasterPortEventPost(EV_MASTER_EXECUTE);
    }

  return xNeedPoll;
}

/* Get Modbus Master send RTU's buffer address pointer.*/

void vMBMasterGetRTUSndBuf(uint8_t **pucFrame)
{
  *pucFrame = (uint8_t *) ucMasterRTUSndBuf;
}

/* Get Modbus Master send PDU's buffer address pointer.*/

void vMBMasterGetPDUSndBuf(uint8_t **pucFrame)
{
  *pucFrame = (uint8_t *) & ucMasterRTUSndBuf[MB_SER_PDU_PDU_OFF];
}

/* Set Modbus Master send PDU's buffer length.*/

void vMBMasterSetPDUSndLength(uint16_t SendPDULength)
{
  usMasterSendPDULength = SendPDULength;
}

/* Get Modbus Master send PDU's buffer length.*/

uint16_t usMBMasterGetPDUSndLength(void)
{
  return usMasterSendPDULength;
}

/* Set Modbus Master current timer mode.*/

void vMBMasterSetCurTimerMode(eMBMasterTimerMode eMBTimerMode)
{
  eMasterCurTimerMode = eMBTimerMode;
}

/* The master request is broadcast? */

bool xMBMasterRequestIsBroadcast(void)
{
  return xFrameIsBroadcast;
}
#endif
