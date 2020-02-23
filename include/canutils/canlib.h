/****************************************************************************
 * apps/include/canutils/canlib.h
 * Various non-standard APIs to support canutils.  All non-standard and
 * intended only for internal use.
 *
 *   Copyright (C) 2016 Sebastien Lorquet Nutt. All rights reserved.
 *   Author: Sebastien Lorquet <sebastien@lorquet.fr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_INCLUDE_CANUTILS_CANLIB_H
#define __APPS_INCLUDE_CANUTILS_CANLIB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: canlib_setbaud
 *
 * Description:
 *   Wrapper for CANIOC_SET_BITTIMING
 *
 * Input Parameter:
 *   fd   - file descriptor of an opened can device
 *   baud - baud rate to use on the CAN bus
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_setbaud(int fd, int bauds);

/****************************************************************************
 * Name: canlib_getbaud
 *
 * Description:
 *   Wrapper for CANIOC_GET_BITTIMING
 *
 * Input Parameter:
 *   fd   - file descriptor of an opened can device
 *   baud - pointer to a buffer to store the current baud rate
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_getbaud(int fd, FAR int *bauds);

/****************************************************************************
 * Name: canlib_setloopback
 *
 * Description:
 *   Wrapper for CANIOC_SET_CONNMODES. When loopback mode is enabled, the CAN
 *   peripheral transmits on the bus, but only receives its own sent messages.
 *
 * Input Parameter:
 *   fd       - file descriptor of an opened can device
 *   loopback - whether to use loopback mode.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: canlib_getloopback
 *
 * Description:
 *   Wrapper for CANIOC_GET_CONNMODES.
 *
 * Input Parameter:
 *   fd       - file descriptor of an opened can device
 *   loopback - pointer to a buffer to store the current loopback mode state.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_setloopback(int fd, bool loop);

int canlib_getloopback(int fd, FAR bool *loop);

/****************************************************************************
 * Name: canlib_setsilent
 *
 * Description:
 *   Wrapper for CANIOC_SET_CONNMODES. When silent mode is enabled, the CAN
 *   peripheral never transmits on the bus, but receives all bus traffic.
 *
 * Input Parameter:
 *   fd       - file descriptor of an opened can device
 *   loopback - whether to use loopback mode.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_setsilent(int fd, bool silent);

/****************************************************************************
 * Name: canlib_getsilent
 *
 * Description:
 *   Wrapper for CANIOC_GET_CONNMODES.
 *
 * Input Parameter:
 *   fd       - file descriptor of an opened can device
 *   loopback - pointer to a buffer to store the current silent mode state.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  Otherwise -1 (ERROR)
 *   is returned with the errno variable set to indicate the
 *   nature of the error.
 *
 ****************************************************************************/

int canlib_getsilent(int fd, FAR bool *silent);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_CANUTILS_CANLIB_H */
