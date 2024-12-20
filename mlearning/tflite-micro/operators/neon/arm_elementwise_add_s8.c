/****************************************************************************
 * apps/mlearning/tflite-micro/operators/neon/arm_elementwise_add_s8.c
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
#include "arm_nnfunctions.h"
#include "arm_nnsupportfunctions.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Note: __SHIFT is expected to be <=0 */

__STATIC_FORCEINLINE int32x4_t
arm_requantize_neon(const int32x4_t val,
                    const int32_t multiplier,
                    const int32_t shift)
{
  int32x4_t dividend = vqrdmulhq_n_s32(
      vshlq_s32(val, vdupq_n_s32(LEFT_SHIFT(shift))), multiplier);
  int32_t exponent = RIGHT_SHIFT(shift);
  int32x4_t shift__ = vdupq_n_s32(-exponent);
  int32x4_t fixup__ = vshrq_n_s32(vandq_s32(dividend, shift__), 31);
  int32x4_t fixed_up_dividend = vqaddq_s32(dividend, fixup__);
  return vrshlq_s32(fixed_up_dividend, shift__);
}

arm_cmsis_nn_status
arm_elementwise_add_s8(const int8_t *input_1_vect,
                       const int8_t *input_2_vect,
                       const int32_t input_1_offset,
                       const int32_t input_1_mult,
                       const int32_t input_1_shift,
                       const int32_t input_2_offset,
                       const int32_t input_2_mult,
                       const int32_t input_2_shift,
                       const int32_t left_shift,
                       int8_t *output,
                       const int32_t out_offset,
                       const int32_t out_mult,
                       const int32_t out_shift,
                       const int32_t out_activation_min,
                       const int32_t out_activation_max,
                       const int32_t block_size)
{
  int32_t loop_count = block_size / 8;
  const int8_t *input_1 = input_1_vect;
  const int8_t *input_2 = input_2_vect;
  int8_t *output_ = output;

  while (loop_count)
    {
      int8x8_t res;
      int8x8_t input_1_s8;
      int8x8_t input_2_s8;
      int16x8_t i1_val_16;
      int16x8_t input_1_s16;
      int16x8_t input_2_s16;
      int32x4_t input_1_s16_low;
      int32x4_t input_1_s16_high;
      int32x4_t input_2_s16_low;
      int32x4_t input_2_s16_high;

      input_1_s8 = vld1_s8(input_1);
      input_2_s8 = vld1_s8(input_2);
      input_1_s16 = vmovl_s8(input_1_s8);
      input_2_s16 = vmovl_s8(input_2_s8);
      input_1 += 8;
      input_2 += 8;

      input_1_s16_low  = vaddw_s16(
          vdupq_n_s32(input_1_offset), vget_low_s16(input_1_s16));
      input_1_s16_high = vaddw_s16(
          vdupq_n_s32(input_1_offset), vget_high_s16(input_1_s16));
      input_2_s16_low  = vaddw_s16(
          vdupq_n_s32(input_2_offset), vget_low_s16(input_2_s16));
      input_2_s16_high = vaddw_s16(
          vdupq_n_s32(input_2_offset), vget_high_s16(input_2_s16));

      input_1_s16_low  = vshlq_s32(
          input_1_s16_low, vdupq_n_s32(left_shift));
      input_2_s16_low  = vshlq_s32(
          input_2_s16_low, vdupq_n_s32(left_shift));
      input_1_s16_high = vshlq_s32(
          input_1_s16_high, vdupq_n_s32(left_shift));
      input_2_s16_high = vshlq_s32(
          input_2_s16_high, vdupq_n_s32(left_shift));

      input_1_s16_low  = arm_requantize_neon(
          input_1_s16_low, input_1_mult, input_1_shift);
      input_1_s16_high = arm_requantize_neon(
          input_1_s16_high, input_1_mult, input_1_shift);
      input_2_s16_low  = arm_requantize_neon(
          input_2_s16_low, input_2_mult, input_2_shift);
      input_2_s16_high = arm_requantize_neon(
          input_2_s16_high, input_2_mult, input_2_shift);

      input_1_s16_low  = vaddq_s32(
          input_1_s16_low, input_2_s16_low);
      input_1_s16_high = vaddq_s32(
          input_1_s16_high, input_2_s16_high);

      input_1_s16_low  = arm_requantize_neon(
          input_1_s16_low, out_mult, out_shift);
      input_1_s16_high = arm_requantize_neon(
          input_1_s16_high, out_mult, out_shift);

      input_1_s16_low  = vaddq_s32(
          input_1_s16_low, vdupq_n_s32(out_offset));
      input_1_s16_high = vaddq_s32(
          input_1_s16_high, vdupq_n_s32(out_offset));

      input_1_s16_low  = vmaxq_s32(
          input_1_s16_low, vdupq_n_s32(out_activation_min));
      input_1_s16_high = vmaxq_s32(
          input_1_s16_high, vdupq_n_s32(out_activation_min));
      input_1_s16_low  = vminq_s32(
          input_1_s16_low, vdupq_n_s32(out_activation_max));
      input_1_s16_high = vminq_s32(
          input_1_s16_high, vdupq_n_s32(out_activation_max));

      i1_val_16 = vcombine_s16(
          vmovn_s32(input_1_s16_low), vmovn_s32(input_1_s16_high));
      res = vmovn_s16(i1_val_16);

      vst1_s8(output_, res);
      output_ += 8;
      loop_count--;
    }

  loop_count = block_size % 8;
  while (loop_count)
    {
      int32_t a1 = (*input_1++ + input_1_offset) << left_shift;
      int32_t a2 = (*input_2++ + input_2_offset) << left_shift;
      a1 = arm_nn_requantize(a1, input_1_mult, input_1_shift);
      a2 = arm_nn_requantize(a2, input_2_mult, input_2_shift);

      int32_t sum = a1 + a2;
      sum = arm_nn_requantize(sum, out_mult, out_shift);
      sum += out_offset;

      sum = MAX(sum, out_activation_min);
      sum = MIN(sum, out_activation_max);
      *output_ = (int8_t) sum;
      loop_count--;
      output_++;
    }

  return (ARM_CMSIS_NN_SUCCESS);
}
