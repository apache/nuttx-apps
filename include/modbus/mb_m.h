/****************************************************************************
 * apps/include/modbus/mb_m.h
 *
 * FreeModbus Library: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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

#ifndef __APPS_INCLUDE_MODBUS_MB_M_H
#define __APPS_INCLUDE_MODBUS_MB_M_H

/* This module defines the interface for the application. It contains
 * the basic functions and types required to use the Modbus Master protocol stack.
 * A typical application will want to call eMBMasterInit() first. If the device
 * is ready to answer network requests it must then call eMBEnable() to activate
 * the protocol stack. In the main loop the function eMBMasterPoll() must be called
 * periodically. The time interval between pooling depends on the configured
 * Modbus timeout. If an RTOS is available a separate task should be created
 * and the task should always call the function eMBMasterPoll().
 *
 *   // Initialize protocol stack in RTU mode for a Master
 *
 *   eMBMasterInit(MB_RTU, 38400, MB_PAR_EVEN);
 *
 *   // Enable the Modbus Protocol Stack.
 *
 *   eMBMasterEnable();
 *   for(;;)
 *   {
 *     // Call the main polling loop of the Modbus Master protocol stack.
 *
 *     eMBMasterPoll();
 *     ...
 *   }
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Use the default Modbus Master TCP port (502) */

#define MB_MASTER_TCP_PORT_USE_DEFAULT 0

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Error codes used by all function in the Master request. */

typedef enum
{
    MB_MRE_NO_ERR,                  /* no error. */
    MB_MRE_NO_REG,                  /* illegal register address. */
    MB_MRE_ILL_ARG,                 /* illegal argument. */
    MB_MRE_REV_DATA,                /* receive data error. */
    MB_MRE_TIMEDOUT,                /* timeout error occurred. */
    MB_MRE_MASTER_BUSY,             /* master is busy now. */
    MB_MRE_EXE_FUN                  /* execute function error. */
} eMBMasterReqErrCode;

/* TimerMode is Master 3 kind of Timer modes. */

typedef enum
{
    MB_TMODE_T35,                   /* Master receive frame T3.5 timeout. */
    MB_TMODE_RESPOND_TIMEOUT,       /* Master wait respond for slave. */
    MB_TMODE_CONVERT_DELAY          /* Master sent broadcast ,then delay sometime.*/
}eMBMasterTimerMode;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Description:
 *   Initialize the Modbus Master protocol stack.
 *
 *   This functions initializes the ASCII or RTU module and calls the
 *   init functions of the porting layer to prepare the hardware. Please
 *   note that the receiver is still disabled and no Modbus frames are
 *   processed until eMBMasterEnable() has been called.
 *
 * Input Parameters:
 *   eMode If ASCII or RTU mode should be used.
 *   ucPort The port to use. E.g. 1 for COM1 on windows. This value
 *     is platform dependent and some ports simply choose to ignore it.
 *   ulBaudRate The baudrate. E.g. 19200. Supported baudrates depend
 *     on the porting layer.
 *   eParity Parity used for serial transmission.
 *
 * Returned Value:
 *   If no error occurs the function returns eMBErrorCode::MB_ENOERR.
 *   The protocol is then in the disabled state and ready for activation
 *   by calling eMBMasterEnable(). Otherwise one of the following error codes
 *   is returned:
 *    - eMBErrorCode::MB_EPORTERR IF the porting layer returned an error.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterInit(eMBMode eMode, uint8_t ucPort, speed_t ulBaudRate,
                           eMBParity eParity);

/****************************************************************************
 * Description:
 *   Initialize the Modbus Master protocol stack for Modbus TCP.
 *
 *   This function initializes the Modbus TCP Module. Please note that
 *   frame processing is still disabled until eMBEnable() is called.
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
 *
 ****************************************************************************/

eMBErrorCode eMBMasterTCPInit(uint16_t usTCPPort);

