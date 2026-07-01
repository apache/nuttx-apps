/****************************************************************************
 * apps/games/NXDoom/src/doom/p_pspr.c
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
 *  Weapon sprite animation, weapon objects.
 *  Action functions for weapons.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "doomdef.h"

#include "deh_misc.h"

#include "m_random.h"
#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* State. */

#include "doomstat.h"

/* Data. */

#include "p_pspr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LOWERSPEED FRACUNIT * 6
#define RAISESPEED FRACUNIT * 6

#define WEAPONBOTTOM 128 * FRACUNIT
#define WEAPONTOP 32 * FRACUNIT

/****************************************************************************
 * Public Data
 ****************************************************************************/

fixed_t swingx;
fixed_t swingy;

fixed_t bulletslope;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void p_set_psprite(player_t *player, int position, statenum_t stnum)
{
  pspdef_t *psp;
  state_t *state;

  psp = &player->psprites[position];

  do
    {
      if (!stnum)
        {
          /* object removed itself */

          psp->state = NULL;
          break;
        }

      state = &states[stnum];
      psp->state = state;
      psp->tics = state->tics; /* could be 0 */

#if 0 /* Unused as far as I can tell */
      if (state->misc1)
        {
          /* coordinate set */

          psp->sx = state->misc1 << FRACBITS;
          psp->sy = state->misc2 << FRACBITS;
        }
#endif

      /* Call action routine.
       * Modified handling.
       */

      if (state->action.acp2)
        {
          state->action.acp2(player, psp);
          if (!psp->state) break;
        }

      stnum = psp->state->nextstate;
    }
  while (!psp->tics);

  /* an initial state of 0 could cycle through */
}

#if 0 /* UNUSED */
static void p_calc_swing(player_t *player)
{
  fixed_t swing;
  int angle;

  /* OPTIMIZE: tablify this.
   * A LUT would allow for different modes, and add flexibility.
   */

  swing = player->bob;

  angle = (FINEANGLES / 70 * leveltime) & FINEMASK;
  swingx = fixed_mul(swing, finesine[angle]);

  angle = (FINEANGLES / 70 * leveltime + FINEANGLES / 2) & FINEMASK;
  swingy = -fixed_mul(swingx, finesine[angle]);
}
#endif

/* p_bringup_weapon
 * Starts bringing the pending weapon up from the bottom of the screen. Uses
 * player
 */

static void p_bringup_weapon(player_t *player)
{
  statenum_t newstate;

  if (player->pendingweapon == wp_nochange)
    player->pendingweapon = player->readyweapon;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (player->pendingweapon == wp_chainsaw)
    s_start_sound(player->mo, SFX_SAWUP);
#endif

  newstate = weaponinfo[player->pendingweapon].upstate;

  player->pendingweapon = wp_nochange;
  player->psprites[ps_weapon].sy = WEAPONBOTTOM;

  p_set_psprite(player, ps_weapon, newstate);
}

/* Doom does not check the bounds of the ammo array.  As a result,
 * it is possible to use an ammo type > 4 that overflows into the
 * maxammo array and affects that instead.  Through dehacked, for
 * example, it is possible to make a weapon that decreases the max
 * number of ammo for another weapon.  Emulate this.
 */

static void decrease_ammo(player_t *player, int ammonum, int amount)
{
  if (ammonum < NUMAMMO)
    {
      player->ammo[ammonum] -= amount;
    }
  else
    {
      player->maxammo[ammonum - NUMAMMO] -= amount;
    }
}

/* p_bullet_slope
 * Sets a slope so a near miss is at approximately
 * the height of the intended target
 */

static void p_bullet_slope(mobj_t *mo)
{
  angle_t an;

  /* see which target is to be aimed at */

  an = mo->angle;
  bulletslope = p_aim_line_attack(mo, an, 16 * 64 * FRACUNIT);

  if (!linetarget)
    {
      an += 1 << 26;
      bulletslope = p_aim_line_attack(mo, an, 16 * 64 * FRACUNIT);
      if (!linetarget)
        {
          an -= 2 << 26;
          bulletslope = p_aim_line_attack(mo, an, 16 * 64 * FRACUNIT);
        }
    }
}

