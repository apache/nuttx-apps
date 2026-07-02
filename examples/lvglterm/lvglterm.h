/****************************************************************************
 * apps/examples/lvglterm/lvglterm.h
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

#ifndef __APPS_EXAMPLES_LVGLTERM_LVGLTERM_H
#define __APPS_EXAMPLES_LVGLTERM_LVGLTERM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Read and Write ends of the NSH pipes */

#define READ_PIPE  0
#define WRITE_PIPE 1

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Shared state owned by the core (lvglterm.c).  The input variant writes the
 * shell input to g_nsh_stdin, adds its widgets under g_col (styled with
 * g_terminal_style) and may render into g_output.
 */

extern int g_nsh_stdin[2];          /* Input variant -> NSH stdin */
extern lv_obj_t *g_col;             /* Column container (widget parent) */
extern lv_obj_t *g_output;          /* NSH output text area */
extern lv_style_t g_terminal_style; /* Monospaced font style */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Shared helpers provided by the core */

bool lvglterm_has_input(int fd);
void lvglterm_add_output(FAR const char *buf, int len);

/* Provided by the selected input variant (lvglterm_touch.c or
 * lvglterm_kbd.c).  lvglterm_input_create() sets up the input source under
 * g_col and wires it to g_nsh_stdin.  lvglterm_input_poll() runs on every
 * LVGL timer tick (LVGL thread) for periodic work; it may be a no-op.
 */

void lvglterm_input_create(int argc, FAR char *argv[]);
void lvglterm_input_poll(void);

#endif /* __APPS_EXAMPLES_LVGLTERM_LVGLTERM_H */