/****************************************************************************
 * Description:
 *   Release resources used by the protocol stack.
 *
 *   This function disables the Modbus Master protocol stack and release all
 *   hardware resources. It must only be called when the protocol stack
 *   is disabled.
 *
 *   Note that all ports implement this function. A port which wants to
 *   get an callback must define the macro MB_PORT_HAS_CLOSE to 1.
 *
 * Returned Value:
 *   If the resources where released it return eMBErrorCode::MB_ENOERR.
 *   If the protocol stack is not in the disabled state it returns
 *   eMBErrorCode::MB_EILLSTATE.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterClose(void);

/****************************************************************************
 * Description:
 *   Enable the Modbus Master protocol stack.
 *
 *   This function enables processing of Modbus Master frames. Enabling the
 *   protocol stack is only possible if it is in the disabled state.
 *
 * Returned Value:
 *   If the protocol stack is now in the state enabled it returns
 *   eMBErrorCode::MB_ENOERR. If it was not in the disabled state it
 *   return eMBErrorCode::MB_EILLSTATE.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterEnable(void);

/****************************************************************************
 * Description:
 *   Disable the Modbus Master protocol stack.
 *
 *   This function disables processing of Modbus frames.
 *
 * Returned Value:
 *   If the protocol stack has been disabled it returns
 *   eMBErrorCode::MB_ENOERR. If it was not in the enabled state it returns
 *   eMBErrorCode::MB_EILLSTATE.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterDisable(void);

/****************************************************************************
 * Description:
 *   The main pooling loop of the Modbus Master protocol stack.
 *
 *   This function must be called periodically. The timer interval required
 *   is given by the application dependent Modbus slave timeout. Internally
 *   the function calls xMBMasterPortEventGet() and waits for an event from
 *   the receiver or transmitter state machines.
 *
 * Returned Value:
 *   If the protocol stack is not in the enabled state the function
 *   returns eMBErrorCode::MB_EILLSTATE. Otherwise it returns
 *   eMBErrorCode::MB_ENOERR.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterPoll(void);

/****************************************************************************
 * Description:
 *   Registers a callback handler for a given function code.
 *
 *   This function registers a new callback handler for a given function
 *   code. The callback handler supplied is responsible for interpreting
 *   the Modbus PDU and the creation of an appropriate response. In case
 *   of an error it should return one of the possible Modbus exceptions
 *   which results in a Modbus exception frame sent by the protocol stack.
 *
 * Input Parameters:
 *   ucFunctionCode The Modbus function code for which this handler should
 *     be registers. Valid function codes are in the range 1 to 127.
 *   pxHandler The function handler which should be called in case
 *    such a frame is received. If NULL a previously registered function
 *    handler for this function code is removed.
 *
 * Returned Value:
 *   eMBErrorCode::MB_ENOERR if the handler has been installed. If no
 *   more resources are available it returns eMBErrorCode::MB_ENORES. In
 *   this case the values in mbconfig.h should be adjusted. If the argument
 *   was not valid it returns eMBErrorCode::MB_EINVAL.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegisterCB(uint8_t ucFunctionCode,
                                 pxMBFunctionHandler pxHandler);

/****************************************************************************
 * Description:
 *   Callback function used if the value of a Input Register is required by
 *   the protocol stack. The starting register address is given by usAddress
 *   and the last register is given by usAddress + usNRegs - 1.
 *
 *   The protocol stack does not internally allocate any memory for the
 *   registers. This makes the protocol stack very small and also usable on
 *   low end targets. In addition the values don't have to be in the memory
 *   and could for example be stored in a flash.
 *
 *   Whenever the protocol stack requires a value it calls one of the callback
 *   function with the register address and the number of registers to read
 *   as an argument. The application should then read the actual register values
 *   (for example the ADC voltage) and should store the result in the supplied
 *   buffer.
 *
 *   If the protocol stack wants to update a register value because a write
 *   register function was received a buffer with the new register values is
 *   passed to the callback function. The function should then use these values
 *   to update the application register values.
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
 *   - eMBErrorCode::MB_ENOREG If the application does not map an coils
 *       within the requested address range. In this case a
 *       ILLEGAL DATA ADDRESS is sent as a response.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegInputCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                                 uint16_t usNRegs);

/****************************************************************************
 * Description:
 *   Callback function used if a Holding Register value is
 *   read or written by the protocol stack. The starting register address
 *   is given by usAddress and the last register is given by
 *   usAddress + usNRegs - 1.
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
 *     the current values into the buffer pucRegBuffer.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application does not map an coils
 *       within the requested address range. In this case a
 *       ILLEGAL DATA ADDRESS is sent as a response.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegHoldingCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                                   uint16_t usNRegs, eMBRegisterMode eMode);

/****************************************************************************
 * Description:
 *   Callback function used if a Coil Register value is
 *   read or written by the protocol stack. If you are going to use
 *   this function you might use the functions xMBUtilSetBits() and
 *   xMBUtilGetBits() for working with bitfields.
 *
 * Input Parameters:
 *   pucRegBuffer The bits are packed in bytes where the first coil
 *     starting at address usAddress is stored in the LSB of the
 *     first byte in the buffer <code>pucRegBuffer</code>.
 *     If the buffer should be written by the callback function unused
 *     coil values (I.e. if not a multiple of eight coils is used) should
 *     be set to zero.
 *   usAddress The first coil number.
 *   usNCoils Number of coil values requested.
 *   eMode If eMBRegisterMode::MB_REG_WRITE the application values should
 *     be updated from the values supplied in the buffer pucRegBuffer.
 *     If eMBRegisterMode::MB_REG_READ the application should store the
 *     current values in the buffer pucRegBuffer.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application does not map an coils
 *       within the requested address range. In this case a
 *       ILLEGAL DATA ADDRESS is sent as a response.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegCoilsCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                                 uint16_t usNCoils, eMBRegisterMode eMode);

/****************************************************************************
 * Description:
 *   Callback function used if a Input Discrete Register value is
 *   read by the protocol stack.
 *
 *   If you are going to use his function you might use the functions
 *   xMBUtilSetBits() and xMBUtilGetBits() for working with bitfields.
 *
 * Input Parameters:
 *   pucRegBuffer The buffer should be updated with the current
 *     coil values. The first discrete input starting at usAddress must be
 *     stored at the LSB of the first byte in the buffer. If the requested number
 *     is not a multiple of eight the remaining bits should be set to zero.
 *   usAddress The starting address of the first discrete input.
 *   usNDiscrete Number of discrete input values.
 *
 * Returned Value:
 *   The function must return one of the following error codes:
 *   - eMBErrorCode::MB_ENOERR If no error occurred. In this case a normal
 *       Modbus response is sent.
 *   - eMBErrorCode::MB_ENOREG If the application does not map an coils
 *       within the requested address range. In this case a
 *       ILLEGAL DATA ADDRESS is sent as a response.
 *
 ****************************************************************************/

