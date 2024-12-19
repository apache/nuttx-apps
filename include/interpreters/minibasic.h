/****************************************************************************
 * apps/include/interpreters/minibasic.h
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

/* This file was taken from Mini Basic, versino 1.0 developed by Malcolm
 * McLean, Leeds University.  Mini Basic version 1.0 was released the
 * Creative Commons Attribution license which, from my reading, appears to
 * be compatible with the NuttX BSD-style license:
 */

#ifndef __APPS_INCLUDE_INTERPRETERS_MINIBASIC_H
#define __APPS_INCLUDE_INTERPRETERS_MINIBASIC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: basic
 *
 * Description:
 *   Interpret a BASIC script
 *
 * Input Parameters:
 *   script - the script to run
 *   in     - input stream
 *   out    - output stream
 *   err    - error stream
 *
 * Returned Value:
 *   Returns: 0 on success, 1 on error condition.
 *
 ****************************************************************************/

int basic(FAR const char *script, FILE *in, FILE *out, FILE *err);

#endif
