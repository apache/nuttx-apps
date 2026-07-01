/****************************************************************************
 * apps/games/NXDoom/src/doom/m_menu.c
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
 *  DOOM selection menu, options, episode etc.
 *  Sliders and icons. Kinda widget stuff.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdlib.h>

#include "doomdef.h"
#include "doomkeys.h"
#include "dstrings.h"

#include "d_main.h"
#include "deh_main.h"

#include "i_input.h"
#include "i_joystick.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_local.h"

#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_controls.h"
#include "p_saveg.h"
#include "p_setup.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "doomstat.h"

/* Data. */

#include "m_menu.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SKULLXOFF -32
#define LINEHEIGHT 16

#define JOY_BUTTON_MAPPED(x) ((x) >= 0)
#define JOY_BUTTON_PRESSED(x)                                                \
  (JOY_BUTTON_MAPPED(x) && (ev->data1 & (1 << (x))) != 0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

void (*message_routine)(int response);

/* MENU TYPEDEFS */

typedef struct
{
  /* 0 = no cursor here, 1 = ok, 2 = arrows ok */

  short status;

  char name[10];

  /* choice = menu item #.
   * if status = 2,
   *   choice=0:leftarrow,1:rightarrow
   */

  void (*routine)(int choice);

  /* hotkey in menu */

  char alpha_key;
} menuitem_t;

typedef struct menu_s
{
  short numitems;           /* # of menu items */
  struct menu_s *prev_menu; /* previous menu */
  menuitem_t *menu_items;   /* menu items */
  void (*routine)(void);    /* draw routine */
  short x;
  short y;       /* x,y of menu */
  short last_on; /* last item user was on in menu */
} menu_t;

/* DOOM MENU */

enum
{
  MAIN_NEWGAME = 0,
  MAIN_OPTIONS,
  MAIN_LOADGAME,
  MAIN_SAVEGAME,
  MAIN_READTHIS,
  MAIN_QUITDOOM,
  MAIN_END
} main_e;

/* EPISODE SELECT */

enum
{
  EPISODE_EP1,
  EPISODE_EP2,
  EPISODE_EP3,
  EPISODE_EP4,
  EPISODE_END
} episodes_e;

/* NEW GAME */

enum
{
  NEWGAME_KILLTHINGS,
  NEWGAME_TOOROUGH,
  NEWGAME_HURTME,
  NEWGAME_VIOLANCE,
  NEWGAME_NIGHTMARE,
  NEWGAME_END
} newgame_e;

/* OPTIONS MENU */

enum
{
  OPT_ENDGAME,
  OPT_MESSAGES,
  OPT_DETAIL,
  OPT_SCRNSIZE,
  OPT_EMPTY1,
  OPT_MOUSESENS,
  OPT_EMPTY2,
  OPT_SOUNDVOL,
  OPT_END
} options_e;

enum
{
  RDTHSEMPTY1,
  READ1_END
} read_e;

enum
{
  RDTHSEMPTY2,
  READ2_END
} read_e2;

/* SOUND VOLUME MENU */

enum
{
  SOUND_SFXVOL,
  SOUND_SFXEMPTY1,
  SOUND_MUSICVOL,
  SOUND_SFXEMPTY2,
  SOUND_END
} sound_e;

/* LOAD GAME MENU */

enum
{
  LOAD_1,
  LOAD_2,
  LOAD_3,
  LOAD_4,
  LOAD_5,
  LOAD_6,
  LOAD_END
} load_e;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void m_new_game(int choice);
static void m_episode(int choice);
static void m_choose_skill(int choice);
static void m_load_game(int choice);
static void m_save_game(int choice);
static void m_options(int choice);
static void m_end_game(int choice);
static void m_read_this(int choice);
static void m_read_this2(int choice);
static void m_quit_doom(int choice);

static void m_change_messages(int choice);
static void m_change_sensitivity(int choice);
static void m_sound(int choice);
static void m_change_detail(int choice);
static void m_size_display(int choice);

static void m_finish_read_this(int choice);
static void m_load_select(int choice);
static void m_save_select(int choice);
static void m_read_save_strings(void);
static void m_quick_save(void);
static void m_quick_load(void);

static void m_drawmain_menu(void);
static void m_draw_read_this1(void);
static void m_draw_read_this2(void);
static void m_draw_new_game(void);
static void m_draw_episode(void);
static void m_draw_options(void);
static void m_draw_load(void);
static void m_draw_save(void);

static void m_draw_save_load_border(int x, int y);
static void m_setup_next_menu(menu_t *menudef);
static void m_draw_thermo(int x, int y, int therm_width, int therm_dot);
static void m_write_text(int x, int y, const char *string);
static int m_string_width(const char *string);
static int m_string_height(const char *string);
static void m_start_message(const char *string,
        void *routine, boolean input);
static void m_clear_menus(void);

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static void m_sfx_vol(int choice);
static void m_music_vol(int choice);
static void m_draw_sound(void);
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* defaulted values */

int g_mouse_sensitivity = 5;

/* Show messages has default, 0 = off, 1 = on */

int g_show_messages = 1;

/* Blocky mode, has default, 0 = high, 1 = normal */

int g_detail_level = 0;
int screenblocks = 9;

boolean g_menuactive;
boolean inhelpscreens;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* message x & y */

static int g_message_last_menu_active;

/* timed message = no input from user */

static boolean g_message_needs_input;

static char g_gammamsg[5][26] =
{
    GAMMALVL0, GAMMALVL1, GAMMALVL2, GAMMALVL3, GAMMALVL4,
};

/* we are going to be entering a savegame string */

static int g_save_string_enter;
static int g_save_slot;       /* which slot to save in */
static int g_save_char_index; /* which char we're editing */

/* old save description before edit */

static char g_save_old_string[SAVESTRINGSIZE];

static char g_save_game_strings[10][SAVESTRINGSIZE];

static char g_endstring[160];

static short g_item_on;            /* menu item skull is on */
static short g_skull_anim_counter; /* skull animation counter */
static short g_which_skull;        /* which skull to draw */

/* temp for screenblocks (0-9) */

static int g_screen_size;

/* -1 = no quicksave slot picked! */

static int g_quick_save_slot;

/* 1 = message to be printed */

static int g_message_to_print;

/* ...and here is the message string! */

static const char *g_message_string;

static int g_epi;

/* graphic name of skulls
 * warning: initializer-string for array of chars is too long
 */

static const char *g_skull_name[2] =
{
    "M_SKULL1",
    "M_SKULL2",
};

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static int g_quitsounds[8] =
{
    SFX_PLDETH, SFX_DMPAIN, SFX_POPAIN, SFX_SLOP,
    SFX_TELEPT, SFX_POSIT1, SFX_POSIT3, SFX_SGTATK,
};

static int g_quitsounds2[8] =
{
    SFX_VILACT, SFX_GETPOW, SFX_BOSCUB, SFX_SLOP,
    SFX_SKESWG, SFX_KNTDTH, SFX_BSPACT, SFX_SGTATK,
};
#endif

/* was the save action initiated by joypad? */

static boolean g_joypad_save = false;

static char g_tempstring[90];

static const char *g_detail_names[2] =
{
    "M_GDHIGH",
    "M_GDLOW",
};

static const char *g_msg_names[2] =
{
    "M_MSGOFF",
    "M_MSGON",
};

/* current menudef */

static menu_t *g_current_menu;

static menuitem_t g_main_menu[] =
{
  {1, "M_NGAME", m_new_game, 'n'},
  {1, "M_OPTION", m_options, 'o'},
  {1, "M_LOADG", m_load_game, 'l'},
  {1, "M_SAVEG", m_save_game, 's'},
  {1, "M_RDTHIS", m_read_this, 'r'}, /* Another hiccup with Special edition. */
  {1, "M_QUITG", m_quit_doom, 'q'},
};

static menu_t g_main_def =
{
    MAIN_END, NULL, g_main_menu, m_drawmain_menu, 97, 64, 0,
};

static menuitem_t g_episode_menu[] =
{
    {1, "M_EPI1", m_episode, 'k'},
    {1, "M_EPI2", m_episode, 't'},
    {1, "M_EPI3", m_episode, 'i'},
    {1, "M_EPI4", m_episode, 't'},
};

static menu_t g_epi_def =
{
    EPISODE_END,    /* # of menu items */
    &g_main_def,    /* previous menu */
    g_episode_menu, /* menuitem_t -> */
    m_draw_episode, /* drawing routine -> */
    48,
    63,         /* x,y */
    EPISODE_EP1 /* lastOn */
};

static menuitem_t new_game_menu[] =
{
  {1, "M_JKILL", m_choose_skill, 'i'},
  {1, "M_ROUGH", m_choose_skill, 'h'},
  {1, "M_HURT", m_choose_skill, 'h'},
  {1, "M_ULTRA", m_choose_skill, 'u'},
  {1, "M_NMARE", m_choose_skill, 'n'},
};

static menu_t g_new_def =
{
    NEWGAME_END,     /* # of menu items */
    &g_epi_def,      /* previous menu */
    new_game_menu,   /* menuitem_t -> */
    m_draw_new_game, /* drawing routine -> */
    48,
    63,            /* x,y */
    NEWGAME_HURTME /* lastOn */
};

static menuitem_t g_options_menu[] =
{
    {
        1,
        "M_ENDGAM",
        m_end_game,
        'e',
    },
    {
        1,
        "M_MESSG",
        m_change_messages,
        'm',
    },
    {
        1,
        "M_DETAIL",
        m_change_detail,
        'g',
    },
    {
        2,
        "M_SCRNSZ",
        m_size_display,
        's',
    },
    {
        -1,
        "",
        0,
        '\0',
    },
    {
        2,
        "M_MSENS",
        m_change_sensitivity,
        'm',
    },
    {
        -1,
        "",
        0,
        '\0',
    },
    {
        1,
        "M_SVOL",
        m_sound,
        's',
    },
};

static menu_t g_options_def =
{
    OPT_END, &g_main_def, g_options_menu, m_draw_options, 60, 37, 0,
};

/* Read This! MENU 1 & 2 */

static menuitem_t g_read_menu1[] =
{
    {1, "", m_read_this2, 0},
};

static menu_t g_read_def1 =
{
    READ1_END, &g_main_def, g_read_menu1, m_draw_read_this1, 280, 185, 0,
};

static menuitem_t g_read_menu2[] =
{
    {1, "", m_finish_read_this, 0},
};

static menu_t g_read_def2 =
{
    READ2_END, &g_read_def1, g_read_menu2, m_draw_read_this2, 330, 175, 0,
};

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static menuitem_t g_sound_menu[] =
{
    {
        2,
        "M_SFXVOL",
        m_sfx_vol,
        's',
    },
    {
        -1,
        "",
        0,
        '\0',
    },
    {
        2,
        "M_MUSVOL",
        m_music_vol,
        'm',
    },
    {
        -1,
        "",
        0,
        '\0',
    },
};

static menu_t g_sound_def =
{
    SOUND_END, &g_options_def, g_sound_menu, m_draw_sound, 80, 64, 0,
};
#endif

static menuitem_t g_load_menu[] =
{
  {1, "", m_load_select, '1'},
  {1, "", m_load_select, '2'},
  {1, "", m_load_select, '3'},
  {1, "", m_load_select, '4'},
  {1, "", m_load_select, '5'},
  {1, "", m_load_select, '6'},
};

static menu_t g_load_def =
{
  LOAD_END, &g_main_def, g_load_menu, m_draw_load, 80, 54, 0,
};

/* SAVE GAME MENU */

static menuitem_t g_save_menu[] =
{
  {1, "", m_save_select, '1'},
  {1, "", m_save_select, '2'},
  {1, "", m_save_select, '3'},
  {1, "", m_save_select, '4'},
  {1, "", m_save_select, '5'},
  {1, "", m_save_select, '6'},
};

static menu_t g_save_def =
{
    LOAD_END, &g_main_def, g_save_menu, m_draw_save, 80, 54, 0,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Generate a default save slot name when the user saves to
 * an empty slot via the joypad.
 */

static void set_default_save_name(int slot)
{
  /* map from IWAD or PWAD? */

  if (w_is_iwad_lump(maplumpinfo) && strcmp(savegamedir, ""))
    {
      snprintf(g_save_game_strings[g_item_on], SAVESTRINGSIZE, "%s",
               maplumpinfo->name);
    }
  else
    {
      char *wadname = m_string_duplicate(w_wad_name_for_lump(maplumpinfo));
      char *ext = strrchr(wadname, '.');

      if (ext != NULL)
        {
          *ext = '\0';
        }

      snprintf(g_save_game_strings[g_item_on], SAVESTRINGSIZE, "%s (%s)",
               maplumpinfo->name, wadname);
      free(wadname);
    }

  m_force_uppercase(g_save_game_strings[g_item_on]);
  g_joypad_save = false;
}

/* m_responder calls this when user is finished */

static void m_do_save(int slot)
{
  g_save_game(slot, g_save_game_strings[slot]);
  m_clear_menus();

  /* PICK QUICKSAVE SLOT YET? */

  if (g_quick_save_slot == -2) g_quick_save_slot = slot;
}

static void m_quick_save_response(int key)
{
  if (key == key_menu_confirm)
    {
      m_do_save(g_quick_save_slot);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_SWTCHX);
#endif
    }
}

static void m_quick_load_response(int key)
{
  if (key == key_menu_confirm)
    {
      m_load_select(g_quick_save_slot);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_SWTCHX);
#endif
    }
}

