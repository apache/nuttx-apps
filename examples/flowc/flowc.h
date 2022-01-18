/****************************************************************************
 * apps/examples/flowc/flowc.h
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

#ifndef __APPS_EXAMPLES_FLOWC_FLOWC_H
#define __APPS_EXAMPLES_FLOWC_FLOWC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifdef EXAMPLES_FLOWC_HOST
#else
# include <debug.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef EXAMPLES_FLOWC_HOST
#  define FAR
#endif

#define MAX_DEVNAME 64
#define SENDSIZE    (0x7f - 0x20)
#define NSENDS      1024
#define TOTALSIZE   (NSENDS * SENDSIZE)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int flowc_sender(int argc, char **argv);
int flowc_receiver(int argc, char **argv);

#endif /* __APPS_EXAMPLES_FLOWC_FLOWC_H */
