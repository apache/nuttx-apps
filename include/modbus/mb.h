/****************************************************************************
 * apps/include/modbus/mb.h
 *
 * FreeModbus Library: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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

#ifndef __APPS_INCLUDE_MODBUS_MB_H
#define __APPS_INCLUDE_MODBUS_MB_H

/* This module defines the interface for the application. It contains
 * the basic functions and types required to use the Modbus protocol stack.
 * A typical application will want to call eMBInit() first. If the device
 * is ready to answer network requests it must then call eMBEnable() to activate
 * the protocol stack. In the main loop the function eMBPoll() must be called
 * periodically. The time interval between pooling depends on the configured
 * Modbus timeout. If an RTOS is available a separate task should be created
 * and the task should always call the function eMBPoll().
 *
 *   // Initialize protocol stack in RTU mode for a slave with address 10 = 0x0A
 *
 *   eMBInit(MB_RTU, 0x0A, 38400, MB_PAR_EVEN);
 *
 *   // Enable the Modbus Protocol Stack.
 *
 *   eMBEnable();
 *   for(;;)
 *     {
 *       // Call the main polling loop of the Modbus protocol stack.
 *       eMBPoll();
 *       ...
 *     }
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <termios.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "mbport.h"
#include "mbproto.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


/* Use the default Modbus TCP port (502) */

#define MB_TCP_PORT_USE_DEFAULT 0

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Modbus serial transmission modes (RTU/ASCII).
 *
 * Modbus serial supports two transmission modes. Either ASCII or RTU. RTU
 * is faster but has more hardware requirements and requires a network with
 * a low jitter. ASCII is slower and more reliable on slower links (E.g. modems)
 */

typedef enum
{
  MB_RTU,                     /* RTU transmission mode. */
  MB_ASCII,                   /* ASCII transmission mode. */
  MB_TCP                      /* TCP mode. */
} eMBMode;

/* If register should be written or read.
 *
 * This value is passed to the callback functions which support either
 * reading or writing register values. Writing means that the application
 * registers should be updated and reading means that the modbus protocol
 * stack needs to know the current register values.
 *
 * See eMBRegHoldingCB(), eMBRegCoilsCB(), eMBRegDiscreteCB() and
 * eMBRegInputCB().
 */

typedef enum
{
  MB_REG_READ,                /* Read register values and pass to protocol stack. */
  MB_REG_WRITE                /* Update register values. */
} eMBRegisterMode;

/* Errorcodes used by all function in the protocol stack. */

typedef enum
{
  MB_ENOERR,                  /* no error. */
  MB_ENOREG,                  /* illegal register address. */
  MB_EINVAL,                  /* illegal argument. */
  MB_EPORTERR,                /* porting layer error. */
  MB_ENORES,                  /* insufficient resources. */
  MB_EIO,                     /* I/O error. */
  MB_EILLSTATE,               /* protocol stack in illegal state. */
  MB_ETIMEDOUT                /* timeout error occurred. */
} eMBErrorCode;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Initialize the Modbus protocol stack.
 *
 * This functions initializes the ASCII or RTU module and calls the
 * init functions of the porting layer to prepare the hardware. Please
 * note that the receiver is still disabled and no Modbus frames are
 * processed until eMBEnable() has been called.
 *
 * Input Parameters:
 *   eMode If ASCII or RTU mode should be used.
 *   ucSlaveAddress The slave address. Only frames sent to this
 *     address or to the broadcast address are processed.
 *   ucPort The port to use. E.g. 1 for COM1 on windows. This value
 *     is platform dependent and some ports simply choose to ignore it.
 *   ulBaudRate The baudrate. E.g. B19200. Supported baudrates depend
 *     on the porting layer.
 *   eParity Parity used for serial transmission.
 *
 * Returned Value:
 *   If no error occurs the function returns eMBErrorCode::MB_ENOERR.
 *   The protocol is then in the disabled state and ready for activation
 *   by calling eMBEnable(). Otherwise one of the following error codes
 *   is returned:
 *    - eMBErrorCode::MB_EINVAL If the slave address was not valid. Valid
 *        slave addresses are in the range 1 - 247.
 *    - eMBErrorCode::MB_EPORTERR IF the porting layer returned an error.
 */

