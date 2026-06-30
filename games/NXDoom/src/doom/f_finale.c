/****************************************************************************
 * apps/games/NXDoom/src/doom/f_finale.c
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
 *  Game completion, final screen animation.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

/* Functions. */

#include "deh_main.h"
#include "i_swap.h"
#include "i_system.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

/* Data. */

#include "d_main.h"
#include "dstrings.h"

#include "doomstat.h"
#include "r_state.h"

#include "hu_stuff.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TEXTSPEED 3
#define TEXTWAIT 250

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  F_STAGE_TEXT,
  F_STAGE_ARTSCREEN,
  F_STAGE_CAST,
} finalestage_t;

/* ?
 * #include "doomstat.h"
 * #include "r_local.h"
 * #include "f_finale.h"
 */

typedef struct
{
  gamemission_t mission;
  int episode;
  int level;
  const char *background;
  const char *text;
} textscreen_t;

typedef struct
{
  const char *name;
  mobjtype_t type;
} castinfo_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static textscreen_t g_textscreens[] =
{
  {doom, 1, 8, "FLOOR4_8", E1TEXT},
  {doom, 2, 8, "SFLR6_1", E2TEXT},
  {doom, 3, 8, "MFLR8_4", E3TEXT},
  {doom, 4, 8, "MFLR8_3", E4TEXT},
  {doom2, 1, 6, "SLIME16", C1TEXT},
  {doom2, 1, 11, "RROCK14", C2TEXT},
  {doom2, 1, 20, "RROCK07", C3TEXT},
  {doom2, 1, 30, "RROCK17", C4TEXT},
  {doom2, 1, 15, "RROCK13", C5TEXT},
  {doom2, 1, 31, "RROCK19", C6TEXT},
  {pack_tnt, 1, 6, "SLIME16", T1TEXT},
  {pack_tnt, 1, 11, "RROCK14", T2TEXT},
  {pack_tnt, 1, 20, "RROCK07", T3TEXT},
  {pack_tnt, 1, 30, "RROCK17", T4TEXT},
  {pack_tnt, 1, 15, "RROCK13", T5TEXT},
  {pack_tnt, 1, 31, "RROCK19", T6TEXT},
  {pack_plut, 1, 6, "SLIME16", P1TEXT},
  {pack_plut, 1, 11, "RROCK14", P2TEXT},
  {pack_plut, 1, 20, "RROCK07", P3TEXT},
  {pack_plut, 1, 30, "RROCK17", P4TEXT},
  {pack_plut, 1, 15, "RROCK13", P5TEXT},
  {pack_plut, 1, 31, "RROCK19", P6TEXT},
};

/* Final DOOM 2 animation
 * Casting by id Software. in order of appearance
 */