/* These keys evaluate to a "null" key in Vanilla Doom that allows weird
 * jumping in the menus. Preserve this behavior for accuracy.
 */

static boolean is_null_key(int key)
{
  return key == KEY_PAUSE || key == KEY_CAPSLOCK || key == KEY_SCRLCK ||
         key == KEY_NUMLOCK;
}

static void m_start_message(const char *string, void *routine, boolean input)
{
  g_message_last_menu_active = g_menuactive;
  g_message_to_print = 1;
  g_message_string = string;
  message_routine = routine;
  g_message_needs_input = input;
  g_menuactive = true;
  return;
}

static const char *m_select_end_message(void)
{
  const char **endmsg;

  if (logical_gamemission == doom)
    {
      /* Doom 1 */

      endmsg = doom1_endmsg;
    }
  else
    {
      /* Doom 2 */

      endmsg = doom2_endmsg;
    }

  return endmsg[gametic % NUM_QUITMESSAGES];
}

static void m_quit_response(int key)
{
  if (key != key_menu_confirm) return;
  if (!netgame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gamemode == commercial)
        s_start_sound(NULL, g_quitsounds2[(gametic >> 2) & 7]);
      else
        s_start_sound(NULL, g_quitsounds[(gametic >> 2) & 7]);
#endif
      i_wait_vbl(105);
    }

  i_quit();
}

