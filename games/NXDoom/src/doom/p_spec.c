/****************************************************************************
 * apps/games/NXDoom/src/doom/p_spec.c
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
 * DESCRIPTION:
 *  Implements special effects:
 *  Texture animation, height or lighting changes according to adjacent
 *  sectors, respective utility functions, etc.
 *  Line Tag handling. Line and Sector triggers.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "deh_main.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"
#include "z_zone.h"

#include "p_local.h"
#include "r_local.h"

#include "g_game.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* State. */

#include "r_state.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAXANIMS 32

/* Animating line specials */

#define MAXLINEANIMS 64

/* version <= 1.2 did not have a limit and could handle up to 66 scrolling
 * linedefs before displaying adverse effects. All other versions have a
 * limit of 64.
 */

#define MAXLINEANIMS1_2 66

/* Thanks to entryway for the Vanilla overflow emulation.
 * 20 adjoining sectors max!
 */

#define MAX_ADJOINING_SECTORS 20

/* Donut overrun emulation
 *
 * Derived from the code from PrBoom+.  Thanks go to Andrey Budko (entryway)
 * as usual :-)
 */

#define DONUT_FLOORHEIGHT_DEFAULT 0x00000000
#define DONUT_FLOORPIC_DEFAULT 0x16

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Animating textures and planes
 * There is another anim_t used in wi_stuff, unrelated.
 */

typedef struct
{
  boolean istexture;
  int picnum;
  int basepic;
  int numpics;
  int speed;
} anim_t;

/* source animation definition */

typedef struct
{
  int istexture; /* if false, it is a flat */
  char endname[9];
  char startname[9];
  int speed;
} animdef_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* p_init_pic_anims */

/* Floor/ceiling animation sequences, defined by first and last frame, i.e.
 * the flat (64x64 tile) name to be used.
 *
 * The full animation sequence is given using all the flats between the start
 * and end entry, in the order found in the WAD file.
 */

static animdef_t g_animdefs[] =
{
  {false, "NUKAGE3", "NUKAGE1", 8},
  {false, "FWATER4", "FWATER1", 8},
  {false, "SWATER4", "SWATER1", 8},
  {false, "LAVA4", "LAVA1", 8},
  {false, "BLOOD3", "BLOOD1", 8},
  {false, "RROCK08", "RROCK05", 8}, /* DOOM II flat animations. */
  {false, "SLIME04", "SLIME01", 8},
  {false, "SLIME08", "SLIME05", 8},
  {false, "SLIME12", "SLIME09", 8},
  {true, "BLODGR4", "BLODGR1", 8},
  {true, "SLADRIP3", "SLADRIP1", 8},
  {true, "BLODRIP4", "BLODRIP1", 8},
  {true, "FIREWALL", "FIREWALA", 8},
  {true, "GSTFONT3", "GSTFONT1", 8},
  {true, "FIRELAVA", "FIRELAV3", 8},
  {true, "FIREMAG3", "FIREMAG1", 8},
  {true, "FIREBLU2", "FIREBLU1", 8},
  {true, "ROCKRED3", "ROCKRED1", 8},
  {true, "BFALL4", "BFALL1", 8},
  {true, "SFALL4", "SFALL1", 8},
  {true, "WFALL4", "WFALL1", 8},
  {true, "DBRAIN4", "DBRAIN1", 8},
  { -1, "", "", 0},
};

static anim_t g_anims[MAXANIMS];
static anim_t *g_lastanim;

static short g_numlinespecials;
static line_t *g_linespeciallist[MAXLINEANIMS1_2];

/* p_update_specials
 * Animate planes, scroll walls, etc.
 */

