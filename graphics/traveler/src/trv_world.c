/****************************************************************************
 * apps/graphics/traveler/src/trv_world.c
 * This file contains the logic that creates and destroys the world.
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

/****************************************************************************
 * Included files
 ****************************************************************************/

#include "trv_types.h"
#include "trv_main.h"
#include "trv_fsutils.h"
#include "trv_paltable.h"
#include "trv_world.h"
#include "trv_plane.h"
#include "trv_bitmaps.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "fsutils/inifile.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Everything related to the camera POV is defined in the camera section. */

#define CAMERA_SECTION_NAME    "camera"

/* These values define the initial camera position. */

#define CAMERA_INITIAL_X       "initialcamerax"
#define CAMERA_INITIAL_Y       "initialcameray"
#define CAMERA_INITIAL_Z       "initialcameraz"

/* These values define the orientation of the g_camera. */

#define CAMERA_INITIAL_YAW     "initialcamerayaw"
#define CAMERA_INITIAL_PITCH   "initialcamerayaw"

/* Everything related to the player is defined in the player section. */

#define PLAYER_SECTION_NAME    "player"

/* These are characteristics of the player. */

#define PLAYER_HEIGHT          "playerheight"
#define PLAYER_WALK_STEPHEIGHT "playerwalkstepheight"
#define PLAYER_RUN_STEPHEIGHT  "playerrunstepheight"

/* Everything related to the world is defined in the world section. */

#define WORLD_SECTION_NAME     "world"

/* Other files: */

#define WORLD_MAP              "worldmap"
#define WORLD_PALETTE          "worldpalette"
#define WORLD_IMAGES           "worldimages"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int trv_ini_short(INIHANDLE inihandle, FAR int16_t *value,
                         FAR const char *section, FAR const char *name);
#if 0 /* Not used */
static int trv_ini_long(INIHANDLE inihandle, FAR  long *value,
                        FAR const char *section, FAR const char *name);
#endif
static int trv_ini_filename(INIHANDLE inihandle, FAR const char *wldpath,
                            FAR const char *section, FAR const char *name,
                            FAR char **filename);
static int trv_manage_wldfile(INIHANDLE inihandle, FAR const char *wldfile);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This is the starting position and orientation of the camera in the world */

struct trv_camera_s g_initial_camera;

/* This is the height of player (distance from the camera Z position to
 * the position of the player's "feet"
 */

trv_coord_t g_player_height;

/* This is size of something that the player can step over when "walking" */

trv_coord_t g_walk_stepheight;

/* This is size of something that the player can step over when "running" */

trv_coord_t g_run_stepheight;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_camera_section_name[]        = CAMERA_SECTION_NAME;
static const char g_camera_initialx_name[]       = CAMERA_INITIAL_X;
static const char g_camera_initialy_name[]       = CAMERA_INITIAL_Y;
static const char g_camera_initialz_name[]       = CAMERA_INITIAL_Z;
static const char g_camera_initialyaw_name[]     = CAMERA_INITIAL_YAW;
static const char g_camera_initialpitch_name[]   = CAMERA_INITIAL_PITCH;

static const char g_player_section_name[]        = PLAYER_SECTION_NAME;
static const char g_player_height_name[]         = PLAYER_HEIGHT;
static const char g_player_walkstepheight_name[] = PLAYER_WALK_STEPHEIGHT;
static const char g_player_runstepheight_name[]  = PLAYER_RUN_STEPHEIGHT;

static const char g_world_section_name[]         = WORLD_SECTION_NAME;
static const char g_world_map_name[]             = WORLD_MAP;
static const char g_world_palette_name[]         = WORLD_PALETTE;
static const char g_world_images_name[]          = WORLD_IMAGES;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_ini_short
 *
 * Description:
 *   Reads a short value from the INI file and assures that it is within
 *   range for an a int16_t
 *
 ****************************************************************************/

static int trv_ini_short(INIHANDLE inihandle, FAR int16_t *value,
                         FAR const char *section, FAR const char *name)
{
  /* Read the long integer from the .INI file.  We supply the default
   * value of INT32_MAX.  If this value is returned, we assume that
   * that is evidence that the requested value was not supplied in the
   * .INI file.
   */

  long inivalue = inifile_read_integer(inihandle, section, name, INT32_MAX);

  /* Make sure that it is in range for a int16_t. */

  if (inivalue < INT16_MIN || inivalue > INT16_MAX)
    {
      /* It is not!... */

      *value = 0;

      /* Is this because the integer was not found? or because
       * it is really out of range.
       */

      if (inivalue != INT32_MAX)
        {

          fprintf(stderr, "ERROR: Integer out of range in INI file:\n");
          fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\" value=%ld\n",
                  section, name, (long)inivalue);
          return -ENOENT;

        }
      else
        {
          fprintf(stderr, "ERROR: Required integer not found in INI file:\n");
          fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
                  section, name);
          return -ERANGE;
        }
    }
  else
    {
      *value = (int16_t)inivalue;
      return OK;
    }
}

/****************************************************************************
 * Name: trv_ini_long
 *
 * Description:
 *   Reads a long value from the INI file
 *
 ****************************************************************************/

#if 0 /* Not used */
static uint8_t trv_ini_long(INIHANDLE inihandle, FAR long *value,
                            FAR const char *section, FAR const char *name)
{
  /* Read the long integer from the .INI file.*/

  *value = inifile_read_integer(inihandle, section, name, 0);
  return 0;
}
#endif

