/****************************************************************************
 * apps/examples/watcher/ramdisk.h
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

#ifndef __EXAMPLES_WATCHER_RAMDISK_H
#define __EXAMPLES_WATCHER_RAMDISK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NSECTORS             64
#define SECTORSIZE           512
#define MINOR                0
#define STR_MACRO(m)         #m
#define MKMOUNT_DEVNAME(m)   "/dev/ram" STR_MACRO(m)
#define MOUNT_DEVNAME        MKMOUNT_DEVNAME(MINOR)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int create_ramdisk(void);
int prepare_fs(void);

#endif /* __EXAMPLES_WATCHER_RAMDISK_H */