static void p_gunshot(mobj_t *mo, boolean accurate)
{
  angle_t angle;
  int damage;

  damage = 5 * (p_random() % 3 + 1);
  angle = mo->angle;

  if (!accurate) angle += p_sub_random() << 18;

  p_line_attack(mo, angle, MISSILERANGE, bulletslope, damage);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* p_check_ammo
 * Returns true if there is enough ammo to shoot.
 * If not, selects the next weapon to use.
 */

boolean p_check_ammo(player_t *player)
{
  ammotype_t ammo;
  int count;

  ammo = weaponinfo[player->readyweapon].ammo;

  /* Minimal amount for one shot varies. */

  if (player->readyweapon == wp_bfg)
    count = deh_bfg_cells_per_shot;
  else if (player->readyweapon == wp_supershotgun)
    count = 2; /* Double barrel. */
  else
    count = 1; /* Regular. */

  /* Some do not need ammunition anyway.
   * Return if current ammunition sufficient.
   */

  if (ammo == am_noammo || player->ammo[ammo] >= count) return true;

  /* Out of ammo, pick a weapon to change to.
   * Preferences are set here.
   */

  do
    {
      if (player->weaponowned[wp_plasma] && player->ammo[am_cell] &&
          (gamemode != shareware))
        {
          player->pendingweapon = wp_plasma;
        }
      else if (player->weaponowned[wp_supershotgun] &&
               player->ammo[am_shell] > 2 && (gamemode == commercial))
        {
          player->pendingweapon = wp_supershotgun;
        }
      else if (player->weaponowned[wp_chaingun] && player->ammo[am_clip])
        {
          player->pendingweapon = wp_chaingun;
        }
      else if (player->weaponowned[wp_shotgun] && player->ammo[am_shell])
        {
          player->pendingweapon = wp_shotgun;
        }
      else if (player->ammo[am_clip])
        {
          player->pendingweapon = wp_pistol;
        }
      else if (player->weaponowned[wp_chainsaw])
        {
          player->pendingweapon = wp_chainsaw;
        }
      else if (player->weaponowned[wp_missile] && player->ammo[am_misl])
        {
          player->pendingweapon = wp_missile;
        }
      else if (player->weaponowned[wp_bfg] && player->ammo[am_cell] > 40 &&
               (gamemode != shareware))
        {
          player->pendingweapon = wp_bfg;
        }
      else
        {
          /* If everything fails. */

          player->pendingweapon = wp_fist;
        }
    }
  while (player->pendingweapon == wp_nochange);

  /* Now set appropriate weapon overlay. */

  p_set_psprite(player, ps_weapon,
          weaponinfo[player->readyweapon].downstate);
  return false;
}

static void p_fire_weapon(player_t *player)
{
  statenum_t newstate;

  if (!p_check_ammo(player)) return;

  p_set_mobj_state(player->mo, S_PLAY_ATK1);
  newstate = weaponinfo[player->readyweapon].atkstate;
  p_set_psprite(player, ps_weapon, newstate);
  p_noise_alert(player->mo, player->mo);
}

/* p_drop_weapon
 * Player died, so put the weapon away.
 */

void p_drop_weapon(player_t *player)
{
  p_set_psprite(player, ps_weapon,
          weaponinfo[player->readyweapon].downstate);
}

/* a_weapon_ready
 * The player can fire the weapon or change to another weapon at this time.
 * Follows after getting weapon up, or after previous attack/fire sequence.
 */

void a_weapon_ready(player_t *player, pspdef_t *psp)
{
  statenum_t newstate;
  int angle;

  /* get out of attack state */

  if (player->mo->state == &states[S_PLAY_ATK1] ||
      player->mo->state == &states[S_PLAY_ATK2])
    {
      p_set_mobj_state(player->mo, S_PLAY);
    }

  if (player->readyweapon == wp_chainsaw && psp->state == &states[S_SAW])
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(player->mo, SFX_SAWIDL);
#endif
    }

  /* check for change if player is dead, put the weapon away */

  if (player->pendingweapon != wp_nochange || !player->health)
    {
      /* change weapon (pending weapon should already be validated) */

      newstate = weaponinfo[player->readyweapon].downstate;
      p_set_psprite(player, ps_weapon, newstate);
      return;
    }

  /* check for fire the missile launcher and bfg do not auto fire */

  if (player->cmd.buttons & BT_ATTACK)
    {
      if (!player->attackdown || (player->readyweapon != wp_missile &&
                                  player->readyweapon != wp_bfg))
        {
          player->attackdown = true;
          p_fire_weapon(player);
          return;
        }
    }
  else
    player->attackdown = false;

  /* bob the weapon based on movement speed */

  angle = (128 * leveltime) & FINEMASK;
  psp->sx = FRACUNIT + fixed_mul(player->bob, finecosine[angle]);
  angle &= FINEANGLES / 2 - 1;
  psp->sy = WEAPONTOP + fixed_mul(player->bob, finesine[angle]);
}

/* a_refire
 * The player can re-fire the weapon without lowering it entirely.
 */