static void m_end_game_response(int key)
{
  if (key != key_menu_confirm) return;

  g_current_menu->last_on = g_item_on;
  m_clear_menus();
  d_start_title();
}

static void m_draw_read_this_commercial(void)
{
  inhelpscreens = true;

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP"), PU_CACHE));
}

static void m_verify_nightmare(int key)
{
  if (key != key_menu_confirm) return;

  g_deferred_init_new(NEWGAME_NIGHTMARE, g_epi + 1, 1);
  m_clear_menus();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*  read the strings from the savegame files */

void m_read_save_strings(void)
{
  FILE *handle;
  int i;
  char name[256];

  for (i = 0; i < LOAD_END; i++)
    {
      int retval;
      m_str_copy(name, p_save_game_file(i), sizeof(name));

      handle = fopen(name, "rb");
      if (handle == NULL)
        {
          m_str_copy(g_save_game_strings[i], EMPTYSTRING, SAVESTRINGSIZE);
          g_load_menu[i].status = 0;
          continue;
        }

      retval = fread(&g_save_game_strings[i], 1, SAVESTRINGSIZE, handle);
      fclose(handle);
      g_load_menu[i].status = retval == SAVESTRINGSIZE;
    }
}

void m_draw_load(void)
{
  int i;

  v_draw_patch_direct(72, 28, w_cache_lump_name(("M_LOADG"), PU_CACHE));

  for (i = 0; i < LOAD_END; i++)
    {
      m_draw_save_load_border(g_load_def.x, g_load_def.y + LINEHEIGHT * i);
      m_write_text(g_load_def.x, g_load_def.y + LINEHEIGHT * i,
                   g_save_game_strings[i]);
    }
}

/* Draw border for the savegame description */

void m_draw_save_load_border(int x, int y)
{
  int i;

  v_draw_patch_direct(x - 8, y + 7,
                      w_cache_lump_name(("M_LSLEFT"), PU_CACHE));

  for (i = 0; i < 24; i++)
    {
      v_draw_patch_direct(x, y + 7,
                          w_cache_lump_name(("M_LSCNTR"), PU_CACHE));
      x += 8;
    }

  v_draw_patch_direct(x, y + 7, w_cache_lump_name(("M_LSRGHT"), PU_CACHE));
}

/* User wants to load this game */

void m_load_select(int choice)
{
  char name[256];

  m_str_copy(name, p_save_game_file(choice), sizeof(name));

  g_load_game(name);
  m_clear_menus();
}

/* Selected from DOOM menu */

void m_load_game(int choice)
{
  if (netgame)
    {
      m_start_message((LOADNET), NULL, false);
      return;
    }

  m_setup_next_menu(&g_load_def);
  m_read_save_strings();
}

/* M_SaveGame & Cie. */

void m_draw_save(void)
{
  int i;

  v_draw_patch_direct(72, 28, w_cache_lump_name(("M_SAVEG"), PU_CACHE));
  for (i = 0; i < LOAD_END; i++)
    {
      m_draw_save_load_border(g_load_def.x, g_load_def.y + LINEHEIGHT * i);
      m_write_text(g_load_def.x, g_load_def.y + LINEHEIGHT * i,
                   g_save_game_strings[i]);
    }

  if (g_save_string_enter)
    {
      i = m_string_width(g_save_game_strings[g_save_slot]);
      m_write_text(g_load_def.x + i,
              g_load_def.y + LINEHEIGHT * g_save_slot, "_");
    }
}

/* User wants to save. Start string input for m_responder */

void m_save_select(int choice)
{
  int x;
  int y;

  /* we are going to be intercepting all chars */

  g_save_string_enter = 1;

  /* We need to turn on text input: */

  x = g_load_def.x - 11;
  y = g_load_def.y + choice * LINEHEIGHT - 4;
  i_start_text_input(x, y, x + 8 + 24 * 8 + 8, y + LINEHEIGHT - 2);

  g_save_slot = choice;
  m_str_copy(g_save_old_string, g_save_game_strings[choice], SAVESTRINGSIZE);
  if (!strcmp(g_save_game_strings[choice], EMPTYSTRING))
    {
      g_save_game_strings[choice][0] = 0;

      if (g_joypad_save)
        {
          set_default_save_name(choice);
        }
    }

  g_save_char_index = strlen(g_save_game_strings[choice]);
}

/* Selected from DOOM menu */

void m_save_game(int choice)
{
  if (!usergame)
    {
      m_start_message((SAVEDEAD), NULL, false);
      return;
    }

  if (gamestate != GS_LEVEL) return;

  m_setup_next_menu(&g_save_def);
  m_read_save_strings();
}

void m_quick_save(void)
{
  if (!usergame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_OOF);
#endif
      return;
    }

  if (gamestate != GS_LEVEL) return;

  if (g_quick_save_slot < 0)
    {
      m_start_control_panel();
      m_read_save_strings();
      m_setup_next_menu(&g_save_def);
      g_quick_save_slot = -2; /* means to pick a slot now */
      return;
    }

  snprintf(g_tempstring, sizeof(g_tempstring), QSPROMPT,
           g_save_game_strings[g_quick_save_slot]);
  m_start_message(g_tempstring, m_quick_save_response, true);
}

