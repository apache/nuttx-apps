/****************************************************************************
 * apps/mlearning/tflite-micro/operators/neon/arm_nn_mat_mult_kernel_s8_s16.c
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
 * Project:      CMSIS NN Library
 * Title:        arm_nn_mat_mult_kernel_s8_s16.c
 * Description:  Matrix-multiplication function for convolution
 *
 * $Date:        29 May 2023
 * $Revision:    V.2.0.0
 *
 * Target :  Arm(R) M-Profile Architecture
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <arm_neon.h>
#include "arm_nnfunctions.h"
#include "arm_nnsupportfunctions.h"

/* Matrix-multiplication function for convolution with per-channel
 * requantization. Refer header file for details.
 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int8_t *arm_nn_mat_mult_kernel_s8_s16(const int8_t *input_a,
                                      const int16_t *input_b,
                                      const uint16_t output_ch,
                                      const int32_t *out_shift,
                                      const int32_t *out_mult,
                                      const int32_t out_offset,
                                      const int16_t activation_min,
                                      const int16_t activation_max,
                                      const int32_t num_col_a,
                                      const int32_t aligned_num_col_a,
                                      const int32_t *const output_bias,
                                      int8_t *out_0)
{
  int8_t *out_1 = out_0 + output_ch;
  const int32_t *bias = output_bias;

  uint16_t row_count = output_ch / 4;
  const int8_t *ip_a0 = input_a;

  /* this loop over rows in A */

  while (row_count)
    {
      int32_t col_count = num_col_a / 8;
      const int16_t *ip_b0 = input_b;
      const int16_t *ip_b1 = ip_b0 + aligned_num_col_a;
      const int8_t *ip_a[4] =
        {
          ip_a0,
          ip_a0 + num_col_a,
          ip_a0 + 2 * num_col_a,
          ip_a0 + 3 * num_col_a
        };

      int32_t ch_out[4][2] =
        {
          0
        };

      int32x4_t res[8];

      for (int i = 0; i < 8; i++)
        {
          res[i] = vdupq_n_s32(0);
        }

      /* Init accumulator with bias for channel N and N + 1 */

      if (bias)
        {
          for (int i = 0; i < 4; i++)
            {
              ch_out[i][0] = *bias;
              ch_out[i][1] = *bias++;
            }
        }

      /* Each time eight int8 data of four filters and eight int16 data
       * of two inputs are read.First, the filter data is expanded to
       * int16, and then cross-multiplied to obtain eight
       * calculation results.
       */

      while (col_count)
        {
          int8x8_t filter_s8[4];
          int16x8_t input_s16[2];
          int16x8_t filter_s16[4];

          input_s16[0] = vld1q_s16(ip_b0);
          ip_b0 += 8;
          input_s16[1] = vld1q_s16(ip_b1);
          ip_b1 += 8;

          for (int i = 0; i < 4; i++)
            {
              filter_s8[i] = vld1_s8(ip_a[i]);
              ip_a[i] += 8;
              filter_s16[i] = vmovl_s8(filter_s8[i]);
              res[i * 2]     = vmlal_s16(res[i * 2],
                                         vget_low_s16(filter_s16[i]),
                                         vget_low_s16(input_s16[0]));
              res[i * 2 + 1] = vmlal_s16(res[i * 2 + 1],
                                         vget_low_s16(filter_s16[i]),
                                         vget_low_s16(input_s16[1]));
              res[i * 2]     = vmlal_s16(res[i * 2],
                                         vget_high_s16(filter_s16[i]),
                                         vget_high_s16(input_s16[0]));
              res[i * 2 + 1] = vmlal_s16(res[i * 2 + 1],
                                         vget_high_s16(filter_s16[i]),
                                         vget_high_s16(input_s16[1]));
            }

          col_count--;
        }

      for (int i = 0; i < 4; i++)
        {
          for (int j = 0; j < 2; j++)
            {
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 0);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 1);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 2);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 3);
            }
        }

      col_count = num_col_a % 8;
      while (col_count) /* while over col_count */
        {
          int16_t b0 = *ip_b0++;
          int16_t b1 = *ip_b1++;

          for (int i = 0; i < 4; i++)
            {
              int8_t input_remaining = *(ip_a[i]++);
              ch_out[i][0] += input_remaining * b0;
              ch_out[i][1] += input_remaining * b1;
            }

          col_count--;
        }

      for (int i = 0; i < 4; i++)
        {
          ch_out[i][0] = arm_nn_requantize(
              ch_out[i][0], *out_mult, *out_shift);
          ch_out[i][1] = arm_nn_requantize(
              ch_out[i][1], *out_mult, *out_shift);
          ch_out[i][0] += out_offset;
          ch_out[i][1] += out_offset;
          ch_out[i][0] = MAX(ch_out[i][0], activation_min);
          ch_out[i][1] = MAX(ch_out[i][1], activation_min);
          ch_out[i][0] = MIN(ch_out[i][0], activation_max);
          ch_out[i][1] = MIN(ch_out[i][1], activation_max);
          *out_0++ = (int8_t)ch_out[i][0];
          *out_1++ = (int8_t)ch_out[i][1];
          out_mult++;
          out_shift++;
        }

      /* skip row */

      ip_a0 = ip_a[3];
      row_count--;
    }

  row_count = output_ch % 4;
  if (row_count >= 2)
    {
      int32_t col_count = num_col_a / 8;
      const int8_t *ip_a1 = ip_a0 + num_col_a;
      const int16_t *ip_b0 = input_b;
      const int16_t *ip_b1 = ip_b0 + aligned_num_col_a;
      int32_t ch_out[2][2] =
        {
          0
        };

      int32x4_t res[4];

      /* Init accumulator with bias for channel N and N + 1 */

      if (bias)
        {
          for (int i = 0; i < 2; i++)
            {
              ch_out[i][0] = *bias;
              ch_out[i][1] = *bias++;
            }
        }

      for (int i = 0; i < 4; i++)
        {
          res[i] = vdupq_n_s32(0);
        }

      /* Each time eight int8 data of four filters and eight int16 data
       * of two inputs are read.First, the filter data is expanded to
       * int16, and then cross-multiplied to obtain 8 calculation results.
       */

      while (col_count)
        {
          int8x8_t filter_s8[2];
          int16x8_t input_s16[2];
          int16x8_t filter_s16[2];

          filter_s8[0] = vld1_s8(ip_a0);
          ip_a0 += 8;
          filter_s8[1] = vld1_s8(ip_a1);
          ip_a1 += 8;

          input_s16[0] = vld1q_s16(ip_b0);
          ip_b0 += 8;
          input_s16[1] = vld1q_s16(ip_b1);
          ip_b1 += 8;

          for (int i = 0; i < 2; i++)
            {
              filter_s16[i] = vmovl_s8(filter_s8[i]);
              res[i * 2]     = vmlal_s16(res[i * 2],
                                         vget_low_s16(filter_s16[i]),
                                         vget_low_s16(input_s16[0]));
              res[i * 2 + 1] = vmlal_s16(res[i * 2 + 1],
                                         vget_low_s16(filter_s16[i]),
                                         vget_low_s16(input_s16[1]));
              res[i * 2]     = vmlal_s16(res[i * 2],
                                         vget_high_s16(filter_s16[i]),
                                         vget_high_s16(input_s16[0]));
              res[i * 2 + 1] = vmlal_s16(res[i * 2 + 1],
                                         vget_high_s16(filter_s16[i]),
                                         vget_high_s16(input_s16[1]));
            }

          col_count--;
        }

      for (int i = 0; i < 2; i++)
        {
          for (int j = 0; j < 2; j++)
            {
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 0);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 1);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 2);
              ch_out[i][j] += vgetq_lane_s32(res[i * 2 + j], 3);
            }
        }

      col_count = num_col_a % 8;
      while (col_count) /* while over col_count */
        {
          int8_t a0 = *ip_a0++; /* filter */
          int8_t a1 = *ip_a1++;
          int16_t b0 = *ip_b0++; /* input */
          int16_t b1 = *ip_b1++;

          ch_out[0][0] += a0 * b0;
          ch_out[1][1] += a1 * b1;
          ch_out[1][0] += a1 * b0;
          ch_out[0][1] += a0 * b1;
          col_count--;
        }

      for (int i = 0; i < 2; i++)
        {
          ch_out[i][0] = arm_nn_requantize(
              ch_out[i][0], *out_mult, *out_shift);
          ch_out[i][1] = arm_nn_requantize(
              ch_out[i][1], *out_mult, *out_shift);
          ch_out[i][0] += out_offset;
          ch_out[i][1] += out_offset;
          ch_out[i][0] = MAX(ch_out[i][0], activation_min);
          ch_out[i][1] = MAX(ch_out[i][1], activation_min);
          ch_out[i][0] = MIN(ch_out[i][0], activation_max);
          ch_out[i][1] = MIN(ch_out[i][1], activation_max);
          *out_0++ = (int8_t)ch_out[i][0];
          *out_1++ = (int8_t)ch_out[i][1];
          out_mult++;
          out_shift++;
        }

      /* skip row */

      ip_a0 += num_col_a;
      row_count -= 2;
    }

  /* compute the last odd numbered row if any */

  if (output_ch & 0x1)
    {
      int32_t col_count = num_col_a / 8;
      const int16_t *ip_b0 = input_b;
      const int16_t *ip_b1 = ip_b0 + aligned_num_col_a;
      int32_t ch_out[2] =
        {
          0
        };

      int32x4_t res[2];

      /* load the bias */

      if (bias)
        {
          ch_out[0] = *bias;
          ch_out[1] = *bias++;
        }

      res[0] = vdupq_n_s32(0);
      res[1] = vdupq_n_s32(0);
      while (col_count)
        {
          int8x8_t filter_s8 = vld1_s8(ip_a0);
          int16x8_t filter_s16 = vmovl_s8(filter_s8);
          int16x8_t input_0_s16 = vld1q_s16(ip_b0);
          int16x8_t input_1_s16 = vld1q_s16(ip_b1);
          ip_a0 += 8;
          ip_b0 += 8;
          ip_b1 += 8;
          res[0] = vmlal_s16(res[0],
                             vget_low_s16(filter_s16),
                             vget_low_s16(input_0_s16));
          res[1] = vmlal_s16(res[1],
                             vget_low_s16(filter_s16),
                             vget_low_s16(input_1_s16));
          res[0] = vmlal_s16(res[0],
                             vget_high_s16(filter_s16),
                             vget_high_s16(input_0_s16));
          res[1] = vmlal_s16(res[1],
                             vget_high_s16(filter_s16),
                             vget_high_s16(input_1_s16));
          col_count--;
        }

      ch_out[0] += vgetq_lane_s32(res[0], 0);
      ch_out[0] += vgetq_lane_s32(res[0], 1);
      ch_out[0] += vgetq_lane_s32(res[0], 2);
      ch_out[0] += vgetq_lane_s32(res[0], 3);

      ch_out[1] += vgetq_lane_s32(res[1], 0);
      ch_out[1] += vgetq_lane_s32(res[1], 1);
      ch_out[1] += vgetq_lane_s32(res[1], 2);
      ch_out[1] += vgetq_lane_s32(res[1], 3);

      col_count = num_col_a % 8;
      while (col_count)
        {
          int8_t a0 = *ip_a0++;
          int16_t b0 = *ip_b0++;
          int16_t b1 = *ip_b1++;

          ch_out[0] += a0 * b0;
          ch_out[1] += a0 * b1;
          col_count--;
        }

      ch_out[0] = arm_nn_requantize(
          ch_out[0], *out_mult, *out_shift);
      ch_out[0] += out_offset;
      ch_out[0] = MAX(ch_out[0], activation_min);
      ch_out[0] = MIN(ch_out[0], activation_max);
      *out_0++ = (int8_t)ch_out[0];

      ch_out[1] = arm_nn_requantize(
          ch_out[1], *out_mult, *out_shift);
      ch_out[1] += out_offset;
      ch_out[1] = MAX(ch_out[1], activation_min);
      ch_out[1] = MIN(ch_out[1], activation_max);
      *out_1++ = (int8_t)ch_out[1];

      out_mult++;
      out_shift++;
    }

  out_0 += output_ch;

  /* return the new output pointer with offset */

  return out_0;
}
