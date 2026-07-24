/****************************************************************************
 * apps/games/NXDoom/src/doom/d_main.c
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
 *  DOOM main program (d_doom_main) and game loop (d_doomloop),
 *  plus functions to determine game mode (shareware, registered),
 *  parse command line parameters, configure game parameters (turbo),
 *  and call the startup functions.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "d_iwad.h"

#include "v_diskicon.h"
#include "v_video.h"
#include "w_main.h"
#include "w_wad.h"
#include "z_zone.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_saveg.h"

#include "i_endoom.h"
#include "i_input.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "g_game.h"

#include "am_map.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "wi_stuff.h"

#ifdef CONFIG_GAMES_NXDOOM_NET
#include "net_client.h"
#include "net_dedicated.h"
#include "net_query.h"
#endif

#include "p_setup.h"
#include "r_local.h"
#include "statdump.h"

#include "d_main.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pack
{
  const char *name;
  int mission;
};

struct gameversion
{
  const char *description;
  const char *cmdline;
  game_version_t version;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* D-DoomLoop()
 *
 * Not a globally visible function, just included for source reference,
 * called by d_doom_main, never exits.
 *
 * Manages timing and IO, calls all ?_Responder, ?_Ticker, and ?_Drawer,
 * calls i_get_time, i_start_frame, and i_start_tic
 */

void d_doomloop(void);