/****************************************************************************
 * Name: trv_ini_filename
 *
 * Description:
 *   Reads a file name from the INI file.
 *
 ****************************************************************************/

static int trv_ini_filename(INIHANDLE inihandle, FAR const char *path,
                            FAR const char *section, FAR const char *name,
                            FAR char **filename)
{
  /* Read the string from the .INI file.  We supply the default value of
   * NULL.  If this value is returned, we assume that that is evidence that
   * the requested value was not supplied in the .INI file.
   */

  FAR char *value = inifile_read_string(inihandle, section, name, NULL);

  /* Did we get the file name? */

  if (!value)
    {
      /* No... */

      *filename = NULL;

      fprintf(stderr, "ERROR: Required filename not found in INI file:\n");
      fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
              section, name);
      return -ENOENT;
    }
  else
    {
      *filename = trv_fullpath(path, value);
       inifile_free_string(value);
      return OK;
    }
}
/****************************************************************************
 * Name: trv_manage_wldfile
 *
 * Description:
 *   This is the guts of trv_world_create.  It is implemented as a separate
 *   function in order to simplify error handling
 *
 ****************************************************************************/

static int trv_manage_wldfile(INIHANDLE inihandle, FAR const char *wldpath)
{
  FAR char *filename;
  int ret;

  /* Read the initial camera/player position */

  ret = trv_ini_short(inihandle, &g_initial_camera.x,
                      g_camera_section_name, g_camera_initialx_name);
  if (ret < 0)
    {
      return ret;
    }

  ret = trv_ini_short(inihandle, &g_initial_camera.y,
                      g_camera_section_name, g_camera_initialy_name);
  if (ret < 0)
    {
      return ret;
    }

  ret = trv_ini_short(inihandle, &g_initial_camera.z,
                      g_camera_section_name, g_camera_initialz_name);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the player's yaw/pitch orientation */

  ret = trv_ini_short(inihandle, &g_initial_camera.yaw,
                      g_camera_section_name, g_camera_initialyaw_name);
  if (ret < 0)
    {
      return ret;
    }

  ret = trv_ini_short(inihandle, &g_initial_camera.pitch,
                      g_camera_section_name, g_camera_initialpitch_name);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the height of the player */

  ret = trv_ini_short(inihandle, &g_player_height,
                      g_player_section_name, g_player_height_name);
  if (ret < 0)
    {
      return ret;
    }

  /* Read the player's capability to step on top of things in the world. */

  ret = trv_ini_short(inihandle, &g_walk_stepheight,
                      g_player_section_name, g_player_walkstepheight_name);
  if (ret < 0)
    {
      return ret;
    }

  ret = trv_ini_short(inihandle, &g_run_stepheight,
                      g_player_section_name, g_player_runstepheight_name);
  if (ret < 0)
    {
      return ret;
    }

  /* Get the name of the file containing the world map */

  ret = trv_ini_filename(inihandle, wldpath, g_world_section_name,
                         g_world_map_name, &filename);
  if (ret < 0)
    {
      return ret;
    }

  /* Allocate and load the world */

  ret = trv_initialize_planes();
   if (ret < 0)
    {
      return ret;
    }

  ret = trv_load_planefile(filename);
  if (ret < 0)
    {
      return ret;
    }

  free(filename);

  /* Get the name of the file containing the palette table which is used
   * to adjust the lighting with distance.
   */

  ret = trv_ini_filename(inihandle,wldpath, g_world_section_name,
                         g_world_palette_name, &filename);
  if (ret < 0)
    {
      return ret;
    }

  /* Then load it into g_paltable. */

  ret = trv_load_paltable(filename);
  if (ret < 0)
    {
      return ret;
    }

  free(filename);

  /* Get the name of the file containing the texture data */

  ret = trv_ini_filename(inihandle, wldpath, g_world_section_name,
                         g_world_images_name, &filename);
  if (ret < 0)
    {
      return ret;
    }

  /* Then load the bitmaps */

  ret = trv_initialize_bitmaps();
  if (ret < 0)
    {
      return ret;
    }

  ret = trv_load_bitmapfile(filename, wldpath);
  free(filename);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_world_create
 *
 * Description:
 *   Load the world data structures from information in an INI file
 *
 ****************************************************************************/

int trv_world_create(FAR const char *wldpath, FAR const char *wldfile)
{
  FAR char *fullpath;
  INIHANDLE inihandle;
  int ret;

  /* Open the INI file which contains all of the information that we
   * need to construct the world
   */

  fullpath = trv_fullpath(wldpath, wldfile);
  inihandle = inifile_initialize(fullpath);
  free(fullpath);

  if (!inihandle)
    {
      fprintf(stderr, "ERROR: Could not open INI file=\"%s/%s\"\n",
              wldpath, wldfile);
      return -ENOENT;
    }

  /* Load the world file data */

  ret = trv_manage_wldfile(inihandle, wldpath);

  /* Close the INI file and return */

  inifile_uninitialize(inihandle);
  return ret;
}

/****************************************************************************
 * Name: trv_world_destroy
 *
 * Description:
 *   Destroy the world and release all of its resources
 *
 ****************************************************************************/

void trv_world_destroy(void)
{
  trv_release_planes();
  trv_release_bitmaps();
  trv_release_paltable();
}