static boolean g_level_timer;
static int g_level_time_count;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void donut_overrun(fixed_t *s3_floorheight, short *s3_floorpic,
                          line_t *line, sector_t *pillar_sector)
{
  static int first = 1;
  static int tmp_s3_floorheight;
  static int tmp_s3_floorpic;

  if (first)
    {
      int p;

      /* This is the first time we have had an overrun. */

      first = 0;

      /* Default values */

      tmp_s3_floorheight = DONUT_FLOORHEIGHT_DEFAULT;
      tmp_s3_floorpic = DONUT_FLOORPIC_DEFAULT;

      /* @category compat
       * @arg <x> <y>
       *
       * Use the specified magic values when emulating behavior caused
       * by memory overruns from improperly constructed donuts.
       * In Vanilla Doom this can differ depending on the operating
       * system.  The default (if this option is not specified) is to
       * emulate the behavior when running under Windows 98.
       */

      p = m_check_parm_with_args("-donut", 2);

      if (p > 0)
        {
          /* Dump of needed memory: (fixed_t)0000:0000 and (short)0000:0008
           *
           * C:\>debug
           * -d 0:0
           *
           * DOS 6.22:
           * 0000:0000    (57 92 19 00) F4 06 70 00-(16 00)
           * DOS 7.1:
           * 0000:0000    (9E 0F C9 00) 65 04 70 00-(16 00)
           * Win98:
           * 0000:0000    (00 00 00 00) 65 04 70 00-(16 00)
           * DOSBox under XP:
           * 0000:0000    (00 00 00 F1) ?? ?? ?? 00-(07 00)
           */

          m_str_to_int(myargv[p + 1], &tmp_s3_floorheight);
          m_str_to_int(myargv[p + 2], &tmp_s3_floorpic);

          if (tmp_s3_floorpic >= numflats)
            {
              fprintf(stderr,
                      "DonutOverrun: The second parameter for \"-donut\" "
                      "switch should be greater than 0 and less than number "
                      "of flats (%d). Using default value (%d) instead. \n",
                      numflats, DONUT_FLOORPIC_DEFAULT);
              tmp_s3_floorpic = DONUT_FLOORPIC_DEFAULT;
            }
        }
    }

  *s3_floorheight = (fixed_t)tmp_s3_floorheight;
  *s3_floorpic = (short)tmp_s3_floorpic;
}

/* p_spawn_specials
 * After the map has been loaded, scan for specials that spawn thinkers
 */

