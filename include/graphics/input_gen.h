/****************************************************************************
 * apps/include/graphics/input_gen.h
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

#ifndef __APPS_INCLUDE_GRAPHICS_INPUT_GEN_H
#define __APPS_INCLUDE_GRAPHICS_INPUT_GEN_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include <nuttx/input/buttons.h>
#include <nuttx/input/touchscreen.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define INPUT_GEN_DEV_NUM 4

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Device types */

typedef enum
{
  INPUT_GEN_DEV_NONE      = 0x00,
  INPUT_GEN_DEV_UTOUCH    = 0x01, /* Touchscreen */
  INPUT_GEN_DEV_UBUTTON   = 0x02, /* Button */
  INPUT_GEN_DEV_UMOUSE    = 0x04, /* Mouse */
  INPUT_GEN_DEV_UKEYBOARD = 0x08, /* Keyboard */
  INPUT_GEN_DEV_ALL       = INPUT_GEN_DEV_UTOUCH | INPUT_GEN_DEV_UBUTTON |
                            INPUT_GEN_DEV_UMOUSE | INPUT_GEN_DEV_UKEYBOARD
} input_gen_dev_t;

/* Input generator context */

typedef FAR struct input_gen_ctx_s *input_gen_ctx_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: input_gen_create
 *
 * Description:
 *   Create an input generator context.
 *
 * Input Parameters:
 *   ctx     - A pointer to the input generator context, or NULL if the
 *             context failed to be created.
 *   devices - Device types to be opened.
 *
 * Returned Value:
 *   Success device count.  On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int input_gen_create(FAR input_gen_ctx_t *ctx, uint32_t devices);

/****************************************************************************
 * Name: input_gen_destroy
 *
 * Description:
 *   Destroy an input generator context.
 *
 * Input Parameters:
 *   ctx - The input generator context to destroy.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_destroy(input_gen_ctx_t ctx);

/****************************************************************************
 * Name: input_gen_query_device
 *
 * Description:
 *   Query whether the device with the specified type is opened.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *
 * Returned Value:
 *   True if the device is opened, false otherwise.
 *
 ****************************************************************************/

bool input_gen_query_device(input_gen_ctx_t ctx, input_gen_dev_t device);

/****************************************************************************
 * Name: input_gen_reset_devices
 *
 * Description:
 *   Reset the device state with the specified type.
 *
 * Input Parameters:
 *   ctx     - The input generator context.
 *   devices - The device types to reset state.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_reset_devices(input_gen_ctx_t ctx, uint32_t devices);

/****************************************************************************
 * Name: input_gen_tap
 *
 * Description:
 *   Perform a tap operation.
 *
 * Input Parameters:
 *   ctx - The input generator context.
 *   x   - The x coordinate.
 *   y   - The y coordinate.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_tap(input_gen_ctx_t ctx, int16_t x, int16_t y);

/****************************************************************************
 * Name: input_gen_drag / input_gen_swipe
 *
 * Description:
 *   Perform a drag or swipe operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   x1       - The start x coordinate.
 *   y1       - The start y coordinate.
 *   x2       - The end x coordinate.
 *   y2       - The end y coordinate.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_drag(input_gen_ctx_t ctx, int16_t x1, int16_t y1,
                   int16_t x2, int16_t y2, uint32_t duration);
int input_gen_swipe(input_gen_ctx_t ctx, int16_t x1, int16_t y1,
                    int16_t x2, int16_t y2, uint32_t duration);

/****************************************************************************
 * Name: input_gen_pinch
 *
 * Description:
 *   Perform a pinch operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   x1_start - The start x coordinate of the first finger.
 *   y1_start - The start y coordinate of the first finger.
 *   x2_start - The start x coordinate of the second finger.
 *   y2_start - The start y coordinate of the second finger.
 *   x1_end   - The end x coordinate of the first finger.
 *   y1_end   - The end y coordinate of the first finger.
 *   x2_end   - The end x coordinate of the second finger.
 *   y2_end   - The end y coordinate of the second finger.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_pinch(input_gen_ctx_t ctx, int16_t x1_start, int16_t y1_start,
                    int16_t x2_start, int16_t y2_start, int16_t x1_end,
                    int16_t y1_end, int16_t x2_end, int16_t y2_end,
                    uint32_t duration);

/****************************************************************************
 * Name: input_gen_button_click / input_gen_button_longpress
 *
 * Description:
 *   Perform a button click or long press operation.
 *
 * Input Parameters:
 *   ctx      - The input generator context.
 *   mask     - The button mask.
 *   duration - The duration of the operation.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

int input_gen_button_click(input_gen_ctx_t ctx, btn_buttonset_t mask);
int input_gen_button_longpress(input_gen_ctx_t ctx, btn_buttonset_t mask,
                               uint32_t duration);

/****************************************************************************
 * Name: input_gen_mouse_wheel
 *
 * Description:
 *   Perform a mouse wheel operation.
 *
 * Input Parameters:
 *   ctx   - The input generator context.
 *   wheel - The wheel value.
 *
 * Returned Value:
 *   Zero (OK) is returned on success.  On failure, a negated errno value is
 *   returned.
 *
 ****************************************************************************/

#ifdef CONFIG_INPUT_MOUSE_WHEEL
int input_gen_mouse_wheel(input_gen_ctx_t ctx, int16_t wheel);
#else
#define input_gen_mouse_wheel(ctx, wheel) (-ENODEV)
#endif

/****************************************************************************
 * Name: input_gen_fill_point
 *
 * Description:
 *   Fill the touch point structure.
 *
 * Input Parameters:
 *   point - The touch point structure.
 *   x     - The x coordinate.
 *   y     - The y coordinate.
 *   flags - The TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP flag.
 *
 ****************************************************************************/

void input_gen_fill_point(FAR struct touch_point_s *point,
                          int16_t x, int16_t y, uint8_t flags);

/****************************************************************************
 * Name: input_gen_write_raw
 *
 * Description:
 *   Write raw data to the device.
 *
 * Input Parameters:
 *   ctx    - The input generator context.
 *   device - The device type.
 *   buf    - The buffer to write.
 *   nbytes - The number of bytes to write.
 *
 * Returned Value:
 *   The number of bytes written, or a negated errno value on failure.
 *
 ****************************************************************************/

ssize_t input_gen_write_raw(input_gen_ctx_t ctx, input_gen_dev_t device,
                            FAR const void *buf, size_t nbytes);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_GRAPHICS_INPUT_GEN_H */
