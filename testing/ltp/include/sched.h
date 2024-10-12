/****************************************************************************
 * apps/testing/ltp/include/sched.h
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

#ifndef _LTP_SCHED_H
#define _LTP_SCHED_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include_next <sched.h>

/* add unshare and chroot function declaration to handle
 * -Wimplicit-function-declaration build warnings
 */

int unshare(int flags);
int chroot(const char *path);

#endif /* _LTP_SCHED_H */
