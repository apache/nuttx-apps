/****************************************************************************
 * apps/games/NXDoom/src/doom/p_doors.c
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
 * DESCRIPTION: Door animation code (opening/closing)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "deh_main.h"
#include "doomdef.h"
#include "i_system.h"
#include "p_local.h"
#include "z_zone.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* State. */

#include "doomstat.h"
#include "r_state.h"

/* Data. */

#include "dstrings.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Sliding door frame information */

#if 0
slidename_t g_slide_frame_names[MAXSLIDEDOORS] =
{
{
  "GDOORF1", "GDOORF2", "GDOORF3", "GDOORF4",  /* front */
  "GDOORB1", "GDOORB2", "GDOORB3", "GDOORB4"   /* back */
},
{
  "\0", "\0", "\0", "\0"
}
};

slideframe_t g_slide_frames[MAXSLIDEDOORS];
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* VERTICAL DOORS */

void t_vertical_door(vldoor_t *door)
{
  result_e res;

  switch (door->direction)
    {
    case 0: /* WAITING */
      if (!--door->topcountdown)
        {
          switch (door->type)
            {
            case VLD_BLAZERAISE:
              door->direction = -1; /* time to go back down */
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_BDCLS);
#endif
              break;

            case VLD_NORMAL:
              door->direction = -1; /* time to go back down */
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_DORCLS);
#endif
              break;

            case VLD_CLOSE30THENOPEN:
              door->direction = 1;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_DOROPN);
#endif
              break;

            default:
              break;
            }
        }
      break;

    case 2: /* INITIAL WAIT */
      if (!--door->topcountdown)
        {
          switch (door->type)
            {
            case VLD_RAISEIN5MINS:
              door->direction = 1;
              door->type = VLD_NORMAL;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_DOROPN);
#endif
              break;

            default:
              break;
            }
        }

      break;

    case -1: /* DOWN */
      res = t_move_plane(door->sector, door->speed,
              door->sector->floorheight, false, 1, door->direction);

      if (res == pastdest)
        {
          switch (door->type)
            {
            case VLD_BLAZERAISE:
            case VLD_BLAZECLOSE:
              door->sector->specialdata = NULL;
              p_remove_thinker(&door->thinker); /* unlink and free */
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_BDCLS);
#endif
              break;

            case VLD_NORMAL:
            case VLD_CLOSE:
              door->sector->specialdata = NULL;
              p_remove_thinker(&door->thinker); /* unlink and free */
              break;

            case VLD_CLOSE30THENOPEN:
              door->direction = 0;
              door->topcountdown = TICRATE * 30;
              break;

            default:
              break;
            }
        }
      else if (res == crushed)
        {
          switch (door->type)
            {
            case VLD_BLAZECLOSE:
            case VLD_CLOSE: /* DO NOT GO BACK UP! */
              break;

            default:
              door->direction = 1;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(&door->sector->soundorg, SFX_DOROPN);
#endif
              break;
            }
        }
      break;

    case 1: /* UP */
      res = t_move_plane(door->sector, door->speed, door->topheight,
              false, 1, door->direction);

      if (res == pastdest)
        {
          switch (door->type)
            {
            case VLD_BLAZERAISE:
            case VLD_NORMAL:
              door->direction = 0; /* wait at top */
              door->topcountdown = door->topwait;
              break;

            case VLD_CLOSE30THENOPEN:
            case VLD_BLAZEOPEN:
            case VLD_OPEN:
              door->sector->specialdata = NULL;
              p_remove_thinker(&door->thinker); /* unlink and free */
              break;

            default:
              break;
            }
        }

      break;
    }
}

/* ev_do_locked_door
 * Move a locked door up/down
 */

int ev_do_locked_door(line_t *line, vldoor_e type, mobj_t *thing)
{
  player_t *p;

  p = thing->player;

  if (!p) return 0;

  switch (line->special)
    {
    case 99: /* Blue Lock */
    case 133:
      if (!p->cards[it_bluecard] && !p->cards[it_blueskull])
        {
          p->message = (PD_BLUEO);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return 0;
        }
      break;

    case 134: /* Red Lock */
    case 135:
      if (!p->cards[it_redcard] && !p->cards[it_redskull])
        {
          p->message = (PD_REDO);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return 0;
        }
      break;

    case 136: /* Yellow Lock */
    case 137:
      if (!p->cards[it_yellowcard] && !p->cards[it_yellowskull])
        {
          p->message = (PD_YELLOWO);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return 0;
        }

      break;
    }

  return ev_do_door(line, type);
}

