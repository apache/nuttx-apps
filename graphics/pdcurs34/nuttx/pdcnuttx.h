/****************************************************************************
 * apps/graphics/nuttx/pdcnuttx.h
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H
#define __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H 1

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "nuttx/config.h"
#include "nuttx/nx/nxfonts.h"
#include "curspriv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_PDCURSES_FOUNT_4X6)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_4X6
#elif defined(CONFIG_PDCURSES_FOUNT_5X7)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_5X7
#elif defined(CONFIG_PDCURSES_FOUNT_5X8)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_5X8
#elif defined(CONFIG_PDCURSES_FOUNT_6X9)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_6X9
#elif defined(CONFIG_PDCURSES_FOUNT_6X10)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_6X10
#elif defined(CONFIG_PDCURSES_FOUNT_6X12)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_6X12
#elif defined(CONFIG_PDCURSES_FOUNT_6X13)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_6X13
#elif defined(CONFIG_PDCURSES_FOUNT_6X13B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_6X13B
#elif defined(CONFIG_PDCURSES_FOUNT_7X13)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_7X13
#elif defined(CONFIG_PDCURSES_FOUNT_7X13B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_7X13B
#elif defined(CONFIG_PDCURSES_FOUNT_7X14)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_7X14
#elif defined(CONFIG_PDCURSES_FOUNT_7X14B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_7X14B
#elif defined(CONFIG_PDCURSES_FOUNT_8X13)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_8X13
#elif defined(CONFIG_PDCURSES_FOUNT_8X13B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_8X13B
#elif defined(CONFIG_PDCURSES_FOUNT_9X15)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_9X15
#elif defined(CONFIG_PDCURSES_FOUNT_9X15B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_9X15B
#elif defined(CONFIG_PDCURSES_FOUNT_9X18)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_9X18
#elif defined(CONFIG_PDCURSES_FOUNT_9X18B)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_9X18B
#elif defined(CONFIG_PDCURSES_FOUNT_10X20)
#  define PDCURSES_FONTID ID FONTID_X11_MISC_FIXED_10X20
#else
#  error No fixed width font selected
#endif

#endif /* __APPS_GRAPHICS_PDCURS34_NUTTX_PDCNUTTX_H */