eMBErrorCode eMBMasterRegDiscreteCB(uint8_t *pucRegBuffer, uint16_t usAddress,
                                    uint16_t usNDiscrete);

/****************************************************************************
 * Description:
 *   These Modbus functions are called for user when Modbus run in Master
 *   Mode.
 *
 ****************************************************************************/

eMBMasterReqErrCode eMBMasterReqReadInputRegister(uint8_t ucSndAddr,
  uint16_t usRegAddr, uint16_t usNRegs, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqWriteHoldingRegister(uint8_t ucSndAddr,
  uint16_t usRegAddr, uint16_t usRegData, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqWriteMultipleHoldingRegister(uint8_t ucSndAddr,
  uint16_t usRegAddr, uint16_t usNRegs, uint16_t *pusDataBuffer,
  uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqReadHoldingRegister(uint8_t ucSndAddr,
  uint16_t usRegAddr, uint16_t usNRegs, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqReadWriteMultipleHoldingRegister(uint8_t ucSndAddr,
  uint16_t usReadRegAddr, uint16_t usNReadRegs, uint16_t *pusDataBuffer,
  uint16_t usWriteRegAddr, uint16_t usNWriteRegs, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqReadCoils(uint8_t ucSndAddr,
  uint16_t usCoilAddr, uint16_t usNCoils, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqWriteCoil(uint8_t ucSndAddr,
  uint16_t usCoilAddr, uint16_t usCoilData, uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqWriteMultipleCoils(uint8_t ucSndAddr,
  uint16_t usCoilAddr, uint16_t usNCoils, uint8_t *pucDataBuffer,
  uint32_t lTimeOut);
eMBMasterReqErrCode eMBMasterReqReadDiscreteInputs(uint8_t ucSndAddr,
  uint16_t usDiscreteAddr, uint16_t usNDiscreteIn, uint32_t lTimeOut);

eMBException eMBMasterFuncReportSlaveID(uint8_t *pucFrame, uint16_t *usLen);
eMBException eMBMasterFuncReadInputRegister(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncReadHoldingRegister(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncWriteHoldingRegister(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncWriteMultipleHoldingRegister(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncReadCoils(uint8_t *pucFrame, uint16_t *usLen);
eMBException eMBMasterFuncWriteCoil(uint8_t *pucFrame, uint16_t *usLen);
eMBException eMBMasterFuncWriteMultipleCoils(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncReadDiscreteInputs(uint8_t *pucFrame,
  uint16_t *usLen);
eMBException eMBMasterFuncReadWriteMultipleHoldingRegister(uint8_t *pucFrame,
  uint16_t *usLen);

/* These functions are interface for Modbus Master */

void vMBMasterGetPDUSndBuf(uint8_t **pucFrame);
uint8_t ucMBMasterGetDestAddress(void);
void vMBMasterSetDestAddress(uint8_t Address);
bool xMBMasterGetCBRunInMasterMode(void);
void vMBMasterSetCBRunInMasterMode(bool IsMasterMode);
uint16_t usMBMasterGetPDUSndLength(void);
void vMBMasterSetPDUSndLength(uint16_t SendPDULength);
void vMBMasterSetCurTimerMode(eMBMasterTimerMode eMBTimerMode);
bool xMBMasterRequestIsBroadcast(void);
eMBMasterErrorEventType eMBMasterGetErrorType(void);
void vMBMasterSetErrorType(eMBMasterErrorEventType errorType);
eMBMasterReqErrCode eMBMasterWaitRequestFinish(void);

#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_MODBUS_MB_M_H */

