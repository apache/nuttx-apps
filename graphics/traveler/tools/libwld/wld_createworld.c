/****************************************************************************
 * apps/graphics/traveler/tools/libwld/wld_createworld.c
 * This file contains the logic which creates the world.
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

/*************************************************************************
 * Included files
 *************************************************************************/

#include "trv_types.h"

#include "wld_paltable.h"
#include "wld_color.h"
#include "wld_inifile.h"
#include "wld_world.h"

/*************************************************************************
 * Private Function Prototypes
 *************************************************************************/

static uint8_t wld_manage_worldfile(INIHANDLE handle);
static uint8_t wld_read_shortint(INIHANDLE handle,
                                 int16_t * variable_value,
                                 const char *section_name,
                                 const char *variable_name);
#if 0                           /* Not used */
static uint8_t wld_read_longint(INIHANDLE handle,
                                long *variable_value,
                                const char *section_name,
                                const char *variable_name);
#endif
static uint8_t wld_read_filename(INIHANDLE handle,
                                 char **filename,
                                 const char *section_name,
                                 const char *variable_name);

/*************************************************************************
 * Global Variables
 *************************************************************************/

/* This is the starting position and orientation of the camera in the world */

wld_camera_t g_initial_camera;

/* This is the height of player (distance from the camera Z position to
 * the position of the player's "feet"
 */

wld_coord_t g_player_height;

/* This is size of something that the player can step over when "walking" */

wld_coord_t g_walk_stepheight;

/* This is size of something that the player can step over when "running" */

wld_coord_t g_run_stepheight;

/*************************************************************************
 * Private Constant Data
 *************************************************************************/

static const char g_camera_section_name[] = CAMERA_SECTION_NAME;
static const char g_camera_initialx_name[] = CAMERA_INITIAL_X;
static const char g_camera_initialy_name[] = CAMERA_INITIAL_Y;
static const char g_camera_initialz_name[] = CAMERA_INITIAL_Z;
static const char g_camera_initialyaw_name[] = CAMERA_INITIAL_YAW;
static const char g_camera_initialpitch_name[] = CAMERA_INITIAL_PITCH;

static const char g_player_section_name[] = PLAYER_SECTION_NAME;
static const char g_player_height_name[] = PLAYER_HEIGHT;
static const char g_player_walk_stepheight_name[] = PLAYER_WALK_STEPHEIGHT;
static const char g_player_run_stepheight_name[] = PLAYER_RUN_STEPHEIGHT;

static const char g_world_section_name[] = WORLD_SECTION_NAME;
static const char g_wold_map_name[] = WORLD_MAP;
static const char g_world_palette_name[] = WORLD_PALETTE;
static const char g_world_images_name[] = WORLD_IMAGES;

/*************************************************************************
 * Private Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_manage_worldfile
 * Description: This is the guts of wld_create_world.  It is implemented as
 * a separate file to simplify error handling
 ************************************************************************/

static uint8_t wld_manage_worldfile(INIHANDLE handle)
{
  char *filename;
  uint8_t result;

  /* Read the initial camera/player position */

  result = wld_read_shortint(handle, &g_initial_camera.x,
                             g_camera_section_name, g_camera_initialx_name);
  if (result != 0)
    {
      return result;
    }

  result = wld_read_shortint(handle, &g_initial_camera.y,
                             g_camera_section_name, g_camera_initialy_name);
  if (result != 0)
    {
      return result;
    }

  result = wld_read_shortint(handle, &g_initial_camera.z,
                             g_camera_section_name, g_camera_initialz_name);
  if (result != 0)
    {
      return result;
    }

  /* Get the player's yaw/pitch orientation */

  result = wld_read_shortint(handle, &g_initial_camera.yaw,
                             g_camera_section_name, g_camera_initialyaw_name);
  if (result != 0)
    {
      return result;
    }

  result = wld_read_shortint(handle, &g_initial_camera.pitch,
                             g_camera_section_name, g_camera_initialpitch_name);
  if (result != 0)
    {
      return result;
    }

  /* Get the height of the player */

  result = wld_read_shortint(handle, &g_player_height,
                             g_player_section_name, g_player_height_name);
  if (result != 0)
    {
      return result;
    }

  /* Read the player's capability to step on top of things in the world. */

  result = wld_read_shortint(handle, &g_walk_stepheight,
                             g_player_section_name,
                             g_player_walk_stepheight_name);
  if (result != 0)
    {
      return result;
    }

  result = wld_read_shortint(handle, &g_run_stepheight,
                             g_player_section_name,
                             g_player_run_stepheight_name);
  if (result != 0)
    {
      return result;
    }

  /* Get the name of the file containing the world map */

  result = wld_read_filename(handle, &filename,
                             g_world_section_name, g_wold_map_name);
  if (result != 0)
    {
      return result;
    }

  if (filename == NULL)
    {
      return WORLD_PLANE_FILE_NAME_ERROR;
    }

  /* Allocate and load the world */

  result = wld_initialize_planes();
  if (result != 0)
    {
      return result;
    }

  result = wld_load_planefile(filename);
  if (result != 0)
    {
      return result;
    }

  inifile_free_string(filename);

  /* Create the RGB lookup table */

  wld_rgblookup_allocate();

  /* Get the name of the file containing the palette table which is used to
   * adjust the lighting with distance.
   */

  result = wld_read_filename(handle, &filename, g_world_section_name,
                             g_world_palette_name);
  if (result != 0)
    {
      return result;
    }

  if (filename == NULL)
    {
      return WORLD_PALR_FILE_NAME_ERROR;
    }

  /* Then load it into g_pal_table. */

  result = wld_load_paltable(filename);
  if (result != 0)
    {
      return result;
    }

  inifile_free_string(filename);

  /* Get the name of the file containing the texture data */

  result = wld_read_filename(handle, &filename, g_world_section_name,
                             g_world_images_name);
  if (result != 0)
    {
      return result;
    }

  if (filename == NULL)
    {
      return WORLD_BITMAP_FILE_NAME_ERROR;
    }

  /* Then load the bitmaps */

  result = wld_initialize_bitmaps();
  if (result != 0)
    {
      return result;
    }

  result = wld_load_bitmapfile(filename);
  inifile_free_string(filename);

  return result;
}

