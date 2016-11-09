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
#include "inifile.h"
#include "wld_paltable.h"
#include "wld_world.h"
#include "wld_inifile.h"

/*************************************************************************
 * Private Function Prototypes
 *************************************************************************/

static uint8_t wld_ManageWorldFile(void);
static uint8_t wld_ReadIniShortInteger(int16_t  *variableValue,
                                    const char *sectionName,
                                    const char *variableName);
#if 0 /* Not used */
static uint8_t wld_ReadIniLongInteger(long *variableValue,
                                   const char *sectionName,
                                   const char *variableName);
#endif
static uint8_t wld_ReadIniFileName(char **fileName,
                                const char *sectionName,
                                const char *variableName);

/*************************************************************************
 * Global Variables
 *************************************************************************/

/* This is the starting position and orientation of the camera in the world */

cameraType initialCamera;

/* This is the height of player (distance from the camera Z position to
 * the position of the player's "feet"
 */

coord_t playerHeight;

/* This is size of something that the player can step over when "walking" */

coord_t walkStepHeight;

/* This is size of something that the player can step over when "running" */

coord_t runStepHeight;

/*************************************************************************
 * Private Constant Data
 *************************************************************************/

static const char cameraSectionName[]        = CAMERA_SECTION_NAME;
static const char cameraInitialXName[]       = CAMERA_INITIAL_X;
static const char cameraInitialYName[]       = CAMERA_INITIAL_Y;
static const char cameraInitialZName[]       = CAMERA_INITIAL_Z;
static const char cameraInitialYawName[]     = CAMERA_INITIAL_YAW;
static const char cameraInitialPitchName[]   = CAMERA_INITIAL_PITCH;

static const char playerSectionName[]        = PLAYER_SECTION_NAME;
static const char playerHeightName[]         = PLAYER_HEIGHT;
static const char playerWalkStepHeightName[] = PLAYER_WALK_STEPHEIGHT;
static const char playerRunStepHeightName[]  = PLAYER_RUN_STEPHEIGHT;

static const char worldSectionName[]         = WORLD_SECTION_NAME;
static const char worldMapName[]             = WORLD_MAP;
static const char worldPaletteName[]         = WORLD_PALETTE;
static const char worldImagesName[]          = WORLD_IMAGES;

/*************************************************************************
 * Private Variables
 *************************************************************************/

/*************************************************************************
 * Name:    wld_create_world
 * Description:
 ************************************************************************/

uint8_t wld_create_world(char *wldFile)
{
  uint8_t result;

  /* Open the INI file which contains all of the information that we
   * need to construct the world
   */

  if (!init_inifile(wldFile))
    {
      fprintf(stderr, "Error:  Could not open INI file=\"%s\"\n", wldFile);
      return WORLD_FILE_OPEN_ERROR;
    }

  /* Load the world file data */

  result = wld_ManageWorldFile();

  /* Close the INI file and return */

  uninit_inifile();
  return result;
}

/*************************************************************************
 * Name:    wld_ManageWorldFile
 * Description: This is the guts of wld_create_world.  It is implemented as
 * a separate file to simplify error handling
 ************************************************************************/

