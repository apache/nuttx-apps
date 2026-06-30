/****************************************************************************
 * apps/games/NXDoom/src/doom/g_game.c
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
 * DESCRIPTION:  none
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

#include "deh_main.h"
#include "deh_misc.h"

#include "f_finale.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "z_zone.h"

#include "p_saveg.h"
#include "p_setup.h"
#include "p_tick.h"

#include "d_main.h"

#include "am_map.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "statdump.h"
#include "wi_stuff.h"

/* Needs access to LFB. */

#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* Data. */

#include "dstrings.h"

/* SKY handling - still the wrong place. */

#include "r_data.h"
#include "r_sky.h"

#include "g_game.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SAVEGAMESIZE 0x2c000

#define MAXPLMOVE (forwardmove[1])

#define TURBOTHRESHOLD 0x32

#define BODYQUESIZE 32

#define SLOWTURNTICS 6

#define NUMKEYS 256
#define MAX_JOY_BUTTONS 20

#define DEMOMARKER 0x80

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct weapon_order
{
  weapontype_t weapon;
  weapontype_t weapon_num;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void p_spawn_player(mapthing_t *mthing);
void g_player_reborn(int player);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Gamestate the last time g_ticker was called. */

gamestate_t oldgamestate;

gameaction_t gameaction;
gamestate_t gamestate;
skill_t gameskill;
boolean respawnmonsters;
int gameepisode;
int gamemap;

/* If non-zero, exit the level after this number of minutes. */

int timelimit;

boolean paused;
boolean sendpause; /* send a pause event next tic */
boolean sendsave;  /* send a save event next tic */
boolean usergame;  /* ok to save / end game */

boolean timingdemo; /* if true, exit with report on completion */
boolean nodrawers;  /* for comparative timing purposes */
int starttime;      /* for comparative timing purposes */

boolean viewactive;

int deathmatch;  /* only if started as net death */
boolean netgame; /* only true if packets are broadcast */
boolean playeringame[MAXPLAYERS];
player_t players[MAXPLAYERS];

boolean turbodetected[MAXPLAYERS];

int consoleplayer; /* player taking events and displaying */
int displayplayer; /* view being displayed */
int levelstarttic; /* gametic at level start */
int totalkills;
int totalitems;
int totalsecret; /* for intermission */

char *demoname;
boolean demorecording;
boolean longtics;    /* cph's doom 1.91 longtics hack */
boolean lowres_turn; /* low resolution turning for longtics */
boolean demoplayback;
boolean netdemo;
byte *demobuffer;
byte *demo_p;
byte *demoend;
boolean singledemo; /* quit after playing a demo from cmdline */

boolean precache = true; /* if true, load all graphics at start */

boolean testcontrols = false; /* Invoked by setup to test controls */
int testcontrols_mousespeed;

wbstartstruct_t wminfo; /* params for world map / intermission */

byte consistency[MAXPLAYERS][CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

fixed_t forwardmove[2] =
{
    0x19, 0x32
};

fixed_t sidemove[2] =
{
    0x18, 0x28
};

/* + slow turn */

fixed_t angleturn[3] =
{
  640, 1280, 320
};

/* mouse values are used once */

int mousex;
int mousey;

mobj_t *bodyque[BODYQUESIZE];
int bodyqueslot;

int vanilla_savegame_limit = 1;
int vanilla_demo_limit = 1;

/* G_DoCompleted */

boolean secretexit;

char savename[256];

skill_t d_skill;
int d_episode;
int d_map;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int *weapon_keys[] =
{
  &key_weapon1, &key_weapon2, &key_weapon3, &key_weapon4,
  &key_weapon5, &key_weapon6, &key_weapon7, &key_weapon8,
};

/* Set to -1 or +1 to switch to the previous or next weapon. */

static int next_weapon = 0;

/* Used for prev/next weapon keys. */

static const struct weapon_order weapon_order_table[] =
{
  {wp_fist, wp_fist},
  {wp_chainsaw, wp_fist},
  {wp_pistol, wp_pistol},
  {wp_shotgun, wp_shotgun},
  {wp_supershotgun, wp_shotgun},
  {wp_chaingun, wp_chaingun},
  {wp_missile, wp_missile},
  {wp_plasma, wp_plasma},
  {wp_bfg, wp_bfg},
};

static boolean gamekeydown[NUMKEYS];
static int turnheld; /* for accelerative turning */

static boolean mousearray[MAX_MOUSE_BUTTONS + 1];
static boolean *mousebuttons = &mousearray[1]; /* allow [-1] */

static int dclicktime;
static boolean dclickstate;
static int dclicks;
static int dclicktime2;
static boolean dclickstate2;
static int dclicks2;

/* joystick values are repeated */

static int joyxmove;
static int joyymove;
static int joystrafemove;
static boolean joyarray[MAX_JOY_BUTTONS + 1];
static boolean *joybuttons = &joyarray[1]; /* allow [-1] */

static int savegameslot;
static char savedescription[32];

/* DOOM Par Times */

static const int pars[4][10] =
{
  {0},
  {0, 30, 75, 120, 90, 165, 180, 180, 30, 165},
  {0, 90, 90, 90, 120, 90, 360, 240, 30, 170},
  {0, 90, 45, 90, 150, 90, 90, 165, 30, 135},
};

/* DOOM II Par Times */

static const int cpars[32] =
{
  30,  90,  120, 120, 90,  150, 120, 120, 270, 90,  /*  1-10 */
  210, 150, 150, 150, 210, 150, 420, 150, 210, 150, /* 11-20 */
  240, 150, 180, 150, 150, 300, 330, 420, 300, 180, /* 21-30 */
  120, 30,                                          /* 31-32 */
};

/* Chex Quest Par Times */

static const int chexpars[6] =
{
  0, 120, 360, 480, 200, 360,
};

static const char *defdemoname;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static boolean weapon_selectable(weapontype_t weapon)
{
  /* Can't select the super shotgun in Doom 1. */

  if (weapon == wp_supershotgun && logical_gamemission == doom)
    {
      return false;
    }

  /* These weapons aren't available in shareware. */

  if ((weapon == wp_plasma || weapon == wp_bfg) && gamemission == doom &&
      gamemode == shareware)
    {
      return false;
    }

  /* Can't select a weapon if we don't own it. */

  if (!players[consoleplayer].weaponowned[weapon])
    {
      return false;
    }

  /* Can't select the fist if we have the chainsaw, unless
   * we also have the berserk pack.
   */

  if (weapon == wp_fist && players[consoleplayer].weaponowned[wp_chainsaw] &&
      !players[consoleplayer].powers[pw_strength])
    {
      return false;
    }

  return true;
}

static int g_next_weapon(int direction)
{
  weapontype_t weapon;
  int start_i;
  int i;

  /* Find index in the table. */

  if (players[consoleplayer].pendingweapon == wp_nochange)
    {
      weapon = players[consoleplayer].readyweapon;
    }
  else
    {
      weapon = players[consoleplayer].pendingweapon;
    }

  for (i = 0; i < arrlen(weapon_order_table); ++i)
    {
      if (weapon_order_table[i].weapon == weapon)
        {
          break;
        }
    }

  /* The current weapon must be present in weapon_order_table
   * otherwise something has gone terribly wrong
   */

  if (i >= arrlen(weapon_order_table))
    {
      i_error("Internal error: weapon %d not present in weapon_order_table",
              weapon);
    }

  /* Switch weapon. Don't loop forever. */

  start_i = i;
  do
    {
      i += direction;
      i = (i + arrlen(weapon_order_table)) % arrlen(weapon_order_table);
    }
  while (i != start_i && !weapon_selectable(weapon_order_table[i].weapon));

  return weapon_order_table[i].weapon_num;
}

static int g_cmd_checksum(ticcmd_t *cmd)
{
  size_t i;
  int sum = 0;

  for (i = 0; i < sizeof(*cmd) / 4 - 1; i++)
    sum += ((int *)cmd)[i];

  return sum;
}

static void g_do_load_level(void)
{
  int i;

  /* Set the sky map.
   * First thing, we have a dummy sky texture name,
   *  a flat. The data is in the WAD only because
   *  we look for an actual index, instead of simply
   *  setting one.
   */

  skyflatnum = r_flat_num_for_name((SKYFLATNAME));

  /* The "Sky never changes in Doom II" bug was fixed in
   * the id Anthology version of doom2.exe for Final Doom.
   */

  if ((gamemode == commercial) &&
      (gameversion == exe_final2 || gameversion == exe_chex))
    {
      const char *skytexturename;

      if (gamemap < 12)
        {
          skytexturename = "SKY1";
        }
      else if (gamemap < 21)
        {
          skytexturename = "SKY2";
        }
      else
        {
          skytexturename = "SKY3";
        }

      skytexturename = (skytexturename);

      skytexture = r_texture_num_for_name(skytexturename);
    }

  levelstarttic = gametic; /* for time calculation */

  if (wipegamestate == GS_LEVEL) wipegamestate = -1; /* force a wipe */

  gamestate = GS_LEVEL;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      turbodetected[i] = false;
      if (playeringame[i] && players[i].playerstate == PST_DEAD)
        players[i].playerstate = PST_REBORN;
      memset(players[i].frags, 0, sizeof(players[i].frags));
    }

  p_setup_level(gameepisode, gamemap, 0, gameskill);
  displayplayer = consoleplayer; /* view the guy you are playing */
  gameaction = ga_nothing;
  z_check_heap();

  /* clear cmd building stuff */

  memset(gamekeydown, 0, sizeof(gamekeydown));
  joyxmove = joyymove = joystrafemove = 0;
  mousex = mousey = 0;
  sendpause = sendsave = paused = false;
  memset(mousearray, 0, sizeof(mousearray));
  memset(joyarray, 0, sizeof(joyarray));

  if (testcontrols)
    {
      players[consoleplayer].message = "Press escape to quit.";
    }
}

static void set_joy_buttons(unsigned int buttons_mask)
{
  int i;

  for (i = 0; i < MAX_JOY_BUTTONS; ++i)
    {
      int button_on = (buttons_mask & (1 << i)) != 0;

      /* Detect button press: */

      if (!joybuttons[i] && button_on)
        {
          /* Weapon cycling: */

          if (i == joybprevweapon)
            {
              next_weapon = -1;
            }
          else if (i == joybnextweapon)
            {
              next_weapon = 1;
            }
        }

      joybuttons[i] = button_on;
    }
}

static void set_mouse_buttons(unsigned int buttons_mask)
{
  int i;

  for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
    {
      unsigned int button_on = (buttons_mask & (1 << i)) != 0;

      /* Detect button press: */

      if (!mousebuttons[i] && button_on)
        {
          if (i == mousebprevweapon)
            {
              next_weapon = -1;
            }
          else if (i == mousebnextweapon)
            {
              next_weapon = 1;
            }
        }

      mousebuttons[i] = button_on;
    }
}

/* g_check_spot
 * Returns false if the player cannot be respawned
 * at the given mapthing_t spot because something is occupying it
 */

static boolean g_check_spot(int playernum, mapthing_t *mthing)
{
  fixed_t x;
  fixed_t y;
  subsector_t *ss;
  mobj_t *mo;
  int i;

  if (!players[playernum].mo)
    {
      /* first spawn of level, before corpses */

      for (i = 0; i < playernum; i++)
        {
          if (players[i].mo->x == mthing->x << FRACBITS &&
              players[i].mo->y == mthing->y << FRACBITS)
            {
              return false;
            }
        }

      return true;
    }

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  if (!p_check_position(players[playernum].mo, x, y)) return false;

  /* flush an old corpse if needed */

  if (bodyqueslot >= BODYQUESIZE)
    {
      p_remove_mobj(bodyque[bodyqueslot % BODYQUESIZE]);
    }

  bodyque[bodyqueslot % BODYQUESIZE] = players[playernum].mo;
  bodyqueslot++;

  /* spawn a teleport fog */

  ss = r_point_in_subsector(x, y);

  /* The code in the released source looks like this:
   *
   *    an = ( ANG45 * (((unsigned int) mthing->angle)/45) )
   *         >> ANGLETOFINESHIFT;
   *    mo = p_spawn_mobj (x+20*finecosine[an], y+20*finesine[an]
   *                     , ss->sector->floorheight
   *                     , MT_TFOG);
   *
   * But 'an' can be a signed value in the DOS version. This means that
   * we get a negative index and the lookups into finecosine/finesine
   * end up dereferencing values in finetangent[].
   * A player spawning on a deathmatch start facing directly west spawns
   * "silently" with no spawn fog. Emulate this.
   *
   * This code is imported from PrBoom+.
   */

  fixed_t xa;
  fixed_t ya;
  signed int an;

  /* This calculation overflows in Vanilla Doom, but here we deliberately
   * avoid integer overflow as it is undefined behavior, so the value of
   * 'an' will always be positive.
   */

  an = (ANG45 >> ANGLETOFINESHIFT) * ((signed int)mthing->angle / 45);

  switch (an)
    {
    case 4096:                /* -4096: */
      xa = finetangent[2048]; /* finecosine[-4096] */
      ya = finetangent[0];    /* finesine[-4096] */
      break;
    case 5120:                /* -3072: */
      xa = finetangent[3072]; /* finecosine[-3072] */
      ya = finetangent[1024]; /* finesine[-3072] */
      break;
    case 6144:                /* -2048: */
      xa = finesine[0];       /* finecosine[-2048] */
      ya = finetangent[2048]; /* finesine[-2048] */
      break;
    case 7168:                /* -1024: */
      xa = finesine[1024];    /* finecosine[-1024] */
      ya = finetangent[3072]; /* finesine[-1024] */
      break;
    case 0:
    case 1024:
    case 2048:
    case 3072:
      xa = finecosine[an];
      ya = finesine[an];
      break;
    case 8192:             /* 360 deg: */
      xa = tantoangle[0];  /* finecosine[8192] */
      ya = finesine[8192]; /* finesine[8192] */
      break;
    default:
      i_error("g_check_spot: unexpected angle %d\n", an);
      xa = ya = 0;
      break;
    }

  mo = p_spawn_mobj(x + 20 * xa, y + 20 * ya, ss->sector->floorheight,
                    MT_TFOG);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (players[consoleplayer].viewz != 1)
    {
      /* don't start sound on first frame */

      s_start_sound(mo, SFX_TELEPT);
    }
#else
  UNUSED(mo);
#endif

  return true;
}

static void g_do_reborn(int playernum)
{
  int i;

  if (!netgame)
    {
      /* reload the level from scratch */

      gameaction = ga_loadlevel;
    }
  else
    {
      /* respawn at the start first disassociate the corpse */

      players[playernum].mo->player = NULL;

      /* spawn at random spot if in death match */

      if (deathmatch)
        {
          g_death_match_spawn_player(playernum);
          return;
        }

      if (g_check_spot(playernum, &playerstarts[playernum]))
        {
          p_spawn_player(&playerstarts[playernum]);
          return;
        }

      /* try to spawn at one of the other players spots */

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (g_check_spot(playernum, &playerstarts[i]))
            {
              playerstarts[i].type = playernum + 1; /* fake as other player */
              p_spawn_player(&playerstarts[i]);
              playerstarts[i].type = i + 1; /* restore */
              return;
            }

          /* he's going to be inside something.  Too bad. */
        }

      p_spawn_player(&playerstarts[playernum]);
    }
}