static void m_quick_load(void)
{
  if (netgame)
    {
      m_start_message((QLOADNET), NULL, false);
      return;
    }

  if (g_quick_save_slot < 0)
    {
      m_start_message((QSAVESPOT), NULL, false);
      return;
    }

  snprintf(g_tempstring, sizeof(g_tempstring), QLPROMPT,
           g_save_game_strings[g_quick_save_slot]);
  m_start_message(g_tempstring, m_quick_load_response, true);
}

/* Read This Menus
 * Had a "quick hack to fix romero bug"
 */

static void m_draw_read_this1(void)
{
  inhelpscreens = true;

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP2"), PU_CACHE));
}

/* Read This Menus - optional second page. */

static void m_draw_read_this2(void)
{
  inhelpscreens = true;

  /* We only ever draw the second page if this is
   * gameversion == exe_doom_1_9 and gamemode == registered
   */

  v_draw_patch_direct(0, 0, w_cache_lump_name(("HELP1"), PU_CACHE));
}

/* Change Sfx & Music volumes */

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static void m_draw_sound(void)
{
  v_draw_patch_direct(60, 38, w_cache_lump_name(("M_SVOL"), PU_CACHE));

  m_draw_thermo(g_sound_def.x,
                g_sound_def.y + LINEHEIGHT * (SOUND_SFXVOL + 1), 16,
                g_sfx_volume);

  m_draw_thermo(g_sound_def.x,
                g_sound_def.y + LINEHEIGHT * (SOUND_MUSICVOL + 1), 16,
                g_music_volume);
}
#endif

static void m_sound(int choice)
{
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  m_setup_next_menu(&g_sound_def);
#endif
}

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static void m_sfx_vol(int choice)
{
  switch (choice)
    {
    case 0:
      if (g_sfx_volume) g_sfx_volume--;
      break;
    case 1:
      if (g_sfx_volume < 15) g_sfx_volume++;
      break;
    }

  s_set_sfx_volume(g_sfx_volume * 8);
}
#endif

#ifdef CONFIG_GAMES_NXDOOM_SOUND
static void m_music_vol(int choice)
{
  switch (choice)
    {
    case 0:
      if (g_music_volume) g_music_volume--;
      break;
    case 1:
      if (g_music_volume < 15) g_music_volume++;
      break;
    }

  s_set_music_volume(g_music_volume * 8);
}
#endif

void m_drawmain_menu(void)
{
  v_draw_patch_direct(94, 2, w_cache_lump_name(("M_DOOM"), PU_CACHE));
}

void m_draw_new_game(void)
{
  v_draw_patch_direct(96, 14, w_cache_lump_name(("M_NEWG"), PU_CACHE));
  v_draw_patch_direct(54, 38, w_cache_lump_name(("M_SKILL"), PU_CACHE));
}

void m_new_game(int choice)
{
  if (netgame && !demoplayback)
    {
      m_start_message((NEWGAME), NULL, false);
      return;
    }

  /* Chex Quest disabled the episode select screen, as did Doom II. */

  if (gamemode == commercial || gameversion == exe_chex)
    m_setup_next_menu(&g_new_def);
  else
    m_setup_next_menu(&g_epi_def);
}

void m_draw_episode(void)
{
  v_draw_patch_direct(54, 38, w_cache_lump_name(("M_EPISOD"), PU_CACHE));
}

