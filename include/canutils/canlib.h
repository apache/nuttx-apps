/****************************************************************************
 * apps/include/canutils/canlib.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
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
 *   Wrapper for CANIOC_SET_CONNMODES.
 *   When loopback mode is enabled, the CAN peripheral transmits on the bus,
 *   but only receives its own sent messages.
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