/*************************************************************************
 * Name: wld_read_shortint
 * Description: Reads a long value from the INI file and assures that
 *              it is within range for an a int16_t
 ************************************************************************/

static uint8_t wld_read_shortint(INIHANDLE handle,
                                 int16_t * variable_value,
                                 const char *section_name,
                                 const char *variable_name)
{
  /* Read the long integer from the ini file.  We supply the default value of
   * INT32_MAX.  If this value is returned, we assume that that is evidence
   * that the requested value was not supplied in the ini file. */

  long value = inifile_read_integer(handle, section_name,
                                    variable_name, INT32_MAX);

  /* Make sure that it is in range for a int16_t . */

  if ((value < INT16_MIN) || (value > INT16_MAX))
    {
      /* It is not!... */

      *variable_value = 0;

      /* Is this because the integer was not found? or because it is really out
       * of range. */

      if (value != INT32_MAX)
        {

          fprintf(stderr, "ERROR: Integer out of range in INI file:\n");
          fprintf(stderr,
                  "       Section=\"%s\" Variable name=\"%s\" value=%ld\n",
                  section_name, variable_name, (long)variable_value);
          return WORLD_INTEGER_OUT_OF_RANGE;

        }
      else
        {

          fprintf(stderr, "ERROR: Requird integer not found in INI file:\n");
          fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
                  section_name, variable_name);
          return WORLD_INTEGER_NOT_FOUND;
        }
    }
  else
    {

      *variable_value = (int16_t) value;
      return 0;
    }
}

/*************************************************************************
 * Name: wld_read_shortint
 * Description: Reads a long value from the INI file
 ************************************************************************/

#if 0                           /* Not used */
static uint8_t wld_read_longint(INIHANDLE handle, long *variable_value,
                                const char *section_name,
                                const char *variable_name)
{
  /* Read the long integer from the ini file. */

  *variable_value = inifile_read_integer(handle, section_name,
                                         variable_name, 0);
  return 0;
}
#endif

/*************************************************************************
 * Name: wld_read_filename
 * Description: Reads a file name from the INI file.
 ************************************************************************/

static uint8_t wld_read_filename(INIHANDLE handle,
                                 char **filename,
                                 const char *section_name,
                                 const char *variable_name)
{
  /* Read the string from the ini file.  We supply the default value of NULL.
   * If this value is returned, we assume that that is evidence that the
   * requested value was not supplied in the ini file. */

  char *value = inifile_read_string(handle, section_name, variable_name,
                                    NULL);

  /* Did we get the file name? */

  if (!value)
    {
      /* No... */

      *filename = NULL;

      fprintf(stderr, "ERROR: Required filename not found in INI file:\n");
      fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
              section_name, variable_name);
      return WORLD_FILENAME_NOT_FOUND;
    }
  else
    {
      *filename = value;
      return 0;
    }
}

/*************************************************************************
 * Public Functions
 *************************************************************************/

/*************************************************************************
 * Name: wld_create_world
 * Description:
 ************************************************************************/

uint8_t wld_create_world(const char *wldfile)
{
  INIHANDLE handle;
  uint8_t result;

  /* Open the INI file which contains all of the information that we need to
   * construct the world */

  handle = inifile_initialize(wldfile);
  if (handle == NULL)
    {
      fprintf(stderr, "ERROR:  Could not open INI file=\"%s\"\n", wldfile);
      return WORLD_FILE_OPEN_ERROR;
    }

  /* Load the world file data */

  result = wld_manage_worldfile(handle);

  /* Close the INI file and return */

  inifile_uninitialize(handle);
  return result;
}