void m_choose_skill(int choice)
{
  if (choice == NEWGAME_NIGHTMARE)
    {
      m_start_message((NIGHTMARE), m_verify_nightmare, true);
      return;
    }

  g_deferred_init_new(choice, g_epi + 1, 1);
  m_clear_menus();
}

void m_episode(int choice)
{
  if ((gamemode == shareware) && choice)
    {
      m_start_message((SWSTRING), NULL, false);
      m_setup_next_menu(&g_read_def1);
      return;
    }

  g_epi = choice;
  m_setup_next_menu(&g_new_def);
}

void m_draw_options(void)
{
  v_draw_patch_direct(108, 15, w_cache_lump_name(("M_OPTTTL"), PU_CACHE));

  v_draw_patch_direct(
      g_options_def.x + 175, g_options_def.y + LINEHEIGHT * OPT_DETAIL,
      w_cache_lump_name((g_detail_names[g_detail_level]), PU_CACHE));

  v_draw_patch_direct(
      g_options_def.x + 120, g_options_def.y + LINEHEIGHT * OPT_MESSAGES,
      w_cache_lump_name((g_msg_names[g_show_messages]), PU_CACHE));

  m_draw_thermo(g_options_def.x,
                g_options_def.y + LINEHEIGHT * (OPT_MOUSESENS + 1), 10,
                g_mouse_sensitivity);

  m_draw_thermo(g_options_def.x,
                g_options_def.y + LINEHEIGHT * (OPT_SCRNSIZE + 1), 9,
                g_screen_size);
}

void m_options(int choice)
{
  m_setup_next_menu(&g_options_def);
}

/* Toggle messages on/off */

void m_change_messages(int choice)
{
  /* warning: unused parameter `int choice' */

  choice = 0;
  g_show_messages = 1 - g_show_messages;

  if (!g_show_messages)
    players[consoleplayer].message = (MSGOFF);
  else
    players[consoleplayer].message = (MSGON);

  message_dontfuckwithme = true;
}

void m_end_game(int choice)
{
  choice = 0;
  if (!usergame)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_OOF);
#endif
      return;
    }

  if (netgame)
    {
      m_start_message((NETEND), NULL, false);
      return;
    }

  m_start_message((ENDGAME), m_end_game_response, true);
}

void m_read_this(int choice)
{
  choice = 0;
  m_setup_next_menu(&g_read_def1);
}

void m_read_this2(int choice)
{
  choice = 0;
  m_setup_next_menu(&g_read_def2);
}

void m_finish_read_this(int choice)
{
  choice = 0;
  m_setup_next_menu(&g_main_def);
}

void m_quit_doom(int choice)
{
  snprintf(g_endstring, sizeof(g_endstring), "%s\n\n" DOSY,
           (m_select_end_message()));

  m_start_message(g_endstring, m_quit_response, true);
}

void m_change_sensitivity(int choice)
{
  switch (choice)
    {
    case 0:
      if (g_mouse_sensitivity) g_mouse_sensitivity--;
      break;
    case 1:
      if (g_mouse_sensitivity < 9) g_mouse_sensitivity++;
      break;
    }
}

void m_change_detail(int choice)
{
  choice = 0;
  g_detail_level = 1 - g_detail_level;

  r_set_view_size(screenblocks, g_detail_level);

  if (!g_detail_level)
    players[consoleplayer].message = (DETAILHI);
  else
    players[consoleplayer].message = (DETAILLO);
}

void m_size_display(int choice)
{
  switch (choice)
    {
    case 0:
      if (g_screen_size > 0)
        {
          screenblocks--;
          g_screen_size--;
        }
      break;
    case 1:
      if (g_screen_size < 8)
        {
          screenblocks++;
          g_screen_size++;
        }
      break;
    }

  r_set_view_size(screenblocks, g_detail_level);
}

/* Menu Functions */

void m_draw_thermo(int x, int y, int therm_width, int therm_dot)
{
  int xx;
  int i;

  xx = x;
  v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERML"), PU_CACHE));
  xx += 8;
  for (i = 0; i < therm_width; i++)
    {
      v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERMM"), PU_CACHE));
      xx += 8;
    }

  v_draw_patch_direct(xx, y, w_cache_lump_name(("M_THERMR"), PU_CACHE));

  v_draw_patch_direct((x + 8) + therm_dot * 8, y,
                      w_cache_lump_name(("M_THERMO"), PU_CACHE));
}

/* Find string width from hu_font chars */

int m_string_width(const char *string)
{
  size_t i;
  int w = 0;
  int c;

  for (i = 0; i < strlen(string); i++)
    {
      c = toupper(string[i]) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        w += 4;
      else
        w += SHORT(hu_font[c]->width);
    }

  return w;
}

/* Find string height from hu_font chars */

int m_string_height(const char *string)
{
  size_t i;
  int h;
  int height = SHORT(hu_font[0]->height);

  h = height;
  for (i = 0; i < strlen(string); i++)
    {
      if (string[i] == '\n') h += height;
    }

  return h;
}

/* Write a string using the hu_font */

void m_write_text(int x, int y, const char *string)
{
  int w;
  const char *ch;
  int c;
  int cx;
  int cy;

  ch = string;
  cx = x;
  cy = y;

  while (1)
    {
      c = *ch++;
      if (!c) break;
      if (c == '\n')
        {
          cx = x;
          cy += 12;
          continue;
        }

      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        {
          cx += 4;
          continue;
        }

      w = SHORT(hu_font[c]->width);
      if (cx + w > SCREENWIDTH) break;
      v_draw_patch_direct(cx, cy, hu_font[c]);
      cx += w;
    }
}

/* CONTROL PANEL */