static void g_do_new_game(void)
{
  demoplayback = false;
  netdemo = false;
  netgame = false;
  deathmatch = false;
  playeringame[1] = playeringame[2] = playeringame[3] = 0;
  respawnparm = false;
  fastparm = false;
  nomonsters = false;
  consoleplayer = 0;
  g_init_new(d_skill, d_episode, d_map);
  gameaction = ga_nothing;
}

static void g_do_save_game(void)
{
  char *savegame_file;
  char *temp_savegame_file;
  char *recovery_savegame_file;

  recovery_savegame_file = NULL;
  temp_savegame_file = p_temp_save_game_file();
  savegame_file = p_save_game_file(savegameslot);

  /* Open the savegame file for writing.  We write to a temporary file
   * and then rename it at the end if it was successfully written.
   * This prevents an existing savegame from being overwritten by
   * a corrupted one, or if a savegame buffer overrun occurs.
   */

  save_stream = fopen(temp_savegame_file, "wb");

  if (save_stream == NULL)
    {
      /* Failed to save the game, so we're going to have to abort. But
       * to be nice, save to somewhere else before we call i_error().
       */

      recovery_savegame_file = m_temp_file("recovery.dsg");
      save_stream = fopen(recovery_savegame_file, "wb");
      if (save_stream == NULL)
        {
          i_error("Failed to open either '%s' or '%s' to write savegame.",
                  temp_savegame_file, recovery_savegame_file);
        }
    }

  savegame_error = false;

  p_write_save_game_header(savedescription);

  p_archive_players();
  p_archive_world();
  p_archive_thinkers();
  p_archive_specials();

  p_write_save_game_eof();

  /* Enforce the same savegame size limit as in Vanilla Doom,
   * except if the vanilla_savegame_limit setting is turned off.
   */

  if (vanilla_savegame_limit && ftell(save_stream) > SAVEGAMESIZE)
    {
      i_error("Savegame buffer overrun");
    }

  /* Finish up, close the savegame file. */

  fclose(save_stream);

  if (recovery_savegame_file != NULL)
    {
      /* We failed to save to the normal location, but we wrote a
       * recovery file to the temp directory. Now we can bomb out
       * with an error.
       */

      i_error("Failed to open savegame file '%s' for writing.\n"
              "But your game has been saved to '%s' for recovery.",
              temp_savegame_file, recovery_savegame_file);
    }

  /* Now rename the temporary savegame file to the actual savegame
   * file, overwriting the old savegame if there was one there.
   */

  remove(savegame_file);
  rename(temp_savegame_file, savegame_file);

  gameaction = ga_nothing;
  m_str_copy(savedescription, "", sizeof(savedescription));

  players[consoleplayer].message = (GGSAVED);

  /* draw the pattern into the back screen */

  r_fill_back_screen();
}