void a_refire(player_t *player, pspdef_t *psp)
{
  /* check for fire (if a weaponchange is pending, let it go through instead)
   */

  if ((player->cmd.buttons & BT_ATTACK) &&
      player->pendingweapon == wp_nochange && player->health)
    {
      player->refire++;
      p_fire_weapon(player);
    }
  else
    {
      player->refire = 0;
      p_check_ammo(player);
    }
}

void a_check_reload(player_t *player, pspdef_t *psp)
{
  p_check_ammo(player);
#if 0
  if (player->ammo[am_shell] < 2)
    {
      p_set_psprite(player, ps_weapon, S_DSNR1);
    }
#endif
}

/* a_lower
 * Lowers current weapon, and changes weapon at bottom.
 */

void a_lower(player_t *player, pspdef_t *psp)
{
  psp->sy += LOWERSPEED;

  /* Is already down. */

  if (psp->sy < WEAPONBOTTOM) return;

  /* Player is dead. */

  if (player->playerstate == PST_DEAD)
    {
      psp->sy = WEAPONBOTTOM;

      /* don't bring weapon back up */

      return;
    }

  /* The old weapon has been lowered off the screen, so change the weapon and
   * start raising it
   */

  if (!player->health)
    {
      /* Player is dead, so keep the weapon off screen. */

      p_set_psprite(player, ps_weapon, S_NULL);
      return;
    }

  player->readyweapon = player->pendingweapon;

  p_bringup_weapon(player);
}

void a_raise(player_t *player, pspdef_t *psp)
{
  statenum_t newstate;

  psp->sy -= RAISESPEED;

  if (psp->sy > WEAPONTOP) return;

  psp->sy = WEAPONTOP;

  /* The weapon has been raised all the way, so change to the ready state. */

  newstate = weaponinfo[player->readyweapon].readystate;

  p_set_psprite(player, ps_weapon, newstate);
}

void a_gun_flash(player_t *player, pspdef_t *psp)
{
  p_set_mobj_state(player->mo, S_PLAY_ATK2);
  p_set_psprite(player, ps_flash,
          weaponinfo[player->readyweapon].flashstate);
}

/* WEAPON ATTACKS */

void a_punch(player_t *player, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  int slope;

  damage = (p_random() % 10 + 1) << 1;

  if (player->powers[pw_strength]) damage *= 10;

  angle = player->mo->angle;
  angle += p_sub_random() << 18;
  slope = p_aim_line_attack(player->mo, angle, MELEERANGE);
  p_line_attack(player->mo, angle, MELEERANGE, slope, damage);

  /* turn to face target */

  if (linetarget)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(player->mo, SFX_PUNCH);
#endif
      player->mo->angle = r_point_to_angle2(player->mo->x, player->mo->y,
                                            linetarget->x, linetarget->y);
    }
}

void a_saw(player_t *player, pspdef_t *psp)
{
  angle_t angle;
  int damage;
  int slope;

  damage = 2 * (p_random() % 10 + 1);
  angle = player->mo->angle;
  angle += p_sub_random() << 18;

  /* use meleerange + 1 se the puff doesn't skip the flash */

  slope = p_aim_line_attack(player->mo, angle, MELEERANGE + 1);
  p_line_attack(player->mo, angle, MELEERANGE + 1, slope, damage);

  if (!linetarget)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(player->mo, SFX_SAWFUL);
#endif
      return;
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_SAWHIT);
#endif

  /* turn to face target */

  angle = r_point_to_angle2(player->mo->x, player->mo->y, linetarget->x,
                            linetarget->y);
  if (angle - player->mo->angle > ANG180)
    {
      if ((signed int)(angle - player->mo->angle) < -ANG90 / 20)
        player->mo->angle = angle + ANG90 / 21;
      else
        player->mo->angle -= ANG90 / 20;
    }
  else
    {
      if (angle - player->mo->angle > ANG90 / 20)
        player->mo->angle = angle - ANG90 / 21;
      else
        player->mo->angle += ANG90 / 20;
    }

  player->mo->flags |= MF_JUSTATTACKED;
}

void a_fire_missile(player_t *player, pspdef_t *psp)
{
  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 1);
  p_spawn_player_missile(player->mo, MT_ROCKET);
}

void a_fire_bfg(player_t *player, pspdef_t *psp)
{
  decrease_ammo(player, weaponinfo[player->readyweapon].ammo,
                deh_bfg_cells_per_shot);
  p_spawn_player_missile(player->mo, MT_BFG);
}

