/****************************************************************************
 * apps/include/system/cle.h
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

#ifndef __APPS_INCLUDE_SYSTEM_CLE_H
#define __APPS_INCLUDE_SYSTEM_CLE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdio.h>

#ifdef CONFIG_SYSTEM_CLE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
 * Name: cle/cle_fd
 *
 * Description:
 *   EMACS-like command line editor.  This is actually more like readline
 *   than is the NuttX readline!
 *
 ****************************************************************************/

int cle_fd(FAR char *line, FAR const char *prompt, uint16_t linelen,
           int infd, int outfd);

#ifdef CONFIG_FILE_STREAM
int cle(FAR char *line, FAR const char *prompt, uint16_t linelen,
        FAR FILE *instream, FAR FILE *outstream);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_SYSTEM_CLE */
#endif /* __APPS_INCLUDE_SYSTEM_CLE_H */