/* Generate a string describing a demo version */

static const char *demo_version_description(int version)
{
  static char resultbuf[16];

  switch (version)
    {
    case 104:
      return "v1.4";
    case 105:
      return "v1.5";
    case 106:
      return "v1.6/v1.666";
    case 107:
      return "v1.7/v1.7a";
    case 108:
      return "v1.8";
    case 109:
      return "v1.9";
    case 111:
      return "v1.91 hack demo?";
    default:
      break;
    }

  /* Unknown version. Perhaps this is a pre-v1.4 IWAD?  If the version
   * byte is in the range 0-4 then it can be a v1.0-v1.2 demo.
   */

  if (version >= 0 && version <= 4)
    {
      return "v1.0/v1.1/v1.2";
    }
  else
    {
      snprintf(resultbuf, sizeof(resultbuf), "%i.%i (unknown)",
              version / 100, version % 100);
      return resultbuf;
    }
}

static void g_do_play_demo(void)
{
  skill_t skill;
  int i;
  int lumpnum;
  int episode;
  int map;
  int demoversion;
  boolean olddemo = false;

  lumpnum = w_get_num_for_name(defdemoname);
  gameaction = ga_nothing;
  demobuffer = w_cache_lump_num(lumpnum, PU_STATIC);
  demo_p = demobuffer;

  demoversion = *demo_p++;

  if (demoversion >= 0 && demoversion <= 4)
    {
      olddemo = true;
      demo_p--;
    }

  longtics = false;

  /* Longtics demos use the modified format that is generated by cph's
   * hacked "v1.91" doom exe. This is a non-vanilla extension.
   */

  if (d_non_vanilla_playback(demoversion == DOOM_191_VERSION, lumpnum,
                             "Doom 1.91 demo format"))
    {
      longtics = true;
    }
  else if (demoversion != g_vanilla_version_code() &&
           !(gameversion <= exe_doom_1_2 && olddemo))
    {
      const char *message = "Demo is from a different game version!\n"
                            "(read %i, should be %i)\n"
                            "\n"
                            "*** You may need to upgrade your version "
                            "of Doom to v1.9. ***\n"
                            "    See: https://www.doomworld.com/classicdoom"
                            "/info/patches.php\n"
                            "    This appears to be %s.";

      i_error(message, demoversion, g_vanilla_version_code(),
              demo_version_description(demoversion));
    }

  skill = *demo_p++;
  episode = *demo_p++;
  map = *demo_p++;

  if (!olddemo)
    {
      deathmatch = *demo_p++;
      respawnparm = *demo_p++;
      fastparm = *demo_p++;
      nomonsters = *demo_p++;
      consoleplayer = *demo_p++;
    }
  else
    {
      deathmatch = 0;
      respawnparm = 0;
      fastparm = 0;
      nomonsters = 0;
      consoleplayer = 0;
    }

  for (i = 0; i < MAXPLAYERS; i++)
    playeringame[i] = *demo_p++;

  if (playeringame[1] || m_check_parm("-solo-net") > 0 ||
      m_check_parm("-netdemo") > 0)
    {
      netgame = true;
      netdemo = true;
    }

  /* don't spend a lot of time in loadlevel */

  precache = false;
  g_init_new(skill, episode, map);
  precache = true;
  starttime = i_get_time();

  usergame = false;
  demoplayback = true;
}