void a_fire_plasma(player_t *player, pspdef_t *psp)
{
  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 1);

  p_set_psprite(player, ps_flash,
                weaponinfo[player->readyweapon].flashstate +
                    (p_random() & 1));

  p_spawn_player_missile(player->mo, MT_PLASMA);
}

void a_fire_pistol(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_PISTOL);
#endif

  p_set_mobj_state(player->mo, S_PLAY_ATK2);
  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 1);

  p_set_psprite(player, ps_flash,
          weaponinfo[player->readyweapon].flashstate);

  p_bullet_slope(player->mo);
  p_gunshot(player->mo, !player->refire);
}

void a_fire_shotgun(player_t *player, pspdef_t *psp)
{
  int i;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_SHOTGN);
#endif
  p_set_mobj_state(player->mo, S_PLAY_ATK2);

  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 1);

  p_set_psprite(player, ps_flash,
          weaponinfo[player->readyweapon].flashstate);

  p_bullet_slope(player->mo);

  for (i = 0; i < 7; i++)
    p_gunshot(player->mo, false);
}

void a_fire_shotgun2(player_t *player, pspdef_t *psp)
{
  int i;
  angle_t angle;
  int damage;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_DSHTGN);
#endif
  p_set_mobj_state(player->mo, S_PLAY_ATK2);

  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 2);

  p_set_psprite(player, ps_flash,
          weaponinfo[player->readyweapon].flashstate);

  p_bullet_slope(player->mo);

  for (i = 0; i < 20; i++)
    {
      damage = 5 * (p_random() % 3 + 1);
      angle = player->mo->angle;
      angle += p_sub_random() << ANGLETOFINESHIFT;
      p_line_attack(player->mo, angle, MISSILERANGE,
                    bulletslope + (p_sub_random() << 5), damage);
    }
}

void a_fire_cgun(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_PISTOL);
#endif

  if (!player->ammo[weaponinfo[player->readyweapon].ammo]) return;

  p_set_mobj_state(player->mo, S_PLAY_ATK2);
  decrease_ammo(player, weaponinfo[player->readyweapon].ammo, 1);

  p_set_psprite(player, ps_flash,
                weaponinfo[player->readyweapon].flashstate + psp->state -
                    &states[S_CHAIN1]);

  p_bullet_slope(player->mo);

  p_gunshot(player->mo, !player->refire);
}

/* ? */

void a_light0(player_t *player, pspdef_t *psp)
{
  player->extralight = 0;
}

void a_light1(player_t *player, pspdef_t *psp)
{
  player->extralight = 1;
}

void a_light2(player_t *player, pspdef_t *psp)
{
  player->extralight = 2;
}

/* a_bfg_spray
 * Spawn a BFG explosion on every monster in view
 */

void a_bfg_spray(mobj_t *mo)
{
  int i;
  int j;
  int damage;
  angle_t an;

  /* offset angles from its attack angle */

  for (i = 0; i < 40; i++)
    {
      an = mo->angle - ANG90 / 2 + ANG90 / 40 * i;

      /* mo->target is the originator (player) of the missile */

      p_aim_line_attack(mo->target, an, 16 * 64 * FRACUNIT);

      if (!linetarget) continue;

      p_spawn_mobj(linetarget->x, linetarget->y,
                   linetarget->z + (linetarget->height >> 2), MT_EXTRABFG);

      damage = 0;
      for (j = 0; j < 15; j++)
        damage += (p_random() & 7) + 1;

      p_damage_mobj(linetarget, mo->target, mo->target, damage);
    }
}

/* a_bfg_sound */

void a_bfg_sound(player_t *player, pspdef_t *psp)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start_sound(player->mo, SFX_BFG);
#endif
}

/* p_setup_psprites
 * Called at start of level for each player.
 */

void p_setup_psprites(player_t *player)
{
  int i;

  /* remove all psprites */

  for (i = 0; i < NUMPSPRITES; i++)
    player->psprites[i].state = NULL;

  /* spawn the gun */

  player->pendingweapon = player->readyweapon;
  p_bringup_weapon(player);
}

/* p_move_psprites
 * Called every tic by player thinking routine.
 */

void p_move_psprites(player_t *player)
{
  int i;
  pspdef_t *psp;

  psp = &player->psprites[0];
  for (i = 0; i < NUMPSPRITES; i++, psp++)
    {
      /* a null state means not active */

      if (psp->state)
        {
          /* drop tic count and possibly change state */

          /* a -1 tic count never changes */

          if (psp->tics != -1)
            {
              psp->tics--;
              if (!psp->tics)
                {
                  p_set_psprite(player, i, psp->state->nextstate);
                }
            }
        }
    }

  player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
  player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}
