/****************************************************************************
 * apps/games/NXDoom/src/doom/p_inter.c
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
 *  Handling interactions (i.e., collisions).
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

/* Data. */

#include "doomdef.h"
#include "dstrings.h"

#include "deh_main.h"
#include "deh_misc.h"
#include "doomstat.h"

#include "i_system.h"
#include "m_random.h"

#include "am_map.h"

#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "p_inter.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BONUSADD 6

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* a weapon is found with two clip loads, a big item has five clip loads */

int maxammo[NUMAMMO] =
{
  200, 50, 300, 50
};

int clipammo[NUMAMMO] =
{
  10, 4, 20, 1
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* GET STUFF */

/* p_give_ammo
 *
 * Num is the number of clip loads, not the individual count (0= 1/2 clip).
 *
 * Returns false if the ammo can't be picked up at all
 */

static boolean p_give_ammo(player_t *player, ammotype_t ammo, int num)
{
  int oldammo;

  if (ammo == am_noammo) return false;

  if (ammo >= NUMAMMO) i_error("p_give_ammo: bad type %i", ammo);

  if (player->ammo[ammo] == player->maxammo[ammo]) return false;

  if (num)
    num *= clipammo[ammo];
  else
    num = clipammo[ammo] / 2;

  if (gameskill == sk_baby || gameskill == sk_nightmare)
    {
      /* give double ammo in trainer mode, you'll need in nightmare */

      num <<= 1;
    }

  oldammo = player->ammo[ammo];
  player->ammo[ammo] += num;

  if (player->ammo[ammo] > player->maxammo[ammo])
    player->ammo[ammo] = player->maxammo[ammo];

  /* If non zero ammo, don't change up weapons, player was lower on purpose.
   */

  if (oldammo) return true;

  /* We were down to zero, so select a new weapon. Preferences are not user
   * selectable.
   */

  switch (ammo)
    {
    case am_clip:
      if (player->readyweapon == wp_fist)
        {
          if (player->weaponowned[wp_chaingun])
            player->pendingweapon = wp_chaingun;
          else
            player->pendingweapon = wp_pistol;
        }

      break;

    case am_shell:
      if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
        {
          if (player->weaponowned[wp_shotgun])
            player->pendingweapon = wp_shotgun;
        }

      break;

    case am_cell:
      if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
        {
          if (player->weaponowned[wp_plasma])
            player->pendingweapon = wp_plasma;
        }

      break;

    case am_misl:
      if (player->readyweapon == wp_fist)
        {
          if (player->weaponowned[wp_missile])
            player->pendingweapon = wp_missile;
        }

    default:
      break;
    }

  return true;
}

/* p_give_weapon
 *
 * The weapon name may have a MF_DROPPED flag ored in.
 */

static boolean p_give_weapon(player_t *player, weapontype_t weapon,
                             boolean dropped)
{
  boolean gaveammo;
  boolean gaveweapon;

  if (netgame && (deathmatch != 2) && !dropped)
    {
      /* leave placed weapons forever on net games */

      if (player->weaponowned[weapon]) return false;

      player->bonuscount += BONUSADD;
      player->weaponowned[weapon] = true;

      if (deathmatch)
        p_give_ammo(player, weaponinfo[weapon].ammo, 5);
      else
        p_give_ammo(player, weaponinfo[weapon].ammo, 2);
      player->pendingweapon = weapon;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (player == &players[consoleplayer]) s_start_sound(NULL, SFX_WPNUP);
#endif
      return false;
    }

  if (weaponinfo[weapon].ammo != am_noammo)
    {
      /* give one clip with a dropped weapon, two clips with a found weapon */

      if (dropped)
        gaveammo = p_give_ammo(player, weaponinfo[weapon].ammo, 1);
      else
        gaveammo = p_give_ammo(player, weaponinfo[weapon].ammo, 2);
    }
  else
    gaveammo = false;

  if (player->weaponowned[weapon])
    gaveweapon = false;
  else
    {
      gaveweapon = true;
      player->weaponowned[weapon] = true;
      player->pendingweapon = weapon;
    }

  return (gaveweapon || gaveammo);
}

/* p_give_body
 * Returns false if the body isn't needed at all
 */

static boolean p_give_body(player_t *player, int num)
{
  if (player->health >= MAXHEALTH) return false;

  player->health += num;
  if (player->health > MAXHEALTH) player->health = MAXHEALTH;
  player->mo->health = player->health;

  return true;
}

/* p_give_armor
 * Returns false if the armor is worse than the current armor.
 */

static boolean p_give_armor(player_t *player, int armortype)
{
  int hits;

  hits = armortype * 100;
  if (player->armorpoints >= hits) return false; /* don't pick up */

  player->armortype = armortype;
  player->armorpoints = hits;

  return true;
}

static void p_give_card(player_t *player, card_t card)
{
  if (player->cards[card]) return;

  player->bonuscount = BONUSADD;
  player->cards[card] = 1;
}

static void p_kill_mobj(mobj_t *source, mobj_t *target)
{
  mobjtype_t item;
  mobj_t *mo;

  target->flags &= ~(MF_SHOOTABLE | MF_FLOAT | MF_SKULLFLY);

  if (target->type != MT_SKULL) target->flags &= ~MF_NOGRAVITY;

  target->flags |= MF_CORPSE | MF_DROPOFF;
  target->height >>= 2;

  if (source && source->player)
    {
      /* count for intermission */

      if (target->flags & MF_COUNTKILL) source->player->killcount++;

      if (target->player) source->player->frags[target->player - players]++;
    }
  else if (!netgame && (target->flags & MF_COUNTKILL))
    {
      /* count all monster deaths, even those caused by other monsters */

      players[0].killcount++;
    }

  if (target->player)
    {
      /* count environment kills against you */

      if (!source) target->player->frags[target->player - players]++;

      target->flags &= ~MF_SOLID;
      target->player->playerstate = PST_DEAD;
      p_drop_weapon(target->player);

      if (target->player == &players[consoleplayer] && automapactive)
        {
          /* don't die in auto map, switch view prior to dying */

          am_stop();
        }
    }

  if (target->health < -target->info->spawnhealth &&
      target->info->xdeathstate)
    {
      p_set_mobj_state(target, target->info->xdeathstate);
    }
  else
    p_set_mobj_state(target, target->info->deathstate);

  target->tics -= p_random() & 3;

  if (target->tics < 1) target->tics = 1;

  /* In Chex Quest, monsters don't drop items. */

  if (gameversion == exe_chex)
    {
      return;
    }

  /* Drop stuff.
   * This determines the kind of object spawned during the death frame of a
   * thing.
   */

  switch (target->type)
    {
    case MT_WOLFSS:
    case MT_POSSESSED:
      item = MT_CLIP;
      break;

    case MT_SHOTGUY:
      item = MT_SHOTGUN;
      break;

    case MT_CHAINGUY:
      item = MT_CHAINGUN;
      break;

    default:
      return;
    }

  mo = p_spawn_mobj(target->x, target->y, ONFLOORZ, item);
  mo->flags |= MF_DROPPED; /* special versions of items */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

boolean p_give_power(player_t *player, int /* powertype_t */ power)
{
  if (power == pw_invulnerability)
    {
      player->powers[power] = INVULNTICS;
      return true;
    }

  if (power == pw_invisibility)
    {
      player->powers[power] = INVISTICS;
      player->mo->flags |= MF_SHADOW;
      return true;
    }

  if (power == pw_infrared)
    {
      player->powers[power] = INFRATICS;
      return true;
    }

  if (power == pw_ironfeet)
    {
      player->powers[power] = IRONTICS;
      return true;
    }

  if (power == pw_strength)
    {
      p_give_body(player, 100);
      player->powers[power] = 1;
      return true;
    }

  if (player->powers[power]) return false; /* already got it */

  player->powers[power] = 1;
  return true;
}

void p_touch_special_thing(mobj_t *special, mobj_t *toucher)
{
  player_t *player;
  int i;
  fixed_t delta;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  int sound;
#endif

  delta = special->z - toucher->z;

  if (delta > toucher->height || delta < -8 * FRACUNIT)
    {
      return; /* out of reach */
    }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  sound = SFX_ITEMUP;
#endif
  player = toucher->player;

  /* Dead thing touching. Can happen with a sliding player corpse. */

  if (toucher->health <= 0) return;

  /* Identify by sprite. */

  switch (special->sprite)
    {
      /* armor */

    case SPR_ARM1:
      if (!p_give_armor(player, deh_green_armor_class)) return;
      player->message = (GOTARMOR);
      break;

    case SPR_ARM2:
      if (!p_give_armor(player, deh_blue_armor_class)) return;
      player->message = (GOTMEGA);
      break;

      /* bonus items */

    case SPR_BON1:
      player->health++; /* can go over 100% */
      if (player->health > deh_max_health) player->health = deh_max_health;
      player->mo->health = player->health;
      player->message = (GOTHTHBONUS);
      break;

    case SPR_BON2:
      player->armorpoints++; /* can go over 100% */
      if (player->armorpoints > deh_max_armor && gameversion > exe_doom_1_2)
        {
          player->armorpoints = deh_max_armor;
        }

      /* deh_green_armor_class only applies to the green armor shirt;
       * for the armor helmets, armortype 1 is always used.
       */

      if (!player->armortype) player->armortype = 1;
      player->message = (GOTARMBONUS);
      break;

    case SPR_SOUL:
      player->health += deh_soulsphere_health;
      if (player->health > deh_max_soulsphere)
        player->health = deh_max_soulsphere;
      player->mo->health = player->health;
      player->message = (GOTSUPER);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_MEGA:
      if (gamemode != commercial) return;
      player->health = deh_megasphere_health;
      player->mo->health = player->health;

      /* We always give armor type 2 for the megasphere; dehacked only
       * affects the MegaArmor.
       */

      p_give_armor(player, 2);
      player->message = (GOTMSPHERE);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

      /* cards
       * leave cards for everyone
       */

    case SPR_BKEY:
      if (!player->cards[it_bluecard]) player->message = (GOTBLUECARD);
      p_give_card(player, it_bluecard);
      if (!netgame) break;
      return;

    case SPR_YKEY:
      if (!player->cards[it_yellowcard]) player->message = (GOTYELWCARD);
      p_give_card(player, it_yellowcard);
      if (!netgame) break;
      return;

    case SPR_RKEY:
      if (!player->cards[it_redcard]) player->message = (GOTREDCARD);
      p_give_card(player, it_redcard);
      if (!netgame) break;
      return;

    case SPR_BSKU:
      if (!player->cards[it_blueskull]) player->message = (GOTBLUESKUL);
      p_give_card(player, it_blueskull);
      if (!netgame) break;
      return;

    case SPR_YSKU:
      if (!player->cards[it_yellowskull]) player->message = (GOTYELWSKUL);
      p_give_card(player, it_yellowskull);
      if (!netgame) break;
      return;

    case SPR_RSKU:
      if (!player->cards[it_redskull]) player->message = (GOTREDSKULL);
      p_give_card(player, it_redskull);
      if (!netgame) break;
      return;

      /* medikits, heals */

    case SPR_STIM:
      if (!p_give_body(player, 10)) return;
      player->message = (GOTSTIM);
      break;

    case SPR_MEDI:
      if (!p_give_body(player, 25)) return;

      if (player->health < 25)
        player->message = (GOTMEDINEED);
      else
        player->message = (GOTMEDIKIT);
      break;

      /* power ups */

    case SPR_PINV:
      if (!p_give_power(player, pw_invulnerability)) return;
      player->message = (GOTINVUL);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_PSTR:
      if (!p_give_power(player, pw_strength)) return;
      player->message = (GOTBERSERK);
      if (player->readyweapon != wp_fist) player->pendingweapon = wp_fist;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_PINS:
      if (!p_give_power(player, pw_invisibility)) return;
      player->message = (GOTINVIS);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_SUIT:
      if (!p_give_power(player, pw_ironfeet)) return;
      player->message = (GOTSUIT);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_PMAP:
      if (!p_give_power(player, pw_allmap)) return;
      player->message = (GOTMAP);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

    case SPR_PVIS:
      if (!p_give_power(player, pw_infrared)) return;
      player->message = (GOTVISOR);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameversion > exe_doom_1_2) sound = SFX_GETPOW;
#endif
      break;

      /* ammo */

    case SPR_CLIP:
      if (special->flags & MF_DROPPED)
        {
          if (!p_give_ammo(player, am_clip, 0)) return;
        }
      else
        {
          if (!p_give_ammo(player, am_clip, 1)) return;
        }

      player->message = (GOTCLIP);
      break;

    case SPR_AMMO:
      if (!p_give_ammo(player, am_clip, 5)) return;
      player->message = (GOTCLIPBOX);
      break;

    case SPR_ROCK:
      if (!p_give_ammo(player, am_misl, 1)) return;
      player->message = (GOTROCKET);
      break;

    case SPR_BROK:
      if (!p_give_ammo(player, am_misl, 5)) return;
      player->message = (GOTROCKBOX);
      break;

    case SPR_CELL:
      if (!p_give_ammo(player, am_cell, 1)) return;
      player->message = (GOTCELL);
      break;

    case SPR_CELP:
      if (!p_give_ammo(player, am_cell, 5)) return;
      player->message = (GOTCELLBOX);
      break;

    case SPR_SHEL:
      if (!p_give_ammo(player, am_shell, 1)) return;
      player->message = (GOTSHELLS);
      break;

    case SPR_SBOX:
      if (!p_give_ammo(player, am_shell, 5)) return;
      player->message = (GOTSHELLBOX);
      break;

    case SPR_BPAK:
      if (!player->backpack)
        {
          for (i = 0; i < NUMAMMO; i++)
            player->maxammo[i] *= 2;
          player->backpack = true;
        }

      for (i = 0; i < NUMAMMO; i++)
        p_give_ammo(player, i, 1);
      player->message = (GOTBACKPACK);
      break;

      /* weapons */

    case SPR_BFUG:
      if (!p_give_weapon(player, wp_bfg, false)) return;
      player->message = (GOTBFG9000);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_MGUN:
      if (!p_give_weapon(player, wp_chaingun,
                         (special->flags & MF_DROPPED) != 0))
        return;
      player->message = (GOTCHAINGUN);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_CSAW:
      if (!p_give_weapon(player, wp_chainsaw, false)) return;
      player->message = (GOTCHAINSAW);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_LAUN:
      if (!p_give_weapon(player, wp_missile, false)) return;
      player->message = (GOTLAUNCHER);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_PLAS:
      if (!p_give_weapon(player, wp_plasma, false)) return;
      player->message = (GOTPLASMA);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_SHOT:
      if (!p_give_weapon(player, wp_shotgun,
                         (special->flags & MF_DROPPED) != 0))
        return;
      player->message = (GOTSHOTGUN);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    case SPR_SGN2:
      if (!p_give_weapon(player, wp_supershotgun,
                         (special->flags & MF_DROPPED) != 0))
        return;
      player->message = (GOTSHOTGUN2);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      sound = SFX_WPNUP;
#endif
      break;

    default:
      i_error("P_SpecialThing: Unknown gettable thing");
    }

  if (special->flags & MF_COUNTITEM) player->itemcount++;
  p_remove_mobj(special);
  player->bonuscount += BONUSADD;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (player == &players[consoleplayer]) s_start_sound(NULL, sound);
#endif
}

/* p_damage_mobj
 * Damages both enemies and players "inflictor" is the thing that caused the
 * damage creature or missile, can be NULL (slime, etc) "source" is the thing
 * to target after taking damage creature or NULL
 *
 * Source and inflictor are the same for melee attacks.
 * Source can be NULL for slime, barrel explosions and other environmental
 * stuff.
 */

void p_damage_mobj(mobj_t *target, mobj_t *inflictor, mobj_t *source,
                   int damage)
{
  unsigned ang;
  int saved;
  player_t *player;
  fixed_t thrust;
  int temp;

  if (!(target->flags & MF_SHOOTABLE)) return; /* shouldn't happen... */

  if (target->health <= 0) return;

  if (target->flags & MF_SKULLFLY)
    {
      target->momx = target->momy = target->momz = 0;
    }

  player = target->player;
  if (player && gameskill == sk_baby)
    damage >>= 1; /* take half damage in trainer mode */

  /* Some close combat weapons should not
   * inflict thrust and push the victim out of reach,
   * thus kick away unless using the chainsaw.
   */

  if (inflictor && !(target->flags & MF_NOCLIP) &&
      (!source || !source->player ||
       source->player->readyweapon != wp_chainsaw))
    {
      ang = r_point_to_angle2(inflictor->x, inflictor->y,
              target->x, target->y);

      thrust = damage * (FRACUNIT >> 3) * 100 / target->info->mass;

      /* make fall forwards sometimes */

      if (damage < 40 && damage > target->health &&
          target->z - inflictor->z > 64 * FRACUNIT && (p_random() & 1))
        {
          ang += ANG180;
          thrust *= 4;
        }

      ang >>= ANGLETOFINESHIFT;
      target->momx += fixed_mul(thrust, finecosine[ang]);
      target->momy += fixed_mul(thrust, finesine[ang]);
    }

  /* player specific */

  if (player)
    {
      /* end of game hell hack */

      if (target->subsector->sector->special == 11 &&
          damage >= target->health)
        {
          damage = target->health - 1;
        }

      /* Below certain threshold, ignore damage in GOD mode, or with INVUL
       * power.
       */

      if (damage < 1000 && ((player->cheats & CF_GODMODE) ||
                            player->powers[pw_invulnerability]))
        {
          return;
        }

      if (player->armortype)
        {
          if (player->armortype == 1)
            saved = damage / 3;
          else
            saved = damage / 2;

          if (player->armorpoints <= saved)
            {
              /* armor is used up */

              saved = player->armorpoints;
              player->armortype = 0;
            }

          player->armorpoints -= saved;
          damage -= saved;
        }

      player->health -= damage; /* mirror mobj health here for Dave */
      if (player->health < 0) player->health = 0;

      player->attacker = source;
      player->damagecount += damage; /* add damage after armor / invuln */

      if (player->damagecount > 100)
        {
          player->damagecount = 100; /* teleport stomp does 10k points... */
        }

      temp = damage < 100 ? damage : 100;

      if (player == &players[consoleplayer])
        {
          i_tactile(40, 10, 40 + temp * 2);
        }
    }

  /* do the damage */

  target->health -= damage;
  if (target->health <= 0)
    {
      p_kill_mobj(source, target);
      return;
    }

  if ((p_random() < target->info->painchance) &&
      !(target->flags & MF_SKULLFLY))
    {
      target->flags |= MF_JUSTHIT; /* fight back! */

      p_set_mobj_state(target, target->info->painstate);
    }

  target->reactiontime = 0; /* we're awake now... */

  if ((!target->threshold || target->type == MT_VILE) && source &&
      (source != target || gameversion < exe_doom_1_5) &&
      source->type != MT_VILE)
    {
      /* if not intent on another player, chase after this one */

      target->target = source;
      target->threshold = BASETHRESHOLD;
      if (target->state == &states[target->info->spawnstate] &&
          target->info->seestate != S_NULL)
        p_set_mobj_state(target, target->info->seestate);
    }
}
