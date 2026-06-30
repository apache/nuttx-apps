/****************************************************************************
 * apps/games/NXDoom/pcsound/pcsound.c
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 *    PC speaker interface.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pcsound.h"
#include "pcsound_internal.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static pcsound_driver_t *g_drivers[] =
{
#ifdef HAVE_LINUX_KD_H
  &pcsound_linux_driver,
#endif
  NULL,
};

static pcsound_driver_t *g_pcsound_driver = NULL;

/****************************************************************************
 * Public Data
 ****************************************************************************/

int pcsound_sample_rate;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pc_sound_set_sample_rate(int rate)
{
  pcsound_sample_rate = rate;
}

int pc_sound_init(pcsound_callback_func callback_func)
{
  char *driver_name;
  int i;

  if (g_pcsound_driver != NULL)
    {
      return 1;
    }

  /* Check if the environment variable is set */

  driver_name = getenv("PCSOUND_DRIVER");

  if (driver_name != NULL)
    {
      for (i = 0; g_drivers[i] != NULL; ++i)
        {
          if (!strcmp(g_drivers[i]->name, driver_name))
            {
              /* Found the driver! */

              if (g_drivers[i]->init_func(callback_func))
                {
                  g_pcsound_driver = g_drivers[i];
                }
              else
                {
                  printf("Failed to initialize PC sound driver: %s\n",
                         g_drivers[i]->name);
                  break;
                }
            }
        }
    }
  else
    {
      /* Try all drivers until we find a working one */

      for (i = 0; g_drivers[i] != NULL; ++i)
        {
          if (g_drivers[i]->init_func(callback_func))
            {
              g_pcsound_driver = g_drivers[i];
              break;
            }
        }
    }

  if (g_pcsound_driver != NULL)
    {
      printf("Using PC sound driver: %s\n", g_pcsound_driver->name);
      return 1;
    }
  else
    {
      printf("Failed to find a working PC sound driver.\n");
      return 0;
    }
}

void pc_sound_shutdown(void)
{
  g_pcsound_driver->shutdown_func();
  g_pcsound_driver = NULL;
}
