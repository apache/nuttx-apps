/****************************************************************************
 * apps/audioutils/fmsynth/test/opfunc_test.c
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include <audioutils/fmsynth_op.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FS  (48000)
#define DUMP_PERIOD  (FS * 3 / 2000)
#define ACCURACY_TEST_PERIOD  (FS * 3)

#define HZ  (4186)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static opfunc_t func_sin;
static opfunc_t func_tri;
static opfunc_t func_saw;
static opfunc_t func_sqa;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * name: prepare_opfuncs
 ****************************************************************************/

static void prepare_opfuncs(void)
{
  fmsynth_op_t *op;

  op = fmsynthop_create();

  fmsynthop_select_opfunc(op, FMSYNTH_OPFUNC_SIN);
  func_sin = op->wavegen;

  fmsynthop_select_opfunc(op, FMSYNTH_OPFUNC_TRIANGLE);
  func_tri = op->wavegen;

  fmsynthop_select_opfunc(op, FMSYNTH_OPFUNC_SAWTOOTH);
  func_saw = op->wavegen;

  fmsynthop_select_opfunc(op, FMSYNTH_OPFUNC_SQUARE);
  func_sqa = op->wavegen;

  fmsynthop_delete(op);
}

/****************************************************************************
 * name: wavegen_dump
 ****************************************************************************/

static void wavegen_dump(void)
{
  int t;
  float deltaact;

  deltaact = (float)FMSYNTH_PI * 2. * (float)HZ / (float)FS;

  printf("===== Wave generator Dump ====\n");
  printf("SIN, TRIANGLE, SAWTOOTH, SQUARE\n");
  for (t = 0; t < DUMP_PERIOD; t++)
    {
      printf("%d, %d, %d, %d\n",
              func_sin((int)(deltaact * t)),
              func_tri((int)(deltaact * t)),
              func_saw((int)(deltaact * t)),
              func_sqa((int)(deltaact * t))
      );
    }

  printf("\n");
}

/****************************************************************************
 * name: sin_accuracy_test
 ****************************************************************************/

static void sin_accuracy_test(void)
{
  int t;
  float delta;
  float deltaact;
  float max_diff = 0.f;
  float ref_sin;
  int sin_val;
  float norm_sin;
  float diff;

  delta  = M_PI * 2. * (float)HZ / (float)FS;
  deltaact = (float)FMSYNTH_PI * 2. * (float)HZ / (float)FS;

  printf("===== Local SIN function ACCURACY TEST ====\n");
  for (t = 0; t < ACCURACY_TEST_PERIOD; t++)
    {
      sin_val = func_sin((int)(deltaact * t));
      ref_sin = sinf(delta * t);

      norm_sin = (float)sin_val / (float)SHRT_MAX;
      printf("t=%d, operator-sin(%d)=%d, norm_sin=%f, sinf(%f)=%f ",
              t, (int)(deltaact * t), sin_val, norm_sin, delta * t, ref_sin);

      diff = fabsf(norm_sin - ref_sin);
      max_diff = max_diff < diff ? diff :  max_diff;

      if (diff >= 0.005)
        {
          printf("  BIG-DIFF  : %f\n", diff);
        }
      else
        {
          printf("\n");
        }
    }

  printf("\n\nMAX DIFF = %f\n\n", max_diff);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * name: main
 ****************************************************************************/

int main(void)
{
  prepare_opfuncs();
  sin_accuracy_test();
  wavegen_dump();

  return 0;
}