static uint8_t wld_ManageWorldFile(void)
{
  char *fileName;
  uint8_t result;

  /* Read the initial camera/player position */

  result = wld_ReadIniShortInteger(&initialCamera.x, 
                                  cameraSectionName,
                                  cameraInitialXName);
  if (result != 0) return result;

  result = wld_ReadIniShortInteger(&initialCamera.y,
                                  cameraSectionName,
                                  cameraInitialYName);
  if (result != 0) return result;

  result = wld_ReadIniShortInteger(&initialCamera.z,
                                  cameraSectionName,
                                  cameraInitialZName);
  if (result != 0) return result;

  /* Get the player's yaw/pitch orientation */

  result = wld_ReadIniShortInteger(&initialCamera.yaw,
                                  cameraSectionName,
                                  cameraInitialYawName);
  if (result != 0) return result;

  result = wld_ReadIniShortInteger(&initialCamera.pitch, 
                                  cameraSectionName,
                                  cameraInitialPitchName);
  if (result != 0) return result;

  /* Get the height of the player */

  result = wld_ReadIniShortInteger(&playerHeight,
                                  playerSectionName,
                                  playerHeightName);
  if (result != 0) return result;

  /* Read the player's capability to step on top of things in the world. */

  result = wld_ReadIniShortInteger(&walkStepHeight,
                                  playerSectionName,
                                  playerWalkStepHeightName);
  if (result != 0) return result;

  result = wld_ReadIniShortInteger(&runStepHeight,
                                  playerSectionName, 
                                  playerRunStepHeightName);
  if (result != 0) return result;

  /* Get the name of the file containing the world map */

  result = wld_ReadIniFileName(&fileName, worldSectionName, worldMapName);
  if (result != 0) return result;
  if (fileName == NULL) return WORLD_PLANE_FILE_NAME_ERROR;

  /* Allocate and load the world */

  result = wld_initialize_planes();
  if (result != 0) return result;

  result = wld_load_planefile(fileName);
  if (result != 0) return result;

  free_ini_string(fileName);

  /* Get the name of the file containing the palette table which is used
   * to adjust the lighting with distance.
   */

  result = wld_ReadIniFileName(&fileName, worldSectionName, worldPaletteName);
  if (result != 0) return result;
  if (fileName == NULL) return WORLD_PALR_FILE_NAME_ERROR;

  /* Then load it into palTable. */

  result = wld_load_paltable(fileName);
  if (result != 0) return result;

  free_ini_string(fileName);

  /* Get the name of the file containing the texture data */

  result = wld_ReadIniFileName(&fileName, worldSectionName, worldImagesName);
  if (result != 0) return result;
  if (fileName == NULL) return WORLD_BITMAP_FILE_NAME_ERROR;

  /* Then load the bitmaps */

  result = wld_initialize_bitmaps();
  if (result != 0) return result;

  result = wld_load_bitmapfile(fileName);
  free_ini_string(fileName);

  return result;
}

/*************************************************************************
 * Name:    wld_ReadIniShortInteger
 * Description: Reads a long value from the INI file and assures that
 *              it is within range for an a int16_t 
 ************************************************************************/

static uint8_t wld_ReadIniShortInteger(int16_t  *variableValue,
                                    const char *sectionName,
                                    const char *variableName)
{
  /* Read the long integer from the ini file.  We supply the default
   * value of MAX_SINT32.  If this value is returned, we assume that
   * that is evidence that the requested value was not supplied in the
   * ini file.
   */

  long value = ini_read_integer(sectionName, variableName,
                                MAX_SINT32);

  /* Make sure that it is in range for a int16_t . */

  if ((value < MIN_SINT16) || (value > MAX_SINT16))
    {
      /* It is not!... */

      *variableValue = 0;

      /* Is this because the integer was not found? or because
       * it is really out of range.
       */

      if (value != MAX_SINT32)
        {

          fprintf(stderr, "Error: Integer out of range in INI file:\n");
          fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\" value=%ld\n",
                  sectionName, variableName, (long)variableValue);
          return WORLD_INTEGER_OUT_OF_RANGE;

        }
      else
        {

          fprintf(stderr, "Error: Requird integer not found in INI file:\n");
          fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
                  sectionName, variableName);
          return WORLD_INTEGER_NOT_FOUND;
        }
    }
  else
    {

      *variableValue = (int16_t )value;
      return 0;
    }
}

/*************************************************************************
 * Name:    wld_ReadIniShortInteger
 * Description: Reads a long value from the INI file
 ************************************************************************/

#if 0 /* Not used */
static uint8_t wld_ReadIniLongInteger(long *variableValue,
                                   const char *sectionName,
                                   const char *variableName)
{
  /* Read the long integer from the ini file.*/

  *variableValue = ini_read_integer(sectionName, variableName, 0);
  return 0;
}
#endif

/*************************************************************************
 * Name:    wld_ReadIniShortInteger
 * Description: Reads a file name from the the INI file.
 ************************************************************************/

static uint8_t wld_ReadIniFileName(char **fileName,
                                const char *sectionName,
                                const char *variableName)
{
  /* Read the string from the ini file.  We supply the default
   * value of NULL.  If this value is returned, we assume that
   * that is evidence that the requested value was not supplied in the
   * ini file.
   */

  char *value = ini_read_string(sectionName, variableName, NULL);

  /* Did we get the file name? */

  if (!value)
    {
      /* No... */

      *fileName = NULL;

      fprintf(stderr, "Error: Required filename not found in INI file:\n");
      fprintf(stderr, "       Section=\"%s\" Variable name=\"%s\"\n",
              sectionName, variableName);
      return WORLD_FILENAME_NOT_FOUND;
    }
  else
    {
      *fileName = value;
      return 0;
    }
}