int ev_do_door(line_t *line, vldoor_e type)
{
  int secnum;
  int rtn;
  sector_t *sec;
  vldoor_t *door;

  secnum = -1;
  rtn = 0;

  while ((secnum = p_find_sector_from_line_tag(line, secnum)) >= 0)
    {
      sec = &sectors[secnum];
      if (sec->specialdata) continue;

      /* new door thinker */

      rtn = 1;
      door = z_malloc(sizeof(*door), PU_LEVSPEC, 0);
      p_add_thinker(&door->thinker);
      sec->specialdata = door;

      door->thinker.function.acp1 = (actionf_p1)t_vertical_door;
      door->sector = sec;
      door->type = type;
      door->topwait = VDOORWAIT;
      door->speed = VDOORSPEED;

      switch (type)
        {
        case VLD_BLAZECLOSE:
          door->topheight = p_find_lowest_ceiling_surrounding(sec);
          door->topheight -= 4 * FRACUNIT;
          door->direction = -1;
          door->speed = VDOORSPEED * 4;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&door->sector->soundorg, SFX_BDCLS);
#endif
          break;

        case VLD_CLOSE:
          door->topheight = p_find_lowest_ceiling_surrounding(sec);
          door->topheight -= 4 * FRACUNIT;
          door->direction = -1;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&door->sector->soundorg, SFX_DORCLS);
#endif
          break;

        case VLD_CLOSE30THENOPEN:
          door->topheight = sec->ceilingheight;
          door->direction = -1;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(&door->sector->soundorg, SFX_DORCLS);
#endif
          break;

        case VLD_BLAZERAISE:
        case VLD_BLAZEOPEN:
          door->direction = 1;
          door->topheight = p_find_lowest_ceiling_surrounding(sec);
          door->topheight -= 4 * FRACUNIT;
          door->speed = VDOORSPEED * 4;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          if (door->topheight != sec->ceilingheight)
            s_start_sound(&door->sector->soundorg, SFX_BDOPN);
#endif
          break;

        case VLD_NORMAL:
        case VLD_OPEN:
          door->direction = 1;
          door->topheight = p_find_lowest_ceiling_surrounding(sec);
          door->topheight -= 4 * FRACUNIT;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          if (door->topheight != sec->ceilingheight)
            s_start_sound(&door->sector->soundorg, SFX_DOROPN);
#endif
          break;

        default:
          break;
        }
    }

  return rtn;
}

/* ev_vertical_door : open a door manually, no tag value */

void ev_vertical_door(line_t *line, mobj_t *thing)
{
  player_t *player;
  sector_t *sec;
  vldoor_t *door;
  int side;

  side = 0; /* only front sides can be used */

  /* Check for locks */

  player = thing->player;

  switch (line->special)
    {
    case 26: /* Blue Lock */
    case 32:
      if (!player) return;

      if (!player->cards[it_bluecard] && !player->cards[it_blueskull])
        {
          player->message = (PD_BLUEK);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return;
        }
      break;

    case 27: /* Yellow Lock */
    case 34:
      if (!player) return;

      if (!player->cards[it_yellowcard] && !player->cards[it_yellowskull])
        {
          player->message = (PD_YELLOWK);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return;
        }
      break;

    case 28: /* Red Lock */
    case 33:
      if (!player) return;

      if (!player->cards[it_redcard] && !player->cards[it_redskull])
        {
          player->message = (PD_REDK);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_OOF);
#endif
          return;
        }
      break;
    }

  /* if the sector has an active thinker, use it */

  if (line->sidenum[side ^ 1] == -1)
    {
      i_error("ev_vertical_door: DR special type on 1-sided linedef");
    }

  sec = sides[line->sidenum[side ^ 1]].sector;

  if (sec->specialdata)
    {
      door = sec->specialdata;
      switch (line->special)
        {
        case 1: /* ONLY FOR "RAISE" DOORS, NOT "OPEN"s */
        case 26:
        case 27:
        case 28:
        case 117:
          if (door->direction == -1)
            door->direction = 1; /* go back up */
          else
            {
              if (!thing->player) return; /* JDC: bad guys never close doors */

              /* When is a door not a door?
               * In Vanilla, door->direction is set, even though
               * "specialdata" might not actually point at a door.
               */

              if (door->thinker.function.acp1 ==
                      (actionf_p1)t_vertical_door)
                {
                  door->direction = -1; /* start going down immediately */
                }
              else if (door->thinker.function.acp1 ==
                      (actionf_p1)t_plat_raise)
                {
                  /* Erm, this is a plat, not a door.
                   * This notably causes a problem in ep1-0500.lmp where
                   * a plat and a door are cross-referenced; the door
                   * doesn't open on 64-bit.
                   * The direction field in vldoor_t corresponds to the wait
                   * field in plat_t.  Let's set that to -1 instead.
                   */

                  plat_t *plat;

                  plat = (plat_t *)door;
                  plat->wait = -1;
                }
              else
                {
                  /* This isn't a door OR a plat.  Now we're in trouble. */

                  fprintf(stderr, "ev_vertical_door: Tried to close "
                                  "something that wasn't a door.\n");

                  /* Try closing it anyway. At least it will work on 32-bit
                   * machines.
                   */

                  door->direction = -1;
                }
            }

          return;
        }
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  /* for proper sound */

  switch (line->special)
    {
    case 117: /* BLAZING DOOR RAISE */
    case 118: /* BLAZING DOOR OPEN */
      s_start_sound(&sec->soundorg, SFX_BDOPN);
      break;

    case 1: /* NORMAL DOOR SOUND */
    case 31:
      s_start_sound(&sec->soundorg, SFX_DOROPN);
      break;

    default: /* LOCKED DOOR SOUND */
      s_start_sound(&sec->soundorg, SFX_DOROPN);
      break;
    }
#endif

  /* new door thinker */

  door = z_malloc(sizeof(*door), PU_LEVSPEC, 0);
  p_add_thinker(&door->thinker);
  sec->specialdata = door;
  door->thinker.function.acp1 = (actionf_p1)t_vertical_door;
  door->sector = sec;
  door->direction = 1;
  door->speed = VDOORSPEED;
  door->topwait = VDOORWAIT;

  switch (line->special)
    {
    case 1:
    case 26:
    case 27:
    case 28:
      door->type = VLD_NORMAL;
      break;

    case 31:
    case 32:
    case 33:
    case 34:
      door->type = VLD_OPEN;
      line->special = 0;
      break;

    case 117: /* blazing door raise */
      door->type = VLD_BLAZERAISE;
      door->speed = VDOORSPEED * 4;
      break;
    case 118: /* blazing door open */
      door->type = VLD_BLAZEOPEN;
      line->special = 0;
      door->speed = VDOORSPEED * 4;
      break;
    }

  /* find the top and bottom of the movement range */

  door->topheight = p_find_lowest_ceiling_surrounding(sec);
  door->topheight -= 4 * FRACUNIT;
}

