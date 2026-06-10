/****************************************************************************
 * apps/examples/webpanel/ws_terminal.h
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

#ifndef __APPS_EXAMPLES_WEBPANEL_WS_TERMINAL_H
#define __APPS_EXAMPLES_WEBPANEL_WS_TERMINAL_H

/****************************************************************************
 * Included Files
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
 * Name: ws_terminal_start
 *
 * Description:
 *   Start the websocket terminal daemon task.
 *
 * Input Parameters:
 *   None.
 *
 * Returned Value:
 *   Task PID on success; negated errno value on failure.
 *
 ****************************************************************************/

int ws_terminal_start(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_EXAMPLES_WEBPANEL_WS_TERMINAL_H */
