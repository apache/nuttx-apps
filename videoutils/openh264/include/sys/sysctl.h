/****************************************************************************
 * apps/videoutils/openh264/include/sys/sysctl.h
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

#ifndef __APPS_VIDEOUTILS_OPENH264_INCLUDE_SYS_SYSCTL_H
#define __APPS_VIDEOUTILS_OPENH264_INCLUDE_SYS_SYSCTL_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define sysctl(args) (-1)
#define sysctlbyname(name, oldp, oldlenp, newp, newlen) (-1)
#define sysctlnametomib(name, mibp, sizep) (-1)

#endif /* __APPS_VIDEOUTILS_OPENH264_INCLUDE_SYS_SYSCTL_H */
