/****************************************************************************
 * apps/games/NXDoom/src/config.h
 *
 * SPDX-License-Identifer: GPLv2
 *
 * NuttX compatible config.h file.
 *
 * TODO: can we take a cleaner approach later by baking this into logic?
 *
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HAVE_DECL_STRCASECMP 1
#define HAVE_DECL_STRNCASECMP 1

#define PACKAGE_TARNAME "nxdoom"
#define PACKAGE_NAME "NXDoom"
#define PACKAGE_STRING "NXDoom v0.0.0"
#define PROGRAM_PREFIX ""

#define DISABLE_SDL2MIXER
#define DISABLE_SDL2NET
