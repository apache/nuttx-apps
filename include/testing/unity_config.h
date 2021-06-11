/****************************************************************************
 * apps/include/testing/unity_config.h
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

/* See Also: Unity/docs/UnityConfigurationGuide.pdf */

#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Exclude setjmp */

#ifdef CONFIG_TESTING_UNITY_EXCLUDE_SETJMP
#  define UNITY_EXCLUDE_SETJMP_H
#endif

/* Enable output coloring */

#ifdef CONFIG_TESTING_UNITY_OUTPUT_COLOR
#  define UNITY_OUTPUT_COLOR 1
#endif

#endif /* UNITY_CONFIG_H */
