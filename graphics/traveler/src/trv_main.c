/****************************************************************************
 * apps/graphics/traveler/trv_debug.h
 * This file contains the main logic for the NuttX version of Traveler
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
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "trv_types.h"
#include "trv_bitmaps.h"
#include "trv_world.h"
#include "trv_doors.h"
#include "trv_pov.h"
#include "trv_raycntl.h"
#include "trv_rayrend.h"
#include "trv_input.h"
#include "trv_graphics.h"
#include "trv_debug.h"
#include "trv_main.h"

#if CONFIG_GRAPHICS_TRAVELER_PERFMON
#  include <sys/types.h>
#  include <sys/time.h>
#  include <unistd.h>
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_exit(int exitCode);
static void trv_usage(char *execname);
#if CONFIG_GRAPHICS_TRAVELER_PERFMON
static double trv_current_time(void);
#endif

/****************************************************************************
 * Public Data
 *************************************************************************/

bool g_trv_terminate;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void trv_exit(int exitcode) noreturn_function;
static void trv_usage(char *execname);
#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
static double trv_current_time(void);
#endif

/****************************************************************************
 * Private Data
 *************************************************************************/

static const char g_default_worldfile[] = "transfrm.wld";
static FAR struct trv_graphics_info_s g_trv_ginfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_exit
 *
 * Description:
 ****************************************************************************/

static void trv_exit(int exitcode)
{
  /* Release memory held by the ray casting engine */

  trv_world_destroy();

  /* Close off input */

  trv_input_terminate();
  trv_graphics_terminate(&g_trv_ginfo);

  exit(exitcode);
}

/****************************************************************************
 * Name: trv_usage
 *
 * Description:
 ****************************************************************************/

static void trv_usage(char *execname)
{
  fprintf(stderr, "Usage: %s [-b] [-p<path>] [world]\n", execname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "  -p<path> Selects the path to the world data file\n");
  fprintf(stderr, "  world    Selects the world file name\n");
  exit(EXIT_FAILURE);
}

/****************************************************************************
 * Name: trv_current_time
 *
 * Description:
 ****************************************************************************/

#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
static double trv_current_time(void)
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (double) tv.tv_sec + (double) tv.tv_usec / 1000000.0;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int traveler_main(int argc, char *argv[])
#endif
{
  FAR const char *world_filename;
#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
  int32_t frame_count = 0;
  double elapsed_time = 0.0;
  double start_time;
#endif
  int ret;
  int i;

  /* Check for command line arguments */

  world_filename = g_default_worldfile;
  for (i = 1; i < argc; i++)
    {
      FAR char *ptr = argv[i];
      if (*ptr == '-')
        {
          ptr++;
          switch (*ptr)
            {
            case 'p' :
              ptr++;
              printf("World data path = %s\n", ptr);
              if (chdir(ptr))
                {
                  fprintf(stderr, "Bad path name\n");
                  trv_usage(argv[0]);
                }
              break;

            default:
              fprintf(stderr, "Invalid Switch\n");
              trv_usage(argv[0]);
              break;
            }
        }
      else
        {
          world_filename = ptr;
        }
    }

  trv_debug("World data file = %s\n", world_filename);

  /* Initialize the graphics interface */

  trv_graphics_initialize(&g_trv_ginfo);

  /* Load the word data structures */

  ret = trv_world_create(world_filename);
  if (ret < 0)
    {
      trv_abort("ERROR: %d loading world file %s: %d\n",
                world_filename, ret);
    }

  /* Set the player's POV in the new world */

  trv_pov_reset();

  /* Initialize the world's door */

  trv_door_initialize();

  /* Set up to receive input */

  trv_input_initialize();

  g_trv_terminate = false;
  while (!g_trv_terminate)
    {
#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
      start_time = trv_current_time();
#endif
      trv_input_read();

      /* Select the POV to use on this viewing cycle */

      trv_pov_new();

      /* Process door animations */

      trv_door_animate();

      /* Paint the back drop */

      trv_rend_backdrop(&g_trv_player);

      /* Render the 3-D view */

      trv_raycaster(&g_trv_player, &g_trv_ginfo);

      /* Display the world. */

      trv_display_update(&g_trv_ginfo);
#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
      frame_count++;
      elapsed_time += trv_current_time() - start_time;
      if (frame_count == 100)
        {
          fprintf(stderr, "fps = %3.2f\n", (double) frame_count / elapsed_time);
          frame_count = 0;
          elapsed_time = 0.0;
        }
#endif
    }

  trv_exit(EXIT_SUCCESS);
  return 0;
}

/****************************************************************************
 * Name: trv_abort
 *
 * Description:
 ****************************************************************************/

void trv_abort(FAR char *message, ...)
{
  va_list args;

  va_start(args, message);
  vfprintf(stderr, message, args);
  putc('\n', stderr);
  va_end(args);

  trv_exit(EXIT_FAILURE);
}
