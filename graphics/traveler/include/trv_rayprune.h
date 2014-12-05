/****************************************************************************
 * apps/graphics/traveler/include/trv_rayprune.h
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYPRUNE_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYPRUNE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern struct trv_rect_head_s g_ray_xplane;  /* List of X=plane rectangles */
extern struct trv_rect_head_s g_ray_yplane;  /* List of Y=plane rectangles */
extern struct trv_rect_head_s g_ray_zplane;  /* List of Z=plane rectangles */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void trv_ray_yawprune(int16_t yawstart, int16_t yawend);
void trv_ray_pitchprune(int16_t pitchstart, int16_t pitchend);
void trv_ray_pitchunprune(void);
void trv_ray_yawunprune(void);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYPRUNE_H */
