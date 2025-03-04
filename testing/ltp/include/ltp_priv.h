/****************************************************************************
 * apps/testing/ltp/include/ltp_priv.h
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

#ifndef _LTP_LTP_PRIV_H
#define _LTP_LTP_PRIV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include_next "ltp_priv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The LTP kernel test cases are divided into two categories according to:
 * the execution method:
 * The first category is executed directly using the built-in main function,
 * and the second category is executed using the test framework defined in
 * tst_test.h.
 * According to the design of the LTP kernel test framework, these two types
 * of cases cannot be linked together. If they are linked together, when
 * the first type of case finishes execution, it will check whether the
 * tst_test variable exists. If it does, it will directly report an error,
 * prompting "executed from newlib!".
 * The implementation of this checking process is the NO_NEWLIB_ASSERT macro
 * located in the file "./include/old/ltp_priv.h".
 * However, in the implementation of NuttX, both types of cases are
 * integrated together. So when we execute the first type of case in NuttX,
 * we will find that an error "executed from newlib!" is reported when the
 * case finishes execution.
 * To avoid this kind of error that has nothing to do with the case itself,
 * we choose to directly define the NO_NEWLIB_ASSERT macro as empty to ensure
 * the successful execution of the first type of case in NuttX.
 */

#undef NO_NEWLIB_ASSERT
#define NO_NEWLIB_ASSERT(file, lineno)

#endif /* _LTP_LTP_PRIV_H */
