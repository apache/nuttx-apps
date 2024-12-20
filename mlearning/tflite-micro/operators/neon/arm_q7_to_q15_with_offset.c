/****************************************************************************
 * apps/mlearning/tflite-micro/operators/neon/arm_q7_to_q15_with_offset.c
 *
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2010-2023 Arm Limited and/or its affiliates
 * <open-source-office@arm.com>
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arm_neon.h>
#include "arm_nnsupportfunctions.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void arm_q7_to_q15_with_offset(const int8_t *src,
                               int16_t *dst,
                               int32_t block_size,
                               int16_t offset)
{
  int32_t block_cnt;

  block_cnt = block_size / 8;
  int16x8_t offset_s16 = vdupq_n_s16(offset);
  while (block_cnt)
    {
      int8x8_t src_s8 = vld1_s8(src);
      int16x8_t src_s16 = vmovl_s8(src_s8);
      src += 8;
      src_s16 = vaddq_s16(offset_s16, src_s16);
      block_cnt--;
      vst1q_s16(dst, src_s16);
      dst += 8;
    }

  block_cnt = block_size % 8;
  while (block_cnt > 0)
    {
      *dst++ = (int16_t)*src++ + offset;

      /* Decrement the loop counter */

      block_cnt--;
    }
}
