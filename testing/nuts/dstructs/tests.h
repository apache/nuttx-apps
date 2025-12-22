/****************************************************************************
 * apps/testing/nuts/dstructs/tests.h
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

#ifndef __APPS_TESTING_NUTS_DSTRUCTS_H
#define __APPS_TESTING_NUTS_DSTRUCTS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "../nuts.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_TESTING_NUTS_DSTRUCTS_LIST
int nuts_dstructs_list(void);
#else
#define nuts_dstructs_list()
#endif /* CONFIG_TESTING_NUTS_DSTRUCTS_LIST */

#ifdef CONFIG_TESTING_NUTS_DSTRUCTS_CBUF
int nuts_dstructs_cbuf(void);
#else
#define nuts_dstructs_cbuf()
#endif /* CONFIG_TESTING_NUTS_DSTRUCTS_CBUF */

#ifdef CONFIG_TESTING_NUTS_DSTRUCTS_HMAP
int nuts_dstructs_hmap(void);
#else
#define nuts_dstructs_hmap()
#endif /* CONFIG_TESTING_NUTS_DSTRUCTS_HMAP */

#endif /* __APPS_TESTING_NUTS_DSTRUCTS_H */
