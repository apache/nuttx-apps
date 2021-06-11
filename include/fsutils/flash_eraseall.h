/****************************************************************************
 * apps/include/fsutils/flash_eraseall.h
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

#ifndef __APPS_INCLUDE_FSUTILS_FLASH_ERASEALL_H
#define __APPS_INCLUDE_FSUTILS_FLASH_ERASEALL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: flash_eraseall
 *
 * Description:
 *   Call a block driver with the MDIOC_BULKERASE ioctl command.  This will
 *   cause the MTD driver to erase all of the flash.
 *
 ****************************************************************************/

int flash_eraseall(FAR const char *driver);

#endif /* __APPS_INCLUDE_FSUTILS_FLASH_ERASEALL_H */