static castinfo_t g_castorder[] =
{
  {CC_ZOMBIE, MT_POSSESSED},
  {CC_SHOTGUN, MT_SHOTGUY},
  {CC_HEAVY, MT_CHAINGUY},
  {CC_IMP, MT_TROOP},
  {CC_DEMON, MT_SERGEANT},
  {CC_LOST, MT_SKULL},
  {CC_CACO, MT_HEAD},
  {CC_HELL, MT_KNIGHT},
  {CC_BARON, MT_BRUISER},
  {CC_ARACH, MT_BABY},
  {CC_PAIN, MT_PAIN},
  {CC_REVEN, MT_UNDEAD},
  {CC_MANCU, MT_FATSO},
  {CC_ARCH, MT_VILE},
  {CC_SPIDER, MT_SPIDER},
  {CC_CYBER, MT_CYBORG},
  {CC_HERO, MT_PLAYER},
  {NULL, 0},
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Stage of animation: */

finalestage_t finalestage;
unsigned int finalecount;

const char *finaletext;
const char *finaleflat;

int castnum;
int casttics;
state_t *caststate;
boolean castdeath;
int castframes;
int castonmelee;
boolean castattacking;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void f_start_cast(void)
{
  wipegamestate = -1; /* force a screen wipe */
  castnum = 0;
  caststate = &states[mobjinfo[g_castorder[castnum].type].seestate];
  casttics = caststate->tics;
  castdeath = false;
  finalestage = F_STAGE_CAST;
  castframes = 0;
  castonmelee = 0;
  castattacking = false;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_change_music(MUS_EVIL, true);
#endif
}

static void f_text_write(void)
{
  byte *src;
  pixel_t *dest;
  int x;
  int y;
  int w;
  signed int count;
  const char *ch;
  int c;
  int cx;
  int cy;

  /* erase the entire screen to a tiled background */

  src = w_cache_lump_name(finaleflat, PU_CACHE);
  dest = i_video_buffer;

  for (y = 0; y < SCREENHEIGHT; y++)
    {
      for (x = 0; x < SCREENWIDTH / 64; x++)
        {
          memcpy(dest, src + ((y & 63) << 6), 64);
          dest += 64;
        }

      if (SCREENWIDTH & 63)
        {
          memcpy(dest, src + ((y & 63) << 6), SCREENWIDTH & 63);
          dest += (SCREENWIDTH & 63);
        }
    }

  v_mark_rect(0, 0, SCREENWIDTH, SCREENHEIGHT);

  /* draw some of the text onto the screen */

  cx = 10;
  cy = 10;
  ch = finaletext;

  count = ((signed int)finalecount - 10) / TEXTSPEED;
  if (count < 0) count = 0;
  for (; count; count--)
    {
      c = *ch++;
      if (!c) break;
      if (c == '\n')
        {
          cx = 10;
          cy += 11;
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
      v_draw_patch(cx, cy, hu_font[c]);
      cx += w;
    }
}

static void f_cast_ticker(void)
{
  int st;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
  int sfx;
#endif

  if (--casttics > 0) return; /* not time to change state yet */

  if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
      /* switch from deathstate to next monster */

      castnum++;
      castdeath = false;
      if (g_castorder[castnum].name == NULL) castnum = 0;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (mobjinfo[g_castorder[castnum].type].seesound)
        {
          s_start_sound(NULL, mobjinfo[g_castorder[castnum].type].seesound);
        }

#endif

      caststate = &states[mobjinfo[g_castorder[castnum].type].seestate];
      castframes = 0;
    }
  else
    {
      /* just advance to next state in animation */

      if (caststate == &states[S_PLAY_ATK1])
        {
          goto stopattack; /* Oh, gross hack! */
        }

      st = caststate->nextstate;
      caststate = &states[st];
      castframes++;

      /* sound hacks.... */

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      switch (st)
        {
        case S_PLAY_ATK1:
          sfx = SFX_DSHTGN;
          break;
        case S_POSS_ATK2:
          sfx = SFX_PISTOL;
          break;
        case S_SPOS_ATK2:
          sfx = SFX_SHOTGN;
          break;
        case S_VILE_ATK2:
          sfx = SFX_VILATK;
          break;
        case S_SKEL_FIST2:
          sfx = SFX_SKESWG;
          break;
        case S_SKEL_FIST4:
          sfx = SFX_SKEPCH;
          break;
        case S_SKEL_MISS2:
          sfx = SFX_SKEATK;
          break;
        case S_FATT_ATK8:
        case S_FATT_ATK5:
        case S_FATT_ATK2:
          sfx = SFX_FIRSHT;
          break;
        case S_CPOS_ATK2:
        case S_CPOS_ATK3:
        case S_CPOS_ATK4:
          sfx = SFX_SHOTGN;
          break;
        case S_TROO_ATK3:
          sfx = SFX_CLAW;
          break;
        case S_SARG_ATK2:
          sfx = SFX_SGTATK;
          break;
        case S_BOSS_ATK2:
        case S_BOS2_ATK2:
        case S_HEAD_ATK2:
          sfx = SFX_FIRSHT;
          break;
        case S_SKULL_ATK2:
          sfx = SFX_SKLATK;
          break;
        case S_SPID_ATK2:
        case S_SPID_ATK3:
          sfx = SFX_SHOTGN;
          break;
        case S_BSPI_ATK2:
          sfx = SFX_PLASMA;
          break;
        case S_CYBER_ATK2:
        case S_CYBER_ATK4:
        case S_CYBER_ATK6:
          sfx = SFX_RLAUNC;
          break;
        case S_PAIN_ATK3:
          sfx = SFX_SKLATK;
          break;
        default:
          sfx = 0;
          break;
        }

      if (sfx) s_start_sound(NULL, sfx);
#endif
    }

  if (castframes == 12)
    {
      /* go into attack frame */

      castattacking = true;

      if (castonmelee)
        {
          caststate =
              &states[mobjinfo[g_castorder[castnum].type].meleestate];
        }
      else
        {
          caststate =
              &states[mobjinfo[g_castorder[castnum].type].missilestate];
        }

      castonmelee ^= 1;

      if (caststate == &states[S_NULL])
        {
          if (castonmelee)
            {
              caststate =
                  &states[mobjinfo[g_castorder[castnum].type].meleestate];
            }
          else
            {
              caststate =
                  &states[mobjinfo[g_castorder[castnum].type].missilestate];
            }
        }
    }

  if (castattacking)
    {
      if (castframes == 24 ||
          caststate == &states[mobjinfo[g_castorder[castnum].type].seestate])
        {
        stopattack:
          castattacking = false;
          castframes = 0;
          caststate = &states[mobjinfo[g_castorder[castnum].type].seestate];
        }
    }

  casttics = caststate->tics;
  if (casttics == -1) casttics = 15;
}

static boolean f_cast_responder(event_t *ev)
{
  if (ev->type != ev_keydown) return false;

  if (castdeath) return true; /* already in dying frames */

  /* go into death frame */

  castdeath = true;
  caststate = &states[mobjinfo[g_castorder[castnum].type].deathstate];
  casttics = caststate->tics;
  castframes = 0;
  castattacking = false;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (mobjinfo[g_castorder[castnum].type].deathsound)
    {
      s_start_sound(NULL, mobjinfo[g_castorder[castnum].type].deathsound);
    }
#endif

  return true;
}

static void f_cast_print(const char *text)
{
  const char *ch;
  int c;
  int cx;
  int w;
  int width;

  /* find width */

  ch = text;
  width = 0;

  while (ch)
    {
      c = *ch++;
      if (!c) break;
      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        {
          width += 4;
          continue;
        }

      w = SHORT(hu_font[c]->width);
      width += w;
    }

  /* draw it */

  cx = SCREENWIDTH / 2 - width / 2;
  ch = text;
  while (ch)
    {
      c = *ch++;
      if (!c) break;
      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
        {
          cx += 4;
          continue;
        }

      w = SHORT(hu_font[c]->width);
      v_draw_patch(cx, 180, hu_font[c]);
      cx += w;
    }
}

static void f_cast_drawer(void)
{
  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  int lump;
  boolean flip;
  patch_t *patch;

  /* erase the entire screen to a background */

  v_draw_patch(0, 0, w_cache_lump_name(("BOSSBACK"), PU_CACHE));

  f_cast_print((g_castorder[castnum].name));

  /* draw the current frame in the middle of the screen */

  sprdef = &sprites[caststate->sprite];
  sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
  lump = sprframe->lump[0];
  flip = (boolean)sprframe->flip[0];

  patch = w_cache_lump_num(lump + firstspritelump, PU_CACHE);
  if (flip)
    v_draw_patch_flipped(SCREENWIDTH / 2, 170, patch);
  else
    v_draw_patch(SCREENWIDTH / 2, 170, patch);
}

static void f_draw_patch_col(int x, patch_t *patch, int col)
{
  column_t *column;
  byte *source;
  pixel_t *dest;
  pixel_t *desttop;
  int count;

  column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
  desttop = i_video_buffer + x;

  /* step through the posts in a column */

  while (column->topdelta != 0xff)
    {
      source = (byte *)column + 3;
      dest = desttop + column->topdelta * SCREENWIDTH;
      count = column->length;

      while (count--)
        {
          *dest = *source++;
          dest += SCREENWIDTH;
        }

      column = (column_t *)((byte *)column + column->length + 4);
    }
}

static void f_bunny_scroll(void)
{
  signed int scrolled;
  int x;
  patch_t *p1;
  patch_t *p2;
  char name[10];
  int stage;
  static int laststage;

  p1 = w_cache_lump_name(("PFUB2"), PU_LEVEL);
  p2 = w_cache_lump_name(("PFUB1"), PU_LEVEL);

  v_mark_rect(0, 0, SCREENWIDTH, SCREENHEIGHT);

  scrolled = (SCREENWIDTH - ((signed int)finalecount - 230) / 2);
  if (scrolled > SCREENWIDTH) scrolled = SCREENWIDTH;
  if (scrolled < 0) scrolled = 0;

  for (x = 0; x < SCREENWIDTH; x++)
    {
      if (x + scrolled < SCREENWIDTH)
        f_draw_patch_col(x, p1, x + scrolled);
      else
        f_draw_patch_col(x, p2, x + scrolled - SCREENWIDTH);
    }

  if (finalecount < 1130) return;
  if (finalecount < 1180)
    {
      v_draw_patch((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2,
                   w_cache_lump_name(("END0"), PU_CACHE));
      laststage = 0;
      return;
    }

  stage = (finalecount - 1180) / 5;
  if (stage > 6) stage = 6;
  if (stage > laststage)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(NULL, SFX_PISTOL);
#endif
      laststage = stage;
    }

  snprintf(name, 10, "END%i", stage);
  v_draw_patch((SCREENWIDTH - 13 * 8) / 2, (SCREENHEIGHT - 8 * 8) / 2,
               w_cache_lump_name(name, PU_CACHE));
}

static void f_art_screen_drawer(void)
{
  const char *lumpname;

  if (gameepisode == 3)
    {
      f_bunny_scroll();
    }
  else
    {
      switch (gameepisode)
        {
        case 1:
          if (gameversion >= exe_ultimate)
            {
              lumpname = "CREDIT";
            }
          else
            {
              lumpname = "HELP2";
            }
          break;
        case 2:
          lumpname = "VICTORY2";
          break;
        case 4:
          lumpname = "ENDPIC";
          break;
        default:
          return;
        }

      lumpname = (lumpname);

      v_draw_patch(0, 0, w_cache_lump_name(lumpname, PU_CACHE));
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* f_start_finale */

void f_start_finale(void)
{
  size_t i;

  gameaction = ga_nothing;
  gamestate = GS_FINALE;
  viewactive = false;
  automapactive = false;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  if (logical_gamemission == doom)
    {
      s_change_music(MUS_VICTOR, true);
    }
  else
    {
      s_change_music(MUS_READ_M, true);
    }
#endif

  /* Find the right screen and set the text and background */

  for (i = 0; i < arrlen(g_textscreens); ++i)
    {
      textscreen_t *screen = &g_textscreens[i];

      /* Hack for Chex Quest */

      if (gameversion == exe_chex && screen->mission == doom)
        {
          screen->level = 5;
        }

      if (logical_gamemission == screen->mission &&
          (logical_gamemission != doom || gameepisode == screen->episode) &&
          gamemap == screen->level)
        {
          finaletext = screen->text;
          finaleflat = screen->background;
        }
    }

  /* Do dehacked substitutions of strings */

  finaletext = (finaletext);
  finaleflat = (finaleflat);

  finalestage = F_STAGE_TEXT;
  finalecount = 0;
}

boolean f_responder(event_t *event)
{
  if (finalestage == F_STAGE_CAST) return f_cast_responder(event);

  return false;
}

void f_ticker(void)
{
  size_t i;

  /* check for skipping */

  if ((gamemode == commercial) && (finalecount > 50))
    {
      /* go on to the next level */

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (players[i].cmd.buttons) break;
        }

      if (i < MAXPLAYERS)
        {
          if (gamemap == 30)
            f_start_cast();
          else
            gameaction = ga_worlddone;
        }
    }

  /* advance animation */

  finalecount++;

  if (finalestage == F_STAGE_CAST)
    {
      f_cast_ticker();
      return;
    }

  if (gamemode == commercial) return;

  if (finalestage == F_STAGE_TEXT &&
      finalecount > strlen(finaletext) * TEXTSPEED + TEXTWAIT)
    {
      finalecount = 0;
      finalestage = F_STAGE_ARTSCREEN;
      wipegamestate = -1; /* force a wipe */
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (gameepisode == 3) s_start_music(MUS_BUNNY);
#endif
    }
}

void f_drawer(void)
{
  switch (finalestage)
    {
    case F_STAGE_CAST:
      f_cast_drawer();
      break;
    case F_STAGE_TEXT:
      f_text_write();
      break;
    case F_STAGE_ARTSCREEN:
      f_art_screen_drawer();
      break;
    }
}
