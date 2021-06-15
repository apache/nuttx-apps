/****************************************************************************
 * apps/examples/ft80x/ft80x.h
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

#ifndef __APPS_EXAMPLES_FT80X_FT80X_H
#define __APPS_EXAMPLES_FT80X_FT80X_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdio.h>
#include <debug.h>

#ifdef CONFIG_EXAMPLES_FT80X

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NOTE: These rely on internal definitions from compiler.h and debug.h.
 * Could be a porting issue.
 */

#if !defined(GRAPHICS_FT80X_DEBUG_ERROR)
#  define ft80x_err _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_err(format, ...) \
     fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_err printf
#endif

#if !defined(GRAPHICS_FT80X_DEBUG_WARN)
#  define ft80x_warn _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_warn(format, ...) \
     fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_warn printf
#endif

#if !defined(GRAPHICS_FT80X_DEBUG_INFO)
#  define ft80x_info _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_info(format, ...) \
     printf(EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_info printf
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This is the type of one display example entry point */

typedef CODE int (*ft80x_example_t)(int fd,
                                    FAR struct ft80x_dlbuffer_s *buffer);

/* Bitmap header */

struct ft80x_bitmaphdr_s
{
  uint8_t format;
  int16_t width;
  int16_t height;
  int16_t stride;
  int32_t offset;
  FAR const uint8_t *data;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
extern const struct ft80x_bitmaphdr_s g_lenaface_bmhdr;
#endif

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

/* GPU Primitive display examples */

#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
int ft80x_prim_bitmaps(int fd, FAR struct ft80x_dlbuffer_s *buffer);
#endif
int ft80x_prim_points(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_lines(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_linestrip(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_edgestrip_r(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_rectangles(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_scissor(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_stencil(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_prim_alphablend(int fd, FAR struct ft80x_dlbuffer_s *buffer);

/* Co-processor display examples */

int ft80x_coproc_button(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_clock(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_gauge(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_keys(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_interactive(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_progressbar(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_scrollbar(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_slider(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_dial(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_toggle(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_number(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_calibrate(int fd, FAR struct ft80x_dlbuffer_s *buffer);
int ft80x_coproc_spinner(int fd, FAR struct ft80x_dlbuffer_s *buffer);
#ifndef CONFIG_EXAMPLES_FT80X_EXCLUDE_BITMAPS
int ft80x_coproc_screensaver(int fd, FAR struct ft80x_dlbuffer_s *buffer);
#endif
int ft80x_coproc_logo(int fd, FAR struct ft80x_dlbuffer_s *buffer);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_EXAMPLES_FT80X */
#endif /* __APPS_EXAMPLES_FT80X_FT80X_H */