/* g_playerfinishlevel
 * Can when a player completes a level.
 */

static void g_player_finish_level(int player)
{
  player_t *p;

  p = &players[player];

  memset(p->powers, 0, sizeof(p->powers));
  memset(p->cards, 0, sizeof(p->cards));
  p->mo->flags &= ~MF_SHADOW; /* cancel invisibility */
  p->extralight = 0;          /* cancel gun flashes */
  p->fixedcolormap = 0;       /* cancel ir gogles */
  p->damagecount = 0;         /* no palette changes */
  p->bonuscount = 0;
}

static void g_do_completed(void)
{
  int i;

  gameaction = ga_nothing;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          g_player_finish_level(i); /* take away cards and stuff */
        }
    }

  if (automapactive) am_stop();

  if (gamemode != commercial)
    {
      /* Chex Quest ends after 5 levels, rather than 8. */

      if (gameversion == exe_chex)
        {
          if (gamemap == 5)
            {
              gameaction = ga_victory;
              return;
            }
        }
      else
        {
          switch (gamemap)
            {
            case 8:
              gameaction = ga_victory;
              return;
            case 9:
              for (i = 0; i < MAXPLAYERS; i++)
                players[i].didsecret = true;
              break;
            }
        }
    }

  /* #if 0  Hmmm - why? */

  if ((gamemap == 8) && (gamemode != commercial))
    {
      /* victory */

      gameaction = ga_victory;
      return;
    }

  if ((gamemap == 9) && (gamemode != commercial))
    {
      /* exit secret level */

      for (i = 0; i < MAXPLAYERS; i++)
        players[i].didsecret = true;
    }

  /* #endif */

  wminfo.didsecret = players[consoleplayer].didsecret;
  wminfo.epsd = gameepisode - 1;
  wminfo.last = gamemap - 1;

  /* wminfo.next is 0 biased, unlike gamemap */

  if (gamemode == commercial)
    {
      if (secretexit) switch (gamemap)
          {
          case 15:
            wminfo.next = 30;
            break;
          case 31:
            wminfo.next = 31;
            break;
          }
      else
        switch (gamemap)
          {
          case 31:
          case 32:
            wminfo.next = 15;
            break;
          default:
            wminfo.next = gamemap;
          }
    }
  else
    {
      if (secretexit)
        wminfo.next = 8; /* go to secret level */
      else if (gamemap == 9)
        {
          /* returning from secret level */

          switch (gameepisode)
            {
            case 1:
              wminfo.next = 3;
              break;
            case 2:
              wminfo.next = 5;
              break;
            case 3:
              wminfo.next = 6;
              break;
            case 4:
              wminfo.next = 2;
              break;
            }
        }
      else
        wminfo.next = gamemap; /* go to next level */
    }

  wminfo.maxkills = totalkills;
  wminfo.maxitems = totalitems;
  wminfo.maxsecret = totalsecret;
  wminfo.maxfrags = 0;

  /* Set par time. Exceptions are added for purposes of
   * statcheck regression testing.
   */

  if (gamemode == commercial)
    {
      /* map33 reads its par time from beyond the cpars[] array */

      if (gamemap == 33)
        {
          int cpars32;

          memcpy(&cpars32, (GAMMALVL0), sizeof(int));
          cpars32 = LONG(cpars32);

          wminfo.partime = TICRATE * cpars32;
        }
      else
        {
          wminfo.partime = TICRATE * cpars[gamemap - 1];
        }
    }

  /* Doom episode 4 doesn't have a par time, so this
   * overflows into the cpars array.
   */

  else if (gameepisode < 4)
    {
      if (gameversion == exe_chex && gameepisode == 1 && gamemap < 6)
        {
          wminfo.partime = TICRATE * chexpars[gamemap];
        }
      else
        {
          wminfo.partime = TICRATE * pars[gameepisode][gamemap];
        }
    }
  else
    {
      wminfo.partime = TICRATE * cpars[gamemap];
    }

  wminfo.pnum = consoleplayer;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      wminfo.plyr[i].in = playeringame[i];
      wminfo.plyr[i].skills = players[i].killcount;
      wminfo.plyr[i].sitems = players[i].itemcount;
      wminfo.plyr[i].ssecret = players[i].secretcount;
      wminfo.plyr[i].stime = leveltime;
      memcpy(wminfo.plyr[i].frags, players[i].frags,
             sizeof(wminfo.plyr[i].frags));
    }

  gamestate = GS_INTERMISSION;
  viewactive = false;
  automapactive = false;

  stat_copy(&wminfo);

  wi_start(&wminfo);
}

static void g_do_world_done(void)
{
  gamestate = GS_LEVEL;
  gamemap = wminfo.next + 1;
  g_do_load_level();
  gameaction = ga_nothing;
  viewactive = true;
}

/* Increase the size of the demo buffer to allow unlimited demos */

static void increase_demo_buffer(void)
{
  int current_length;
  byte *new_demobuffer;
  byte *new_demop;
  int new_length;

  /* Find the current size */

  current_length = demoend - demobuffer;

  /* Generate a new buffer twice the size */

  new_length = current_length * 2;

  new_demobuffer = z_malloc(new_length, PU_STATIC, 0);
  new_demop = new_demobuffer + (demo_p - demobuffer);

  /* Copy over the old data */

  memcpy(new_demobuffer, demobuffer, current_length);

  /* Free the old buffer and point the demo pointers at the new buffer. */

  z_free(demobuffer);

  demobuffer = new_demobuffer;
  demo_p = new_demop;
  demoend = demobuffer + new_length;
}

/* DEMO RECORDING */

static void g_read_demo_ticcmd(ticcmd_t *cmd)
{
  if (*demo_p == DEMOMARKER)
    {
      /* end of demo data stream */

      g_check_demo_status();
      return;
    }

  cmd->forwardmove = ((signed char)*demo_p++);
  cmd->sidemove = ((signed char)*demo_p++);

  /* If this is a longtics demo, read back in higher resolution */

  if (longtics)
    {
      cmd->angleturn = *demo_p++;
      cmd->angleturn |= (*demo_p++) << 8;
    }
  else
    {
      cmd->angleturn = ((unsigned char)*demo_p++) << 8;
    }

  cmd->buttons = (unsigned char)*demo_p++;
}

static void g_write_demo_ticcmd(ticcmd_t *cmd)
{
  byte *demo_start;

  if (gamekeydown[key_demo_quit]) /* press q to end demo recording */
    g_check_demo_status();

  demo_start = demo_p;

  *demo_p++ = cmd->forwardmove;
  *demo_p++ = cmd->sidemove;

  /* If this is a longtics demo, record in higher resolution */

  if (longtics)
    {
      *demo_p++ = (cmd->angleturn & 0xff);
      *demo_p++ = (cmd->angleturn >> 8) & 0xff;
    }
  else
    {
      *demo_p++ = cmd->angleturn >> 8;
    }

  *demo_p++ = cmd->buttons;

  /* reset demo pointer back */

  demo_p = demo_start;

  if (demo_p > demoend - 16)
    {
      if (vanilla_demo_limit)
        {
          /* no more space */

          g_check_demo_status();
          return;
        }
      else
        {
          /* Vanilla demo limit disabled: unlimited
           * demo lengths!
           */

          increase_demo_buffer();
        }
    }

  g_read_demo_ticcmd(cmd); /* make SURE it is exactly the same */
}

