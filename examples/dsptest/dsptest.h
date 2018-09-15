/****************************************************************************
 * apps/examples/dsptest/dsptest.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Mateusz Szafoni <raiden00@railab.me>
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

#ifndef __APPS_EXAMPLES_DSPTEST_DSPTEST_H
#define __APPS_EXAMPLES_DSPTEST_DSPTEST_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <float.h>

#include <dsp.h>

#include <testing/unity.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEST_SINCOS_DELTA       0.06
#define TEST_SINCOS2_DELTA      0.01
#define TEST_ATAN2_DELTA        0.015

#define TEST_SEPARATOR()                                                  \
  printf("\n========================================================\n"); \
  printf("-> %s\n", __FILE__);                                            \
  printf("========================================================\n\n");

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* test_misc.c **************************************************************/

void test_misc(void);

/* test_pid.c ***************************************************************/

void test_pid(void);

/* test_transform.c *********************************************************/

void test_transform(void);

/* test_foc.c ***************************************************************/

void test_foc(void);

/* test_svm.c ***************************************************************/

void test_svm(void);

/* test_observer.c **********************************************************/

void test_observer(void);

/* test_motor.c *************************************************************/

void test_motor(void);

#endif /* __APPS_EXAMPLES_DSPTEST_DSPTEST_H */
