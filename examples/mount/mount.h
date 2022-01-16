/****************************************************************************
 * apps/examples/mount/mount.h
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

#ifndef __APPS_EXAMPLES_MOUNT_MOUNT_H
#define __APPS_EXAMPLES_MOUNT_MOUNT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configure the test */

#if defined(CONFIG_EXAMPLES_MOUNT_DEVNAME)
#  undef  CONFIG_EXAMPLES_MOUNT_NSECTORS
#  undef  CONFIG_EXAMPLES_MOUNT_SECTORSIZE
#  undef  CONFIG_EXAMPLES_MOUNT_RAMDEVNO
#  define MOUNT_DEVNAME CONFIG_EXAMPLES_MOUNT_DEVNAME
#else
#  if !defined(CONFIG_FS_FAT)
#    error "CONFIG_FS_FAT required in this configuration"
#  endif
#  if !defined(CONFIG_EXAMPLES_MOUNT_SECTORSIZE)
#    define CONFIG_EXAMPLES_MOUNT_SECTORSIZE 512
#  endif
#  if !defined(CONFIG_EXAMPLES_MOUNT_NSECTORS)
#    define CONFIG_EXAMPLES_MOUNT_NSECTORS   2048
#  endif
#  if !defined(CONFIG_EXAMPLES_MOUNT_RAMDEVNO)
#     define CONFIG_EXAMPLES_MOUNT_RAMDEVNO  0
#  endif
#  define STR_RAMDEVNO(m)    #m
#  define MKMOUNT_DEVNAME(m) "/dev/ram" STR_RAMDEVNO(m)
#  define MOUNT_DEVNAME      MKMOUNT_DEVNAME(CONFIG_EXAMPLES_MOUNT_RAMDEVNO)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern const char g_source[]; /* Mount 'source' path */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_MOUNT_DEVNAME
extern int create_ramdisk(void);
#endif

#endif /* __APPS_EXAMPLES_MOUNT_MOUNT_H */
