/****************************************************************************
 * apps/industry/nxmodbus/transport/nxmb_lrc.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/compiler.h>

#include <stdint.h>

#include "nxmb_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxmb_lrc
 *
 * Description:
 *   Calculate Modbus ASCII LRC checksum.
 *
 * Input Parameters:
 *   buf - Pointer to data buffer
 *   len - Length of data in bytes
 *
 * Returned Value:
 *   LRC value (8-bit)
 *
 ****************************************************************************/

uint8_t nxmb_lrc(FAR const uint8_t *buf, uint16_t len)
{
  uint8_t  lrc = 0;
  uint16_t i;

  if (buf == NULL || len == 0)
    {
      return lrc;
    }

  for (i = 0; i < len; i++)
    {
      lrc += buf[i];
    }

  return (uint8_t)(-((int8_t)lrc));
}