eMBErrorCode eMBInit(eMBMode eMode, uint8_t ucSlaveAddress,
                     uint8_t ucPort, speed_t ulBaudRate, eMBParity eParity);

/* Initialize the Modbus protocol stack for Modbus TCP.
 *
 * This function initializes the Modbus TCP Module. Please note that
 * frame processing is still disabled until eMBEnable() is called.
 *
 * Input Parameters:
 *   usTCPPort The TCP port to listen on.
 *
 * Returned Value:
 *   If the protocol stack has been initialized correctly the function
 *   returns eMBErrorCode::MB_ENOERR. Otherwise one of the following error
 *   codes is returned:
 *    - eMBErrorCode::MB_EINVAL If the slave address was not valid. Valid
 *        slave addresses are in the range 1 - 247.
 *    - eMBErrorCode::MB_EPORTERR IF the porting layer returned an error.
 */

eMBErrorCode eMBTCPInit(uint16_t usTCPPort);

/* Release resources used by the protocol stack.
 *
 * This function disables the Modbus protocol stack and release all
 * hardware resources. It must only be called when the protocol stack
 * is disabled.
 *
 * Note all ports implement this function. A port which wants to
 * get an callback must define the macro CONFIG_MB_HAVE_CLOSE to 1.
 *
 * Returned Value:
 *   If the resources where released it return eMBErrorCode::MB_ENOERR.
 *   If the protocol stack is not in the disabled state it returns
 *   eMBErrorCode::MB_EILLSTATE.
 */

eMBErrorCode eMBClose(void);

/* Enable the Modbus protocol stack.
 *
 * This function enables processing of Modbus frames. Enabling the protocol
 * stack is only possible if it is in the disabled state.
 *
 * Returned Value:
 *   If the protocol stack is now in the state enabled it returns
 *   eMBErrorCode::MB_ENOERR. If it was not in the disabled state it
 *   return eMBErrorCode::MB_EILLSTATE.
 */

eMBErrorCode eMBEnable(void);

/* Disable the Modbus protocol stack.
 *
 * This function disables processing of Modbus frames.
 *
 * Returned Value:
 *   If the protocol stack has been disabled it returns
 *  eMBErrorCode::MB_ENOERR. If it was not in the enabled state it returns
 *  eMBErrorCode::MB_EILLSTATE.
 */

eMBErrorCode eMBDisable(void);

/* The main pooling loop of the Modbus protocol stack.
 *
 * This function must be called periodically. The timer interval required
 * is given by the application dependent Modbus slave timeout. Internally the
 * function calls xMBPortEventGet() and waits for an event from the receiver or
 * transmitter state machines.
 *
 * Returned Value:
 *   If the protocol stack is not in the enabled state the function
 *   returns eMBErrorCode::MB_EILLSTATE. Otherwise it returns
 *   eMBErrorCode::MB_ENOERR.
 */

eMBErrorCode eMBPoll(void);

/* Configure the slave id of the device.
 *
 * This function should be called when the Modbus function Report Slave ID
 * is enabled (By defining CONFIG_MB_FUNC_OTHER_REP_SLAVEID_ENABLED in .config).
 *
 * Input Parameters:
 *   ucSlaveID Values is returned in the Slave ID byte of the
 *     Report Slave ID response.
 *   xIsRunning If true the Run Indicator Status byte is set to 0xFF.
 *     otherwise the Run Indicator Status is 0x00.
 *   pucAdditional Values which should be returned in the Additional
 *     bytes of the  Report Slave ID response.
 *   usAdditionalLen Length of the buffer <code>pucAdditonal</code>.
 *
 * Returned Value:
 *   If the static buffer defined by CONFIG_MB_FUNC_OTHER_REP_SLAVEID_BUF
 *   is too small it returns eMBErrorCode::MB_ENORES. Otherwise
 *   it returns eMBErrorCode::MB_ENOERR.
 */

