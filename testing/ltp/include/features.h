/****************************************************************************
 * apps/testing/ltp/include/features.h
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

/* when we build target at sim platform, the sim build will reference the
 * "/usr/include/features.h" in the host system. The implementation of the
 * "/usr/include/features.h" file in the host system uses the "_FEATURES_H"
 * macro to prevent duplicate inclusions.
 * To avoid inclusion failures, we need to define a different macro to solve
 * this problem.
 */

#ifndef _LTP_FEATURES_H
#define _LTP_FEATURES_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#if __has_include_next("features.h")
#include_next <features.h>
#endif

#endif /* _LTP_FEATURES_H */
