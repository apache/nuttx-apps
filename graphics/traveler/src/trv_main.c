/****************************************************************************
 * apps/graphics/traveler/trv_main.c
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
#include "trv_color.h"
#include "trv_debug.h"
#include "trv_main.h"

#if defined(CONFIG_GRAPHICS_TRAVELER_PERFMON) || \
    defined(CONFIG_GRAPHICS_TRAVELER_LIMITFPS)
#  include <sys/types.h>
#  include <sys/time.h>
#endif

#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_GRAPHICS_TRAVELER_DEFPATH
#  define CONFIG_GRAPHICS_TRAVELER_DEFPATH "/mnt/world"
#endif

/* Frame rate governer */

#ifdef CONFIG_GRAPHICS_TRAVELER_MAXFPS
#  define CONFIG_GRAPHICS_TRAVELER_MAXFPS 30
#endif

#define MIN_FRAME_USEC (1000000 / CONFIG_GRAPHICS_TRAVELER_MAXFPS)

/****************************************************************************
 * Public Data
 ****************************************************************************/

bool g_trv_terminate;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_default_worldfile[] = "transfrm.wld";
static const char g_default_worldpath[] = CONFIG_GRAPHICS_TRAVELER_DEFPATH;
static FAR struct trv_graphics_info_s g_trv_ginfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: trv_exit
 *
 * Description:
 ****************************************************************************/

static void trv_exit(int exitcode) noreturn_function;
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

#if defined(CONFIG_GRAPHICS_TRAVELER_PERFMON) || \
    defined(CONFIG_GRAPHICS_TRAVELER_LIMITFPS)
static void trv_current_time(FAR struct timespec *tp)
{
  int ret;

#ifdef CONFIG_CLOCK_MONOTONIC
  ret = clock_gettime(CLOCK_MONOTONIC, tp);
#else
  ret = clock_gettime(CLOCK_REALTIME, tp);
#endif

  if (ret < 0)
    {
      trv_abort("ERROR: clock_gettime failed: %d\n", errno);
    }
}
#endif

/****************************************************************************
 * Name: trv_timespec2usec
 *
 * Description:
 ****************************************************************************/

#if defined(CONFIG_GRAPHICS_TRAVELER_PERFMON) || \
    defined(CONFIG_GRAPHICS_TRAVELER_LIMITFPS)
static uint32_t trv_timespec2usec(FAR const struct timespec *tp)
{
  uint64_t usec = (uint64_t)tp->tv_sec * 1000*1000 + (uint64_t)(tp->tv_nsec / 1000);
  if (usec > UINT32_MAX)
    {
      trv_abort("ERROR: Time difference out of range: %llu\n", usec);
    }

  return (uint32_t)usec;
}
#endif

/****************************************************************************
 * Name: trv_current_time
 *
 * Description:
 ****************************************************************************/

#if defined(CONFIG_GRAPHICS_TRAVELER_PERFMON) || \
    defined(CONFIG_GRAPHICS_TRAVELER_LIMITFPS)
static uint32_t trv_elapsed_time(FAR struct timespec *now,
                                 FAR const struct timespec *then)
{
  struct timespec elapsed;

  /* Get the current time */

  trv_current_time(now);

  /* Get the seconds part of the difference */

  if (now->tv_sec < then->tv_sec)
    {
      goto errout;
    }

  elapsed.tv_sec  = now->tv_sec - then->tv_sec;

  /* Get the nanoseconds part of the difference, handling borrow from
   * seconds
   */

  elapsed.tv_nsec = 0;
  if (now->tv_nsec < then->tv_nsec)
    {
      if (elapsed.tv_sec <= 0)
        {
          goto errout;
        }

      elapsed.tv_sec--;
      elapsed.tv_nsec = 1000*1000*1000;
    }

  elapsed.tv_nsec = elapsed.tv_nsec + now->tv_nsec - then->tv_nsec;

  /* And return the time differenc in microsecond */

  return trv_timespec2usec(&elapsed);

errout:
  trv_abort("ERROR: Bad time difference\n");
  return 0;
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

#ifdef BUILD_MODULE
int main(int argc, FAR char *argv[])
#else
int traveler_main(int argc, char *argv[])
#endif
{
  FAR const char *wldpath;
  FAR const char *wldfile;
#if defined(CONFIG_GRAPHICS_TRAVELER_PERFMON) || \
    defined(CONFIG_GRAPHICS_TRAVELER_LIMITFPS)
#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
  struct timespec start_time;
  uint32_t frame_count = 0;
#endif
#ifdef CONFIG_GRAPHICS_TRAVELER_LIMITFPS
  struct timespec frame_start;
#endif
  struct timespec now;
  uint32_t elapsed_usec;
#endif
  int ret;
  int i;

  /* Defaults */

  wldpath = g_default_worldpath;
  wldfile = g_default_worldfile;

  /* Check for command line arguments */

  for (i = 1; i < argc; i++)
    {
      FAR char *ptr = argv[i];
      if (*ptr == '-')
        {
          ptr++;
          switch (*ptr)
            {
            case 'p' :
              wldpath = ptr++;
              break;

            default:
              fprintf(stderr, "Invalid Switch\n");
              trv_usage(argv[0]);
              break;
            }
        }
      else
        {
          wldfile = ptr;
        }
    }

  trv_debug("World data file: %s\n", wldfile);
  trv_debug("World data path: %s\n", wldpath);

  /* Initialize the graphics interface */

  trv_graphics_initialize(&g_trv_ginfo);

  /* Load the word data structures */

  ret = trv_world_create(wldpath, wldfile);
  if (ret < 0)
    {
      trv_abort("ERROR: Failed to load world file %s: %d\n",
                wldfile, ret);
    }

  /* Release color mapping tables */

  trv_color_endmapping();

  /* Set the player's POV in the new world */

  trv_pov_reset();

  /* Initialize the world's door */

  trv_door_initialize();

  /* Set up to receive input */

  trv_input_initialize();

#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
  /* Get the start time for performance monitoring */

  trv_current_time(&start_time);
#endif

  g_trv_terminate = false;
  while (!g_trv_terminate)
    {
#ifdef CONFIG_GRAPHICS_TRAVELER_LIMITFPS
      /* Get the start time from frame rate limiting */

      trv_current_time(&frame_start);
#endif

      trv_input_read();

      /* Select the POV to use on this viewing cycle */

      trv_pov_new();

      /* Process door animations */

      trv_door_animate();

      /* Paint the back drop */

      trv_rend_backdrop(&g_player, &g_trv_ginfo);

      /* Render the 3-D view */

      trv_raycaster(&g_player, &g_trv_ginfo);

      /* Display the world. */

      trv_display_update(&g_trv_ginfo);

#ifdef CONFIG_GRAPHICS_TRAVELER_LIMITFPS
       /* In the unlikely event that we are running "too" fast, we can delay
        * here to enforce a maixmum frame rate.
        */

      elapsed_usec = trv_elapsed_time(&now, &frame_start);
      if (elapsed_usec < MIN_FRAME_USEC)
        {
           usleep(MIN_FRAME_USEC - elapsed_usec);
        }
#endif

#ifdef CONFIG_GRAPHICS_TRAVELER_PERFMON
      /* Show the realized frame rate */

      frame_count++;
      if (frame_count >= 100)
        {
          elapsed_usec = trv_elapsed_time(&now, &start_time);

          fprintf(stderr, "fps = %3.2f\n", (double)(frame_count * 1000000) / (double)elapsed_usec);

          frame_count        = 0;
          start_time.tv_sec  = now.tv_sec;
          start_time.tv_nsec = now.tv_nsec;
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