eMBErrorCode eMBSetSlaveID(uint8_t ucSlaveID, bool xIsRunning,
                           uint8_t const *pucAdditional,
                           uint16_t usAdditionalLen);

/* Registers a callback handler for a given function code.
 *
 * This function registers a new callback handler for a given function code.
 * The callback handler supplied is responsible for interpreting the Modbus PDU and
 * the creation of an appropriate response. In case of an error it should return
 * one of the possible Modbus exceptions which results in a Modbus exception frame
 * sent by the protocol stack.
 *
 * Input Parameters:
 *   ucFunctionCode The Modbus function code for which this handler should
 *     be registers. Valid function codes are in the range 1 to 127.
 *   pxHandler The function handler which should be called in case
 *     such a frame is received. If \c NULL a previously registered function handler
 *     for this function code is removed.
 *
 * Returned Value:
 *   eMBErrorCode::MB_ENOERR if the handler has been installed. If no
 *   more resources are available it returns eMBErrorCode::MB_ENORES. In this
 *   case the values in config.h should be adjusted. If the argument was not
 *   valid it returns eMBErrorCode::MB_EINVAL.
 */

eMBErrorCode eMBRegisterCB(uint8_t ucFunctionCode,
                           pxMBFunctionHandler pxHandler);

/* The protocol stack does not internally allocate any memory for the
 * registers. This makes the protocol stack very small and also usable on
 * low end targets. In addition the values don't have to be in the memory
 * and could for example be stored in a flash.<br>
 * Whenever the protocol stack requires a value it calls one of the callback
 * function with the register address and the number of registers to read
 * as an argument. The application should then read the actual register values
 * (for example the ADC voltage) and should store the result in the supplied
 * buffer.<br>
 * If the protocol stack wants to update a register value because a write
 * register function was received a buffer with the new register values is
 * passed to the callback function. The function should then use these values
 * to update the application register values.
 */

/* Callback function used if the value of a Input Register is required by
 * the protocol stack. The starting register address is given by \c
 * usAddress and the last register is given by usAddress + usNRegs - 1.
 *
 * Input Parameters:
 *   pucRegBuffer A buffer where the callback function should write
 *     the current value of the modbus registers to.
 *   usAddress The starting address of the register. Input registers
 *     are in the range 1 - 65535.
 *   usNRegs Number of registers the callback function must supply.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application can not supply values
 *       for registers within this range. In this case a
 *        ILLEGAL DATA ADDRESS  exception frame is sent as a response.
 *   - eMBErrorCode::MB_ETIMEDOUT If the requested register block is
 *       currently not available and the application dependent response
 *       timeout would be violated. In this case a  SLAVE DEVICE BUSY
 *       exception is sent as a response.
 *   - eMBErrorCode::MB_EIO If an unrecoverable error occurred. In this case
 *       a  SLAVE DEVICE FAILURE  exception is sent as a response.
 */

eMBErrorCode eMBRegInputCB(uint8_t * pucRegBuffer, uint16_t usAddress,
                           uint16_t usNRegs);

