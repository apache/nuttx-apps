/****************************************************************************
 * apps/testing/ltp/include/linux/loop.h
 *
 * SPDX-License-Identifier: Apache-2.0
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

#ifndef _LTP_LINUX_LOOP_H
#define _LTP_LINUX_LOOP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/fs/loop.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_DEV_LOOP

#define LOOP_SET_FD       0x4C00
#define LOOP_CLR_FD       0x4C01
#define LOOP_SET_STATUS   0x4C02
#define LOOP_GET_STATUS   0x4C03
#define LOOP_SET_STATUS64 0x4C04
#define LOOP_GET_STATUS64 0x4C05
#define LOOP_CHANGE_FD    0x4C06

#define LO_NAME_SIZE      64
#define LO_KEY_SIZE       32

#endif

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

#ifdef CONFIG_DEV_LOOP

struct loop_info
{
  int                lo_number;                   /* ioctl r/o */
  uint16_t           lo_device;                   /* ioctl r/o */
  unsigned long      lo_inode;                    /* ioctl r/o */
  uint16_t           lo_rdevice;                  /* ioctl r/o */
  int                lo_offset;
  int                lo_encrypt_key_size;         /* ioctl w/o */
  int                lo_flags;
  char               lo_name[LO_NAME_SIZE];
  unsigned char      lo_encrypt_key[LO_KEY_SIZE]; /* ioctl w/o */
  unsigned long      lo_init[2];
  char               reserved[4];
};

#endif

#endif /* _LTP_LINUX_LOOP_H */
