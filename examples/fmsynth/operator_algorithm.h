/****************************************************************************
 * apps/examples/fmsynth/operator_algorithm.h
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

#ifndef __APPS_EXAMPLES_FMSYNTH_OPERATOR_ALGORITHM_H
#define __APPS_EXAMPLES_FMSYNTH_OPERATOR_ALGORITHM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <audioutils/fmsynth.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

FAR fmsynth_op_t *fmsynthutil_algorithm0(void);
FAR fmsynth_op_t *fmsynthutil_algorithm1(void);
FAR fmsynth_op_t *fmsynthutil_algorithm2(void);
void FAR fmsynthutil_delete_ops(FAR fmsynth_op_t *op);

#endif  /* __APPS_EXAMPLES_FMSYNTH_OPERATOR_ALGORITHM_H */