/* Called at the start.
 * Called by the game initialization functions.
 */

static void g_init_player(int player)
{
  /* clear everything else to defaults */

  g_player_reborn(player);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* g_build_ticcmd
 * Builds a ticcmd from all of the available inputs
 * or reads it from the demo buffer.
 * If recording a demo, write it out
 */

void g_build_ticcmd(ticcmd_t *cmd, int maketic)
{
  int i;
  boolean strafe;
  boolean bstrafe;
  int speed;
  int tspeed;
  int forward;
  int side;

  memset(cmd, 0, sizeof(ticcmd_t));

  cmd->consistency =
      consistency[consoleplayer]
                 [maketic % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS];

  strafe = gamekeydown[key_strafe] || mousebuttons[mousebstrafe] ||
           joybuttons[joybstrafe];

  /* fraggle: support the old "joyb_speed = 31" hack which
   * allowed an autorun effect
   */

  speed = key_speed >= NUMKEYS || joybspeed >= MAX_JOY_BUTTONS ||
          gamekeydown[key_speed] || joybuttons[joybspeed] ||
          mousebuttons[mousebspeed];

  forward = side = 0;

  /* use two stage accelerative turning
   * on the keyboard and joystick
   */

  if (joyxmove < 0 || joyxmove > 0 || gamekeydown[key_right] ||
      gamekeydown[key_left] || mousebuttons[mousebturnright] ||
      mousebuttons[mousebturnleft])
    {
      turnheld += ticdup;
    }
  else
    {
      turnheld = 0;
    }

  if (turnheld < SLOWTURNTICS)
    {
      tspeed = 2; /* slow turn */
    }
  else
    {
      tspeed = speed;
    }

  /* let movement keys cancel each other out */

  if (strafe)
    {
      if (gamekeydown[key_right] || mousebuttons[mousebturnright])
        {
          side += sidemove[speed];
        }

      if (gamekeydown[key_left] || mousebuttons[mousebturnleft])
        {
          side -= sidemove[speed];
        }

      if (use_analog && joyxmove)
        {
          joyxmove = joyxmove * joystick_move_sensitivity / 10;
          joyxmove = (joyxmove > FRACUNIT) ? FRACUNIT : joyxmove;
          joyxmove = (joyxmove < -FRACUNIT) ? -FRACUNIT : joyxmove;
          side += fixed_mul(sidemove[speed], joyxmove);
        }
      else if (joystick_move_sensitivity)
        {
          if (joyxmove > 0) side += sidemove[speed];
          if (joyxmove < 0) side -= sidemove[speed];
        }
    }
  else
    {
      if (gamekeydown[key_right] || mousebuttons[mousebturnright])
        cmd->angleturn -= angleturn[tspeed];
      if (gamekeydown[key_left] || mousebuttons[mousebturnleft])
        cmd->angleturn += angleturn[tspeed];
      if (use_analog && joyxmove)
        {
          /* Cubic response curve allows for finer control when stick
           * deflection is small.
           */

          joyxmove = fixed_mul(fixed_mul(joyxmove, joyxmove), joyxmove);
          joyxmove = joyxmove * joystick_turn_sensitivity / 10;
          cmd->angleturn -= fixed_mul(angleturn[1], joyxmove);
        }
      else if (joystick_turn_sensitivity)
        {
          if (joyxmove > 0) cmd->angleturn -= angleturn[tspeed];
          if (joyxmove < 0) cmd->angleturn += angleturn[tspeed];
        }
    }

  if (gamekeydown[key_up])
    {
      forward += forwardmove[speed];
    }

  if (gamekeydown[key_down])
    {
      forward -= forwardmove[speed];
    }

  if (use_analog && joyymove)
    {
      joyymove = joyymove * joystick_move_sensitivity / 10;
      joyymove = (joyymove > FRACUNIT) ? FRACUNIT : joyymove;
      joyymove = (joyymove < -FRACUNIT) ? -FRACUNIT : joyymove;
      forward -= fixed_mul(forwardmove[speed], joyymove);
    }
  else if (joystick_move_sensitivity)
    {
      if (joyymove < 0) forward += forwardmove[speed];
      if (joyymove > 0) forward -= forwardmove[speed];
    }

  if (gamekeydown[key_strafeleft] || joybuttons[joybstrafeleft] ||
      mousebuttons[mousebstrafeleft])
    {
      side -= sidemove[speed];
    }

  if (gamekeydown[key_straferight] || joybuttons[joybstraferight] ||
      mousebuttons[mousebstraferight])
    {
      side += sidemove[speed];
    }

  if (use_analog && joystrafemove)
    {
      joystrafemove = joystrafemove * joystick_move_sensitivity / 10;
      joystrafemove = (joystrafemove > FRACUNIT) ? FRACUNIT : joystrafemove;
      joystrafemove =
          (joystrafemove < -FRACUNIT) ? -FRACUNIT : joystrafemove;
      side += fixed_mul(sidemove[speed], joystrafemove);
    }
  else if (joystick_move_sensitivity)
    {
      if (joystrafemove < 0) side -= sidemove[speed];
      if (joystrafemove > 0) side += sidemove[speed];
    }

  /* buttons */

  cmd->chatchar = hu_dequeue_chat_char();

  if (gamekeydown[key_fire] || mousebuttons[mousebfire] ||
      joybuttons[joybfire])
    cmd->buttons |= BT_ATTACK;

  if (gamekeydown[key_use] || joybuttons[joybuse] || mousebuttons[mousebuse])
    {
      cmd->buttons |= BT_USE;
      dclicks = 0; /* clear double clicks if hit use button */
    }

  /* If the previous or next weapon button is pressed, the
   * next_weapon variable is set to change weapons when
   * we generate a ticcmd.  Choose a new weapon.
   */

  if (gamestate == GS_LEVEL && next_weapon != 0)
    {
      i = g_next_weapon(next_weapon);
      cmd->buttons |= BT_CHANGE;
      cmd->buttons |= i << BT_WEAPONSHIFT;
    }
  else
    {
      /* Check weapon keys. */

      for (i = 0; i < arrlen(weapon_keys); ++i)
        {
          int key = *weapon_keys[i];

          if (gamekeydown[key])
            {
              cmd->buttons |= BT_CHANGE;
              cmd->buttons |= i << BT_WEAPONSHIFT;
              break;
            }
        }
    }

  next_weapon = 0;

  /* mouse */

  if (mousebuttons[mousebforward])
    {
      forward += forwardmove[speed];
    }

  if (mousebuttons[mousebbackward])
    {
      forward -= forwardmove[speed];
    }

  if (dclick_use)
    {
      /* forward double click */

      if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1)
        {
          dclickstate = mousebuttons[mousebforward];
          if (dclickstate) dclicks++;
          if (dclicks == 2)
            {
              cmd->buttons |= BT_USE;
              dclicks = 0;
            }
          else
            dclicktime = 0;
        }
      else
        {
          dclicktime += ticdup;
          if (dclicktime > 20)
            {
              dclicks = 0;
              dclickstate = 0;
            }
        }

      /* strafe double click */

      bstrafe = mousebuttons[mousebstrafe] || joybuttons[joybstrafe];
      if (bstrafe != dclickstate2 && dclicktime2 > 1)
        {
          dclickstate2 = bstrafe;
          if (dclickstate2) dclicks2++;
          if (dclicks2 == 2)
            {
              cmd->buttons |= BT_USE;
              dclicks2 = 0;
            }
          else
            dclicktime2 = 0;
        }
      else
        {
          dclicktime2 += ticdup;
          if (dclicktime2 > 20)
            {
              dclicks2 = 0;
              dclickstate2 = 0;
            }
        }
    }

  forward += mousey;

  if (strafe)
    side += mousex * 2;
  else
    cmd->angleturn -= mousex * 0x8;

  if (mousex == 0)
    {
      /* No movement in the previous frame */

      testcontrols_mousespeed = 0;
    }

  mousex = mousey = 0;

  if (forward > MAXPLMOVE)
    forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
    forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
    side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
    side = -MAXPLMOVE;

  cmd->forwardmove += forward;
  cmd->sidemove += side;

  /* special buttons */

  if (sendpause)
    {
      sendpause = false;
      cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

  if (sendsave)
    {
      sendsave = false;
      cmd->buttons =
          BT_SPECIAL | BTS_SAVEGAME | (savegameslot << BTS_SAVESHIFT);
    }

  /* low-res turning */

  if (lowres_turn)
    {
      static signed short carry = 0;
      signed short desired_angleturn;

      desired_angleturn = cmd->angleturn + carry;

      /* round angleturn to the nearest 256 unit boundary
       * for recording demos with single byte values for turn
       */

      cmd->angleturn = (desired_angleturn + 128) & 0xff00;

      /* Carry forward the error from the reduced resolution to the
       * next tic, so that successive small movements can accumulate.
       */

      carry = desired_angleturn - cmd->angleturn;
    }
}

/* g_responder
 * Get info needed to make ticcmd_ts for the players.
 */

boolean g_responder(event_t *ev)
{
  /* allow spy mode changes even during the demo */

  if (gamestate == GS_LEVEL && ev->type == ev_keydown &&
      ev->data1 == key_spy && (singledemo || !deathmatch))
    {
      /* spy mode */

      do
        {
          displayplayer++;
          if (displayplayer == MAXPLAYERS) displayplayer = 0;
        }
      while (!playeringame[displayplayer] && displayplayer != consoleplayer);
      return true;
    }

  /* any other key pops up menu if in demos */

  if (gameaction == ga_nothing && !singledemo &&
      (demoplayback || gamestate == GS_DEMOSCREEN))
    {
      if (ev->type == ev_keydown || (ev->type == ev_mouse && ev->data1) ||
          (ev->type == ev_joystick && ev->data1))
        {
          m_start_control_panel();
          joywait = i_get_time() + 5;
          return true;
        }

      return false;
    }

  if (gamestate == GS_LEVEL)
    {
#if 0
      if (devparm && ev->type == ev_keydown && ev->data1 == ';')
        {
          g_death_match_spawn_player(0);
          return true;
        }

#endif
      if (hu_responder(ev)) return true; /* chat ate the event */
      if (st_responder(ev)) return true; /* status window ate it */
      if (am_responder(ev)) return true; /* automap ate it */
    }

  if (gamestate == GS_FINALE)
    {
      if (f_responder(ev)) return true; /* finale ate the event */
    }

  if (testcontrols && ev->type == ev_mouse)
    {
      /* If we are invoked by setup to test the controls, save the
       * mouse speed so that we can display it on-screen.
       * Perform a low pass filter on this so that the thermometer
       * appears to move smoothly.
       */

      testcontrols_mousespeed = abs(ev->data2);
    }

  /* If the next/previous weapon keys are pressed, set the next_weapon
   * variable to change weapons when the next ticcmd is generated.
   */

  if (ev->type == ev_keydown && ev->data1 == key_prevweapon)
    {
      next_weapon = -1;
    }
  else if (ev->type == ev_keydown && ev->data1 == key_nextweapon)
    {
      next_weapon = 1;
    }

  switch (ev->type)
    {
    case ev_keydown:
      if (ev->data1 == key_pause)
        {
          sendpause = true;
        }
      else if (ev->data1 < NUMKEYS)
        {
          gamekeydown[ev->data1] = true;
        }

      return true; /* eat key down events */

    case ev_keyup:
      if (ev->data1 < NUMKEYS) gamekeydown[ev->data1] = false;
      return false; /* always let key up events filter down */

    case ev_mouse:
      set_mouse_buttons(ev->data1);
      mousex = ev->data2 * (g_mouse_sensitivity + 5) / 10;
      mousey = ev->data3 * (g_mouse_sensitivity + 5) / 10;
      return true; /* eat events */

    case ev_joystick:
      set_joy_buttons(ev->data1);
      joyxmove = ev->data2;
      joyymove = ev->data3;
      joystrafemove = ev->data4;
      return true; /* eat events */

    default:
      break;
    }

  return false;
}

/* g_ticker
 * Make ticcmd_ts for the players.
 */

void g_ticker(void)
{
  int i;
  int buf;
  ticcmd_t *cmd;

  /* do player reborns if needed */

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i] && players[i].playerstate == PST_REBORN)
        {
          g_do_reborn(i);
        }
    }

  /* do things to change the game state */

  while (gameaction != ga_nothing)
    {
      switch (gameaction)
        {
        case ga_loadlevel:
          g_do_load_level();
          break;
        case ga_newgame:
          g_do_new_game();
          break;
        case ga_loadgame:
          g_do_load_game();
          break;
        case ga_savegame:
          g_do_save_game();
          break;
        case ga_playdemo:
          g_do_play_demo();
          break;
        case ga_completed:
          g_do_completed();
          break;
        case ga_victory:
          f_start_finale();
          break;
        case ga_worlddone:
          g_do_world_done();
          break;
        case ga_screenshot:
          v_screenshot("DOOM%02i.%s");
          players[consoleplayer].message = ("screen shot");
          gameaction = ga_nothing;
          break;
        case ga_nothing:
          break;
        }
    }

  /* get commands, check consistency, and build new consistency check */

  buf = (gametic / ticdup) % CONFIG_GAMES_NXDOOM_NET_BACKUPTICS;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          cmd = &players[i].cmd;

          memcpy(cmd, &netcmds[i], sizeof(ticcmd_t));

          if (demoplayback) g_read_demo_ticcmd(cmd);
          if (demorecording) g_write_demo_ticcmd(cmd);

          /* check for turbo cheats
           * check ~ 4 seconds whether to display the turbo message.
           * store if the turbo threshold was exceeded in any tics
           * over the past 4 seconds.  offset the checking period
           * for each player so messages are not displayed at the
           * same time.
           */

          if (cmd->forwardmove > TURBOTHRESHOLD)
            {
              turbodetected[i] = true;
            }

          if ((gametic & 31) == 0 && ((gametic >> 5) % MAXPLAYERS) == i &&
              turbodetected[i])
            {
              static char turbomessage[80];
              snprintf(turbomessage, sizeof(turbomessage), "%s is turbo!",
                       player_names[i]);
              players[consoleplayer].message = turbomessage;
              turbodetected[i] = false;
            }

          if (netgame && !netdemo && !(gametic % ticdup))
            {
              if (gametic > CONFIG_GAMES_NXDOOM_NET_BACKUPTICS &&
                  consistency[i][buf] != cmd->consistency)
                {
                  i_error("consistency failure (%i should be %i)",
                          cmd->consistency, consistency[i][buf]);
                }

              if (players[i].mo)
                consistency[i][buf] = players[i].mo->x;
              else
                consistency[i][buf] = rndindex;
            }
        }
    }

  /* check for special buttons */

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          if (players[i].cmd.buttons & BT_SPECIAL)
            {
              switch (players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                  paused ^= 1;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
                  if (paused)
                    s_pause_sound();
                  else
                    s_resume_sound();
                  break;
#endif

                case BTS_SAVEGAME:
                  if (!savedescription[0])
                    {
                      m_str_copy(savedescription, "NET GAME",
                                 sizeof(savedescription));
                    }

                  savegameslot = (players[i].cmd.buttons & BTS_SAVEMASK) >>
                                 BTS_SAVESHIFT;
                  gameaction = ga_savegame;
                  break;
                }
            }
        }
    }

  /* Have we just finished displaying an intermission screen? */

  if (oldgamestate == GS_INTERMISSION && gamestate != GS_INTERMISSION)
    {
      wi_end();
    }

  oldgamestate = gamestate;

  /* do main actions */

  switch (gamestate)
    {
    case GS_LEVEL:
      p_ticker();
      st_ticker();
      am_ticker();
      hu_ticker();
      break;

    case GS_INTERMISSION:
      wi_ticker();
      break;

    case GS_FINALE:
      f_ticker();
      break;

    case GS_DEMOSCREEN:
      d_page_ticker();
      break;
    }
}

