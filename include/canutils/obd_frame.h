/****************************************************************************
 * apps/include/canutils/obd_frame.h
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

#ifndef __APPS_INCLUDE_CANUTILS_OBD_FRAME_H
#define __APPS_INCLUDE_CANUTILS_OBD_FRAME_H

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

#define OBD_SF_DATA_LEN(x)       (x & 0xf) /* Data Length of Single Frame */

/* First Frame fields */

#define OBD_FF_DATA_LEN_D0(x)    ((x & 0xf) << 8) /* Data Length of First Frame D0 */
#define OBD_FF_DATA_LEN_D1(x)    (x & 0xff)       /* Data Length of First Frame D1 */

/* Consecutive Frame fields */

#define OBD_CF_SEQ_NUM(x)        (x & 0xf) /* Consecutive Sequence Number */

/* Flow Control Frame fields */

#define OBD_FC_FLOW_STATUS(x)    (x & 0xf) /* Flow Control Status */

#endif /* __APPS_INCLUDE_CANUTILS_OBD_FRAME_H */
