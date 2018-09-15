/****************************************************************************
 * include/canutils/obd_frame.h
 *
 *   Copyright (C) 2017 Alan Carvalho de Assis. All rights reserved.
 *   Author: Alan Carvalho de Assis <acassis@gmail.com>
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

#ifndef __APPS_INCLUDE_CANUTILS_OBD_FRAME_H
#define __APPS_INCLUDE_CANUTILS_OBD_FRAME_H 1

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Frame Type
 *
 * Bits 7-4 of CAN Data 0
 *
 */

#define OBD_FRAME_TYPE(x)        (x & 0xf0) /* Mask bits 4-7 */

#define OBD_SINGLE_FRAME         (0 << 4)  /* Single frame */
#define OBD_FIRST_FRAME          (1 << 4)  /* First frame */
#define OBD_CONSEC_FRAME         (2 << 4)  /* Consecutive frame */
#define OBD_FLWCTRL_FRAME        (3 << 4)  /* Flow control frame */

/* Single Frame fields */

#define OBD_SF_DATA_LEN(x)       (x & 0xf) /* Data Lenght of Single Frame */

/* First Frame fields */

#define OBD_FF_DATA_LEN_D0(x)    ((x & 0xf) << 8) /* Data Lenght of First Frame D0 */
#define OBD_FF_DATA_LEN_D1(x)    (x & 0xff)       /* Data Lenght of First Frame D1 */

/* Consecutive Frame fields */

#define OBD_CF_SEQ_NUM(x)        (x & 0xf) /* Consecutive Sequence Number */

/* Flow Control Frame fields */

#define OBD_FC_FLOW_STATUS(x)    (x & 0xf) /* Flow Control Status */

#endif /* __APPS_INCLUDE_CANUTILS_OBD_FRAME_H */