boolean m_responder(event_t *ev)
{
  int ch;
  int key;
  int i;
  static int mousewait = 0;
  static int mousey = 0;
  static int lasty = 0;
  static int mousex = 0;
  static int lastx = 0;
  int dir;

  /* In testcontrols mode, none of the function keys should do anything
   * - the only key is escape to quit.
   */

  if (testcontrols)
    {
      if (ev->type == ev_quit ||
          (ev->type == ev_keydown &&
           (ev->data1 == key_menu_activate || ev->data1 == key_menu_quit)))
        {
          i_quit();
          return true;
        }

      return false;
    }

  /* "close" button pressed on window? */

  if (ev->type == ev_quit)
    {
      /* First click on close button = bring up quit confirm message.
       * Second click on close button = confirm quit
       */

      if (g_menuactive && g_message_to_print &&
          message_routine == m_quit_response)
        {
          m_quit_response(key_menu_confirm);
        }
      else
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_quit_doom(0);
        }

      return true;
    }

  /* key is the key pressed, ch is the actual character typed */

  ch = 0;
  key = -1;

  if (ev->type == ev_joystick)
    {
      /* Simulate key presses from joystick events to interact with the menu.
       */

      if (g_menuactive)
        {
          if (JOY_GET_DPAD(ev->data6) != JOY_DIR_NONE)
            {
              dir = JOY_GET_DPAD(ev->data6);
            }
          else if (JOY_GET_LSTICK(ev->data6) != JOY_DIR_NONE)
            {
              dir = JOY_GET_LSTICK(ev->data6);
            }
          else
            {
              dir = JOY_GET_RSTICK(ev->data6);
            }

          if (dir & JOY_DIR_UP)
            {
              key = key_menu_up;
              joywait = i_get_time() + 5;
            }
          else if (dir & JOY_DIR_DOWN)
            {
              key = key_menu_down;
              joywait = i_get_time() + 5;
            }

          if (dir & JOY_DIR_LEFT)
            {
              key = key_menu_left;
              joywait = i_get_time() + 5;
            }
          else if (dir & JOY_DIR_RIGHT)
            {
              key = key_menu_right;
              joywait = i_get_time() + 5;
            }

          if (JOY_BUTTON_PRESSED(joybfire))
            {
              /* Simulate a 'Y' keypress when Doom show a Y/N dialog with
               * Fire button.
               */

              if (g_message_to_print && g_message_needs_input)
                {
                  key = key_menu_confirm;
                }

              /* Simulate pressing "Enter" when we are supplying a save slot
               * name
               */

              else if (g_save_string_enter)
                {
                  key = KEY_ENTER;
                }
              else
                {
                  /* if selecting a save slot via joypad, set a flag */

                  if (g_current_menu == &g_save_def)
                    {
                      g_joypad_save = true;
                    }

                  key = key_menu_forward;
                }

              joywait = i_get_time() + 5;
            }

          if (JOY_BUTTON_PRESSED(joybuse))
            {
              /* Simulate a 'N' keypress when Doom show a Y/N dialog with Use
               * button.
               */

              if (g_message_to_print && g_message_needs_input)
                {
                  key = key_menu_abort;
                }

              /* If user was entering a save name, back out */

              else if (g_save_string_enter)
                {
                  key = KEY_ESCAPE;
                }
              else
                {
                  key = key_menu_back;
                }

              joywait = i_get_time() + 5;
            }
        }

      if (JOY_BUTTON_PRESSED(joybmenu))
        {
          key = key_menu_activate;
          joywait = i_get_time() + 5;
        }
    }
  else
    {
      if (ev->type == ev_mouse && mousewait < i_get_time() && g_menuactive)
        {
          mousey += ev->data3;
          if (mousey < lasty - 30)
            {
              key = key_menu_down;
              mousewait = i_get_time() + 5;
              mousey = lasty -= 30;
            }
          else if (mousey > lasty + 30)
            {
              key = key_menu_up;
              mousewait = i_get_time() + 5;
              mousey = lasty += 30;
            }

          mousex += ev->data2;
          if (mousex < lastx - 30)
            {
              key = key_menu_left;
              mousewait = i_get_time() + 5;
              mousex = lastx -= 30;
            }
          else if (mousex > lastx + 30)
            {
              key = key_menu_right;
              mousewait = i_get_time() + 5;
              mousex = lastx += 30;
            }

          if (ev->data1 & 1)
            {
              key = key_menu_forward;
              mousewait = i_get_time() + 15;
            }

          if (ev->data1 & 2)
            {
              key = key_menu_back;
              mousewait = i_get_time() + 15;
            }
        }
      else
        {
          if (ev->type == ev_keydown)
            {
              key = ev->data1;
              ch = ev->data2;
            }
        }
    }

  if (key == -1) return false;

  /* Save Game string input */

  if (g_save_string_enter)
    {
      switch (key)
        {
        case KEY_BACKSPACE:
          if (g_save_char_index > 0)
            {
              g_save_char_index--;
              g_save_game_strings[g_save_slot][g_save_char_index] = 0;
            }
          break;

        case KEY_ESCAPE:
          g_save_string_enter = 0;
          i_stop_text_input();
          m_str_copy(g_save_game_strings[g_save_slot], g_save_old_string,
                     SAVESTRINGSIZE);
          break;

        case KEY_ENTER:
          g_save_string_enter = 0;
          i_stop_text_input();
          if (g_save_game_strings[g_save_slot][0]) m_do_save(g_save_slot);
          break;

        default:
          /* Savegame name entry. This is complicated.
           * Vanilla has a bug where the shift key is ignored when entering
           * a savegame name. If vanilla_keyboard_mapping is on, we want
           * to emulate this bug by using ev->data1. But if it's turned off,
           * it implies the user doesn't care about Vanilla emulation:
           * instead, use ev->data3 which gives the fully-translated and
           * modified key input.
           */

          if (ev->type != ev_keydown)
            {
              break;
            }

          if (vanilla_keyboard_mapping)
            {
              ch = ev->data1;
            }
          else
            {
              ch = ev->data3;
            }

          ch = toupper(ch);

          if (ch != ' ' &&
              (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
            {
              break;
            }

          if (ch >= 32 && ch <= 127 &&
              g_save_char_index < SAVESTRINGSIZE - 1 &&
              m_string_width(g_save_game_strings[g_save_slot]) <
                  (SAVESTRINGSIZE - 2) * 8)
            {
              g_save_game_strings[g_save_slot][g_save_char_index++] = ch;
              g_save_game_strings[g_save_slot][g_save_char_index] = 0;
            }

          break;
        }

      return true;
    }

  /* Take care of any messages that need input */

  if (g_message_to_print)
    {
      if (g_message_needs_input)
        {
          if (key != ' ' && key != KEY_ESCAPE && key != key_menu_confirm &&
              key != key_menu_abort)
            {
              return false;
            }
        }

      g_menuactive = g_message_last_menu_active;
      g_message_to_print = 0;
      if (message_routine) message_routine(key);

      g_menuactive = false;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_SWTCHX);
#endif
      return true;
    }

  if ((devparm && key == key_menu_help) ||
      (key != 0 && key == key_menu_screenshot))
    {
      g_screenshot();
      return true;
    }

  /* F-Keys */

  if (!g_menuactive)
    {
      if (key == key_menu_decscreen) /* Screen size down */
        {
          if (automapactive || chat_on) return false;
          m_size_display(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_STNMOV);
#endif
          return true;
        }
      else if (key == key_menu_incscreen) /* Screen size up */
        {
          if (automapactive || chat_on) return false;
          m_size_display(1);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_STNMOV);
#endif
          return true;
        }
      else if (key == key_menu_help) /* Help key */
        {
          m_start_control_panel();

          if (gameversion >= exe_ultimate)
            g_current_menu = &g_read_def2;
          else
            g_current_menu = &g_read_def1;

          g_item_on = 0;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          return true;
        }
      else if (key == key_menu_save) /* Save */
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_save_game(0);
          return true;
        }
      else if (key == key_menu_load) /* Load */
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_load_game(0);
          return true;
        }
      else if (key == key_menu_volume) /* Sound Volume */
        {
          m_start_control_panel();
          g_item_on = SOUND_SFXVOL;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          g_current_menu = &g_sound_def;
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          return true;
        }
      else if (key == key_menu_detail) /* Detail toggle */
        {
          m_change_detail(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          return true;
        }
      else if (key == key_menu_qsave) /* Quicksave */
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_quick_save();
          return true;
        }
      else if (key == key_menu_endgame) /* End game */
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_end_game(0);
          return true;
        }
      else if (key == key_menu_messages) /* Toggle messages */
        {
          m_change_messages(0);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          return true;
        }
      else if (key == key_menu_qload) /* Quickload */
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_quick_load();
          return true;
        }
      else if (key == key_menu_quit) /* Quit DOOM */
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          m_quit_doom(0);
          return true;
        }
      else if (key == key_menu_gamma) /* gamma toggle */
        {
          usegamma++;
          if (usegamma > 4) usegamma = 0;
          players[consoleplayer].message = (g_gammamsg[usegamma]);
          i_set_palette(w_cache_lump_name(("PLAYPAL"), PU_CACHE));
          return true;
        }
    }

  /* Pop-up menu? */

  if (!g_menuactive)
    {
      if (key == key_menu_activate)
        {
          m_start_control_panel();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
          return true;
        }

      return false;
    }

  /* Keys usable within menu */

  if (key == key_menu_down)
    {
      /* Move down to next item */

      do
        {
          if (g_item_on + 1 > g_current_menu->numitems - 1)
            g_item_on = 0;
          else
            g_item_on++;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_PSTOP);
#endif
        }
      while (g_current_menu->menu_items[g_item_on].status == -1);

      return true;
    }
  else if (key == key_menu_up)
    {
      /* Move back up to previous item */

      do
        {
          if (!g_item_on)
            g_item_on = g_current_menu->numitems - 1;
          else
            g_item_on--;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_PSTOP);
#endif
        }
      while (g_current_menu->menu_items[g_item_on].status == -1);

      return true;
    }
  else if (key == key_menu_left)
    {
      /* Slide slider left */

      if (g_current_menu->menu_items[g_item_on].routine &&
          g_current_menu->menu_items[g_item_on].status == 2)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_STNMOV);