/* PLAYER STRUCTURE FUNCTIONS
 * also see p_spawn_player in P_Things
 */

/* g_player_reborn
 * Called after a player dies almost everything is cleared and initialized
 */

void g_player_reborn(int player)
{
  player_t *p;
  int i;
  int frags[MAXPLAYERS];
  int killcount;
  int itemcount;
  int secretcount;

  memcpy(frags, players[player].frags, sizeof(frags));
  killcount = players[player].killcount;
  itemcount = players[player].itemcount;
  secretcount = players[player].secretcount;

  p = &players[player];
  memset(p, 0, sizeof(*p));

  memcpy(players[player].frags, frags, sizeof(players[player].frags));
  players[player].killcount = killcount;
  players[player].itemcount = itemcount;
  players[player].secretcount = secretcount;

  p->usedown = p->attackdown = true; /* don't do anything immediately */
  p->playerstate = PST_LIVE;
  p->health = deh_initial_health; /* Use dehacked value */
  p->readyweapon = p->pendingweapon = wp_pistol;
  p->weaponowned[wp_fist] = true;
  p->weaponowned[wp_pistol] = true;
  p->ammo[am_clip] = deh_initial_bullets;

  for (i = 0; i < NUMAMMO; i++)
    p->maxammo[i] = maxammo[i];
}

