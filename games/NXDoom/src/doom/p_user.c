/****************************************************************************
 * apps/games/NXDoom/src/doom/p_user.c
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
 *  Player related stuff.
 *  Bobbing POV/weapon, movement.
 *  Pending weapon.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "doomdef.h"

#include "p_local.h"

#include "doomstat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Index of the special effects (INVUL inverse) map. */

#define INVERSECOLORMAP 32

/* Movement. */

/* 16 pixels of bob */

#define MAXBOB 0x100000

#define ANG5 (ANG90 / 18)

/****************************************************************************
 * Public Data
 ****************************************************************************/

boolean onground;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* P_Thrust
 * Moves the given origin along a given angle.
 */

static void p_thrust(player_t *player, angle_t angle, fixed_t move)
{
  angle >>= ANGLETOFINESHIFT;

  player->mo->momx += fixed_mul(move, finecosine[angle]);
  player->mo->momy += fixed_mul(move, finesine[angle]);
}

/* Calculate the walking / running height adjustment */

static void p_calc_height(player_t *player)
{
  int angle;
  fixed_t bob;

  /* Regular movement bobbing
   * (needs to be calculated for gun swing
   * even if not on ground)
   * OPTIMIZE: tablify angle
   * Note: a LUT allows for effects
   *  like a ramp with low health.
   */

  player->bob = fixed_mul(player->mo->momx, player->mo->momx) +
                fixed_mul(player->mo->momy, player->mo->momy);

  player->bob >>= 2;

  if (player->bob > MAXBOB) player->bob = MAXBOB;

  if ((player->cheats & CF_NOMOMENTUM) || !onground)
    {
      player->viewz = player->mo->z + VIEWHEIGHT;

      if (player->viewz > player->mo->ceilingz - 4 * FRACUNIT)
        player->viewz = player->mo->ceilingz - 4 * FRACUNIT;

      player->viewz = player->mo->z + player->viewheight;
      return;
    }

  angle = (FINEANGLES / 20 * leveltime) & FINEMASK;
  bob = fixed_mul(player->bob / 2, finesine[angle]);

  /* move viewheight */

  if (player->playerstate == PST_LIVE)
    {
      player->viewheight += player->deltaviewheight;

      if (player->viewheight > VIEWHEIGHT)
        {
          player->viewheight = VIEWHEIGHT;
          player->deltaviewheight = 0;
        }

      if (player->viewheight < VIEWHEIGHT / 2)
        {
          player->viewheight = VIEWHEIGHT / 2;
          if (player->deltaviewheight <= 0) player->deltaviewheight = 1;
        }

      if (player->deltaviewheight)
        {
          player->deltaviewheight += FRACUNIT / 4;
          if (!player->deltaviewheight) player->deltaviewheight = 1;
        }
    }

  player->viewz = player->mo->z + player->viewheight + bob;

  if (player->viewz > player->mo->ceilingz - 4 * FRACUNIT)
    player->viewz = player->mo->ceilingz - 4 * FRACUNIT;
}

static void p_move_player(player_t *player)
{
  ticcmd_t *cmd;

  cmd = &player->cmd;

  player->mo->angle += (cmd->angleturn << FRACBITS);

  /* Do not let the player control movement if not onground. */

  onground = (player->mo->z <= player->mo->floorz);

  if (cmd->forwardmove && onground)
    p_thrust(player, player->mo->angle, cmd->forwardmove * 2048);

  if (cmd->sidemove && onground)
    p_thrust(player, player->mo->angle - ANG90, cmd->sidemove * 2048);

  if ((cmd->forwardmove || cmd->sidemove) &&
      player->mo->state == &states[S_PLAY])
    {
      p_set_mobj_state(player->mo, S_PLAY_RUN1);
    }
}

/* P_DeathThink
 * Fall on your face when dying.
 * Decrease POV height to floor height.
 */

