/****************************************************************************
 * apps/modbus/nuttx/portserial_m.c
 *
 * FreeModbus Library: NuttX Modbus Master Port
 * Original work (c) 2006 Christian Walter <wolti@sil.at>
 * Modified work (c) 2016 Vytautas Lukenskas <lukevyta@gmail.com>
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <termios.h>

#include "port.h"

#include "modbus/mb.h"
#include "modbus/mb_m.h"
#include "modbus/mbport.h"

#if defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_MB_ASCII_ENABLED
#define BUF_SIZE    513         /* must hold a complete ASCII frame. */
#else
#define BUF_SIZE    256         /* must hold a complete RTU frame. */
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int      iSerialFd = -1;
static bool     bRxEnabled;
static bool     bTxEnabled;

static uint32_t ulTimeoutMs;
static uint8_t  ucBuffer[BUF_SIZE];
static int      uiRxBufferPos;
static int      uiTxBufferPos;

static struct termios xOldTIO;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static bool prvbMBMasterPortSerialRead(uint8_t *pucBuffer, uint16_t usNBytes,
                                       uint16_t *usNBytesRead);
static bool prvbMBMasterPortSerialWrite(uint8_t *pucBuffer,
                                        uint16_t usNBytes);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool prvbMBMasterPortSerialRead(uint8_t *pucBuffer, uint16_t usNBytes,
                                       uint16_t *usNBytesRead)
{
  bool            bResult = true;
  ssize_t         res;
  fd_set          rfds;
  struct timeval  tv;

  tv.tv_sec = 0;
  tv.tv_usec = 5000;
  FD_ZERO(&rfds);
  FD_SET(iSerialFd, &rfds);

  /* Wait until character received or timeout. Recover in case of an
   * interrupted read system call.
   */

  do
    {
      if (select(iSerialFd + 1, &rfds, NULL, NULL, &tv) == -1)
        {
          if (errno != EINTR)
            {
              bResult = false;
            }
        }
      else if (FD_ISSET(iSerialFd, &rfds))
        {
          if ((res = read(iSerialFd, pucBuffer, usNBytes)) == -1)
            {
              bResult = false;
            }
          else
            {
              *usNBytesRead = (uint16_t)res;
              break;
            }
        }
      else
        {
          *usNBytesRead = 0;
          break;
        }
    }
  while (bResult == true);

  return bResult;
}

