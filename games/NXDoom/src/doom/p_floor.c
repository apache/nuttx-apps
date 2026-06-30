/****************************************************************************
 * apps/games/NXDoom/src/doom/p_floor.c
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
 *  Floor animation: raising stairs.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "doomdef.h"
#include "p_local.h"
#include "z_zone.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* State. */

#include "doomstat.h"
#include "r_state.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* e6y */

#define STAIRS_UNINITIALIZED_CRUSH_FIELD_VALUE 10

/* FLOORS */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Move a plane (floor or ceiling) and check for crushing */

result_e t_move_plane(sector_t *sector, fixed_t speed, fixed_t dest,
                      boolean crush, int floor_or_ceiling, int direction)
{
  boolean flag;
  fixed_t lastpos;

  switch (floor_or_ceiling)
    {
    case 0: /* FLOOR */
      switch (direction)
        {
        case -1: /* DOWN */
          if (sector->floorheight - speed < dest)
            {
              lastpos = sector->floorheight;
              sector->floorheight = dest;
              flag = p_change_sector(sector, crush);
              if (flag == true)
                {
                  sector->floorheight = lastpos;
                  p_change_sector(sector, crush);

                  /* return crushed; */
                }

              return pastdest;
            }
          else
            {
              lastpos = sector->floorheight;
              sector->floorheight -= speed;
              flag = p_change_sector(sector, crush);
              if (flag == true)
                {
                  sector->floorheight = lastpos;
                  p_change_sector(sector, crush);
                  return crushed;
                }
            }

          break;

        case 1: /* UP */
          if (sector->floorheight + speed > dest)
            {
              lastpos = sector->floorheight;
              sector->floorheight = dest;
              flag = p_change_sector(sector, crush);
              if (flag == true)
                {
                  sector->floorheight = lastpos;
                  p_change_sector(sector, crush);

                  /* return crushed; */
                }

              return pastdest;
            }
          else
            {
              /* COULD GET CRUSHED */

              lastpos = sector->floorheight;
              sector->floorheight += speed;
              flag = p_change_sector(sector, crush);
              if (flag == true)
                {
                  if (crush == true) return crushed;
                  sector->floorheight = lastpos;
                  p_change_sector(sector, crush);
                  return crushed;
                }
            }
          break;
        }
      break;

    case 1: /* CEILING */
      switch (direction)
        {
        case -1: /* DOWN */
          if (sector->ceilingheight - speed < dest)
            {
              lastpos = sector->ceilingheight;
              sector->ceilingheight = dest;
              flag = p_change_sector(sector, crush);

              if (flag == true)
                {
                  sector->ceilingheight = lastpos;
                  p_change_sector(sector, crush);

                  /* return crushed; */
                }

              return pastdest;
            }
          else
            {
              /* COULD GET CRUSHED */

              lastpos = sector->ceilingheight;
              sector->ceilingheight -= speed;
              flag = p_change_sector(sector, crush);

              if (flag == true)
                {
                  if (crush == true) return crushed;
                  sector->ceilingheight = lastpos;
                  p_change_sector(sector, crush);
                  return crushed;
                }
            }
          break;

        case 1: /* UP */
          if (sector->ceilingheight + speed > dest)
            {
              lastpos = sector->ceilingheight;
              sector->ceilingheight = dest;
              flag = p_change_sector(sector, crush);
              if (flag == true)
                {
                  sector->ceilingheight = lastpos;
                  p_change_sector(sector, crush);

                  /* return crushed; */
                }

              return pastdest;
            }
          else
            {
              lastpos = sector->ceilingheight;
              sector->ceilingheight += speed;
              flag = p_change_sector(sector, crush);

#if 0 /* UNUSED */
              if (flag == true)
                {
                  sector->ceilingheight = lastpos;
                  p_change_sector(sector, crush);
                  return crushed;
                }
#endif
            }
          break;
        }
      break;
    }

  return ok;
}

/* MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN) */