void d_connect_net_game(void);
void d_check_net_game(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Location where savegames are stored */

char *savegamedir;

/* location of IWAD and WAD files */

char *iwadfile;

boolean devparm;     /* started game with -devparm */
boolean nomonsters;  /* checkparm of -nomonsters */
boolean respawnparm; /* checkparm of -respawn */
boolean fastparm;    /* checkparm of -fast */

skill_t startskill;
int startepisode;
int startmap;
boolean autostart;
int startloadgame;

boolean advancedemo;

/* Store demo, do not accept any inputs */

boolean storedemo;

/* If true, the main game loop has started. */

boolean main_loop_started = false;

int show_endoom = 1;
int show_diskicon = 1;

/* wipegamestate can be set to -1 to force a wipe on the next draw */

gamestate_t wipegamestate = GS_DEMOSCREEN;

/* DEMO LOOP */

int demosequence;
int pagetic;
const char *pagename;

/*      print title for every printed line */

char title[128];

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Copyright message banners
 * Some dehacked mods replace these.  These are only displayed if they are
 * replaced by dehacked.
 */

static const char *g_copyright_banners[] =
{
  "========================================================================"
  "===\n"
  "ATTENTION:  This version of DOOM has been modified.  If you would like "
  "to\n"
  "get a copy of the original game, call 1-800-IDGAMES or see the readme "
  "file.\n"
  "        You will not receive technical support for modified games.\n"
  "                      press enter to continue\n"
  "========================================================================"
  "===\n",

  "========================================================================"
  "===\n"
  "                 Commercial product - do not distribute!\n"
  "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
  "========================================================================"
  "===\n",

  "========================================================================"
  "===\n"
  "                                Shareware!\n"
  "========================================================================"
  "===\n",
};

/* These are the lumps that will be checked in IWAD,
 * if any one is not present, execution will be aborted.
 */

static char g_name[23][8] =
{
  "e2m1",   "e2m2",   "e2m3",   "e2m4",   "e2m5",     "e2m6",
  "e2m7",   "e2m8",   "e2m9",   "e3m1",   "e3m3",     "e3m3",
  "e3m4",   "e3m5",   "e3m6",   "e3m7",   "e3m8",     "e3m9",
  "dphoof", "bfgga0", "heada1", "cybra1", "spida1d1",
};

static char *g_gamedescription;

/* Add configuration file variable bindings. */

static const char *const g_chat_macro_defaults[10] =
{
  HUSTR_CHATMACRO0, HUSTR_CHATMACRO1, HUSTR_CHATMACRO2, HUSTR_CHATMACRO3,
  HUSTR_CHATMACRO4, HUSTR_CHATMACRO5, HUSTR_CHATMACRO6, HUSTR_CHATMACRO7,
  HUSTR_CHATMACRO8, HUSTR_CHATMACRO9,
};

/* Strings for dehacked replacements of the startup banner
 *
 * These are from the original source: some of them are perhaps
 * not used in any dehacked patches
 */

static const char *g_banners[] =
{
  /* doom2.wad */

  "                         "
  "DOOM 2: Hell on Earth v%i.%i"
  "                           ",

  /* doom2.wad v1.666 */

  "                         "
  "DOOM 2: Hell on Earth v%i.%i66"
  "                          ",

  /* doom1.wad */

  "                            "
  "DOOM Shareware Startup v%i.%i"
  "                           ",

  /* doom.wad */

  "                            "
  "DOOM Registered Startup v%i.%i"
  "                           ",

  /* Registered DOOM uses this */

  "                          "
  "DOOM System Startup v%i.%i"
  "                          ",

  /* Doom v1.666 */

  "                          "
  "DOOM System Startup v%i.%i66"
  "                          "

  /* doom.wad (Ultimate DOOM) */

  "                         "
  "The Ultimate DOOM Startup v%i.%i"
  "                        ",

  /* tnt.wad */

  "                     "
  "DOOM 2: TNT - Evilution v%i.%i"
  "                           ",

  /* plutonia.wad */

  "                   "
  "DOOM 2: Plutonia Experiment v%i.%i"
  "                           ",
};

static const struct gameversion g_gameversions[] =
{
  {"Doom 1.2", "1.2", exe_doom_1_2},
  {"Doom 1.5", "1.5", exe_doom_1_5},
  {"Doom 1.666", "1.666", exe_doom_1_666},
  {"Doom 1.7/1.7a", "1.7", exe_doom_1_7},
  {"Doom 1.8", "1.8", exe_doom_1_8},
  {"Doom 1.9", "1.9", exe_doom_1_9},
  {"Hacx", "hacx", exe_hacx},
  {"Ultimate Doom", "ultimate", exe_ultimate},
  {"Final Doom", "final", exe_final},
  {"Final Doom (alt)", "final2", exe_final2},
  {"Chex Quest", "chex", exe_chex},
  {NULL, NULL, 0},
};

static const struct pack g_packs[] =
{
  {"doom2", doom2},
  {"tnt", pack_tnt},
  {"plutonia", pack_plut},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void enable_loading_disk(void)
{
  const char *disk_lump_name;

  if (show_diskicon)
    {
      if (m_check_parm("-cdrom") > 0)
        {
          disk_lump_name = ("STCDROM");
        }
      else
        {
          disk_lump_name = ("STDISK");
        }

      v_enable_loading_disk(disk_lump_name, SCREENWIDTH - LOADING_DISK_W,
                            SCREENHEIGHT - LOADING_DISK_H);
    }
}

static void d_bind_variables(void)
{
  int i;

  m_apply_platform_defaults();

  i_bind_input_variables();
  i_bind_video_variables();
  i_bind_joystick_variables();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  i_bind_sound_variables();
#endif

  m_bind_base_controls();
  m_bind_weapon_controls();
  m_bind_map_controls();
  m_bind_menu_controls();
  m_bind_chat_controls(MAXPLAYERS);

  key_multi_msgplayer[0] = HUSTR_KEYGREEN;
  key_multi_msgplayer[1] = HUSTR_KEYINDIGO;
  key_multi_msgplayer[2] = HUSTR_KEYBROWN;
  key_multi_msgplayer[3] = HUSTR_KEYRED;

#ifdef CONFIG_GAMES_NXDOOM_NET
  net_bind_variables();
#endif

  m_bind_int_variable("mouse_sensitivity", &g_mouse_sensitivity);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  m_bind_int_variable("sfx_volume", &g_sfx_volume);
  m_bind_int_variable("music_volume", &g_music_volume);
  m_bind_int_variable("snd_channels", &snd_channels);
#endif
  m_bind_int_variable("show_messages", &g_show_messages);
  m_bind_int_variable("screenblocks", &screenblocks);
  m_bind_int_variable("detaillevel", &g_detail_level);
  m_bind_int_variable("vanilla_savegame_limit", &vanilla_savegame_limit);
  m_bind_int_variable("vanilla_demo_limit", &vanilla_demo_limit);
  m_bind_int_variable("show_endoom", &show_endoom);
  m_bind_int_variable("show_diskicon", &show_diskicon);

  /* Multiplayer chat macros */

  for (i = 0; i < 10; ++i)
    {
      char buf[12];

      chat_macros[i] = m_string_duplicate(g_chat_macro_defaults[i]);
      snprintf(buf, sizeof(buf), "chatmacro%i", i);
      m_bind_string_variable(buf, &chat_macros[i]);
    }
}

/* Function called at exit to display the ENDOOM screen */

static void d_endoom(void)
{
#ifdef CONFIG_GAMES_NXDOOM_ENDOOM
  byte *endoom;

  /* Don't show ENDOOM if we have it disabled, or we're running
   * in screensaver or control test mode. Only show it once the
   * game has actually started.
   */

  if (!show_endoom || !main_loop_started || screensaver_mode ||
      m_check_parm("-testcontrols") > 0)
    {
      return;
    }

  endoom = w_cache_lump_name(("ENDOOM"), PU_STATIC);

  i_endoom(endoom);
#else
  return;
#endif
}

static boolean is_french_iwad(void)
{
  return (gamemission == doom2 && w_check_num_for_name("M_RDTHIS") < 0 &&
          w_check_num_for_name("M_EPISOD") < 0 &&
          w_check_num_for_name("M_EPI1") < 0 &&
          w_check_num_for_name("M_EPI2") < 0 &&
          w_check_num_for_name("M_EPI3") < 0 &&
          w_check_num_for_name("WIOSTF") < 0 &&
          w_check_num_for_name("WIOBJ") >= 0);
}

/* Load dehacked patches needed for certain IWADs. */

static void load_iwad_deh(void)
{
  /* The Freedoom IWADs have DEHACKED lumps that must be loaded. */

  if (gamevariant == freedoom || gamevariant == freedm)
    {
      /* Old versions of Freedoom (before 2014-09) did not have technically
       * valid DEHACKED lumps, so ignore errors and just continue if this
       * is an old IWAD.
       */

      deh_load_lump_by_name("DEHACKED", false, true);
    }

  /* If this is the HACX IWAD, we need to load the DEHACKED lump. */

  if (gameversion == exe_hacx)
    {
      if (!deh_load_lump_by_name("DEHACKED", true, false))
        {
          i_error("DEHACKED lump not found.  Please check that this is the "
                  "Hacx v1.2 IWAD.");
        }
    }

  /* Chex Quest needs a separate Dehacked patch which must be downloaded
   * and installed next to the IWAD.
   */

  if (gameversion == exe_chex)
    {
      char *chex_deh = NULL;
      char *dirname;

      /* Look for chex.deh in the same directory as the IWAD file. */

      dirname = m_dir_name(iwadfile);
      chex_deh = m_string_join(dirname, DIR_SEPARATOR_S, "chex.deh", NULL);
      free(dirname);

      /* If the dehacked patch isn't found, try searching the WAD
       * search path instead.  We might find it...
       */

      if (!m_file_exists(chex_deh))
        {
          free(chex_deh);
          chex_deh = d_find_wad_by_name("chex.deh");
        }

      /* Still not found? */

      if (chex_deh == NULL)
        {
          i_error("Unable to find Chex Quest dehacked file (chex.deh).\n"
                  "The dehacked file is required in order to emulate\n"
                  "chex.exe correctly.  It can be found in your nearest\n"
                  "/idgames repository mirror at:\n\n"
                  "   themes/chex/chexdeh.zip");
        }

      if (!deh_loadfile(chex_deh))
        {
          i_error("Failed to load chex.deh needed for emulating chex.exe.");
        }
    }

  if (is_french_iwad())
    {
      char *french_deh = NULL;
      char *dirname;

      /* Look for french.deh in the same directory as the IWAD file. */

      dirname = m_dir_name(iwadfile);
      french_deh =
          m_string_join(dirname, DIR_SEPARATOR_S, "french.deh", NULL);
      printf("French version\n");
      free(dirname);

      /* If the dehacked patch isn't found, try searching the WAD
       * search path instead.  We might find it...
       */

      if (!m_file_exists(french_deh))
        {
          free(french_deh);
          french_deh = d_find_wad_by_name("french.deh");
        }

      /* Still not found? */

      if (french_deh == NULL)
        {
          i_error(
            "Unable to find French Doom II dehacked file\n"
            "(french.deh).  The dehacked file is required in order to\n"
            "emulate French doom2.exe correctly.  It can be found in\n"
            "your nearest /idgames repository mirror at:\n\n"
            "   utils/exe_edit/patches/french.zip"
          );
        }

      if (!deh_loadfile(french_deh))
        {
          i_error("Failed to load french.deh needed for emulating French\n"
                  "doom2.exe.");
        }
    }
}

static void g_check_demo_status_at_exit(void)
{
  g_check_demo_status();
}

/* draw current display, possibly wiping it from the previous */

static boolean d_display(void)
{
  static boolean viewactivestate = false;
  static boolean g_menuactivestate = false;
  static boolean inhelpscreensstate = false;
  static boolean d_fullscreen = false;
  static gamestate_t oldgamestate = -1;
  static int borderdrawcount;
  int y;
  boolean wipe;
  boolean redrawsbar;

  redrawsbar = false;

  /* change the view size if needed */

  if (setsizeneeded)
    {
      r_execute_set_view_size();
      oldgamestate = -1; /* force background redraw */
      borderdrawcount = 3;
    }

  /* save the current screen if about to wipe */

  if (gamestate != wipegamestate)
    {
      wipe = true;
      wipe_start_screen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
  else
    {
      wipe = false;
    }

  if (gamestate == GS_LEVEL && gametic)
    {
      hu_erase();
    }

  /* do buffered drawing */

  switch (gamestate)
    {
    case GS_LEVEL:

      if (!gametic)
        {
          break;
        }

      if (automapactive)
        {
          am_drawer();
        }

      if (wipe || (viewheight != SCREENHEIGHT && d_fullscreen))
        {
          redrawsbar = true;
        }

      if (inhelpscreensstate && !inhelpscreens)
        {
          redrawsbar = true; /* just put away the help screen */
        }

      st_drawer(viewheight == SCREENHEIGHT, redrawsbar);
      d_fullscreen = viewheight == SCREENHEIGHT;
      break;

    case GS_INTERMISSION:
      wi_drawer();
      break;

    case GS_FINALE:
      f_drawer();
      break;

    case GS_DEMOSCREEN:
      d_page_drawer();
      break;
    }

  /* draw buffered stuff to screen */

  i_update_no_blit();

  /* draw the view directly */

  if (gamestate == GS_LEVEL && !automapactive && gametic)
    {
      r_render_player_view(&players[displayplayer]);
    }

  if (gamestate == GS_LEVEL && gametic)
    {
      hu_drawer();
    }

  /* clean up border stuff */

  if (gamestate != oldgamestate && gamestate != GS_LEVEL)
    i_set_palette(w_cache_lump_name(("PLAYPAL"), PU_CACHE));

  /* see if the border needs to be initially drawn */

  if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
      viewactivestate = false; /* view was not active */
      r_fill_back_screen();    /* draw the pattern into the back screen */
    }

  /* see if the border needs to be updated to the screen */

  if (gamestate == GS_LEVEL && !automapactive &&
      scaledviewwidth != SCREENWIDTH)
    {
      if (g_menuactive || g_menuactivestate || !viewactivestate)
        borderdrawcount = 3;
      if (borderdrawcount)
        {
          r_draw_view_border(); /* erase old menu stuff */
          borderdrawcount--;
        }
    }

  if (testcontrols)
    {
      /* Box showing current mouse speed */

      v_draw_mouse_speed_box(testcontrols_mousespeed);
    }

  g_menuactivestate = g_menuactive;
  viewactivestate = viewactive;
  inhelpscreensstate = inhelpscreens;
  oldgamestate = wipegamestate = gamestate;

  /* draw pause pic */

  if (paused)
    {
      if (automapactive)
        y = 4;
      else
        y = viewwindowy + 4;
      v_draw_patch_direct(viewwindowx + (scaledviewwidth - 68) / 2, y,
                          w_cache_lump_name(("M_PAUSE"), PU_CACHE));
    }

  /* menus go directly to the screen */

  m_drawer();   /* menu is drawn even on top of everything */
  net_update(); /* send out any new accumulation */

  return wipe;
}

/****************************************************************************
 * Name: d_grab_mouse_callback
 *
 * Description:
 *   Called to determine whether to grab the mouse pointer
 *
 ****************************************************************************/

static boolean d_grab_mouse_callback(void)
{
  /* Drone players don't need mouse focus */

#ifdef CONFIG_GAMES_NXDOOM_NET
  if (drone) return false;
#endif

  /* when menu is active or game is paused, release the mouse */

  if (g_menuactive || paused) return false;

  /* only grab mouse when playing levels (but not demos) */

  return (gamestate == GS_LEVEL) && !demoplayback && !advancedemo;
}

/****************************************************************************
 * Name: d_run_frame
 ****************************************************************************/

static void d_run_frame(void)
{
  int nowtime;
  int tics;
  static int wipestart;
  static boolean wipe;

  if (wipe)
    {
      do
        {
          nowtime = i_get_time();
          tics = nowtime - wipestart;
          usleep(1000);
        }
      while (tics <= 0);

      wipestart = nowtime;
      wipe = !wipe_screen_wipe(WIPE_MELT, 0, 0,
                  SCREENWIDTH, SCREENHEIGHT, tics);
      i_update_no_blit();
      m_drawer();        /* menu is drawn even on top of wipes */
      i_finish_update(); /* page flip or blit buffer */
      return;
    }

  /* frame synchronous IO operations */

  i_start_frame();

  try_run_tics(); /* will run at least one tic */

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_update_sounds(players[consoleplayer].mo); /* move positional sounds */
#endif

  /* Update display, next frame, with current state if no profiling is on */

  if (screenvisible && !nodrawers)
    {
      if ((wipe = d_display()))
        {
          /* start wipe on this frame */

          wipe_endscreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

          wipestart = i_get_time() - 1;
        }
      else
        {
          /* normal update */

          i_finish_update(); /* page flip or blit buffer */
        }
    }
}

/* Get game name: if the startup banner has been replaced, use that.
 * Otherwise, use the name given
 */

static char *get_game_name(const char *gamename)
{
  size_t i;

  for (i = 0; i < arrlen(g_banners); ++i)
    {
      const char *deh_sub;

      /* Has the banner been replaced? */

      deh_sub = (g_banners[i]);

      if (deh_sub != g_banners[i])
        {
          size_t gamename_size;
          int version;
          char *deh_gamename;

          /* Has been replaced.
           * We need to expand via printf to include the Doom version number
           * We also need to cut off spaces to get the basic name
           */

          gamename_size = strlen(deh_sub) + 10;
          deh_gamename = malloc(gamename_size);
          if (deh_gamename == NULL)
            {
              i_error("GetGameName: Failed to allocate new string");
            }

          version = g_vanilla_version_code();
          snprintf(deh_gamename, gamename_size, g_banners[i], version / 100,
                   version % 100);

          while (deh_gamename[0] != '\0' && isspace(deh_gamename[0]))
            {
              memmove(deh_gamename, deh_gamename + 1, gamename_size - 1);
            }

          while (deh_gamename[0] != '\0' &&
                 isspace(deh_gamename[strlen(deh_gamename) - 1]))
            {
              deh_gamename[strlen(deh_gamename) - 1] = '\0';
            }

          return deh_gamename;
        }
    }

  return m_string_duplicate(gamename);
}

static void set_mission_for_pack_name(const char *pack_name)
{
  int i;

  for (i = 0; i < arrlen(g_packs); ++i)
    {
      if (!strcasecmp(pack_name, g_packs[i].name))
        {
          gamemission = g_packs[i].mission;
          return;
        }
    }

  printf("Valid mission packs are:\n");

  for (i = 0; i < arrlen(g_packs); ++i)
    {
      printf("\t%s\n", g_packs[i].name);
    }

  i_error("Unknown mission pack name: %s", pack_name);
}

/* Find out what version of Doom is playing. */

static void d_identify_version(void)
{
  /* gamemission is set up by the d_find_iwad function.  But if
   * we specify '-iwad', we have to identify using
   * IdentifyIWADByName.  However, if the iwad does not match
   * any known IWAD name, we may have a dilemma.  Try to
   * identify by its contents.
   */

  if (gamemission == none)
    {
      unsigned int i;

      for (i = 0; i < numlumps; ++i)
        {
          if (!strncasecmp(lumpinfo[i]->name, "MAP01", 8))
            {
              gamemission = doom2;
              break;
            }
          else if (!strncasecmp(lumpinfo[i]->name, "E1M1", 8))
            {
              gamemission = doom;
              break;
            }
        }

      if (gamemission == none)
        {
          /* Still no idea.  I don't think this is going to work. */

          i_error("Unknown or invalid IWAD file.");
        }
    }

  /* Make sure gamemode is set up correctly */

  if (logical_gamemission == doom)
    {
      /* Doom 1.  But which version? */

      if (w_check_num_for_name("E4M1") > 0)
        {
          /* Ultimate Doom */

          gamemode = retail;
        }
      else if (w_check_num_for_name("E3M1") > 0)
        {
          gamemode = registered;
        }
      else
        {
          gamemode = shareware;
        }
    }
  else
    {
      int p;

      /* Doom 2 of some kind. */

      gamemode = commercial;

      /* We can manually override the gamemission that we got from the
       * IWAD detection code. This allows us to eg. play Plutonia 2
       * with Freedoom and get the right level names.
       */

      /* @category compat
       * @arg <pack>
       *
       * Explicitly specify a Doom II "mission pack" to run as, instead of
       * detecting it based on the filename. Valid values are: "doom2",
       * "tnt" and "plutonia".
       *
       */

      p = m_check_parm_with_args("-pack", 1);
      if (p > 0)
        {
          set_mission_for_pack_name(myargv[p + 1]);
        }
    }
}

/* Set the gamedescription string */

static void d_set_game_description(void)
{
  if (logical_gamemission == doom)
    {
      /* Doom 1.  But which version? */

      if (gamevariant == freedoom)
        {
          g_gamedescription = get_game_name("Freedoom: Phase 1");
        }
      else if (gamemode == retail)
        {
          /* Ultimate Doom */

          g_gamedescription = get_game_name("The Ultimate DOOM");
        }
      else if (gamemode == registered)
        {
          g_gamedescription = get_game_name("DOOM Registered");
        }
      else if (gamemode == shareware)
        {
          g_gamedescription = get_game_name("DOOM Shareware");
        }
    }
  else
    {
      /* Doom 2 of some kind.  But which mission? */

      if (gamevariant == freedm)
        {
          g_gamedescription = get_game_name("FreeDM");
        }
      else if (gamevariant == freedoom)
        {
          g_gamedescription = get_game_name("Freedoom: Phase 2");
        }
      else if (logical_gamemission == doom2)
        {
          g_gamedescription = get_game_name("DOOM 2: Hell on Earth");
        }
      else if (logical_gamemission == pack_plut)
        {
          g_gamedescription = get_game_name("DOOM 2: Plutonia Experiment");
        }
      else if (logical_gamemission == pack_tnt)
        {
          g_gamedescription = get_game_name("DOOM 2: TNT - Evilution");
        }
    }

  if (g_gamedescription == NULL)
    {
      g_gamedescription = m_string_duplicate("Unknown");
    }
}

static boolean d_addfile(char *filename)
{
  wad_file_t *handle;

  printf(" adding %s\n", filename);
  handle = w_add_file(filename);

  return handle != NULL;
}

/* Prints a message only if it has been modified by dehacked. */

static void print_dehacked_banners(void)
{
  size_t i;

  for (i = 0; i < arrlen(g_copyright_banners); ++i)
    {
      const char *deh_s;

      deh_s = (g_copyright_banners[i]);

      if (deh_s != g_copyright_banners[i])
        {
          printf("%s", deh_s);

          /* Make sure the modified banner always ends in a newline
           * character. If it doesn't, add a newline. This fixes av.wad.
           */

          if (deh_s[strlen(deh_s) - 1] != '\n')
            {
              printf("\n");
            }
        }
    }
}

/* Initialize the game version */

static void init_game_version(void)
{
  byte *demolump;
  char demolumpname[6];
  int demoversion;
  int p;
  int i;
  boolean status;

  /* @arg <version>
   * @category compat
   *
   * Emulate a specific version of Doom. Valid values are "1.2",
   * "1.5", "1.666", "1.7", "1.8", "1.9", "ultimate", "final",
   * "final2", "hacx" and "chex".
   *
   */

  p = m_check_parm_with_args("-gameversion", 1);

  if (p)
    {
      for (i = 0; g_gameversions[i].description != NULL; ++i)
        {
          if (!strcmp(myargv[p + 1], g_gameversions[i].cmdline))
            {
              gameversion = g_gameversions[i].version;
              break;
            }
        }

      if (g_gameversions[i].description == NULL)
        {
          printf("Supported game versions:\n");

          for (i = 0; g_gameversions[i].description != NULL; ++i)
            {
              printf("\t%s (%s)\n", g_gameversions[i].cmdline,
                     g_gameversions[i].description);
            }

          i_error("Unknown game version '%s'", myargv[p + 1]);
        }
    }
  else
    {
      /* Determine automatically */

      if (gamemission == pack_chex)
        {
          /* chex.exe - identified by iwad filename */

          gameversion = exe_chex;
        }
      else if (gamemission == pack_hacx)
        {
          /* hacx.exe: identified by iwad filename */

          gameversion = exe_hacx;
        }
      else if (gamemode == shareware || gamemode == registered ||
               (gamemode == commercial && gamemission == doom2))
        {
          /* original */

          gameversion = exe_doom_1_9;

          /* Detect version from demo lump */

          for (i = 1; i <= 3; ++i)
            {
              snprintf(demolumpname, 6, "demo%i", i);
              if (w_check_num_for_name(demolumpname) > 0)
                {
                  demolump = w_cache_lump_name(demolumpname, PU_STATIC);
                  demoversion = demolump[0];
                  w_release_lump_name(demolumpname);
                  status = true;
                  switch (demoversion)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                      gameversion = exe_doom_1_2;
                      break;
                    case 106:
                      gameversion = exe_doom_1_666;
                      break;
                    case 107:
                      gameversion = exe_doom_1_7;
                      break;
                    case 108:
                      gameversion = exe_doom_1_8;
                      break;
                    case 109:
                      gameversion = exe_doom_1_9;
                      break;
                    default:
                      status = false;
                      break;
                    }

                  if (status)
                    {
                      break;
                    }
                }
            }
        }
      else if (gamemode == retail)
        {
          gameversion = exe_ultimate;
        }
      else if (gamemode == commercial)
        {
          /* Final Doom: tnt or plutonia
           * Defaults to emulating the first Final Doom executable,
           * which has the crash in the demo loop; however, having
           * this as the default should mean that it plays back
           * most demos correctly.
           */

          gameversion = exe_final;
        }
    }

  /* Deathmatch 2.0 did not exist until Doom v1.4 */

  if (gameversion <= exe_doom_1_2 && deathmatch == 2)
    {
      deathmatch = 1;
    }

  /* The original exe does not support retail - 4th episode not supported */

  if (gameversion < exe_ultimate && gamemode == retail)
    {
      gamemode = registered;
    }

  /* EXEs prior to the Final Doom exes do not support Final Doom. */

  if (gameversion < exe_final && gamemode == commercial &&
      (gamemission == pack_tnt || gamemission == pack_plut))
    {
      gamemission = doom2;
    }
}

static void print_game_version(void)
{
  int i;

  for (i = 0; g_gameversions[i].description != NULL; ++i)
    {
      if (g_gameversions[i].version == gameversion)
        {
          printf("Emulating the behavior of the "
                 "'%s' executable.\n",
                 g_gameversions[i].description);
          break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* d_process_events
 * Send all the events of the given timestamp down the responder chain
 */

void d_process_events(void)
{
  event_t *ev;

  /* IF STORE DEMO, DO NOT ACCEPT INPUT */

  if (storedemo) return;

  while ((ev = d_pop_event()) != NULL)
    {
      if (m_responder(ev)) continue; /* menu ate the event */
      g_responder(ev);
    }
}

/* d_doomloop */

void d_doomloop(void)
{
  if (gamevariant == bfgedition &&
      (demorecording || (gameaction == ga_playdemo) || netgame))
    {
      printf(" WARNING: You are playing using one of the Doom Classic\n"
             " IWAD files shipped with the Doom 3: BFG Edition. These are\n"
             " known to be incompatible with the regular IWAD files and\n"
             " may cause demos and network games to get out of sync.\n");
    }

  if (demorecording)
    {
      g_begin_recording();
    }

  main_loop_started = true;

  i_set_window_title(g_gamedescription);
  i_graphics_check_commandline();
  i_set_grab_mouse_callback(d_grab_mouse_callback);
  i_init_graphics();
  enable_loading_disk();

  try_run_tics();

  v_restore_buffer();
  r_execute_set_view_size();

  d_start_game_loop();

  if (testcontrols)
    {
      wipegamestate = gamestate;
    }

  while (1)
    {
      /* Safe point (outside any framebuffer/heap access) for nxstore's
       * SIGTERM-driven close request to actually take effect - see
       * i_install_quit_signal() in i_system.h.
       */

      i_poll_quit_signal();
      d_run_frame();
    }
}

/* d_page_ticker
 * Handles timing for warped projection
 */

void d_page_ticker(void)
{
  if (--pagetic < 0) d_advance_demo();
}

/****************************************************************************
 * Name: d_page_drawer
 ****************************************************************************/

void d_page_drawer(void)
{
  v_draw_patch(0, 0, w_cache_lump_name(pagename, PU_CACHE));
}

/****************************************************************************
 * Name: d_advance_demo
 *
 * Description:
 *   Called after each demo or intro demosequence finishes
 *
 ****************************************************************************/

void d_advance_demo(void)
{
  advancedemo = true;
}

/****************************************************************************
 * Name: d_do_advance_demo
 *
 * Description:
 *   This cycles through the demo sequences.
 *   FIXME - version dependent demo numbers?
 *
 ****************************************************************************/

void d_do_advance_demo(void)
{
  players[consoleplayer].playerstate = PST_LIVE; /* not reborn */
  advancedemo = false;
  usergame = false; /* no save / end game here */
  paused = false;
  gameaction = ga_nothing;

  /* The Ultimate Doom executable changed the demo sequence to add
   * a DEMO4 demo.  Final Doom was based on Ultimate, so also
   * includes this change; however, the Final Doom IWADs do not
   * include a DEMO4 lump, so the game bombs out with an error
   * when it reaches this point in the demo sequence.
   *
   * However! There is an alternate version of Final Doom that
   * includes a fixed executable.
   */

  if (gameversion == exe_ultimate || gameversion == exe_final)
    demosequence = (demosequence + 1) % 7;
  else
    demosequence = (demosequence + 1) % 6;

  switch (demosequence)
    {
    case 0:
      if (gamemode == commercial)
        pagetic = TICRATE * 11;
      else
        pagetic = 170;
      gamestate = GS_DEMOSCREEN;
      pagename = ("TITLEPIC");

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gamemode == commercial)
        {
          s_start_music(MUS_DM2TTL);
        }
      else
        {
          s_start_music(MUS_INTRO);
        }
#endif

      break;
    case 1:
      g_defered_play_demo(("demo1"));
      break;
    case 2:
      pagetic = 200;
      gamestate = GS_DEMOSCREEN;
      pagename = ("CREDIT");
      break;
    case 3:
      g_defered_play_demo(("demo2"));
      break;
    case 4:
      gamestate = GS_DEMOSCREEN;
      if (gamemode == commercial)
        {
          pagetic = TICRATE * 11;
          pagename = ("TITLEPIC");
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_music(MUS_DM2TTL);
#endif
        }
      else
        {
          pagetic = 200;

          if (gameversion >= exe_ultimate)
            pagename = ("CREDIT");
          else
            pagename = ("HELP2");
        }
      break;
    case 5:
      g_defered_play_demo(("demo3"));
      break;

      /* THE DEFINITIVE DOOM Special Edition demo */

    case 6:
      g_defered_play_demo(("demo4"));
      break;
    }

  /* The Doom 3: BFG Edition version of doom2.wad does not have a
   * TITLETPIC lump. Use INTERPIC instead as a workaround.
   */

  if (gamevariant == bfgedition && !strcasecmp(pagename, "TITLEPIC") &&
      w_check_num_for_name("titlepic") < 0)
    {
      pagename = ("INTERPIC");
    }
}

/* d_start_title */

void d_start_title(void)
{
  gameaction = ga_nothing;
  demosequence = -1;
  d_advance_demo();
}

/* d_doom_main */

void d_doom_main(void)
{
  int p;
  char file[256];
  char demolumpname[9];

  i_at_exit(d_endoom, false);

  /* print banner */

  i_print_banner(PACKAGE_STRING);

  printf("z_init: Init zone memory allocation daemon. \n");
  z_init();

#ifdef CONFIG_GAMES_NXDOOM_NET
  /* @category net
   *
   * Start a dedicated server, routing packets but not participating
   * in the game itself.
   *
   */

  if (m_check_parm("-dedicated") > 0)
    {
      printf("Dedicated server mode.\n");
      net_dedicated_server();

      /* Never returns */
    }

  /* @category net
   *
   * Query the Internet master server for a global list of active
   * servers.
   *
   */

  if (m_check_parm("-search"))
    {
      net_master_query();
      exit(0);
    }

  /* @arg <address>
   * @category net
   *
   * Query the status of the server running on the given IP
   * address.
   *
   */

  p = m_check_parm_with_args("-query", 1);

  if (p)
    {
      net_query_address(myargv[p + 1]);
      exit(0);
    }

  /* @category net
   *
   * Search the local LAN for running servers.
   *
   */

  if (m_check_parm("-localsearch"))
    {
      net_lan_query();
      exit(0);
    }
#endif

  /* @category game
   * @vanilla
   *
   * Disable monsters.
   *
   */

  nomonsters = m_check_parm("-nomonsters");

  /* @category game
   * @vanilla
   *
   * Monsters respawn after being killed.
   *
   */

  respawnparm = m_check_parm("-respawn");

  /* @category game
   * @vanilla
   *
   * Monsters move faster.
   *
   */

  fastparm = m_check_parm("-fast");

  /* @vanilla
   *
   * Developer mode. F1 saves a screenshot in the current working
   * directory.
   *
   */

  devparm = m_check_parm("-devparm");

  i_display_fps_dots(devparm);

  /* @category net
   * @vanilla
   *
   * Start a deathmatch game.
   *
   */

  if (m_check_parm("-deathmatch")) deathmatch = 1;

  /* @category net
   * @vanilla
   *
   * Start a deathmatch 2.0 game.  Weapons do not stay in place and
   * all items respawn after 30 seconds.
   *
   */

  if (m_check_parm("-altdeath")) deathmatch = 2;

  if (devparm)
    {
      printf(D_DEVSTR);
    }

  /* find which dir to use for config files */

  /* Auto-detect the configuration dir. */

  m_set_config_dir(NULL);

  /* @category game
   * @arg <x>
   * @vanilla
   *
   * Turbo mode.  The player's speed is multiplied by x%.  If unspecified,
   * x defaults to 200.  Values are rounded up to 10 and down to 400.
   *
   */

  if ((p = m_check_parm("-turbo")))
    {
      int scale = 200;

      if (p < myargc - 1) scale = atoi(myargv[p + 1]);
      if (scale < 10) scale = 10;
      if (scale > 400) scale = 400;
      printf("turbo scale: %i%%\n", scale);
      forwardmove[0] = forwardmove[0] * scale / 100;
      forwardmove[1] = forwardmove[1] * scale / 100;
      sidemove[0] = sidemove[0] * scale / 100;
      sidemove[1] = sidemove[1] * scale / 100;
    }

  /* init subsystems */

  printf("v_init: allocate screens.\n");
  v_init();

  /* Load configuration files before initialising other subsystems. */

  printf("m_load_defaults: Load system defaults.\n");
  m_set_config_filenames("default.cfg", PROGRAM_PREFIX "doom.cfg");
  d_bind_variables();
  m_load_defaults();

  /* Save configuration at exit. */

  i_at_exit(m_save_defaults, false);

  /* Find main IWAD file and load it. */

  iwadfile = d_find_iwad(IWAD_MASK_DOOM, &gamemission);

  /* None found? */

  if (iwadfile == NULL)
    {
      i_error("Game mode indeterminate.  No IWAD file was found.  Try\n"
              "specifying one with the '-iwad' command line parameter.\n");
    }

  modifiedgame = false;

  printf("W_Init: Init WADfiles.\n");
  d_addfile(iwadfile);

  w_check_correct_iwad(doom);

  /* Now that we've loaded the IWAD, we can figure out what gamemission
   * we're playing and which version of Vanilla Doom we need to emulate.
   */

  d_identify_version();
  init_game_version();

  /* Check which IWAD variant we are using. */

  if (w_check_num_for_name("FREEDOOM") >= 0)
    {
      if (w_check_num_for_name("FREEDM") >= 0)
        {
          gamevariant = freedm;
        }
      else
        {
          gamevariant = freedoom;
        }
    }
  else if (w_check_num_for_name("DMENUPIC") >= 0)
    {
      gamevariant = bfgedition;
    }

  /* @category mod
   *
   * Disable automatic loading of Dehacked patches for certain
   * IWAD files.
   *
   */

  if (!m_parm_exists("-nodeh"))
    {
      /* Some IWADs have dehacked patches that need to be loaded for
       * them to be played properly.
       */

      load_iwad_deh();
    }

  /* Doom 3: BFG Edition includes modified versions of the classic
   * IWADs which can be identified by an additional DMENUPIC lump.
   * Furthermore, the M_GDHIGH lumps have been modified in a way that
   * makes them incompatible to Vanilla Doom and the modified version
   * of doom2.wad is missing the TITLEPIC lump.
   * We specifically check for DMENUPIC here, before PWADs have been
   * loaded which could probably include a lump of that name.
   */

  if (gamevariant == bfgedition)
    {
      printf("BFG Edition: Using workarounds as needed.\n");

      /* BFG Edition changes the names of the secret levels to
       * censor the Wolfenstein references. It also has an extra
       * secret level (MAP33). In Vanilla Doom (meaning the DOS
       * version), MAP33 overflows into the Plutonia level names
       * array, so HUSTR_33 is actually PHUSTR_1.
       */

      deh_add_string_replacement(HUSTR_31, "level 31: idkfa");
      deh_add_string_replacement(HUSTR_32, "level 32: keen");
      deh_add_string_replacement(PHUSTR_1, "level 33: betray");

      /* The BFG edition doesn't have the "low detail" menu option (fair
       * enough). But bizarrely, it reuses the M_GDHIGH patch as a label
       * for the options menu (says "Fullscreen:"). Why the perpetrators
       * couldn't just add a new graphic lump and had to reuse this one,
       * I don't know.
       *
       * The end result is that M_GDHIGH is too wide and causes the game
       * to crash. As a workaround to get a minimum level of support for
       * the BFG edition IWADs, use the "ON"/"OFF" graphics instead.
       */

      deh_add_string_replacement("M_GDHIGH", "M_MSGON");
      deh_add_string_replacement("M_GDLOW", "M_MSGOFF");

      /* The BFG edition's "Screen Size:" graphic has also been changed
       * to say "Gamepad:". Fortunately, it (along with the original
       * Doom IWADs) has an unused graphic that says "Display". So we
       * can swap this in instead, and it kind of makes sense.
       */

      deh_add_string_replacement("M_SCRNSZ", "M_DISP");
    }

  /* @category mod
   *
   * Disable auto-loading of .wad and .deh files.
   *
   */

  if (!m_parm_exists("-noautoload") && gamemode != shareware)
    {
      char *autoload_dir;

      /* common auto-loaded files for all Doom flavors */

      if (gamemission < pack_chex)
        {
          autoload_dir = m_get_autoload_dir("doom-all");
          if (autoload_dir != NULL)
            {
              deh_auto_load_patches(autoload_dir);
              w_auto_load_wads(autoload_dir);
              free(autoload_dir);
            }
        }

      /* auto-loaded files per IWAD */

      autoload_dir = m_get_autoload_dir(
                     d_save_game_iwad_name(gamemission, gamevariant));

      if (autoload_dir != NULL)
        {
          deh_auto_load_patches(autoload_dir);
          w_auto_load_wads(autoload_dir);
          free(autoload_dir);
        }
    }

  /* Load Dehacked patches specified on the command line with -deh.
   * Note that there's a very careful and deliberate ordering to how
   * Dehacked patches are loaded. The order we use is:
   *  1. IWAD dehacked patches.
   *  2. Command line dehacked patches specified with -deh.
   *  3. PWAD dehacked patches in DEHACKED lumps.
   */

  deh_parse_command_line();

  /* Load PWAD files. */

  modifiedgame = w_parse_command_line();

  /* Debug:
   *    w_print_directory();
   */

  /* @arg <demo>
   * @category demo
   * @vanilla
   *
   * Play back the demo named demo.lmp.
   *
   */

  p = m_check_parm_with_args("-playdemo", 1);

  if (!p)
    {
      /* @arg <demo>
       * @category demo
       * @vanilla
       *
       * Play back the demo named demo.lmp, determining the framerate
       * of the screen.
       *
       */

      p = m_check_parm_with_args("-timedemo", 1);
    }

  if (p)
    {
      char *uc_filename = strdup(myargv[p + 1]);
      m_force_uppercase(uc_filename);

      /* With Vanilla you have to specify the file without extension,
       * but make that optional.
       */

      if (m_string_ends_with(uc_filename, ".LMP"))
        {
          m_str_copy(file, myargv[p + 1], sizeof(file));
        }
      else
        {
          snprintf(file, sizeof(file), "%s.lmp", myargv[p + 1]);
        }

      free(uc_filename);

      if (d_addfile(file))
        {
          m_str_copy(demolumpname, lumpinfo[numlumps - 1]->name,
                     sizeof(demolumpname));
        }
      else
        {
          /* If file failed to load, still continue trying to play
           * the demo in the same way as Vanilla Doom.  This makes
           * tricks like "-playdemo demo1" possible.
           */

          m_str_copy(demolumpname, myargv[p + 1], sizeof(demolumpname));
        }

      printf("Playing demo %s.\n", file);
    }

  i_at_exit(g_check_demo_status_at_exit, true);

  /* Generate the WAD hash table.  Speed things up a bit. */

  w_generate_hash_table();

  /* Load DEHACKED lumps from WAD files - but only if we give the right
   * command line parameter.
   */

  /* @category mod
   *
   * Load Dehacked patches from DEHACKED lumps contained in one of the
   * loaded PWAD files.
   *
   */

  if (m_parm_exists("-dehlump"))
    {
      int i;
      int loaded = 0;
      int numiwadlumps = numlumps;

      while (!w_is_iwad_lump(lumpinfo[numiwadlumps - 1]))
        {
          numiwadlumps--;
        }

      for (i = numiwadlumps; i < numlumps; ++i)
        {
          if (!strncmp(lumpinfo[i]->name, "DEHACKED", 8))
            {
              deh_load_lump(i, false, false);
              loaded++;
            }
        }

      printf("  loaded %i DEHACKED lumps from PWAD files.\n", loaded);
    }

  /* Set the gamedescription string. This is only possible now that
   * we've finished loading Dehacked patches.
   */

  d_set_game_description();

  savegamedir =
      m_get_save_game_dir(d_save_game_iwad_name(gamemission, gamevariant));

  /* Check for -file in shareware */

  if (modifiedgame && (gamevariant != freedoom))
    {
      int i;

      if (gamemode == shareware)
        i_error(("\nYou cannot -file with the shareware "
                 "version. Register!"));

      /* Check for fake IWAD with right name,
       * but w/o all the lumps of the registered version.
       */

      if (gamemode == registered)
        {
          for (i = 0; i < 23; i++)
            {
              if (w_check_num_for_name(g_name[i]) < 0)
                {
                  i_error(("\nThis is not the registered version."));
                }
            }
        }
    }

  if (w_check_num_for_name("SS_START") >= 0 ||
      w_check_num_for_name("FF_END") >= 0)
    {
      i_print_divider();
      printf(" WARNING: The loaded WAD file contains modified sprites or\n"
             " floor textures.  You may want to use the '-merge' command\n"
             " line option instead of '-file'.\n");
    }

  i_print_startup_banner(g_gamedescription);
  print_dehacked_banners();

  printf("i_init: Setting up machine state.\n");
  i_check_is_screensaver();
  i_init_timer();
  i_init_joystick();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  i_init_sound(doom);
  i_init_music();
#endif

#ifdef CONFIG_GAMES_NXDOOM_NET
  printf("net_init: Init network subsystem.\n");
  net_init();
#endif

  /* Initial netgame startup. Connect to server etc. */

  d_connect_net_game();

  /* get skill / episode / map from params */

  startskill = sk_medium;
  startepisode = 1;
  startmap = 1;
  autostart = false;

  /* @category game
   * @arg <skill>
   * @vanilla
   *
   * Set the game skill, 1-5 (1: easiest, 5: hardest).  A skill of
   * 0 disables all monsters.
   *
   */

  p = m_check_parm_with_args("-skill", 1);

  if (p)
    {
      startskill = myargv[p + 1][0] - '1';
      autostart = true;
    }

  /* @category game
   * @arg <n>
   * @vanilla
   *
   * Start playing episode n (1-4).
   *
   */

  p = m_check_parm_with_args("-episode", 1);

  if (p)
    {
      startepisode = myargv[p + 1][0] - '0';
      startmap = 1;
      autostart = true;
    }

  timelimit = 0;

  /* @arg <n>
   * @category net
   * @vanilla
   *
   * For multiplayer games: exit each level after n minutes.
   *
   */

  p = m_check_parm_with_args("-timer", 1);

  if (p)
    {
      timelimit = atoi(myargv[p + 1]);
    }

  /* @category net
   * @vanilla
   *
   * Austin Virtual Gaming: end levels after 20 minutes.
   *
   */

  p = m_check_parm("-avg");

  if (p)
    {
      timelimit = 20;
    }

  /* @category game
   * @arg [<x> <y> | <xy>]
   * @vanilla
   *
   * Start a game immediately, warping to ExMy (Doom 1) or MAPxy
   * (Doom 2).
   *
   */

  p = m_check_parm_with_args("-warp", 1);

  if (p)
    {
      if (gamemode == commercial)
        startmap = atoi(myargv[p + 1]);
      else
        {
          startepisode = myargv[p + 1][0] - '0';

          if (p + 2 < myargc)
            {
              startmap = myargv[p + 2][0] - '0';
            }
          else
            {
              startmap = 1;
            }
        }

      autostart = true;
    }

  /* Undocumented:
   * Invoked by setup to test the controls.
   */

  p = m_check_parm("-testcontrols");

  if (p > 0)
    {
      startepisode = 1;
      startmap = 1;
      autostart = true;
      testcontrols = true;
    }

  /* Check for load game parameter
   * We do this here and save the slot number, so that the network code
   * can override it or send the load slot to other players.
   */

  /* @category game
   * @arg <s>
   * @vanilla
   *
   * Load the game in slot s.
   *
   */

  p = m_check_parm_with_args("-loadgame", 1);

  if (p)
    {
      startloadgame = atoi(myargv[p + 1]);
    }
  else
    {
      startloadgame = -1; /* Not loading a game */
    }

  printf("m_init: Init miscellaneous info.\n");
  m_init();

  printf("r_init: Init DOOM refresh daemon - ");
  r_init();

  printf("\np_init: Init Playloop state.\n");
  p_init();

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  printf("s_init: Setting up sound.\n");
  s_init(g_sfx_volume * 8, g_music_volume * 8);
#endif

  printf("d_check_net_game: Checking network game status.\n");
  d_check_net_game();

  print_game_version();

  printf("hu_init: Setting up heads up display.\n");
  hu_init();

  printf("st_init: Init status bar.\n");
  st_init();

  /* If Doom II without a MAP01 lump, this is a store demo.
   * Moved this here so that MAP01 isn't constantly looked up
   * in the main loop.
   */

  if (gamemode == commercial && w_check_num_for_name("map01") < 0)
    storedemo = true;

  if (m_check_parm_with_args("-statdump", 1))
    {
      i_at_exit(stat_dump, true);
      printf("External statistics registered.\n");
    }

  /* @arg <x>
   * @category demo
   * @vanilla
   *
   * Record a demo named x.lmp.
   */

  p = m_check_parm_with_args("-record", 1);

  if (p)
    {
      g_record_demo(myargv[p + 1]);
      autostart = true;
    }

  p = m_check_parm_with_args("-playdemo", 1);
  if (p)
    {
      singledemo = true; /* quit after one demo */
      g_defered_play_demo(demolumpname);
      d_doomloop(); /* never returns */
    }

  p = m_check_parm_with_args("-timedemo", 1);
  if (p)
    {
      g_time_demo(demolumpname);
      d_doomloop(); /* never returns */
    }

  if (startloadgame >= 0)
    {
      m_str_copy(file, p_save_game_file(startloadgame), sizeof(file));
      g_load_game(file);
    }

  if (gameaction != ga_loadgame)
    {
      if (autostart || netgame)
        g_init_new(startskill, startepisode, startmap);
      else
        d_start_title(); /* start up intro loop */
    }

  d_doomloop(); /* never returns */
}