static bool prvbMBMasterPortSerialWrite(uint8_t *pucBuffer,
                                        uint16_t usNBytes)
{
  ssize_t res;
  size_t  left = (size_t) usNBytes;
  size_t  done = 0;

  while (left > 0)
    {
      if ((res = write(iSerialFd, pucBuffer + done, left)) == -1)
        {
          if (errno != EINTR)
            {
              break;
            }

          /* call write again because of interrupted system call. */

          continue;
        }

      done += res;
      left -= res;
    }

  return left == 0 ? true : false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void vMBMasterPortSerialEnable(bool bEnableRx, bool bEnableTx)
{
  /* it is not allowed that both receiver and transmitter are enabled. */

  DEBUGASSERT(!bEnableRx || !bEnableTx);

  if (bEnableRx)
    {
      tcflush(iSerialFd, TCIFLUSH);
      uiRxBufferPos = 0;
      bRxEnabled = true;
    }
  else
    {
      bRxEnabled = false;
    }

  if (bEnableTx)
    {
      bTxEnabled = true;
      uiTxBufferPos = 0;
    }
  else
    {
      bTxEnabled = false;
    }
}

bool xMBMasterPortSerialInit(uint8_t ucPort, speed_t ulBaudRate,
                       uint8_t ucDataBits, eMBParity eParity)
{
  char szDevice[16];
  bool bStatus = true;
  struct termios xNewTIO;

  snprintf(szDevice, 16, "/dev/ttyS%d", ucPort);

  if ((iSerialFd = open(szDevice, O_RDWR | O_NOCTTY)) < 0)
    {
      vMBMasterPortLog(MB_LOG_ERROR, "SER-INIT",
                       "Can't open serial port %s: %d\n",
                       szDevice, errno);
      bStatus = false;
    }
  else if (tcgetattr(iSerialFd, &xOldTIO) != 0)
    {
      vMBMasterPortLog(MB_LOG_ERROR, "SER-INIT",
                       "Can't get settings from port %s: %d\n",
                       szDevice, errno);
    }
  else
    {
      memset(&xNewTIO, 0, sizeof(struct termios));

      xNewTIO.c_iflag |= IGNBRK | INPCK;
      xNewTIO.c_cflag |= CREAD | CLOCAL;

      switch (eParity)
        {
          case MB_PAR_NONE:
            break;

          case MB_PAR_EVEN:
            xNewTIO.c_cflag |= PARENB;
            break;

          case MB_PAR_ODD:
            xNewTIO.c_cflag |= PARENB | PARODD;
            break;

          default:
            bStatus = false;
        }

      switch (ucDataBits)
        {
          case 8:
            xNewTIO.c_cflag |= CS8;
            break;

          case 7:
            xNewTIO.c_cflag |= CS7;
            break;

          default:
            bStatus = false;
        }

      if (bStatus)
        {
          /* Set the new baud.  The following might be compatible with other
           * OSs for the following reason.
           *
           * (1) In NuttX, cfset[i|o]speed always return OK so failures will
           *     really only be reported when tcsetattr() is called.
           * (2) NuttX does not support separate input and output speeds so
           *     it is not necessary to call both cfsetispeed() and
           *     cfsetospeed(), and
           * (3) In NuttX, the input value to cfiset[i|o]speed is not
           *     encoded, but is the absolute baud value.  The following
           *     might not be
           */

          if (cfsetispeed(&xNewTIO, ulBaudRate) != 0 /* || cfsetospeed(&xNewTIO, ulBaudRate) != 0 */)
            {
              vMBMasterPortLog(MB_LOG_ERROR, "SER-INIT",
                               "Can't set baud rate %ld for port %s: %d\n",
                               ulBaudRate, szDevice, errno);
            }
          else if (tcsetattr(iSerialFd, TCSANOW, &xNewTIO) != 0)
            {
              vMBMasterPortLog(MB_LOG_ERROR, "SER-INIT",
                               "Can't set settings for port %s: %d\n",
                               szDevice, errno);
            }
          else
            {
              vMBMasterPortSerialEnable(false, false);
              bStatus = true;
            }
        }
    }

  return bStatus;
}

bool xMBMasterPortSerialSetTimeout(uint32_t ulNewTimeoutMs)
{
  if (ulNewTimeoutMs > 0)
    {
      ulTimeoutMs = ulNewTimeoutMs;
    }
  else
    {
      ulTimeoutMs = 1;
    }

  return true;
}

void vMBMasterPortClose(void)
{
  if (iSerialFd != -1)
    {
      tcsetattr(iSerialFd, TCSANOW, &xOldTIO);
      close(iSerialFd);
      iSerialFd = -1;
    }
}

bool xMBMasterPortSerialPoll(void)
{
  bool     bStatus = true;
  uint16_t usBytesRead;
  int      i;

  while (bRxEnabled)
    {
      if (prvbMBMasterPortSerialRead(&ucBuffer[0], BUF_SIZE, &usBytesRead))
        {
          if (usBytesRead == 0)
            {
              /* timeout with no bytes. */

              break;
            }
          else if (usBytesRead > 0)
            {
              for (i = 0; i < usBytesRead; i++)
                {
                  /* Call the modbus stack and let him fill the buffers. */

                  pxMBMasterFrameCBByteReceived();
                }

              uiRxBufferPos = 0;
            }
        }
      else
        {
          vMBMasterPortLog(MB_LOG_ERROR, "SER-POLL",
                           "read failed on serial device: %d\n",
                           errno);
          bStatus = false;
        }
    }

  if (bTxEnabled)
    {
      while (bTxEnabled)
        {
          pxMBMasterFrameCBTransmitterEmpty();

          /* Call the modbus stack to let him fill the buffer. */
        }

      if (!prvbMBMasterPortSerialWrite(&ucBuffer[0], uiTxBufferPos))
        {
          vMBMasterPortLog(MB_LOG_ERROR, "SER-POLL",
                           "write failed on serial device: %d\n",
                           errno);
          bStatus = false;
        }
    }

  return bStatus;
}

bool xMBMasterPortSerialPutByte(int8_t ucByte)
{
  DEBUGASSERT(uiTxBufferPos < BUF_SIZE);
  ucBuffer[uiTxBufferPos] = ucByte;
  uiTxBufferPos++;
  return true;
}

bool xMBMasterPortSerialGetByte(int8_t *pucByte)
{
  DEBUGASSERT(uiRxBufferPos < BUF_SIZE);
  *pucByte = ucBuffer[uiRxBufferPos];
  uiRxBufferPos++;
  return true;
}

#endif /* defined(CONFIG_MB_RTU_MASTER) || defined(CONFIG_MB_ASCII_MASTER) */
