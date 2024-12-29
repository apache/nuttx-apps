/****************************************************************************
 * apps/examples/fmsynth/music_scale.h
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

#ifndef __APPS_EXAMPLES_FMSYNTH_UTILS_MUSIC_SCALE_H
#define __APPS_EXAMPLES_FMSYNTH_UTILS_MUSIC_SCALE_H

/****************************************************************************
 * Pre-processor definisions
 ****************************************************************************/

#define MUSIC_SCALE_C  (0)
#define MUSIC_SCALE_CS (1)
#define MUSIC_SCALE_D  (2)
#define MUSIC_SCALE_DS (3)
#define MUSIC_SCALE_E  (4)
#define MUSIC_SCALE_F  (5)
#define MUSIC_SCALE_FS (6)
#define MUSIC_SCALE_G  (7)
#define MUSIC_SCALE_GS (8)
#define MUSIC_SCALE_A  (9)
#define MUSIC_SCALE_AS (10)
#define MUSIC_SCALE_B  (11)
#define OCTAVE(n, s) ((s) + ((n) * 12))

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

EXTERN const float musical_scale[];

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __APPS_EXAMPLES_FMSYNTH_UTILS_MUSIC_SCALE_H */