#endif
          g_current_menu->menu_items[g_item_on].routine(0);
        }

      return true;
    }
  else if (key == key_menu_right)
    {
      /* Slide slider right */

      if (g_current_menu->menu_items[g_item_on].routine &&
          g_current_menu->menu_items[g_item_on].status == 2)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_STNMOV);
#endif
          g_current_menu->menu_items[g_item_on].routine(1);
        }

      return true;
    }
  else if (key == key_menu_forward)
    {
      /* Activate menu item */

      if (g_current_menu->menu_items[g_item_on].routine &&
          g_current_menu->menu_items[g_item_on].status)
        {
          g_current_menu->last_on = g_item_on;
          if (g_current_menu->menu_items[g_item_on].status == 2)
            {
              g_current_menu->menu_items[g_item_on].routine(
                  1); /* right arrow */
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, SFX_STNMOV);
#endif
            }
          else
            {
              g_current_menu->menu_items[g_item_on].routine(g_item_on);
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, SFX_PISTOL);
#endif
            }
        }

      return true;
    }
  else if (key == key_menu_activate)
    {
      /* Deactivate menu */

      g_current_menu->last_on = g_item_on;
      m_clear_menus();
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_SWTCHX);
#endif
      return true;
    }
  else if (key == key_menu_back)
    {
      /* Go back to previous menu */

      g_current_menu->last_on = g_item_on;
      if (g_current_menu->prev_menu)
        {
          g_current_menu = g_current_menu->prev_menu;
          g_item_on = g_current_menu->last_on;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(NULL, SFX_SWTCHN);
#endif
        }

      return true;
    }

  /* Keyboard shortcut?
   * Vanilla Doom has a weird behavior where it jumps to the scroll bars
   * when the certain keys are pressed, so emulate this.
   */

  else if (ch != 0 || is_null_key(key))
    {
      for (i = g_item_on + 1; i < g_current_menu->numitems; i++)
        {
          if (g_current_menu->menu_items[i].alpha_key == ch)
            {
              g_item_on = i;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, SFX_PSTOP);
#endif
              return true;
            }
        }

      for (i = 0; i <= g_item_on; i++)
        {
          if (g_current_menu->menu_items[i].alpha_key == ch)
            {
              g_item_on = i;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(NULL, SFX_PSTOP);
#endif
              return true;
            }
        }
    }

  return false;
}

