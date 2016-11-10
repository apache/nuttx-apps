/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_inifile.h
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_INIFILE_H
#define __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_INIFILE_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Everything related to the camera POV is defined in the
 * camera section. */

#define CAMERA_SECTION_NAME    "camera"

/* These values define the initial camera postion. */

#define CAMERA_INITIAL_X       "initialcamerax"
#define CAMERA_INITIAL_Y       "initialcameray"
#define CAMERA_INITIAL_Z       "initialcameraz"

/* These values define the orientation of the camera. */

#define CAMERA_INITIAL_YAW     "initialcamerayaw"
#define CAMERA_INITIAL_PITCH   "initialcamerayaw"

/* Everything related to the player is defined in the player section. */

#define PLAYER_SECTION_NAME    "player"

/* These are charwld_erictics of the player. */

#define PLAYER_HEIGHT          "playerheight"
#define PLAYER_WALK_STEPHEIGHT "playerwalkstepheight"
#define PLAYER_RUN_STEPHEIGHT  "playerrunstepheight"

/* Everything related to the world is defined in the world section. */

#define WORLD_SECTION_NAME     "world"

/* Other files: */

#define WORLD_MAP              "worldmap"
#define WORLD_PALETTE          "worldpalette"
#define WORLD_IMAGES           "worldimages"

#endif /* __APPS_GRAPHICS_TRAVELER_TOOSL_LIBWLD_WLD_INIFILE_H */
