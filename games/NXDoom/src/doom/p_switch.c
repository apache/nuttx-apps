/****************************************************************************
 * apps/games/NXDoom/src/doom/p_switch.c
 *
 * SPDX-License-Identifer: GPLv2
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
 *
 * DESCRIPTION:
 *  Switches, buttons. Two-state animation. Exits.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "deh_main.h"
#include "doomdef.h"
#include "i_system.h"
#include "p_local.h"

#include "g_game.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* State. */

#include "doomstat.h"
#include "r_state.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE */

static switchlist_t g_alph_switch_list[] =
{
  {"SW1BRCOM", "SW2BRCOM", 1}, /* Doom shareware episode 1 switches */
  {"SW1BRN1", "SW2BRN1", 1},
  {"SW1BRN2", "SW2BRN2", 1},
  {"SW1BRNGN", "SW2BRNGN", 1},
  {"SW1BROWN", "SW2BROWN", 1},
  {"SW1COMM", "SW2COMM", 1},
  {"SW1COMP", "SW2COMP", 1},
  {"SW1DIRT", "SW2DIRT", 1},
  {"SW1EXIT", "SW2EXIT", 1},
  {"SW1GRAY", "SW2GRAY", 1},
  {"SW1GRAY1", "SW2GRAY1", 1},
  {"SW1METAL", "SW2METAL", 1},
  {"SW1PIPE", "SW2PIPE", 1},
  {"SW1SLAD", "SW2SLAD", 1},
  {"SW1STARG", "SW2STARG", 1},
  {"SW1STON1", "SW2STON1", 1},
  {"SW1STON2", "SW2STON2", 1},
  {"SW1STONE", "SW2STONE", 1},
  {"SW1STRTN", "SW2STRTN", 1},
  {"SW1BLUE", "SW2BLUE", 2}, /* Doom registered episodes 2 & 3 switches */
  {"SW1CMT", "SW2CMT", 2},
  {"SW1GARG", "SW2GARG", 2},
  {"SW1GSTON", "SW2GSTON", 2},
  {"SW1HOT", "SW2HOT", 2},
  {"SW1LION", "SW2LION", 2},
  {"SW1SATYR", "SW2SATYR", 2},
  {"SW1SKIN", "SW2SKIN", 2},
  {"SW1VINE", "SW2VINE", 2},
  {"SW1WOOD", "SW2WOOD", 2},
  {"SW1PANEL", "SW2PANEL", 3}, /* Doom II switches */
  {"SW1ROCK", "SW2ROCK", 3},
  {"SW1MET2", "SW2MET2", 3},
  {"SW1WDMET", "SW2WDMET", 3},
  {"SW1BRIK", "SW2BRIK", 3},
  {"SW1MOD1", "SW2MOD1", 3},
  {"SW1ZIM", "SW2ZIM", 3},
  {"SW1STON6", "SW2STON6", 3},
  {"SW1TEK", "SW2TEK", 3},
  {"SW1MARB", "SW2MARB", 3},
  {"SW1SKULL", "SW2SKULL", 3},
};

static int g_numswitches;

/****************************************************************************
 * Public Data
 ****************************************************************************/