/* g_death_match_spawn_player
 * Spawns a player at one of the random death match spots
 * called at level load and each death
 */

void g_death_match_spawn_player(int playernum)
{
  int i;
  int j;
  int selections;

  selections = deathmatch_p - deathmatchstarts;
  if (selections < 4)
    i_error("Only %i deathmatch spots, 4 required", selections);

  for (j = 0; j < 20; j++)
    {
      i = p_random() % selections;
      if (g_check_spot(playernum, &deathmatchstarts[i]))
        {
          deathmatchstarts[i].type = playernum + 1;
          p_spawn_player(&deathmatchstarts[i]);
          return;
        }
    }

  /* no good spot, so the player will probably get stuck */

  p_spawn_player(&playerstarts[playernum]);
}

void g_screenshot(void)
{
  gameaction = ga_screenshot;
}

void g_exit_level(void)
{
  secretexit = false;
  gameaction = ga_completed;
}

/* Here's for the german edition. */

void g_secret_exit_level(void)
{
  /* IF NO WOLF3D LEVELS, NO SECRET EXIT! */

  if ((gamemode == commercial) && (w_check_num_for_name("map31") < 0))
    secretexit = false;
  else
    secretexit = true;
  gameaction = ga_completed;
}

void g_world_done(void)
{
  gameaction = ga_worlddone;

  if (secretexit) players[consoleplayer].didsecret = true;

  if (gamemode == commercial)
    {
      switch (gamemap)
        {
        case 15:
        case 31:
          if (!secretexit) break;
        case 6:
        case 11:
        case 20:
        case 30:
          f_start_finale();
          break;
        }
    }
}

/* g_initfromsavegame
 * Can be called by the startup code or the menu task.
 */

void g_load_game(char *name)
{
  m_str_copy(savename, name, sizeof(savename));
  gameaction = ga_loadgame;
}

void g_do_load_game(void)
{
  int savedleveltime;

  gameaction = ga_nothing;

  save_stream = fopen(savename, "rb");

  if (save_stream == NULL)
    {
      i_error("Could not load savegame %s", savename);
    }

  savegame_error = false;

  if (!p_read_save_game_header())
    {
      fclose(save_stream);
      return;
    }

  savedleveltime = leveltime;

  /* load a base level */

  g_init_new(gameskill, gameepisode, gamemap);

  leveltime = savedleveltime;

  /* dearchive all the modifications */

  p_unarchive_players();
  p_unarchive_world();
  p_unarchive_thinkers();
  p_unarchive_specials();

  if (!p_read_save_game_eof()) i_error("Bad savegame");

  fclose(save_stream);

  if (setsizeneeded) r_execute_set_view_size();

  /* draw the pattern into the back screen */

  r_fill_back_screen();
}

/* g_save_game
 * Called by the menu task.
 * Description is a 24 byte text string
 */

void g_save_game(int slot, char *description)
{
  savegameslot = slot;
  m_str_copy(savedescription, description, sizeof(savedescription));
  sendsave = true;
}

/* g_init_new
 * Can be called by the startup code or the menu task,
 * consoleplayer, displayplayer, playeringame[] should be set.
 */

void g_deferred_init_new(skill_t skill, int episode, int map)
{
  d_skill = skill;
  d_episode = episode;
  d_map = map;
  gameaction = ga_newgame;
}