/* Spawn a door that closes after 30 seconds */

void p_spawn_door_close_in30(sector_t *sec)
{
  vldoor_t *door;

  door = z_malloc(sizeof(*door), PU_LEVSPEC, 0);

  p_add_thinker(&door->thinker);

  sec->specialdata = door;
  sec->special = 0;

  door->thinker.function.acp1 = (actionf_p1)t_vertical_door;
  door->sector = sec;
  door->direction = 0;
  door->type = VLD_NORMAL;
  door->speed = VDOORSPEED;
  door->topcountdown = 30 * TICRATE;
}

/* Spawn a door that opens after 5 minutes */

void p_spawn_door_raise_in_5min(sector_t *sec, int secnum)
{
  vldoor_t *door;

  door = z_malloc(sizeof(*door), PU_LEVSPEC, 0);

  p_add_thinker(&door->thinker);

  sec->specialdata = door;
  sec->special = 0;

  door->thinker.function.acp1 = (actionf_p1)t_vertical_door;
  door->sector = sec;
  door->direction = 2;
  door->type = VLD_RAISEIN5MINS;
  door->speed = VDOORSPEED;
  door->topheight = p_find_lowest_ceiling_surrounding(sec);
  door->topheight -= 4 * FRACUNIT;
  door->topwait = VDOORWAIT;
  door->topcountdown = 5 * 60 * TICRATE;
}

/* UNUSED
 * Separate into p_slidoor.c?
 */

#if 0 /* ABANDONED TO THE MISTS OF TIME!!! */

/* ev_sliding_door : slide a door horizontally
 * (animate midtexture, then set noblocking line)
 */

void p_init_sliding_door_frames(void)
{
  int i;
  int f1;
  int f2;
  int f3;
  int f4;

  /* DOOM II ONLY... */

  if (gamemode != commercial) return;

  for (i = 0; i < MAXSLIDEDOORS; i++)
    {
      if (!g_slide_frame_names[i].front_frame1[0]) break;

      f1 = r_texture_num_for_name(g_slide_frame_names[i].front_frame1);
      f2 = r_texture_num_for_name(g_slide_frame_names[i].front_frame2);
      f3 = r_texture_num_for_name(g_slide_frame_names[i].front_frame3);
      f4 = r_texture_num_for_name(g_slide_frame_names[i].front_frame4);

      g_slide_frames[i].front_frames[0] = f1;
      g_slide_frames[i].front_frames[1] = f2;
      g_slide_frames[i].front_frames[2] = f3;
      g_slide_frames[i].front_frames[3] = f4;

      f1 = r_texture_num_for_name(g_slide_frame_names[i].back_frame1);
      f2 = r_texture_num_for_name(g_slide_frame_names[i].back_frame2);
      f3 = r_texture_num_for_name(g_slide_frame_names[i].back_frame3);
      f4 = r_texture_num_for_name(g_slide_frame_names[i].back_frame4);

      g_slide_frames[i].back_frames[0] = f1;
      g_slide_frames[i].back_frames[1] = f2;
      g_slide_frames[i].back_frames[2] = f3;
      g_slide_frames[i].back_frames[3] = f4;
    }
}

