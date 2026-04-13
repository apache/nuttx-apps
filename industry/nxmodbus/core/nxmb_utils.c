/****************************************************************************
 * apps/industry/nxmodbus/core/nxmb_utils.c
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
 * Name: nxmb_util_set_bits
 *
 * Description:
 *   Set bits in a byte buffer.
 *
 * Input Parameters:
 *   buf        - Byte buffer
 *   bit_offset - Bit offset from start of buffer
 *   nbits      - Number of bits to set (max 8)
 *   value      - Value to set
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nxmb_util_set_bits(FAR uint8_t *buf, uint16_t bit_offset, uint8_t nbits,
                        uint8_t value)
{
  uint16_t word_buf;
  uint16_t mask;
  uint16_t byte_offset;
  uint16_t pre_bits;
  uint16_t val = value;

  byte_offset = bit_offset / 8;
  pre_bits    = bit_offset - (byte_offset * 8);

  val <<= pre_bits;
  mask = (uint16_t)((1 << nbits) - 1);
  mask <<= pre_bits;

  word_buf = buf[byte_offset];
  if (pre_bits + nbits > 8)
    {
      word_buf |= buf[byte_offset + 1] << 8;
    }

  word_buf = (word_buf & (~mask)) | val;

  buf[byte_offset] = (uint8_t)(word_buf & 0xff);
  if (pre_bits + nbits > 8)
    {
      buf[byte_offset + 1] = (uint8_t)(word_buf >> 8);
    }
}

/****************************************************************************
 * Name: nxmb_util_get_bits
 *
 * Description:
 *   Get bits from a byte buffer.
 *
 * Input Parameters:
 *   buf        - Byte buffer
 *   bit_offset - Bit offset from start of buffer
 *   nbits      - Number of bits to get (max 8)
 *
 * Returned Value:
 *   Bit value
 *
 ****************************************************************************/

uint8_t nxmb_util_get_bits(FAR const uint8_t *buf, uint16_t bit_offset,
                           uint8_t nbits)
{
  uint16_t word_buf;
  uint16_t mask;
  uint16_t byte_offset;
  uint16_t pre_bits;

  byte_offset = bit_offset / 8;
  pre_bits    = bit_offset - (byte_offset * 8);

  mask = (uint16_t)((1 << nbits) - 1);

  word_buf = buf[byte_offset];
  if (pre_bits + nbits > 8)
    {
      word_buf |= buf[byte_offset + 1] << 8;
    }

  word_buf >>= pre_bits;
  word_buf &= mask;

  return (uint8_t)word_buf;
}

