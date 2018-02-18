/****************************************************************************
 * apps/examples/ft80x/ft80x.h
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef GRAPHICS_FT80X_DEBUG_ERROR
#    define  ft80x_err(format, ...) \
       fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#  else
#    define  ft80x_err(format, ...)
#  endif

#  ifdef GRAPHICS_FT80X_DEBUG_WARN
#    define  ft80x_warn(format, ...) \
       fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#  else
#    define  ft80x_warn(format, ...)
#  endif

#  ifdef GRAPHICS_FT80X_DEBUG_INFO
#    define  ft80x_info(format, ...) \
       printf(EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#  else
#    define  ft80x_info(format, ...)
#  endif
#else
#  ifdef GRAPHICS_FT80X_DEBUG_ERROR
#    define  ft80x_err printf
#  else
#    define  ft80x_err (void)
#  endif

#  ifdef GRAPHICS_FT80X_DEBUG_WARN
#    define  ft80x_warn printf
#  else
#    define  ft80x_warn (void)
#  endif

#  ifdef GRAPHICS_FT80X_DEBUG_INFO
#    define  ft80x_info printf
#  else
#    define  ft80x_info (void)
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This is the type of one display example entry point */

typedef CODE int (*ft80x_example_t)(int fd,
                                    FAR struct ft80x_dlbuffer_s *buffer);

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

int ft80x_rectangles(int fd, FAR struct ft80x_dlbuffer_s *buffer);

/* Co-processor display examples */

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_EXAMPLES_FT80X */
#endif /* __APPS_EXAMPLES_FT80X_FT80X_H */