/* Return index into "g_slide_frames" array for which door type to use */

static int p_find_sliding_door(line_t *line)
{
  int i;
  int val;

  for (i = 0; i < MAXSLIDEDOORS; i++)
    {
      val = sides[line->sidenum[0]].midtexture;
      if (val == g_slide_frames[i].front_frames[0]) return i;
    }

  return -1;
}

static void t_sliding_door(slidedoor_t *door)
{
  switch (door->status)
    {
    case sd_opening:
      if (!door->timer--)
        {
          if (++door->frame == SNUMFRAMES)
            {
              /* IF DOOR IS DONE OPENING... */

              sides[door->line->sidenum[0]].midtexture = 0;
              sides[door->line->sidenum[1]].midtexture = 0;
              door->line->flags &= ML_BLOCKING ^ 0xff;

              if (door->type == SDT_OPENONLY)
                {
                  door->frontsector->specialdata = NULL;
                  p_remove_thinker(&door->thinker);
                  break;
                }

              door->timer = SDOORWAIT;
              door->status = sd_waiting;
            }
          else
            {
              /* IF DOOR NEEDS TO ANIMATE TO NEXT FRAME... */

              door->timer = SWAITTICS;

              sides[door->line->sidenum[0]].midtexture =
                  g_slide_frames[door->which_door_index]
                      .front_frames[door->frame];
              sides[door->line->sidenum[1]].midtexture =
                  g_slide_frames[door->which_door_index]
                      .back_frames[door->frame];
            }
        }
      break;

    case sd_waiting:

      /* IF DOOR IS DONE WAITING... */

      if (!door->timer--)
        {
          /* CAN DOOR CLOSE? */

          if (door->frontsector->thinglist != NULL ||
              door->backsector->thinglist != NULL)
            {
              door->timer = SDOORWAIT;
              break;
            }

          /* door->frame = SNUMFRAMES-1; */

          door->status = sd_closing;
          door->timer = SWAITTICS;
        }
      break;

    case sd_closing:
      if (!door->timer--)
        {
          if (--door->frame < 0)
            {
              /* IF DOOR IS DONE CLOSING... */

              door->line->flags |= ML_BLOCKING;
              door->frontsector->specialdata = NULL;
              p_remove_thinker(&door->thinker);
              break;
            }
          else
            {
              /* IF DOOR NEEDS TO ANIMATE TO NEXT FRAME... */

              door->timer = SWAITTICS;

              sides[door->line->sidenum[0]].midtexture =
                  g_slide_frames[door->which_door_index]
                      .front_frames[door->frame];
              sides[door->line->sidenum[1]].midtexture =
                  g_slide_frames[door->which_door_index]
                      .back_frames[door->frame];
            }
        }
      break;
    }
}

void ev_sliding_door(line_t *line, mobj_t *thing)
{
  sector_t *sec;
  slidedoor_t *door;

  /* DOOM II ONLY... */

  if (gamemode != commercial) return;

  /* Make sure door isn't already being animated */

  sec = line->frontsector;
  door = NULL;
  if (sec->specialdata)
    {
      if (!thing->player) return;

      door = sec->specialdata;
      if (door->type == SDT_OPENANDCLOSE)
        {
          if (door->status == sd_waiting) door->status = sd_closing;
        }
      else
        return;
    }

  /* Init sliding door vars */

  if (!door)
    {
      door = z_malloc(sizeof(*door), PU_LEVSPEC, 0);
      p_add_thinker(&door->thinker);
      sec->specialdata = door;

      door->type = SDT_OPENANDCLOSE;
      door->status = sd_opening;
      door->which_door_index = p_find_sliding_door(line);

      if (door->which_door_index < 0)
        i_error("ev_sliding_door: Can't use texture for sliding door!");

      door->frontsector = sec;
      door->backsector = line->backsector;
      door->thinker.function = t_sliding_door;
      door->timer = SWAITTICS;
      door->frame = 0;
      door->line = line;
    }
}
#endif /* UNUSED */