int switchlist[MAXSWITCHES * 2];
button_t buttonlist[MAXBUTTONS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Start a button counting down till it turns off. */

static void p_start_button(line_t *line, bwhere_e w, int texture, int time)
{
  int i;

  /* See if button is already pressed */

  for (i = 0; i < MAXBUTTONS; i++)
    {
      if (buttonlist[i].btimer && buttonlist[i].line == line)
        {
          return;
        }
    }

  for (i = 0; i < MAXBUTTONS; i++)
    {
      if (!buttonlist[i].btimer)
        {
          buttonlist[i].line = line;
          buttonlist[i].where = w;
          buttonlist[i].btexture = texture;
          buttonlist[i].btimer = time;
          buttonlist[i].soundorg = &line->frontsector->soundorg;
          return;
        }
    }

  i_error("P_StartButton: no button slots left!");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* p_init_switch_list
 * Only called at game initialization.
 */

void p_init_switch_list(void)
{
  int i;
  int slindex;
  int episode;

  /* Note that this is called "episode" here but it's actually something
   * quite different. As we progress from Shareware->Registered->Doom II
   * we support more switch textures.
   */

  switch (gamemode)
    {
    case registered:
    case retail:
      episode = 2;
      break;
    case commercial:
      episode = 3;
      break;
    default:
      episode = 1;
      break;
    }

  slindex = 0;

  for (i = 0; i < arrlen(g_alph_switch_list); i++)
    {
      if (g_alph_switch_list[i].episode <= episode)
        {
          switchlist[slindex++] =
              r_texture_num_for_name((g_alph_switch_list[i].name1));
          switchlist[slindex++] =
              r_texture_num_for_name((g_alph_switch_list[i].name2));
        }
    }

  g_numswitches = slindex / 2;
  switchlist[slindex] = -1;
}

/* Function that changes wall texture.
 * Tell it if switch is ok to use again (1=yes, it's a button).
 */

void p_change_switch_texture(line_t *line, int use_again)
{
  int tex_top;
  int tex_mid;
  int tex_bot;
  int i;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  int sound;
#endif

  if (!use_again) line->special = 0;

  tex_top = sides[line->sidenum[0]].toptexture;
  tex_mid = sides[line->sidenum[0]].midtexture;
  tex_bot = sides[line->sidenum[0]].bottomtexture;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  sound = SFX_SWTCHN;

  /* EXIT SWITCH? */

  if (line->special == 11) sound = SFX_SWTCHX;
#endif

  for (i = 0; i < g_numswitches * 2; i++)
    {
      if (switchlist[i] == tex_top)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(buttonlist->soundorg, sound);
#endif
          sides[line->sidenum[0]].toptexture = switchlist[i ^ 1];

          if (use_again)
            {
              p_start_button(line, top, switchlist[i], BUTTONTIME);
            }

          return;
        }
      else
        {
          if (switchlist[i] == tex_mid)
            {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(buttonlist->soundorg, sound);
#endif
              sides[line->sidenum[0]].midtexture = switchlist[i ^ 1];

              if (use_again)
                p_start_button(line, middle, switchlist[i], BUTTONTIME);

              return;
            }
          else
            {
              if (switchlist[i] == tex_bot)
                {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
                  s_start_sound(buttonlist->soundorg, sound);
#endif
                  sides[line->sidenum[0]].bottomtexture = switchlist[i ^ 1];

                  if (use_again)
                    p_start_button(line, bottom, switchlist[i], BUTTONTIME);

                  return;
                }
            }
        }
    }
}

/* p_use_special_line
 * Called when a thing uses a special line.
 * Only the front sides of lines are usable.
 */

boolean p_use_special_line(mobj_t *thing, line_t *line, int side)
{
  /* Err...
   * Use the back sides of VERY SPECIAL lines...
   */

  if (side)
    {
      switch (line->special)
        {
        case 124:

          /* Sliding door open & close
           * UNUSED?
           */

          break;

        default:
          return false;
          break;
        }
    }

  /* Switches that other things can activate. */

  if (!thing->player)
    {
      /* never open secret doors */

      if (line->flags & ML_SECRET) return false;

      switch (line->special)
        {
        case 1:  /* MANUAL DOOR RAISE */
        case 32: /* MANUAL BLUE */
        case 33: /* MANUAL RED */
        case 34: /* MANUAL YELLOW */
          break;

        default:
          return false;
          break;
        }
    }

  /* do something */

  switch (line->special)
    {
      /* MANUALS */

    case 1:  /* Vertical Door */
    case 26: /* Blue Door/Locked */
    case 27: /* Yellow Door /Locked */
    case 28: /* Red Door /Locked */

    case 31: /* Manual door open */
    case 32: /* Blue locked door open */
    case 33: /* Red locked door open */
    case 34: /* Yellow locked door open */

    case 117: /* Blazing door raise */
    case 118: /* Blazing door open */
      ev_vertical_door(line, thing);
      break;

      /* UNUSED - Door Slide Open&Close
       *  case 124:
       *  ev_sliding_door (line, thing);
       *  break;
       */

      /* SWITCHES */

    case 7: /* Build Stairs */
      if (ev_build_stairs(line, STAIR_BUILD8))
        p_change_switch_texture(line, 0);
      break;

    case 9: /* Change Donut */
      if (ev_do_donut(line)) p_change_switch_texture(line, 0);
      break;

    case 11: /* Exit level */
      p_change_switch_texture(line, 0);
      g_exit_level();
      break;

    case 14: /* Raise Floor 32 and change texture */
      if (ev_do_plat(line, PLAT_RAISEANDCHANGE, 32))
        p_change_switch_texture(line, 0);
      break;

    case 15: /* Raise Floor 24 and change texture */
      if (ev_do_plat(line, PLAT_RAISEANDCHANGE, 24))
        p_change_switch_texture(line, 0);
      break;

    case 18: /* Raise Floor to next highest floor */
      if (ev_do_floor(line, FLOOR_RAISEFLOORTONEAREST))
        p_change_switch_texture(line, 0);
      break;

    case 20: /* Raise Plat next highest floor and change texture */
      if (ev_do_plat(line, PLAT_RAISETONEARESTANDCHANGE, 0))
        p_change_switch_texture(line, 0);
      break;

    case 21: /* PlatDownWaitUpStay */
      if (ev_do_plat(line, PLAT_DOWNWAITUPSTAY, 0))
        p_change_switch_texture(line, 0);
      break;

    case 23: /* Lower Floor to Lowest */
      if (ev_do_floor(line, FLOOR_LOWERFLOORTOLOWEST))
        p_change_switch_texture(line, 0);
      break;

    case 29: /* Raise Door */
      if (ev_do_door(line, VLD_NORMAL)) p_change_switch_texture(line, 0);
      break;

    case 41: /* Lower Ceiling to Floor */
      if (ev_do_ceiling(line, CEIL_LOWERTOFLOOR))
        p_change_switch_texture(line, 0);
      break;

    case 71: /* Turbo Lower Floor */
      if (ev_do_floor(line, FLOOR_TURBOLOWER))
        p_change_switch_texture(line, 0);
      break;

    case 49: /* Ceiling Crush And Raise */
      if (ev_do_ceiling(line, CEIL_CRUSHANDRAISE))
        p_change_switch_texture(line, 0);
      break;

    case 50: /* Close Door */
      if (ev_do_door(line, VLD_CLOSE)) p_change_switch_texture(line, 0);
      break;

    case 51: /* Secret EXIT */
      p_change_switch_texture(line, 0);
      g_secret_exit_level();
      break;

    case 55: /* Raise Floor Crush */
      if (ev_do_floor(line, FLOOR_RAISEFLOORCRUSH))
        p_change_switch_texture(line, 0);
      break;

    case 101: /* Raise Floor */
      if (ev_do_floor(line, FLOOR_RAISEFLOOR))
        p_change_switch_texture(line, 0);
      break;

    case 102: /* Lower Floor to Surrounding floor height */
      if (ev_do_floor(line, FLOOR_LOWERFLOOR))
        p_change_switch_texture(line, 0);
      break;

    case 103: /* Open Door */
      if (ev_do_door(line, VLD_OPEN)) p_change_switch_texture(line, 0);
      break;

    case 111: /* Blazing Door Raise (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZERAISE)) p_change_switch_texture(line, 0);
      break;

    case 112: /* Blazing Door Open (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZEOPEN)) p_change_switch_texture(line, 0);
      break;

    case 113: /* Blazing Door Close (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZECLOSE)) p_change_switch_texture(line, 0);
      break;

    case 122: /* Blazing PlatDownWaitUpStay */
      if (ev_do_plat(line, PLAT_BLAZEDWUS, 0))
        p_change_switch_texture(line, 0);
      break;

    case 127: /* Build Stairs Turbo 16 */
      if (ev_build_stairs(line, STAIR_TURBO16))
        p_change_switch_texture(line, 0);
      break;

    case 131: /* Raise Floor Turbo */
      if (ev_do_floor(line, FLOOR_RAISEFLOORTURBO))
        p_change_switch_texture(line, 0);
      break;

    case 133: /* BlzOpenDoor BLUE */
    case 135: /* BlzOpenDoor RED */
    case 137: /* BlzOpenDoor YELLOW */
      if (ev_do_locked_door(line, VLD_BLAZEOPEN, thing))
        p_change_switch_texture(line, 0);
      break;

    case 140: /* Raise Floor 512 */
      if (ev_do_floor(line, FLOOR_RAISEFLOOR512))
        p_change_switch_texture(line, 0);
      break;

      /* BUTTONS */

    case 42: /* Close Door */
      if (ev_do_door(line, VLD_CLOSE)) p_change_switch_texture(line, 1);
      break;

    case 43: /* Lower Ceiling to Floor */
      if (ev_do_ceiling(line, CEIL_LOWERTOFLOOR))
        p_change_switch_texture(line, 1);
      break;

    case 45: /* Lower Floor to Surrounding floor height */
      if (ev_do_floor(line, FLOOR_LOWERFLOOR))
        p_change_switch_texture(line, 1);
      break;

    case 60: /* Lower Floor to Lowest */
      if (ev_do_floor(line, FLOOR_LOWERFLOORTOLOWEST))
        p_change_switch_texture(line, 1);
      break;

    case 61: /* Open Door */
      if (ev_do_door(line, VLD_OPEN)) p_change_switch_texture(line, 1);
      break;

    case 62: /* PlatDownWaitUpStay */
      if (ev_do_plat(line, PLAT_DOWNWAITUPSTAY, 1))
        p_change_switch_texture(line, 1);
      break;

    case 63: /* Raise Door */
      if (ev_do_door(line, VLD_NORMAL)) p_change_switch_texture(line, 1);
      break;

    case 64: /* Raise Floor to ceiling */
      if (ev_do_floor(line, FLOOR_RAISEFLOOR))
        p_change_switch_texture(line, 1);
      break;

    case 66: /* Raise Floor 24 and change texture */
      if (ev_do_plat(line, PLAT_RAISEANDCHANGE, 24))
        p_change_switch_texture(line, 1);
      break;

    case 67: /* Raise Floor 32 and change texture */
      if (ev_do_plat(line, PLAT_RAISEANDCHANGE, 32))
        p_change_switch_texture(line, 1);
      break;

    case 65: /* Raise Floor Crush */
      if (ev_do_floor(line, FLOOR_RAISEFLOORCRUSH))
        p_change_switch_texture(line, 1);
      break;

    case 68: /* Raise Plat to next highest floor and change texture */
      if (ev_do_plat(line, PLAT_RAISETONEARESTANDCHANGE, 0))
        p_change_switch_texture(line, 1);
      break;

    case 69: /* Raise Floor to next highest floor */
      if (ev_do_floor(line, FLOOR_RAISEFLOORTONEAREST))
        p_change_switch_texture(line, 1);
      break;

    case 70: /* Turbo Lower Floor */
      if (ev_do_floor(line, FLOOR_TURBOLOWER))
        p_change_switch_texture(line, 1);
      break;

    case 114: /* Blazing Door Raise (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZERAISE)) p_change_switch_texture(line, 1);
      break;

    case 115: /* Blazing Door Open (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZEOPEN)) p_change_switch_texture(line, 1);
      break;

    case 116: /* Blazing Door Close (faster than TURBO!) */
      if (ev_do_door(line, VLD_BLAZECLOSE)) p_change_switch_texture(line, 1);
      break;

    case 123: /* Blazing PlatDownWaitUpStay */
      if (ev_do_plat(line, PLAT_BLAZEDWUS, 0))
        p_change_switch_texture(line, 1);
      break;

    case 132: /* Raise Floor Turbo */
      if (ev_do_floor(line, FLOOR_RAISEFLOORTURBO))
        p_change_switch_texture(line, 1);
      break;

    case 99:  /* BlzOpenDoor BLUE */
    case 134: /* BlzOpenDoor RED */
    case 136: /* BlzOpenDoor YELLOW */
      if (ev_do_locked_door(line, VLD_BLAZEOPEN, thing))
        p_change_switch_texture(line, 1);
      break;

    case 138:

      /* Light Turn On */

      ev_light_turn_on(line, 255);
      p_change_switch_texture(line, 1);
      break;

    case 139:

      /* Light Turn Off */

      ev_light_turn_on(line, 35);
      p_change_switch_texture(line, 1);
      break;
    }

  return true;
}