void g_init_new(skill_t skill, int episode, int map)
{
  const char *skytexturename;
  int i;

  if (paused)
    {
      paused = false;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_resume_sound();
#endif
    }

  /* Note: This commented-out block of code was added at some point
   * between the DOS version(s) and the Doom source release. It isn't
   * found in disassemblies of the DOS version and causes IDCLEV and
   * the -warp command line parameter to behave differently.
   * This is left here for posterity.
   * This was quite messy with SPECIAL and commented parts.
   * Supposedly hacks to make the latest edition work.
   * It might not work properly.
   *
   * if (episode < 1)
   *   episode = 1;
   * if ( gamemode == retail )
   * {
   *   if (episode > 4)
   *     episode = 4;
   * }
   * else if ( gamemode == shareware )
   * {
   *   if (episode > 1)
   *        episode = 1;  // only start episode 1 on shareware
   * }
   * else
   * {
   *   if (episode > 3)
   *     episode = 3;
   * }
   */

  if (skill > sk_nightmare) skill = sk_nightmare;

  if (gameversion >= exe_ultimate)
    {
      if (episode == 0)
        {
          episode = 4;
        }
    }
  else
    {
      if (episode < 1)
        {
          episode = 1;
        }

      if (episode > 3)
        {
          episode = 3;
        }
    }

  if (episode > 1 && gamemode == shareware)
    {
      episode = 1;
    }

  if (map < 1) map = 1;

  if ((map > 9) && (gamemode != commercial)) map = 9;

  m_clear_random();

  if (skill == sk_nightmare || respawnparm)
    respawnmonsters = true;
  else
    respawnmonsters = false;

  if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare))
    {
      for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
        states[i].tics >>= 1;
      mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
      mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
      mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
    }
  else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
      for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
        states[i].tics <<= 1;
      mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
      mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
      mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
    }

  /* force players to be initialized upon first level load */

  for (i = 0; i < MAXPLAYERS; i++)
    players[i].playerstate = PST_REBORN;

  usergame = true; /* will be set false if a demo */
  paused = false;
  demoplayback = false;
  automapactive = false;
  viewactive = true;
  gameepisode = episode;
  gamemap = map;
  gameskill = skill;

  /* Set the sky to use.
   *
   * Note: This IS broken, but it is how Vanilla Doom behaves.
   * See http: *doomwiki.org/wiki/Sky_never_changes_in_Doom_II.
   *
   * Because we set the sky here at the start of a game, not at the
   * start of a level, the sky texture never changes unless we
   * restore from a saved game.  This was fixed before the Doom
   * source release, but this IS the way Vanilla DOS Doom behaves.
   */

  if (gamemode == commercial)
    {
      skytexturename = ("SKY3");
      skytexture = r_texture_num_for_name(skytexturename);
      if (gamemap < 21)
        {
          skytexturename = (gamemap < 12 ? "SKY1" : "SKY2");
          skytexture = r_texture_num_for_name(skytexturename);
        }
    }
  else
    {
      switch (gameepisode)
        {
        default:
        case 1:
          skytexturename = "SKY1";
          break;
        case 2:
          skytexturename = "SKY2";
          break;
        case 3:
          skytexturename = "SKY3";
          break;
        case 4: /* Special Edition sky */
          skytexturename = "SKY4";
          break;
        }

      skytexturename = (skytexturename);
      skytexture = r_texture_num_for_name(skytexturename);
    }

  g_do_load_level();
}

void g_record_demo(const char *name)
{
  size_t demoname_size;
  int i;
  int maxsize;

  usergame = false;
  demoname_size = strlen(name) + 5;
  demoname = z_malloc(demoname_size, PU_STATIC, NULL);
  snprintf(demoname, demoname_size, "%s.lmp", name);
  maxsize = 0x20000;

  /* @arg <size>
   * @category demo
   * @vanilla
   *
   * Specify the demo buffer size (KiB)
   */

  i = m_check_parm_with_args("-maxdemo", 1);
  if (i) maxsize = atoi(myargv[i + 1]) * 1024;
  demobuffer = z_malloc(maxsize, PU_STATIC, NULL);
  demoend = demobuffer + maxsize;

  demorecording = true;
}

/* Get the demo version code appropriate for the version set in gameversion.
 */

int g_vanilla_version_code(void)
{
  switch (gameversion)
    {
    case exe_doom_1_666:
      return 106;
    case exe_doom_1_7:
      return 107;
    case exe_doom_1_8:
      return 108;
    case exe_doom_1_9:
    default: /* All other versions are variants on v1.9: */
      return 109;
    }
}

void g_begin_recording(void)
{
  int i;

  demo_p = demobuffer;

  /* @category demo
   *
   * Record a high resolution "Doom 1.91" demo.
   */

  longtics = d_non_vanilla_record(m_parm_exists("-longtics"),
                                  "Doom 1.91 demo format");

  /* If not recording a longtics demo, record in low res */

  lowres_turn = !longtics;

  if (longtics)
    {
      *demo_p++ = DOOM_191_VERSION;
    }
  else if (gameversion > exe_doom_1_2)
    {
      *demo_p++ = g_vanilla_version_code();
    }

  *demo_p++ = gameskill;
  *demo_p++ = gameepisode;
  *demo_p++ = gamemap;
  if (longtics || gameversion > exe_doom_1_2)
    {
      *demo_p++ = deathmatch;
      *demo_p++ = respawnparm;
      *demo_p++ = fastparm;
      *demo_p++ = nomonsters;
      *demo_p++ = consoleplayer;
    }

  for (i = 0; i < MAXPLAYERS; i++)
    *demo_p++ = playeringame[i];
}

void g_defered_play_demo(const char *name)
{
  defdemoname = name;
  gameaction = ga_playdemo;
}

void g_time_demo(char *name)
{
  /* @category video
   * @vanilla
   *
   * Disable rendering the screen entirely.
   */

  nodrawers = m_check_parm("-nodraw");

  timingdemo = true;
  singletics = true;

  defdemoname = name;
  gameaction = ga_playdemo;
}

/* g_check_demo_status
 *
 * Called after a death or level completion to allow demos to be cleaned up
 * Returns true if a new demo loop action will take place
 */

boolean g_check_demo_status(void)
{
  int endtime;

  if (timingdemo)
    {
      float fps;
      int realtics;

      endtime = i_get_time();
      realtics = endtime - starttime;
      fps = ((float)gametic * TICRATE) / realtics;

      /* Prevent recursive calls */

      timingdemo = false;
      demoplayback = false;

      i_error("timed %i gametics in %i realtics (%f fps)", gametic, realtics,
              fps);
    }

  if (demoplayback)
    {
      w_release_lump_name(defdemoname);
      demoplayback = false;
      netdemo = false;
      netgame = false;
      deathmatch = false;
      playeringame[1] = playeringame[2] = playeringame[3] = 0;
      respawnparm = false;
      fastparm = false;
      nomonsters = false;
      consoleplayer = 0;

      if (singledemo)
        i_quit();
      else
        d_advance_demo();

      return true;
    }

  if (demorecording)
    {
      *demo_p++ = DEMOMARKER;
      m_write_file(demoname, demobuffer, demo_p - demobuffer);
      z_free(demobuffer);
      demorecording = false;
      i_error("Demo %s recorded", demoname);
    }

  return false;
}
