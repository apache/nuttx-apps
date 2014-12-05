/****************************************************************************
 * apps/graphics/traveler/include/trv_rayavoid.h
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

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYAVOID_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYAVOID_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_world.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

trv_coord_t trv_rayclip_player_xmotion(FAR struct trv_camera_s *pov,
                                       trv_coord_t dist, int16_t yaw,
                                       trv_coord_t height);
trv_coord_t trv_rayclip_player_ymotion(FAR struct trv_camera_s *pov,
                                       trv_coord_t dist, int16_t yaw,
                                       trv_coord_t height);
trv_coord_t trv_ray_adjust_zpos(FAR struct trv_camera_s *pov,
                                trv_coord_t height);
FAR struct trv_rect_data_s *trv_ray_test_xplane(FAR struct trv_camera_s *pov,
                                                trv_coord_t dist, int16_t yaw,
                                                trv_coord_t height);
FAR struct trv_rect_data_s *trv_ray_test_yplane(FAR struct trv_camera_s *pov,
                                                trv_coord_t dist, int16_t yaw,
                                                trv_coord_t height);

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_RAYAVOID_H */