static unsigned int num_scrollers(void)
{
  unsigned int i;
  unsigned int scrollers = 0;

  for (i = 0; i < numlines; i++)
    {
      if (48 == lines[i].special)
        {
          scrollers++;
        }
    }

  return scrollers;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void p_init_pic_anims(void)
{
  int i;

  /* Init animation */

  g_lastanim = g_anims;
  for (i = 0; g_animdefs[i].istexture != -1; i++)
    {
      const char *startname;
      const char *endname;

      startname = (g_animdefs[i].startname);
      endname = (g_animdefs[i].endname);

      if (g_animdefs[i].istexture)
        {
          /* different episode ? */

          if (r_check_texture_num_for_name(startname) == -1) continue;

          g_lastanim->picnum = r_texture_num_for_name(endname);
          g_lastanim->basepic = r_texture_num_for_name(startname);
        }
      else
        {
          if (w_check_num_for_name(startname) == -1) continue;

          g_lastanim->picnum = r_flat_num_for_name(endname);
          g_lastanim->basepic = r_flat_num_for_name(startname);
        }

      g_lastanim->istexture = g_animdefs[i].istexture;
      g_lastanim->numpics = g_lastanim->picnum - g_lastanim->basepic + 1;

      if (g_lastanim->numpics < 2)
        i_error("p_init_pic_anims: bad cycle from %s to %s", startname,
                endname);

      g_lastanim->speed = g_animdefs[i].speed;
      g_lastanim++;
    }
}

/* UTILITIES */

/* get_side()
 * Will return a side_t* given the number of the current sector, the line
 * number, and the side (0/1) that you want.
 */

side_t *get_side(int current_sector, int line, int side)
{
  return &sides[(sectors[current_sector].lines[line])->sidenum[side]];
}

/* get_sector()
 * Will return a sector_t* given the number of the current sector, the line
 * number and the side (0/1) that you want.
 */

sector_t *get_sector(int current_sector, int line, int side)
{
  return sides[(sectors[current_sector].lines[line])->sidenum[side]].sector;
}

/* two_sided()
 * Given the sector number and the line number, it will tell you whether the
 * line is two-sided or not.
 */

int two_sided(int sector, int line)
{
  return (sectors[sector].lines[line])->flags & ML_TWOSIDED;
}

/* get_next_sector()
 * Return sector_t * of sector next to current. NULL if not two-sided line
 */

sector_t *get_next_sector(line_t *line, sector_t *sec)
{
  if (!(line->flags & ML_TWOSIDED)) return NULL;

  if (line->frontsector == sec) return line->backsector;

  return line->frontsector;
}

/* p_find_lowest_floor_surrounding()
 * FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
 */

fixed_t p_find_lowest_floor_surrounding(sector_t *sec)
{
  int i;
  line_t *check;
  sector_t *other;
  fixed_t floor = sec->floorheight;

  for (i = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      other = get_next_sector(check, sec);

      if (!other) continue;

      if (other->floorheight < floor) floor = other->floorheight;
    }

  return floor;
}

/* p_find_highest_floor_surrounding()
 * FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
 */

fixed_t p_find_highest_floor_surrounding(sector_t *sec)
{
  int i;
  line_t *check;
  sector_t *other;
  fixed_t floor = -500 * FRACUNIT;

  for (i = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      other = get_next_sector(check, sec);

      if (!other) continue;

      if (other->floorheight > floor) floor = other->floorheight;
    }

  return floor;
}

/* p_find_next_highest_floor
 * FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
 * Note: this should be doable w/o a fixed array.
 */

fixed_t p_find_next_highest_floor(sector_t *sec, int currentheight)
{
  int i;
  int h;
  int min;
  line_t *check;
  sector_t *other;
  fixed_t height = currentheight;
  fixed_t heightlist[MAX_ADJOINING_SECTORS + 2];

  for (i = 0, h = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      other = get_next_sector(check, sec);

      if (!other) continue;

      if (other->floorheight > height)
        {
          /* Emulation of memory (stack) overflow */

          if (h == MAX_ADJOINING_SECTORS + 1)
            {
              height = other->floorheight;
            }
          else if (h == MAX_ADJOINING_SECTORS + 2)
            {
              /* Fatal overflow: game crashes at 22 sectors */

              i_error("Sector with more than 22 adjoining sectors. "
                      "Vanilla will crash here");
            }

          heightlist[h++] = other->floorheight;
        }
    }

  /* Find lowest height in list */

  if (!h)
    {
      return currentheight;
    }

  min = heightlist[0];

  /* Range checking? */

  for (i = 1; i < h; i++)
    {
      if (heightlist[i] < min)
        {
          min = heightlist[i];
        }
    }

  return min;
}

/* FIND LOWEST CEILING IN THE SURROUNDING SECTORS */

fixed_t p_find_lowest_ceiling_surrounding(sector_t *sec)
{
  int i;
  line_t *check;
  sector_t *other;
  fixed_t height = INT_MAX;

  for (i = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      other = get_next_sector(check, sec);

      if (!other) continue;

      if (other->ceilingheight < height) height = other->ceilingheight;
    }

  return height;
}

/* FIND HIGHEST CEILING IN THE SURROUNDING SECTORS */

fixed_t p_find_heighest_ceiling_surrounding(sector_t *sec)
{
  int i;
  line_t *check;
  sector_t *other;
  fixed_t height = 0;

  for (i = 0; i < sec->linecount; i++)
    {
      check = sec->lines[i];
      other = get_next_sector(check, sec);

      if (!other) continue;

      if (other->ceilingheight > height) height = other->ceilingheight;
    }

  return height;
}

/* RETURN NEXT SECTOR # THAT LINE TAG REFERS TO */

int p_find_sector_from_line_tag(line_t *line, int start)
{
  int i;

  for (i = start + 1; i < numsectors; i++)
    {
      if (sectors[i].tag == line->tag)
        {
          return i;
        }
    }

  return -1;
}

/* Find minimum light from an adjacent sector */

int p_find_min_surrounding(sector_t *sector, int max)
{
  int i;
  int min;
  line_t *line;
  sector_t *check;

  min = max;
  for (i = 0; i < sector->linecount; i++)
    {
      line = sector->lines[i];
      check = get_next_sector(line, sector);

      if (!check) continue;

      if (check->lightlevel < min) min = check->lightlevel;
    }

  return min;
}

/* EVENTS
 * Events are operations triggered by using, crossing,
 * or shooting special lines, or by timed thinkers.
 */

/* p_cross_special_line - TRIGGER
 * Called every time a thing origin is about to cross a line with a non 0
 * special.
 */

void p_cross_special_line(int linenum, int side, mobj_t *thing)
{
  line_t *line;
  int is_ok;

  line = &lines[linenum];

  if (gameversion <= exe_doom_1_2)
    {
      if (line->special > 98 && line->special != 104)
        {
          return;
        }
    }
  else
    {
      /* Triggers that other things can activate */

      if (!thing->player)
        {
          /* Things that should NOT trigger specials... */

          switch (thing->type)
            {
            case MT_ROCKET:
            case MT_PLASMA:
            case MT_BFG:
            case MT_TROOPSHOT:
            case MT_HEADSHOT:
            case MT_BRUISERSHOT:
              return;

            default:
              break;
            }
        }
    }

  if (!thing->player)
    {
      is_ok = 0;
      switch (line->special)
        {
        case 39:  /* TELEPORT TRIGGER */
        case 97:  /* TELEPORT RETRIGGER */
        case 125: /* TELEPORT MONSTERONLY TRIGGER */
        case 126: /* TELEPORT MONSTERONLY RETRIGGER */
        case 4:   /* RAISE DOOR */
        case 10:  /* PLAT DOWN-WAIT-UP-STAY TRIGGER */
        case 88:  /* PLAT DOWN-WAIT-UP-STAY RETRIGGER */
          is_ok = 1;
          break;
        }

      if (!is_ok) return;
    }

  /* Note: could use some const's here. */

  switch (line->special)
    {
      /* TRIGGERS.
       * All from here to RETRIGGERS.
       */

    case 2: /* Open Door */
      ev_do_door(line, VLD_OPEN);
      line->special = 0;
      break;

    case 3: /* Close Door */
      ev_do_door(line, VLD_CLOSE);
      line->special = 0;
      break;

    case 4: /* Raise Door */
      ev_do_door(line, VLD_NORMAL);
      line->special = 0;
      break;

    case 5: /* Raise Floor */
      ev_do_floor(line, FLOOR_RAISEFLOOR);
      line->special = 0;
      break;

    case 6: /* Fast Ceiling Crush & Raise */
      ev_do_ceiling(line, CEIL_FASTCRUSHANDRAISE);
      line->special = 0;
      break;

    case 8: /* Build Stairs */
      ev_build_stairs(line, STAIR_BUILD8);
      line->special = 0;
      break;

    case 10: /* PlatDownWaitUp */
      ev_do_plat(line, PLAT_DOWNWAITUPSTAY, 0);
      line->special = 0;
      break;

    case 12: /* Light Turn On - brightest near */
      ev_light_turn_on(line, 0);
      line->special = 0;
      break;

    case 13: /* Light Turn On 255 */
      ev_light_turn_on(line, 255);
      line->special = 0;
      break;

    case 16: /* Close Door 30 */
      ev_do_door(line, VLD_CLOSE30THENOPEN);
      line->special = 0;
      break;

    case 17: /* Start Light Strobing */
      ev_start_light_strobing(line);
      line->special = 0;
      break;

    case 19: /* Lower Floor */
      ev_do_floor(line, FLOOR_LOWERFLOOR);
      line->special = 0;
      break;

    case 22: /* Raise floor to nearest height and change texture */
      ev_do_plat(line, PLAT_RAISETONEARESTANDCHANGE, 0);
      line->special = 0;
      break;

    case 25: /* Ceiling Crush and Raise */
      ev_do_ceiling(line, CEIL_CRUSHANDRAISE);
      line->special = 0;
      break;

    case 30:

      /* Raise floor to shortest texture height on either side of lines. */

      ev_do_floor(line, FLOOR_RAISETOTEXTURE);
      line->special = 0;
      break;

    case 35: /* Lights Very Dark */
      ev_light_turn_on(line, 35);
      line->special = 0;
      break;

    case 36: /* Lower Floor (TURBO) */
      ev_do_floor(line, FLOOR_TURBOLOWER);
      line->special = 0;
      break;

    case 37: /* LowerAndChange */
      ev_do_floor(line, FLOOR_LOWERANDCHANGE);
      line->special = 0;
      break;

    case 38: /* Lower Floor To Lowest */
      ev_do_floor(line, FLOOR_LOWERFLOORTOLOWEST);
      line->special = 0;
      break;

    case 39: /* TELEPORT! */
      ev_teleport(line, side, thing);
      line->special = 0;
      break;

    case 40: /* RaiseCeilingLowerFloor */
      ev_do_ceiling(line, CEIL_RAISETOHIGHEST);
      ev_do_floor(line, FLOOR_LOWERFLOORTOLOWEST);
      line->special = 0;
      break;

    case 44: /* Ceiling Crush */
      ev_do_ceiling(line, CEIL_LOWERANDCRUSH);
      line->special = 0;
      break;

    case 52: /* EXIT! */
      g_exit_level();
      break;

    case 53: /* Perpetual Platform Raise */
      ev_do_plat(line, PLAT_PERPETUALRAISE, 0);
      line->special = 0;
      break;

    case 54: /* Platform Stop */
      ev_stop_plat(line);
      line->special = 0;
      break;

    case 56: /* Raise Floor Crush */
      ev_do_floor(line, FLOOR_RAISEFLOORCRUSH);
      line->special = 0;
      break;

    case 57: /* Ceiling Crush Stop */
      ev_ceiling_crush_stop(line);
      line->special = 0;
      break;

    case 58: /* Raise Floor 24 */
      ev_do_floor(line, FLOOR_RAISEFLOOR24);
      line->special = 0;
      break;

    case 59: /* Raise Floor 24 And Change */
      ev_do_floor(line, FLOOR_RAISEFLOOR24ANDCHANGE);
      line->special = 0;
      break;

    case 104: /* Turn lights off in sector(tag) */
      ev_turn_tag_lights_off(line);
      line->special = 0;
      break;

    case 108: /* Blazing Door Raise (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZERAISE);
      line->special = 0;
      break;

    case 109: /* Blazing Door Open (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZEOPEN);
      line->special = 0;
      break;

    case 100: /* Build Stairs Turbo 16 */
      ev_build_stairs(line, STAIR_TURBO16);
      line->special = 0;
      break;

    case 110: /* Blazing Door Close (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZECLOSE);
      line->special = 0;
      break;

    case 119: /* Raise floor to nearest surr. floor */
      ev_do_floor(line, FLOOR_RAISEFLOORTONEAREST);
      line->special = 0;
      break;

    case 121: /* Blazing PlatDownWaitUpStay */
      ev_do_plat(line, PLAT_BLAZEDWUS, 0);
      line->special = 0;
      break;

    case 124: /* Secret EXIT */
      g_secret_exit_level();
      break;

    case 125: /* TELEPORT MonsterONLY */
      if (!thing->player)
        {
          ev_teleport(line, side, thing);
          line->special = 0;
        }
      break;

    case 130: /* Raise Floor Turbo */
      ev_do_floor(line, FLOOR_RAISEFLOORTURBO);
      line->special = 0;
      break;

    case 141: /* Silent Ceiling Crush & Raise */
      ev_do_ceiling(line, CEIL_SILENTCRUSHANDRAISE);
      line->special = 0;
      break;

      /* RETRIGGERS.  All from here till end. */

    case 72: /* Ceiling Crush */
      ev_do_ceiling(line, CEIL_LOWERANDCRUSH);
      break;

    case 73: /* Ceiling Crush and Raise */
      ev_do_ceiling(line, CEIL_CRUSHANDRAISE);
      break;

    case 74: /* Ceiling Crush Stop */
      ev_ceiling_crush_stop(line);
      break;

    case 75: /* Close Door */
      ev_do_door(line, VLD_CLOSE);
      break;

    case 76: /* Close Door 30 */
      ev_do_door(line, VLD_CLOSE30THENOPEN);
      break;

    case 77: /* Fast Ceiling Crush & Raise */
      ev_do_ceiling(line, CEIL_FASTCRUSHANDRAISE);
      break;

    case 79: /* Lights Very Dark */
      ev_light_turn_on(line, 35);
      break;

    case 80: /* Light Turn On - brightest near */
      ev_light_turn_on(line, 0);
      break;

    case 81: /* Light Turn On 255 */
      ev_light_turn_on(line, 255);
      break;

    case 82: /* Lower Floor To Lowest */
      ev_do_floor(line, FLOOR_LOWERFLOORTOLOWEST);
      break;

    case 83: /* Lower Floor */
      ev_do_floor(line, FLOOR_LOWERFLOOR);
      break;

    case 84: /* LowerAndChange */
      ev_do_floor(line, FLOOR_LOWERANDCHANGE);
      break;

    case 86: /* Open Door */
      ev_do_door(line, VLD_OPEN);
      break;

    case 87: /* Perpetual Platform Raise */
      ev_do_plat(line, PLAT_PERPETUALRAISE, 0);
      break;

    case 88: /* PlatDownWaitUp */
      ev_do_plat(line, PLAT_DOWNWAITUPSTAY, 0);
      break;

    case 89: /* Platform Stop */
      ev_stop_plat(line);
      break;

    case 90: /* Raise Door */
      ev_do_door(line, VLD_NORMAL);
      break;

    case 91: /* Raise Floor */
      ev_do_floor(line, FLOOR_RAISEFLOOR);
      break;

    case 92: /* Raise Floor 24 */
      ev_do_floor(line, FLOOR_RAISEFLOOR24);
      break;

    case 93: /* Raise Floor 24 And Change */
      ev_do_floor(line, FLOOR_RAISEFLOOR24ANDCHANGE);
      break;

    case 94: /* Raise Floor Crush */
      ev_do_floor(line, FLOOR_RAISEFLOORCRUSH);
      break;

    case 95: /* Raise floor to nearest height and change texture. */
      ev_do_plat(line, PLAT_RAISETONEARESTANDCHANGE, 0);
      break;

    case 96:

      /* Raise floor to shortest texture height on either side of lines. */

      ev_do_floor(line, FLOOR_RAISETOTEXTURE);
      break;

    case 97: /* TELEPORT! */
      ev_teleport(line, side, thing);
      break;

    case 98: /* Lower Floor (TURBO) */
      ev_do_floor(line, FLOOR_TURBOLOWER);
      break;

    case 105: /* Blazing Door Raise (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZERAISE);
      break;

    case 106: /* Blazing Door Open (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZEOPEN);
      break;

    case 107: /* Blazing Door Close (faster than TURBO!) */
      ev_do_door(line, VLD_BLAZECLOSE);
      break;

    case 120: /* Blazing PlatDownWaitUpStay. */
      ev_do_plat(line, PLAT_BLAZEDWUS, 0);
      break;

    case 126: /* TELEPORT MonsterONLY. */
      if (!thing->player) ev_teleport(line, side, thing);
      break;

    case 128: /* Raise To Nearest Floor */
      ev_do_floor(line, FLOOR_RAISEFLOORTONEAREST);
      break;

    case 129: /* Raise Floor Turbo */
      ev_do_floor(line, FLOOR_RAISEFLOORTURBO);
      break;
    }
}

/* p_shoot_special_line - IMPACT SPECIALS
 * Called when a thing shoots a special line.
 */

void p_shoot_special_line(mobj_t *thing, line_t *line)
{
  int is_ok;

  /* Impacts that other things can activate. */

  if (!thing->player)
    {
      is_ok = 0;
      switch (line->special)
        {
        case 46: /* OPEN DOOR IMPACT */
          is_ok = 1;
          break;
        }

      if (!is_ok) return;
    }

  switch (line->special)
    {
    case 24: /* RAISE FLOOR */
      ev_do_floor(line, FLOOR_RAISEFLOOR);
      p_change_switch_texture(line, 0);
      break;

    case 46: /* OPEN DOOR */
      ev_do_door(line, VLD_OPEN);
      p_change_switch_texture(line, 1);
      break;

    case 47: /* RAISE FLOOR NEAR AND CHANGE */
      ev_do_plat(line, PLAT_RAISETONEARESTANDCHANGE, 0);
      p_change_switch_texture(line, 0);
      break;
    }
}

/* p_player_in_special_sector
 * Called every tic frame that the player origin is in a special sector
 */

void p_player_in_special_sector(player_t *player)
{
  sector_t *sector;

  sector = player->mo->subsector->sector;

  /* Falling, not all the way down yet? */

  if (player->mo->z != sector->floorheight) return;

  /* Has hitten ground. */

  switch (sector->special)
    {
    case 5: /* HELLSLIME DAMAGE */
      if (!player->powers[pw_ironfeet])
        {
          if (!(leveltime & 0x1f)) p_damage_mobj(player->mo, NULL, NULL, 10);
        }

      break;

    case 7: /* NUKAGE DAMAGE */
      if (!player->powers[pw_ironfeet])
        {
          if (!(leveltime & 0x1f)) p_damage_mobj(player->mo, NULL, NULL, 5);
        }

      break;

    case 16: /* SUPER HELLSLIME DAMAGE */
    case 4:  /* STROBE HURT */
      if (!player->powers[pw_ironfeet] || (p_random() < 5))
        {
          if (!(leveltime & 0x1f)) p_damage_mobj(player->mo, NULL, NULL, 20);
        }

      break;

    case 9: /* SECRET SECTOR */
      player->secretcount++;
      sector->special = 0;
      break;

    case 11: /* EXIT SUPER DAMAGE! (for E1M8 finale) */
      player->cheats &= ~CF_GODMODE;

      if (!(leveltime & 0x1f)) p_damage_mobj(player->mo, NULL, NULL, 20);

      if (player->health <= 10) g_exit_level();
      break;

    default:
      i_error("p_player_in_special_sector: "
              "unknown special %i",
              sector->special);
      break;
    };
}

void p_update_specials(void)
{
  anim_t *anim;
  int pic;
  int i;
  line_t *line;

  /* LEVEL TIMER */

  if (g_level_timer == true)
    {
      g_level_time_count--;
      if (!g_level_time_count) g_exit_level();
    }

  /* ANIMATE FLATS AND TEXTURES GLOBALLY */

  for (anim = g_anims; anim < g_lastanim; anim++)
    {
      for (i = anim->basepic; i < anim->basepic + anim->numpics; i++)
        {
          pic = anim->basepic +
              ((leveltime / anim->speed + i) % anim->numpics);

          if (anim->istexture)
            texturetranslation[i] = pic;
          else
            flattranslation[i] = pic;
        }
    }

  /* ANIMATE LINE SPECIALS */

  for (i = 0; i < g_numlinespecials; i++)
    {
      line = g_linespeciallist[i];
      switch (line->special)
        {
        case 48: /* EFFECT FIRSTCOL SCROLL + */
          sides[line->sidenum[0]].textureoffset += FRACUNIT;
          break;
        }
    }

  /* DO BUTTONS */

  for (i = 0; i < MAXBUTTONS; i++)
    if (buttonlist[i].btimer)
      {
        buttonlist[i].btimer--;
        if (!buttonlist[i].btimer)
          {
            switch (buttonlist[i].where)
              {
              case top:
                sides[buttonlist[i].line->sidenum[0]].toptexture =
                    buttonlist[i].btexture;
                break;

              case middle:
                sides[buttonlist[i].line->sidenum[0]].midtexture =
                    buttonlist[i].btexture;
                break;

              case bottom:
                sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                    buttonlist[i].btexture;
                break;
              }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
            s_start_sound(&buttonlist[i].soundorg, SFX_SWTCHN);
#endif
            memset(&buttonlist[i], 0, sizeof(button_t));
          }
      }
}

/* Special Stuff that can not be categorized */

int ev_do_donut(line_t *line)
{
  sector_t *s1;
  sector_t *s2;
  sector_t *s3;
  int secnum;
  int rtn;
  int i;
  floormove_t *floor;
  fixed_t s3_floorheight;
  short s3_floorpic;

  secnum = -1;
  rtn = 0;
  while ((secnum = p_find_sector_from_line_tag(line, secnum)) >= 0)
    {
      s1 = &sectors[secnum];

      /* ALREADY MOVING?  IF SO, KEEP GOING... */

      if (s1->specialdata) continue;

      rtn = 1;
      s2 = get_next_sector(s1->lines[0], s1);

      /* Vanilla Doom does not check if the linedef is one sided.  The
       * game does not crash, but reads invalid memory and causes the
       * sector floor to move "down" to some unknown height.
       * DOSbox prints a warning about an invalid memory access.
       *
       * I'm not sure exactly what invalid memory is being read.  This
       * isn't something that should be done, anyway.
       * Just print a warning and return.
       */

      if (s2 == NULL)
        {
          fprintf(stderr,
                  "ev_do_donut: linedef had no second sidedef! "
                  "Unexpected behavior may occur in Vanilla Doom. \n");
          break;
        }

      for (i = 0; i < s2->linecount; i++)
        {
          s3 = s2->lines[i]->backsector;

          if (s3 == s1) continue;

          if (s3 == NULL)
            {
              /* e6y
               * s3 is NULL, so
               * s3->floorheight is an int at 0000:0000
               * s3->floorpic is a short at 0000:0008
               * Trying to emulate
               */

              fprintf(stderr, "ev_do_donut: WARNING: emulating buffer "
                              "overrun due to NULL back sector. Unexpected "
                              "behavior may occur in Vanilla Doom.\n");

              donut_overrun(&s3_floorheight, &s3_floorpic, line, s1);
            }
          else
            {
              s3_floorheight = s3->floorheight;
              s3_floorpic = s3->floorpic;
            }

          /* Spawn rising slime */

          floor = z_malloc(sizeof(*floor), PU_LEVSPEC, 0);
          p_add_thinker(&floor->thinker);
          s2->specialdata = floor;
          floor->thinker.function.acp1 = (actionf_p1)t_move_floor;
          floor->type = FLOOR_DONUTRAISE;
          floor->crush = false;
          floor->direction = 1;
          floor->sector = s2;
          floor->speed = FLOORSPEED / 2;
          floor->texture = s3_floorpic;
          floor->newspecial = 0;
          floor->floordestheight = s3_floorheight;

          /* Spawn lowering donut-hole */

          floor = z_malloc(sizeof(*floor), PU_LEVSPEC, 0);
          p_add_thinker(&floor->thinker);
          s1->specialdata = floor;
          floor->thinker.function.acp1 = (actionf_p1)t_move_floor;
          floor->type = FLOOR_LOWERFLOOR;
          floor->crush = false;
          floor->direction = -1;
          floor->sector = s1;
          floor->speed = FLOORSPEED / 2;
          floor->floordestheight = s3_floorheight;
          break;
        }
    }

  return rtn;
}

/* SPECIAL SPAWNING */

/* Parses command line parameters. */

void p_spawn_specials(void)
{
  sector_t *sector;
  int i;
  short maxlineanims =
      (gameversion <= exe_doom_1_2) ? MAXLINEANIMS1_2 : MAXLINEANIMS;

  /* See if -TIMER was specified. */

  if (timelimit > 0 && deathmatch)
    {
      g_level_timer = true;
      g_level_time_count = timelimit * 60 * TICRATE;
    }
  else
    {
      g_level_timer = false;
    }

  /* Init special SECTORs. */

  sector = sectors;
  for (i = 0; i < numsectors; i++, sector++)
    {
      if (!sector->special) continue;

      switch (sector->special)
        {
        case 1: /* FLICKERING LIGHTS */
          p_spawn_light_flash(sector);
          break;

        case 2: /* STROBE FAST */
          p_spawn_strobe_flash(sector, FASTDARK, 0);
          break;

        case 3: /* STROBE SLOW */
          p_spawn_strobe_flash(sector, SLOWDARK, 0);
          break;

        case 4: /* STROBE FAST/DEATH SLIME */
          p_spawn_strobe_flash(sector, FASTDARK, 0);
          sector->special = 4;
          break;

        case 8: /* GLOWING LIGHT */
          p_spawn_glowing_light(sector);
          break;

        case 9: /* SECRET SECTOR */
          totalsecret++;
          break;

        case 10: /* DOOR CLOSE IN 30 SECONDS */
          p_spawn_door_close_in30(sector);
          break;

        case 12: /* SYNC STROBE SLOW */
          p_spawn_strobe_flash(sector, SLOWDARK, 1);
          break;

        case 13: /* SYNC STROBE FAST */
          p_spawn_strobe_flash(sector, FASTDARK, 1);
          break;

        case 14: /* DOOR RAISE IN 5 MINUTES */
          p_spawn_door_raise_in_5min(sector, i);
          break;

        case 17: /* first introduced in official v1.4 beta */
          if (gameversion > exe_doom_1_2)
            {
              p_spawn_fire_flicker(sector);
            }
          break;
        }
    }

  /* Init line EFFECTs */

  g_numlinespecials = 0;

  for (i = 0; i < numlines; i++)
    {
      switch (lines[i].special)
        {
        case 48:
          if (g_numlinespecials >= maxlineanims)
            {
              i_error("p_spawn_specials: Too many scrolling wall linedefs "
                      "(%d)! (Vanilla limit is %d)",
                      num_scrollers(), maxlineanims);
            }

          /* EFFECT FIRSTCOL SCROLL+ */

          g_linespeciallist[g_numlinespecials] = &lines[i];
          g_numlinespecials++;
          break;
        }
    }

  /* Init other misc stuff */

  for (i = 0; i < MAXCEILINGS; i++)
    activeceilings[i] = NULL;

  for (i = 0; i < MAXPLATS; i++)
    activeplats[i] = NULL;

  for (i = 0; i < MAXBUTTONS; i++)
    memset(&buttonlist[i], 0, sizeof(button_t));

  /* UNUSED: no horizontal sliders.
   * p_init_sliding_door_frames();
   */
}
