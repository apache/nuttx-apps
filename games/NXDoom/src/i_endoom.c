/****************************************************************************
 * apps/games/NXDoom/src/i_endoom.c
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
 *    Exit text-mode ENDOOM screen.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "doomtype.h"
#include "i_video.h"

#include "txt_main.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ENDOOM_W 80
#define ENDOOM_H 25

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: i_endoom
 *
 * Description:
 *  Displays the text mode ending screen after the game quits
 *
 ****************************************************************************/

void i_endoom(byte *endoom_data)
{
  unsigned char *screendata;
  int y;
  int indent;

  /* Set up text mode screen */

  txt_init();

  txt_set_window_title(PACKAGE_STRING);

  /* SDL2-TODO i_init_window_title();
   */

  /* Write the data to the screen memory */

  screendata = txt_get_screen_data();

  indent = (ENDOOM_W - TXT_SCREEN_W) / 2;

  for (y = 0; y < TXT_SCREEN_H; ++y)
    {
      memcpy(screendata + (y * TXT_SCREEN_W * 2),
             endoom_data + (y * ENDOOM_W + indent) * 2, TXT_SCREEN_W * 2);
    }

  /* Wait for a keypress */

  while (true)
    {
      txt_update_screen();

      if (txt_getchar() > 0)
        {
          break;
        }

      txt_sleep(0);
    }

  /* Shut down text mode screen */

  txt_shutdown();
}