static void p_death_think(player_t *player)
{
  angle_t angle;
  angle_t delta;

  p_move_psprites(player);

  /* fall to the ground */

  if (player->viewheight > 6 * FRACUNIT) player->viewheight -= FRACUNIT;

  if (player->viewheight < 6 * FRACUNIT) player->viewheight = 6 * FRACUNIT;

  player->deltaviewheight = 0;
  onground = (player->mo->z <= player->mo->floorz);
  p_calc_height(player);

  if (player->attacker && player->attacker != player->mo)
    {
      angle = r_point_to_angle2(player->mo->x, player->mo->y,
                                player->attacker->x, player->attacker->y);

      delta = angle - player->mo->angle;

      if (delta < ANG5 || delta > (unsigned)-ANG5)
        {
          /* Looking at killer, so fade damage flash down. */

          player->mo->angle = angle;

          if (player->damagecount) player->damagecount--;
        }
      else if (delta < ANG180)
        player->mo->angle += ANG5;
      else
        player->mo->angle -= ANG5;
    }
  else if (player->damagecount)
    player->damagecount--;

  if (player->cmd.buttons & BT_USE) player->playerstate = PST_REBORN;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void p_player_think(player_t *player)
{
  ticcmd_t *cmd;
  weapontype_t newweapon;

  /* fixme: do this in the cheat code */

  if (player->cheats & CF_NOCLIP)
    player->mo->flags |= MF_NOCLIP;
  else
    player->mo->flags &= ~MF_NOCLIP;

  /* chain saw run forward */

  cmd = &player->cmd;
  if (player->mo->flags & MF_JUSTATTACKED)
    {
      cmd->angleturn = 0;
      cmd->forwardmove = 0xc800 / 512;
      cmd->sidemove = 0;
      player->mo->flags &= ~MF_JUSTATTACKED;
    }

  if (player->playerstate == PST_DEAD)
    {
      p_death_think(player);
      return;
    }

  /* Move around.
   * Reactiontime is used to prevent movement for a bit after a teleport.
   */

  if (player->mo->reactiontime)
    player->mo->reactiontime--;
  else
    p_move_player(player);

  p_calc_height(player);

  if (player->mo->subsector->sector->special)
    {
      p_player_in_special_sector(player);
    }

  /* Check for weapon change. */

  /* A special event has no other buttons. */

  if (cmd->buttons & BT_SPECIAL) cmd->buttons = 0;

  if (cmd->buttons & BT_CHANGE)
    {
      /* The actual changing of the weapon is done
       *  when the weapon psprite can do it
       *  (read: not in the middle of an attack).
       */

      newweapon = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;

      if (newweapon == wp_fist && player->weaponowned[wp_chainsaw] &&
          !(player->readyweapon == wp_chainsaw &&
            player->powers[pw_strength]))
        {
          newweapon = wp_chainsaw;
        }

      if ((gamemode == commercial) && newweapon == wp_shotgun &&
          player->weaponowned[wp_supershotgun] &&
          player->readyweapon != wp_supershotgun)
        {
          newweapon = wp_supershotgun;
        }

      if (player->weaponowned[newweapon] && newweapon != player->readyweapon)
        {
          /* Do not go to plasma or BFG in shareware, even if cheated. */

          if ((newweapon != wp_plasma && newweapon != wp_bfg) ||
              (gamemode != shareware))
            {
              player->pendingweapon = newweapon;
            }
        }
    }

  /* check for use */

  if (cmd->buttons & BT_USE)
    {
      if (!player->usedown)
        {
          p_use_lines(player);
          player->usedown = true;
        }
    }
  else
    player->usedown = false;

  /* cycle psprites */

  p_move_psprites(player);

  /* Counters, time dependent power ups. */

  /* Strength counts up to diminish fade. */

  if (player->powers[pw_strength]) player->powers[pw_strength]++;

  if (player->powers[pw_invulnerability])
    player->powers[pw_invulnerability]--;

  if (player->powers[pw_invisibility])
    {
      if (!--player->powers[pw_invisibility])
        {
          player->mo->flags &= ~MF_SHADOW;
        }
    }

  if (player->powers[pw_infrared]) player->powers[pw_infrared]--;

  if (player->powers[pw_ironfeet]) player->powers[pw_ironfeet]--;

  if (player->damagecount) player->damagecount--;

  if (player->bonuscount) player->bonuscount--;

  /* Handling colormaps. */

  if (player->powers[pw_invulnerability])
    {
      if (player->powers[pw_invulnerability] > 4 * 32 ||
          (player->powers[pw_invulnerability] & 8))
        player->fixedcolormap = INVERSECOLORMAP;
      else
        player->fixedcolormap = 0;
    }
  else if (player->powers[pw_infrared])
    {
      if (player->powers[pw_infrared] > 4 * 32 ||
          (player->powers[pw_infrared] & 8))
        {
          /* almost full bright */

          player->fixedcolormap = 1;
        }
      else
        player->fixedcolormap = 0;
    }
  else
    player->fixedcolormap = 0;
}
