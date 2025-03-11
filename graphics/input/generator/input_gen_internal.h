/****************************************************************************
 * apps/graphics/input/generator/input_gen_internal.h
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

#ifndef __APPS_GRAPHICS_INPUT_GENERATOR_INPUT_GEN_INTERNAL_H
#define __APPS_GRAPHICS_INPUT_GENERATOR_INPUT_GEN_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/input/touchscreen.h>

#include <graphics/input_gen.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct input_gen_dev_s
{
  int fd;
  input_gen_dev_t device;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
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
 * Name: input_gen_search_dev
 *
 * Description:
 *   Get the input generator device by type.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *
 * Returned Value:
 *   A pointer to the input generator device structure, or NULL if not found.
 *
 ****************************************************************************/

FAR struct input_gen_dev_s *input_gen_search_dev(input_gen_ctx_t ctx,
                                                 input_gen_dev_t device);

/****************************************************************************
 * Name: input_gen_dev2path
 *
 * Description:
 *   Convert device type to device path.
 *
 * Input Parameters:
 *   device - Device type
 *
 * Returned Value:
 *   Device path string.
 *
 ****************************************************************************/

FAR const char *input_gen_dev2path(input_gen_dev_t device);

/****************************************************************************
 * Name: input_gen_utouch_write
 *
 * Description:
 *   Write touch sample to the device.
 *
 * Input Parameters:
 *   fd     - File descriptor of the device
 *   sample - Pointer to the touch sample structure
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_utouch_write(int fd, FAR const struct touch_sample_s *sample);

/****************************************************************************
 * Name: input_gen_ubutton_write
 *
 * Description:
 *   Write button state to the device.
 *
 * Input Parameters:
 *   fd   - File descriptor of the device
 *   mask - Button mask
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_ubutton_write(int fd, btn_buttonset_t mask);

/****************************************************************************
 * Name: input_gen_umouse_write
 *
 * Description:
 *   Write mouse wheel state to the device.
 *
 * Input Parameters:
 *   fd    - File descriptor of the device
 *   wheel - Mouse wheel value
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE_WHEEL
int input_gen_umouse_write(int fd, int16_t wheel);
#endif

/****************************************************************************
 * Name: input_gen_grab / input_gen_ungrab
 *
 * Description:
 *   Grab or ungrab the input device.
 *
 * Input Parameters:
 *   dev - The input generator device.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_grab(FAR struct input_gen_dev_s *dev);
int input_gen_ungrab(FAR struct input_gen_dev_s *dev);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_GRAPHICS_INPUT_GENERATOR_INPUT_GEN_INTERNAL_H */
