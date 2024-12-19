/****************************************************************************
 * apps/include/fsutils/inifile.h
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

#ifndef __APPS_INCLUDE_FSUTILS_INIFILE_H
#define __APPS_INCLUDE_FSUTILS_INIFILE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef FAR void *INIHANDLE;

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name:  inifile_initialize
 *
 * Description:
 *   Initialize for access to the INI file 'inifile_name'
 *
 ****************************************************************************/

INIHANDLE inifile_initialize(FAR const char *inifile_name);

/****************************************************************************
 * Name:  inifile_uninitialize
 *
 * Description:
 *   Free resources commit to INI file parsing
 *
 ****************************************************************************/

void inifile_uninitialize(INIHANDLE handle);

/****************************************************************************
 * Name: inifile_read_string
 *
 * Description:
 *   Obtains the specified string value for the specified variable name
 *   within the specified section of the INI file.  The receiver of the
 *   value string should call inifile_free_string when it no longer needs
 *   the memory held by the value string.
 *
 ****************************************************************************/

FAR char *inifile_read_string(INIHANDLE handle,
                              FAR const char *section,
                              FAR const char *variable,
                              FAR const char *defvalue);

/****************************************************************************
 * Name:  inifile_read_integer
 *
 * Description:
 *   Obtains the specified integer value for the specified variable name
 *   within the specified section of the INI file
 *
 ****************************************************************************/

long inifile_read_integer(INIHANDLE handle,
                          FAR const char *section,
                          FAR const char *variable,
                          FAR long defvalue);

/****************************************************************************
 * Name:  inifile_free_string
 *
 * Description:
 *   Release resources allocated for the value string previously obtained
 *   from inifile_read_string.  The purpose of this inline function is to
 *   hide the memory allocator used by this implementation.
 *
 ****************************************************************************/

void inifile_free_string(FAR char *value);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_FSUTILS_INIFILE_H */
