/****************************************************************************
 * apps/games/NXDoom/src/i_timer.c
 *
 * SPDX-License-Identifier: GPLv2
 *
 * Copyright(C) 1993-1996 Id Software, Inc.
 * Copyright(C) 2005-2014 Simon Howard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * DESCRIPTION:
 *      Timer functions.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/clock.h>
#include <unistd.h>

#include "doomtype.h"
#include "i_timer.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The start time of the game used as the base for tic calculations. */

static struct timespec basetime =
{
  .tv_nsec = 0, .tv_sec = 0
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i_get_time
 *
 * Description:
 *  Called by D_DoomLoop.
 *
 * Returns:
 *  The current time in 1/35th second tics.
 ****************************************************************************/

int i_get_time(void)
{
  return (i_get_time_ms() * TICRATE) / 1000;
}

/****************************************************************************
 * Name: i_get_time_ms
 *
 * Returns:
 *  The current time in ms.
 ****************************************************************************/

int i_get_time_ms(void)
{
  struct timespec curtime;

  /* NOTE: we ignore any possible error here */

  clock_gettime(CLOCK_MONOTONIC, &curtime);
  if (basetime.tv_sec == 0 && basetime.tv_nsec == 0)
    {
      clock_gettime(CLOCK_MONOTONIC, &basetime);
    }

  return (clock_time2usec(&curtime) - clock_time2usec(&basetime)) / 1000;
}

/****************************************************************************
 * Name: i_init_timer
 *
 * Description:
 *  Initialize timer.
 ****************************************************************************/

void i_init_timer(void)
{
}

/****************************************************************************
 * Name: i_wait_vbl
 *
 * Description:
 *   Wait for vertical retrace or pause a bit.
 ****************************************************************************/

void i_wait_vbl(int count)
{
  usleep((count * 1000000) / 70);
}
