/****************************************************************************
 * apps/testing/ltp/include/unistd.h
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

#ifndef _LTP_UNISTD_H
#define _LTP_UNISTD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include_next <unistd.h>

/* the following function declaration to handle
 * -Wimplicit-function-declaration build warnings
 */

int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);

#endif /* _LTP_UNISTD_H */