/* Callback function used if a Holding Register value is read or written by
 * the protocol stack. The starting register address is given by \c usAddress
 * and the last register is given by usAddress + usNRegs - 1.
 *
 * Input Parameters:
 *   pucRegBuffer If the application registers values should be updated the
 *     buffer points to the new registers values. If the protocol stack needs
 *     to now the current values the callback function should write them into
 *     this buffer.
 *   usAddress The starting address of the register.
 *   usNRegs Number of registers to read or write.
 *   eMode If eMBRegisterMode::MB_REG_WRITE the application register
 *     values should be updated from the values in the buffer. For example
 *     this would be the case when the Modbus master has issued an
 *     WRITE SINGLE REGISTER command.
 *     If the value eMBRegisterMode::MB_REG_READ the application should copy
 *     the current values into the buffer \c pucRegBuffer.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application can not supply values
 *       for registers within this range. In this case a
 *        ILLEGAL DATA ADDRESS  exception frame is sent as a response.
 *   - eMBErrorCode::MB_ETIMEDOUT If the requested register block is
 *       currently not available and the application dependent response
 *       timeout would be violated. In this case a  SLAVE DEVICE BUSY
 *       exception is sent as a response.
 *   - eMBErrorCode::MB_EIO If an unrecoverable error occurred. In this case
 *       a  SLAVE DEVICE FAILURE  exception is sent as a response.
 */

eMBErrorCode eMBRegHoldingCB(uint8_t * pucRegBuffer, uint16_t usAddress,
                             uint16_t usNRegs, eMBRegisterMode eMode);

/* Callback function used if a Coil Register value is read or written by the
 * protocol stack. If you are going to use this function you might use the
 * functions xMBUtilSetBits() and xMBUtilGetBits() for working with
 * bitfields.
 *
 * Input Parameters:
 *   pucRegBuffer The bits are packed in bytes where the first coil
 *     starting at address \c usAddress is stored in the LSB of the
 *     first byte in the buffer <code>pucRegBuffer</code>.
 *     If the buffer should be written by the callback function unused
 *     coil values (I.e. if not a multiple of eight coils is used) should be set
 *     to zero.
 *   usAddress The first coil number.
 *   usNCoils Number of coil values requested.
 *   eMode If eMBRegisterMode::MB_REG_WRITE the application values should
 *     be updated from the values supplied in the buffer \c pucRegBuffer.
 *     If eMBRegisterMode::MB_REG_READ the application should store the current
 *     values in the buffer \c pucRegBuffer.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application does not map an coils
 *       within the requested address range. In this case a
 *        ILLEGAL DATA ADDRESS  is sent as a response.
 *   - eMBErrorCode::MB_ETIMEDOUT If the requested register block is
 *       currently not available and the application dependent response
 *       timeout would be violated. In this case a  SLAVE DEVICE BUSY
 *       exception is sent as a response.
 *   - eMBErrorCode::MB_EIO If an unrecoverable error occurred. In this case
 *       a  SLAVE DEVICE FAILURE  exception is sent as a response.
 */

eMBErrorCode eMBRegCoilsCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                           uint16_t usNCoils, eMBRegisterMode eMode);

/* Callback function used if a Input Discrete Register value is read by
 * the protocol stack.
 *
 * If you are going to use his function you might use the functions
 * xMBUtilSetBits() and xMBUtilGetBits() for working with bitfields.
 *
 * Input Parameters:
 *   pucRegBuffer The buffer should be updated with the current
 *     coil values. The first discrete input starting at \c usAddress must be
 *     stored at the LSB of the first byte in the buffer. If the requested number
 *     is not a multiple of eight the remaining bits should be set to zero.
 *   usAddress The starting address of the first discrete input.
 *   usNDiscrete Number of discrete input values.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If no such discrete inputs exists.
 *       In this case a  ILLEGAL DATA ADDRESS  exception frame is sent
 *       as a response.
 *   - eMBErrorCode::MB_ETIMEDOUT If the requested register block is
 *       currently not available and the application dependent response
 *       timeout would be violated. In this case a  SLAVE DEVICE BUSY
 *       exception is sent as a response.
 *   - eMBErrorCode::MB_EIO If an unrecoverable error occurred. In this case
 *       a  SLAVE DEVICE FAILURE  exception is sent as a response.
 */

eMBErrorCode eMBRegDiscreteCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                              uint16_t usNDiscrete);

#ifdef __cplusplus
}
#endif
#endif