void m_start_control_panel(void)
{
  /* intro might call this repeatedly */

  if (g_menuactive) return;

  g_menuactive = 1;
  g_current_menu = &g_main_def;        /* JDC */
  g_item_on = g_current_menu->last_on; /* JDC */
}

/* Display OPL debug messages - hack for GENMIDI development. */

#if 0
static void m_draw_opl_dev(void)
{
  char debug[1024];
  char *curr;
  char *p;
  int line;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  i_opl_dev_messages(debug, sizeof(debug));
#endif
  curr = debug;
  line = 0;

  for (; ; )
    {
      p = strchr(curr, '\n');

      if (p != NULL)
        {
          *p = '\0';
        }

      m_write_text(0, line * 8, curr);
      ++line;

      if (p == NULL)
        {
          break;
        }

      curr = p + 1;
    }
}
#endif

/* m_drawer
 * Called after the view has been rendered,
 * but before it has been blitted.
 */

void m_drawer(void)
{
  static short x;
  static short y;
  unsigned int i;
  unsigned int max;
  char string[80];
  const char *name;
  int start;

  inhelpscreens = false;

  /* Horiz. & Vertically center string and print it. */

  if (g_message_to_print)
    {
      start = 0;
      y = SCREENHEIGHT / 2 - m_string_height(g_message_string) / 2;
      while (g_message_string[start] != '\0')
        {
          boolean foundnewline = false;

          for (i = 0; g_message_string[start + i] != '\0'; i++)
            {
              if (g_message_string[start + i] == '\n')
                {
                  m_str_copy(string, g_message_string + start,
                             sizeof(string));
                  if (i < sizeof(string))
                    {
                      string[i] = '\0';
                    }

                  foundnewline = true;
                  start += i + 1;
                  break;
                }
            }

          if (!foundnewline)
            {
              m_str_copy(string, g_message_string + start, sizeof(string));
              start += strlen(string);
            }

          x = SCREENWIDTH / 2 - m_string_width(string) / 2;
          m_write_text(x, y, string);
          y += SHORT(hu_font[0]->height);
        }

      return;
    }

  if (!g_menuactive) return;

  if (g_current_menu->routine)
    g_current_menu->routine(); /* call Draw routine */

  /* DRAW MENU */

  x = g_current_menu->x;
  y = g_current_menu->y;
  max = g_current_menu->numitems;

  for (i = 0; i < max; i++)
    {
      name = (g_current_menu->menu_items[i].name);

      if (name[0] && w_check_num_for_name(name) > 0)
        {
          v_draw_patch_direct(x, y, w_cache_lump_name(name, PU_CACHE));
        }

      y += LINEHEIGHT;
    }

  /* DRAW SKULL */

  v_draw_patch_direct(
      x + SKULLXOFF, g_current_menu->y - 5 + g_item_on * LINEHEIGHT,
      w_cache_lump_name((g_skull_name[g_which_skull]), PU_CACHE));
}

void m_clear_menus(void)
{
  g_menuactive = 0;
}

void m_setup_next_menu(menu_t *menudef)
{
  g_current_menu = menudef;
  g_item_on = g_current_menu->last_on;
}

void m_ticker(void)
{
  if (--g_skull_anim_counter <= 0)
    {
      g_which_skull ^= 1;
      g_skull_anim_counter = 8;
    }
}

void m_init(void)
{
  g_current_menu = &g_main_def;
  g_menuactive = 0;
  g_item_on = g_current_menu->last_on;
  g_which_skull = 0;
  g_skull_anim_counter = 10;
  g_screen_size = screenblocks - 3;
  g_message_to_print = 0;
  g_message_string = NULL;
  g_message_last_menu_active = g_menuactive;
  g_quick_save_slot = -1;

  /* Here we could catch other version dependencies, like HELP1/2, and four
   * episodes.
   */

  /* The same hacks were used in the original Doom EXEs. */

  if (gameversion >= exe_ultimate)
    {
      g_main_menu[MAIN_READTHIS].routine = m_read_this2;
      g_read_def2.prev_menu = NULL;
    }

  if (gameversion >= exe_final && gameversion <= exe_final2)
    {
      g_read_def2.routine = m_draw_read_this_commercial;
    }

  if (gamemode == commercial)
    {
      g_main_menu[MAIN_READTHIS] = g_main_menu[MAIN_QUITDOOM];
      g_main_def.numitems--;
      g_main_def.y += 8;
      g_new_def.prev_menu = &g_main_def;
      g_read_def1.routine = m_draw_read_this_commercial;
      g_read_def1.x = 330;
      g_read_def1.y = 165;
      g_read_menu1[RDTHSEMPTY1].routine = m_finish_read_this;
    }

  /* Versions of doom.exe before the Ultimate Doom release only had
   * three episodes; if we're emulating one of those then don't try
   * to show episode four. If we are, then do show episode four
   * (should crash if missing).
   */

  if (gameversion < exe_ultimate)
    {
      g_epi_def.numitems--;
    }

  /* chex.exe shows only one episode. */

  else if (gameversion == exe_chex)
    {
      g_epi_def.numitems = 1;
    }
}