void t_move_floor(floormove_t *floor)
{
  result_e res;

  res = t_move_plane(floor->sector, floor->speed, floor->floordestheight,
                     floor->crush, 0, floor->direction);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (!(leveltime & 7)) s_start_sound(&floor->sector->soundorg, SFX_STNMOV);
#endif

  if (res == pastdest)
    {
      floor->sector->specialdata = NULL;

      if (floor->direction == 1)
        {
          switch (floor->type)
            {
            case FLOOR_DONUTRAISE:
              floor->sector->special = floor->newspecial;
              floor->sector->floorpic = floor->texture;
            default:
              break;
            }
        }
      else if (floor->direction == -1)
        {
          switch (floor->type)
            {
            case FLOOR_LOWERANDCHANGE:
              floor->sector->special = floor->newspecial;
              floor->sector->floorpic = floor->texture;
            default:
              break;
            }
        }

      p_remove_thinker(&floor->thinker);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(&floor->sector->soundorg, SFX_PSTOP);
#endif
    }
}

/* HANDLE FLOOR TYPES */

int ev_do_floor(line_t *line, floor_e floortype)
{
  int secnum;
  int rtn;
  int i;
  sector_t *sec;
  floormove_t *floor;

  secnum = -1;
  rtn = 0;
  while ((secnum = p_find_sector_from_line_tag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      /* ALREADY MOVING?  IF SO, KEEP GOING... */

      if (sec->specialdata) continue;

      /* new floor thinker */

      rtn = 1;
      floor = z_malloc(sizeof(*floor), PU_LEVSPEC, 0);
      p_add_thinker(&floor->thinker);
      sec->specialdata = floor;
      floor->thinker.function.acp1 = (actionf_p1)t_move_floor;
      floor->type = floortype;
      floor->crush = false;

      switch (floortype)
        {
        case FLOOR_LOWERFLOOR:
          floor->direction = -1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight = p_find_highest_floor_surrounding(sec);
          break;

        case FLOOR_LOWERFLOORTOLOWEST:
          floor->direction = -1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight = p_find_lowest_floor_surrounding(sec);
          break;

        case FLOOR_TURBOLOWER:
          floor->direction = -1;
          floor->sector = sec;
          floor->speed = FLOORSPEED * 4;
          floor->floordestheight = p_find_highest_floor_surrounding(sec);
          if (gameversion <= exe_doom_1_2 ||
              floor->floordestheight != sec->floorheight)
            floor->floordestheight += 8 * FRACUNIT;
          break;

        case FLOOR_RAISEFLOORCRUSH:
          floor->crush = true;
        case FLOOR_RAISEFLOOR:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight = p_find_lowest_ceiling_surrounding(sec);
          if (floor->floordestheight > sec->ceilingheight)
            floor->floordestheight = sec->ceilingheight;
          floor->floordestheight -=
              (8 * FRACUNIT) * (floortype == FLOOR_RAISEFLOORCRUSH);
          break;

        case FLOOR_RAISEFLOORTURBO:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED * 4;
          floor->floordestheight =
              p_find_next_highest_floor(sec, sec->floorheight);
          break;

        case FLOOR_RAISEFLOORTONEAREST:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight =
              p_find_next_highest_floor(sec, sec->floorheight);
          break;

        case FLOOR_RAISEFLOOR24:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight =
              floor->sector->floorheight + 24 * FRACUNIT;
          break;

        case FLOOR_RAISEFLOOR512:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight =
              floor->sector->floorheight + 512 * FRACUNIT;
          break;

        case FLOOR_RAISEFLOOR24ANDCHANGE:
          floor->direction = 1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight =
              floor->sector->floorheight + 24 * FRACUNIT;
          sec->floorpic = line->frontsector->floorpic;
          sec->special = line->frontsector->special;
          break;

        case FLOOR_RAISETOTEXTURE:
          {
            int minsize = INT_MAX;
            side_t *side;

            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            for (i = 0; i < sec->linecount; i++)
              {
                if (two_sided(secnum, i))
                  {
                    side = get_side(secnum, i, 0);
                    if (side->bottomtexture >= 0)
                      if (textureheight[side->bottomtexture] < minsize)
                        minsize = textureheight[side->bottomtexture];
                    side = get_side(secnum, i, 1);
                    if (side->bottomtexture >= 0)
                      if (textureheight[side->bottomtexture] < minsize)
                        minsize = textureheight[side->bottomtexture];
                  }
              }

            floor->floordestheight = floor->sector->floorheight + minsize;
          }
          break;

        case FLOOR_LOWERANDCHANGE:
          floor->direction = -1;
          floor->sector = sec;
          floor->speed = FLOORSPEED;
          floor->floordestheight = p_find_lowest_floor_surrounding(sec);
          floor->texture = sec->floorpic;

          for (i = 0; i < sec->linecount; i++)
            {
              if (two_sided(secnum, i))
                {
                  if (get_side(secnum, i, 0)->sector - sectors == secnum)
                    {
                      sec = get_sector(secnum, i, 1);

                      if (sec->floorheight == floor->floordestheight)
                        {
                          floor->texture = sec->floorpic;
                          floor->newspecial = sec->special;
                          break;
                        }
                    }
                  else
                    {
                      sec = get_sector(secnum, i, 0);

                      if (sec->floorheight == floor->floordestheight)
                        {
                          floor->texture = sec->floorpic;
                          floor->newspecial = sec->special;
                          break;
                        }
                    }
                }
            }

        default:
          break;
        }
    }

  return rtn;
}

/* BUILD A STAIRCASE! */

int ev_build_stairs(line_t *line, stair_e type)
{
  int secnum;
  int height;
  int i;
  int newsecnum;
  int texture;
  int is_ok;
  int rtn;

  sector_t *sec;
  sector_t *tsec;

  floormove_t *floor;

  fixed_t stairsize = 0;
  fixed_t speed = 0;

  secnum = -1;
  rtn = 0;
  while ((secnum = p_find_sector_from_line_tag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];

      /* ALREADY MOVING?  IF SO, KEEP GOING... */

      if (sec->specialdata) continue;

      /* new floor thinker */

      rtn = 1;
      floor = z_malloc(sizeof(*floor), PU_LEVSPEC, 0);
      p_add_thinker(&floor->thinker);
      sec->specialdata = floor;
      floor->thinker.function.acp1 = (actionf_p1)t_move_floor;
      floor->direction = 1;
      floor->sector = sec;

      switch (type)
        {
        case STAIR_BUILD8:
          speed = FLOORSPEED / 4;
          stairsize = 8 * FRACUNIT;
          break;
        case STAIR_TURBO16:
          speed = FLOORSPEED * 4;
          stairsize = 16 * FRACUNIT;
          break;
        }

      floor->speed = speed;
      height = sec->floorheight + stairsize;
      floor->floordestheight = height;

      /* Initialize */

      floor->type = FLOOR_LOWERFLOOR;

      /* e6y
       * Uninitialized crush field will not be equal to 0 or 1 (true)
       * with high probability. So, initialize it with any other value
       */

      floor->crush = STAIRS_UNINITIALIZED_CRUSH_FIELD_VALUE;

      texture = sec->floorpic;

      /* Find next sector to raise
       * 1. Find 2-sided line with same sector side[0]
       * 2. Other side is the next sector to raise
       */

      do
        {
          is_ok = 0;
          for (i = 0; i < sec->linecount; i++)
            {
              if (!((sec->lines[i])->flags & ML_TWOSIDED)) continue;

              tsec = (sec->lines[i])->frontsector;
              newsecnum = tsec - sectors;

              if (secnum != newsecnum) continue;

              tsec = (sec->lines[i])->backsector;
              newsecnum = tsec - sectors;

              if (tsec->floorpic != texture) continue;

              height += stairsize;

              if (tsec->specialdata) continue;

              sec = tsec;
              secnum = newsecnum;
              floor = z_malloc(sizeof(*floor), PU_LEVSPEC, 0);

              p_add_thinker(&floor->thinker);

              sec->specialdata = floor;
              floor->thinker.function.acp1 = (actionf_p1)t_move_floor;
              floor->direction = 1;
              floor->sector = sec;
              floor->speed = speed;
              floor->floordestheight = height;

              /* Initialize */

              floor->type = FLOOR_LOWERFLOOR;

              /* e6y
               * Uninitialized crush field will not be equal to 0 or 1
               * (true) with high probability. So, initialize it with any
               * other value
               */

              floor->crush = STAIRS_UNINITIALIZED_CRUSH_FIELD_VALUE;
              is_ok = 1;
              break;
            }
        }
      while (is_ok);
    }

  return rtn;
}
