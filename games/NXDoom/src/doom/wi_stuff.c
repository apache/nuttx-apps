/****************************************************************************
 * apps/games/NXDoom/src/doom/wi_stuff.c
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
 *  Intermission screens.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "z_zone.h"

#include "m_misc.h"
#include "m_random.h"

#include "deh_main.h"
#include "i_swap.h"
#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#include "sounds.h"
#endif

#include "doomstat.h"

/* Needs access to LFB. */

#include "v_video.h"

#include "wi_stuff.h"

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/

/* Data needed to add patches to full screen intermission pics.
 * Patches are statistics messages, and animations.
 * Loads of by-pixel layout and placement, offsets etc.
 */

/* Different between registered DOOM (1994) and
 *  Ultimate DOOM - Final edition (retail, 1995?).
 * This is supposedly ignored for commercial
 *  release (aka DOOM II), which had 34 maps
 *  in one episode. So there.
 */

#define NUMEPISODES 4
#define NUMMAPS 9

/* in tics
 * U #define PAUSELEN    (TICRATE*2)
 * U #define SCORESTEP    100
 * U #define ANIMPERIOD    32
 * pixel distance from "(YOU)" to "PLAYER N"
 * U #define STARDIST    10
 * U #define WK 1
 */

/* GLOBAL LOCATIONS */

#define WI_TITLEY 2
#define WI_SPACINGY 33

/* SINGLE-PLAYER STUFF */

#define SP_STATSX 50
#define SP_STATSY 50

#define SP_TIMEX 16
#define SP_TIMEY (SCREENHEIGHT - 32)

/* NET GAME STUFF */

#define NG_STATSY 50
#define NG_STATSX (32 + SHORT(star->width) / 2 + 32 * !dofrags)

#define NG_SPACINGX 64

/* DEATHMATCH STUFF */

#define DM_MATRIXX 42
#define DM_MATRIXY 68

#define DM_SPACINGX 40

#define DM_TOTALSX 269

#define DM_KILLERSX 10
#define DM_KILLERSY 100
#define DM_VICTIMSX 5
#define DM_VICTIMSY 50

/* Animation locations for episode 0 (1).
 * Using patches saves a lot of space,
 *  as they replace 320x200 full screen frames.
 */

#define ANIM(type, period, nanims, x, y, nexttic)                            \
  {(type), (period), (nanims), {(x), (y)}, (nexttic), 0, {NULL, NULL, NULL}, \
   0,      0,        0,        0}

/* GENERAL DATA */

/* Locally used stuff. */

/* States for single-player */

#define SP_KILLS 0
#define SP_ITEMS 2
#define SP_SECRET 4
#define SP_FRAGS 6
#define SP_TIME 8
#define SP_PAR ST_TIME

#define SP_PAUSE 1

/* in seconds */

#define SHOWNEXTLOCDELAY 4

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  ANIM_ALWAYS,
  ANIM_RANDOM,
  ANIM_LEVEL
} animenum_t;

/* States for the intermission */

typedef enum
{
  NO_STATE = -1,
  STAT_COUNT,
  SHOW_NEXT_LOC,
} stateenum_t;

typedef struct
{
  int x;
  int y;
} point_t;

/* Animation.
 * There is another anim_t used in p_spec.
 */

typedef struct
{
  animenum_t type;

  /* period in tics between animations */

  int period;

  /* number of animation frames */

  int nanims;

  /* location of animation */

  point_t loc;

  /* ALWAYS: n/a,
   * RANDOM: period deviation (<256),
   * LEVEL: level
   */

  int data1;

  /* ALWAYS: n/a,
   * RANDOM: random base period,
   * LEVEL: n/a
   */

  int data2;

  /* actual graphics for frames of animations */

  patch_t *p[3];

  /* following must be initialized to zero before use! */

  /* next value of bcnt (used in conjunction with period) */

  int nexttic;

  /* last drawn animation frame */

  int lastdrawn;

  /* next frame number to animate */

  int ctr;

  /* used by RANDOM and LEVEL when animating */

  int state;
} anim_t;

typedef void (*load_callback_t)(const char *lumpname, patch_t **variable);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    /* Episode 0 World Map */

    {
        {185, 164}, /* location of level 0 (CJ) */
        {148, 143}, /* location of level 1 (CJ) */
        {69, 122},  /* location of level 2 (CJ) */
        {209, 102}, /* location of level 3 (CJ) */
        {116, 89},  /* location of level 4 (CJ) */
        {166, 55},  /* location of level 5 (CJ) */
        {71, 56},   /* location of level 6 (CJ) */
        {135, 29},  /* location of level 7 (CJ) */
        {71, 24},   /* location of level 8 (CJ) */
    },

    /* Episode 1 World Map should go here */

    {
        {254, 25},  /* location of level 0 (CJ) */
        {97, 50},   /* location of level 1 (CJ) */
        {188, 64},  /* location of level 2 (CJ) */
        {128, 78},  /* location of level 3 (CJ) */
        {214, 92},  /* location of level 4 (CJ) */
        {133, 130}, /* location of level 5 (CJ) */
        {208, 136}, /* location of level 6 (CJ) */
        {148, 140}, /* location of level 7 (CJ) */
        {235, 158}, /* location of level 8 (CJ) */
    },

    /* Episode 2 World Map should go here */

    {
        {156, 168}, /* location of level 0 (CJ) */
        {48, 154},  /* location of level 1 (CJ) */
        {174, 95},  /* location of level 2 (CJ) */
        {265, 75},  /* location of level 3 (CJ) */
        {130, 48},  /* location of level 4 (CJ) */
        {279, 23},  /* location of level 5 (CJ) */
        {198, 48},  /* location of level 6 (CJ) */
        {140, 25},  /* location of level 7 (CJ) */
        {281, 136}, /* location of level 8 (CJ) */
    },
};

static anim_t epsd0animinfo[] =
{
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 224, 104, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 184, 160, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 112, 136, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 72, 112, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 88, 96, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 64, 48, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 192, 40, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 136, 16, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 80, 16, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 64, 24, 0),
};

static anim_t epsd1animinfo[] =
{
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 1),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 2),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 3),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 4),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 5),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 6),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 7),
    ANIM(ANIM_LEVEL, TICRATE / 3, 3, 192, 144, 8),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 8),
};

static anim_t epsd2animinfo[] =
{
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104, 168, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 40, 136, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 160, 96, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104, 80, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 120, 32, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 4, 3, 40, 0, 0),
};

static int NUMANIMS[NUMEPISODES] =
{
    arrlen(epsd0animinfo),
    arrlen(epsd1animinfo),
    arrlen(epsd2animinfo),
};

static anim_t *anims[NUMEPISODES] =
{
    epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo,
};

/* used to accelerate or skip a stage */

static int acceleratestage;

/* wbs->pnum */

static int me;

/* specifies current state */

static stateenum_t state;

/* contains information passed into intermission */

static wbstartstruct_t *wbs;

static wbplayerstruct_t *plrs; /* wbs->plyr[] */

/* used for general timing */

static int cnt;

/* used for timing of background animation */

static int bcnt;

/* signals to refresh everything for one frame */

static int firstrefresh;

static int cnt_kills[MAXPLAYERS];
static int cnt_items[MAXPLAYERS];
static int cnt_secret[MAXPLAYERS];
static int cnt_time;
static int cnt_par;
static int cnt_pause;

/* # of commercial levels */

static int NUMCMAPS;

/* GRAPHICS */

/* You Are Here graphic */

static patch_t *yah[3] =
{
  NULL,
  NULL,
  NULL,
};

/* splat */

static patch_t *splat[2] =
{
  NULL,
  NULL,
};

/* %, : graphics */

static patch_t *percent;
static patch_t *colon;

/* 0-9 graphic */

static patch_t *num[10];

/* minus sign */

static patch_t *wiminus;

/* "Finished!" graphics */

static patch_t *finished;

/* "Entering" graphic */

static patch_t *entering;

/* "secret" */

static patch_t *sp_secret;

/* "Kills", "Scrt", "Items", "Frags" */

static patch_t *kills;
static patch_t *secret;
static patch_t *items;
static patch_t *frags;

/* Time sucks. */

static patch_t *timepatch;
static patch_t *par;
static patch_t *sucks;

/* "killers", "victims" */

static patch_t *killers;
static patch_t *victims;

/* "Total", your face, your dead face */

static patch_t *total;
static patch_t *star;
static patch_t *bstar;

/* "red P[1..MAXPLAYERS]" */

static patch_t *p[MAXPLAYERS];

/* "gray P[1..MAXPLAYERS]" */

static patch_t *bp[MAXPLAYERS];

/* Name graphics of each level (centered) */

static patch_t **lnames;

/* Buffer storing the backdrop */

static patch_t *background;

static int dm_state;
static int dm_frags[MAXPLAYERS][MAXPLAYERS];
static int dm_totals[MAXPLAYERS];

static int cnt_frags[MAXPLAYERS];
static int dofrags;
static int ng_state;

static int sp_state;

static boolean snl_pointeron = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Common load/unload function.  Iterates over all the graphics
 * lumps to be loaded/unloaded into memory.
 */

static void wi_load_unload_data(load_callback_t callback)
{
  int8_t i;
  int8_t j;
  char name[9];
  anim_t *a;

  if (gamemode == commercial)
    {
      for (i = 0; i < NUMCMAPS; i++)
        {
          snprintf(name, sizeof(name), "CWILV%2.2d", i);
          callback(name, &lnames[i]);
        }
    }
  else
    {
      for (i = 0; i < NUMMAPS; i++)
        {
          snprintf(name, sizeof(name), "WILV%d%d", wbs->epsd, i);
          callback(name, &lnames[i]);
        }

      /* you are here */

      callback(("WIURH0"), &yah[0]);

      /* you are here (alt.) */

      callback(("WIURH1"), &yah[1]);

      /* splat */

      callback(("WISPLAT"), &splat[0]);

      if (wbs->epsd < 3)
        {
          for (j = 0; j < NUMANIMS[wbs->epsd]; j++)
            {
              a = &anims[wbs->epsd][j];
              for (i = 0; i < a->nanims; i++)
                {
                  /* MONDO HACK! */

                  if (wbs->epsd != 1 || j != 8)
                    {
                      /* animations */

                      snprintf(name, sizeof(name), "WIA%d%.2d%.2d",
                               wbs->epsd, j, i);
                      callback(name, &a->p[i]);
                    }
                  else
                    {
                      /* HACK ALERT! */

                      a->p[i] = anims[1][4].p[i];
                    }
                }
            }
        }
    }

  /* More hacks on minus sign. */

  if (w_check_num_for_name(("WIMINUS")) > 0)
    callback(("WIMINUS"), &wiminus);
  else
    wiminus = NULL;

  for (i = 0; i < 10; i++)
    {
      /* numbers 0-9 */

      snprintf(name, sizeof(name), "WINUM%d", i);
      callback(name, &num[i]);
    }

  /* percent sign */

  callback(("WIPCNT"), &percent);

  /* "finished" */

  callback(("WIF"), &finished);

  /* "entering" */

  callback(("WIENTER"), &entering);

  /* "kills" */

  callback(("WIOSTK"), &kills);

  /* "scrt" */

  callback(("WIOSTS"), &secret);

  /* "secret" */

  callback(("WISCRT2"), &sp_secret);

  /* french wad uses WIOBJ (?) */

  if (w_check_num_for_name(("WIOBJ")) >= 0)
    {
      /* "items" */

      if (netgame && !deathmatch)
        callback(("WIOBJ"), &items);
      else
        callback(("WIOSTI"), &items);
    }
  else
    {
      callback(("WIOSTI"), &items);
    }

  /* "frgs" */

  callback(("WIFRGS"), &frags);

  /* ":" */

  callback(("WICOLON"), &colon);

  /* "time" */

  callback(("WITIME"), &timepatch);

  /* "sucks" */

  callback(("WISUCKS"), &sucks);

  /* "par" */

  callback(("WIPAR"), &par);

  /* "killers" (vertical) */

  callback(("WIKILRS"), &killers);

  /* "victims" (horiz) */

  callback(("WIVCTMS"), &victims);

  /* "total" */

  callback(("WIMSTT"), &total);

  for (i = 0; i < MAXPLAYERS; i++)
    {
      /* "1,2,3,4" */

      snprintf(name, sizeof(name), "STPB%d", i);
      callback(name, &p[i]);

      /* "1,2,3,4" */

      snprintf(name, sizeof(name), "WIBP%d", i + 1);
      callback(name, &bp[i]);
    }

  /* Background image */

  if (gamemode == commercial)
    {
      m_str_copy(name, ("INTERPIC"), sizeof(name));
    }
  else if (gameversion >= exe_ultimate && wbs->epsd == 3)
    {
      m_str_copy(name, ("INTERPIC"), sizeof(name));
    }
  else
    {
      snprintf(name, sizeof(name), "WIMAP%d", wbs->epsd);
    }

  /* Draw backdrop and save to a temporary buffer */

  callback(name, &background);
}

static void wi_load_callback(const char *name, patch_t **variable)
{
  *variable = w_cache_lump_name(name, PU_STATIC);
}

static void wi_unload_callback(const char *name, patch_t **variable)
{
  w_release_lump_name(name);
  *variable = NULL;
}

static void wi_load_data(void)
{
  if (gamemode == commercial)
    {
      NUMCMAPS = 32;
      lnames = (patch_t **)z_malloc(sizeof(patch_t *) * NUMCMAPS,
                                    PU_STATIC, NULL);
    }
  else
    {
      lnames =
          (patch_t **)z_malloc(sizeof(patch_t *) * NUMMAPS, PU_STATIC, NULL);
    }

  wi_load_unload_data(wi_load_callback);

  /* These two graphics are special cased because we're sharing them with the
   * status bar code
   */

  /* your face */

  star = w_cache_lump_name(("STFST01"), PU_STATIC);

  /* dead face */

  bstar = w_cache_lump_name(("STFDEAD0"), PU_STATIC);
}

static void wi_unload_data(void)
{
  wi_load_unload_data(wi_unload_callback);

  /* We do not free these lumps as they are shared with the status
   * bar code.
   */

  /* w_release_lump_name("STFST01");
   * w_release_lump_name("STFDEAD0");
   */
}

/* slam background */

static void wi_slam_background(void)
{
  v_draw_patch(0, 0, background);
}

/* Draws "<Levelname> Finished!" */

static void wi_draw_lf(void)
{
  int y = WI_TITLEY;

  if (gamemode != commercial || wbs->last < NUMCMAPS)
    {
      /* draw <LevelName> */

      v_draw_patch((SCREENWIDTH - SHORT(lnames[wbs->last]->width)) / 2, y,
                   lnames[wbs->last]);

      /* draw "Finished!" */

      y += (5 * SHORT(lnames[wbs->last]->height)) / 4;

      v_draw_patch((SCREENWIDTH - SHORT(finished->width)) / 2, y, finished);
    }
  else if (wbs->last == NUMCMAPS)
    {
      /* MAP33 - draw "Finished!" only */

      v_draw_patch((SCREENWIDTH - SHORT(finished->width)) / 2, y, finished);
    }
  else if (wbs->last > NUMCMAPS)
    {
      /* > MAP33.  Doom bombs out here with a Bad v_draw_patch error.
       * I'm pretty sure that doom2.exe is just reading into random
       * bits of memory at this point, but let's try to be accurate
       * anyway.  This deliberately triggers a v_draw_patch error.
       */

      patch_t tmp =
        {
          SCREENWIDTH,
          SCREENHEIGHT,
          1,
          1,
            {
              0, 0, 0, 0, 0, 0, 0, 0
            },
        };

      v_draw_patch(0, y, &tmp);
    }
}

/* Draws "Entering <LevelName>" */

static void wi_draw_e(void)
{
  int y = WI_TITLEY;

  /* draw "Entering" */

  v_draw_patch((SCREENWIDTH - SHORT(entering->width)) / 2, y, entering);

  /* draw level */

  y += (5 * SHORT(lnames[wbs->next]->height)) / 4;

  v_draw_patch((SCREENWIDTH - SHORT(lnames[wbs->next]->width)) / 2, y,
               lnames[wbs->next]);
}

static void wi_draw_on_lnode(int n, patch_t *c[])
{
  int i;
  int left;
  int top;
  int right;
  int bottom;
  boolean fits = false;

  i = 0;
  do
    {
      left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
      top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
      right = left + SHORT(c[i]->width);
      bottom = top + SHORT(c[i]->height);

      if (left >= 0 && right < SCREENWIDTH && top >= 0 &&
          bottom < SCREENHEIGHT)
        {
          fits = true;
        }
      else
        {
          i++;
        }
    }
  while (!fits && i != 2 && c[i] != NULL);

  if (fits && i < 2)
    {
      v_draw_patch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y, c[i]);
    }
  else
    {
      printf("Could not place patch on level %d", n + 1); /* DEBUG */
    }
}

static void wi_init_animate_back(void)
{
  int i;
  anim_t *a;

  if (gamemode == commercial) return;

  if (wbs->epsd > 2) return;

  for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
      a = &anims[wbs->epsd][i];

      /* init variables */

      a->ctr = -1;

      /* specify the next time to draw it */

      if (a->type == ANIM_ALWAYS)
        a->nexttic = bcnt + 1 + (m_random() % a->period);
      else if (a->type == ANIM_RANDOM)
        a->nexttic = bcnt + 1 + a->data2 + (m_random() % a->data1);
      else if (a->type == ANIM_LEVEL)
        a->nexttic = bcnt + 1;
    }
}

static void wi_update_animated_block(void)
{
  int i;
  anim_t *a;

  if (gamemode == commercial) return;

  if (wbs->epsd > 2) return;

  for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
      a = &anims[wbs->epsd][i];

      if (bcnt == a->nexttic)
        {
          switch (a->type)
            {
            case ANIM_ALWAYS:
              if (++a->ctr >= a->nanims) a->ctr = 0;
              a->nexttic = bcnt + a->period;
              break;

            case ANIM_RANDOM:
              a->ctr++;
              if (a->ctr == a->nanims)
                {
                  a->ctr = -1;
                  a->nexttic = bcnt + a->data2 + (m_random() % a->data1);
                }
              else
                a->nexttic = bcnt + a->period;
              break;

            case ANIM_LEVEL:

              /* gawd-awful hack for level anims */

              if (!(state == STAT_COUNT && i == 7) && wbs->next == a->data1)
                {
                  a->ctr++;
                  if (a->ctr == a->nanims) a->ctr--;
                  a->nexttic = bcnt + a->period;
                }
              break;
            }
        }
    }
}

static void wi_draw_animated_back(void)
{
  int i;
  anim_t *a;

  if (gamemode == commercial) return;

  if (wbs->epsd > 2) return;

  for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
      a = &anims[wbs->epsd][i];

      if (a->ctr >= 0) v_draw_patch(a->loc.x, a->loc.y, a->p[a->ctr]);
    }
}

/* Draws a number.
 * If digits > 0, then use that many digits minimum, otherwise only use as
 * many as necessary. Returns new x position.
 */

static int wi_draw_num(int x, int y, int n, int digits)
{
  int fontwidth = SHORT(num[0]->width);
  int neg;
  int temp;

  if (digits < 0)
    {
      if (!n)
        {
          /* make variable-length zeros 1 digit long */

          digits = 1;
        }
      else
        {
          /* figure out # of digits in # */

          digits = 0;
          temp = n;

          while (temp)
            {
              temp /= 10;
              digits++;
            }
        }
    }

  neg = n < 0;
  if (neg) n = -n;

  /* if non-number, do not draw it */

  if (n == 1994) return 0;

  /* draw the new number */

  while (digits--)
    {
      x -= fontwidth;
      v_draw_patch(x, y, num[n % 10]);
      n /= 10;
    }

  /* draw a minus sign if necessary */

  if (neg && wiminus) v_draw_patch(x -= 8, y, wiminus);

  return x;
}

static void wi_draw_percent(int x, int y, int p_p)
{
  if (p_p < 0) return;

  v_draw_patch(x, y, percent);
  wi_draw_num(x, y, p_p, -1);
}

/* Display level completion time and par, or "sucks" message if overflow.
 */

static void wi_draw_time(int x, int y, int t)
{
  int div;
  int n;

  if (t < 0) return;

  if (t <= 61 * 59)
    {
      div = 1;

      do
        {
          n = (t / div) % 60;
          x = wi_draw_num(x, y, n, 2) - SHORT(colon->width);
          div *= 60;

          /* draw */

          if (div == 60 || t / div) v_draw_patch(x, y, colon);
        }
      while (t / div);
    }
  else
    {
      /* "sucks" */

      v_draw_patch(x - SHORT(sucks->width), y, sucks);
    }
}

static void wi_init_nostate(void)
{
  state = NO_STATE;
  acceleratestage = 0;
  cnt = 10;
}

static void wi_update_no_state(void)
{
  wi_update_animated_block();

  if (!--cnt)
    {
      /* Don't call wi_end yet.  g_world_done doesn't immediately
       * change gamestate, so wi_drawer is still going to get
       * run until that happens.  If we do that after wi_end
       * (which unloads all the graphics), we're in trouble.
       * wi_end();
       */

      g_world_done();
    }
}

static void wi_init_show_next_loc(void)
{
  state = SHOW_NEXT_LOC;
  acceleratestage = 0;
  cnt = SHOWNEXTLOCDELAY * TICRATE;

  wi_init_animate_back();
}

static void wi_update_show_next_loc(void)
{
  wi_update_animated_block();

  if (!--cnt || acceleratestage)
    wi_init_nostate();
  else
    snl_pointeron = (cnt & 31) < 20;
}

static void wi_draw_show_next_loc(void)
{
  int i;
  int last;

  wi_slam_background();

  /* draw animated background */

  wi_draw_animated_back();

  if (gamemode != commercial)
    {
      if (wbs->epsd > 2)
        {
          wi_draw_e();
          return;
        }

      last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

      /* draw a splat on taken cities. */

      for (i = 0; i <= last; i++)
        wi_draw_on_lnode(i, splat);

      /* splat the secret level? */

      if (wbs->didsecret) wi_draw_on_lnode(8, splat);

      /* draw flashing ptr */

      if (snl_pointeron) wi_draw_on_lnode(wbs->next, yah);
    }

  /* draws which level you are entering.. */

  if ((gamemode != commercial) || wbs->next != 30) wi_draw_e();
}

static void wi_draw_no_state(void)
{
  snl_pointeron = true;
  wi_draw_show_next_loc();
}

static int wi_frag_sum(int playernum)
{
  int i;
  int l_frags = 0;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i] && i != playernum)
        {
          l_frags += plrs[playernum].frags[i];
        }
    }

  /* JDC hack - negative frags. */

  l_frags -= plrs[playernum].frags[playernum];

  /* UNUSED if (frags < 0)
   * frags = 0;
   */

  return l_frags;
}

static void wi_init_deathmatch_stats(void)
{
  int i;
  int j;

  state = STAT_COUNT;
  acceleratestage = 0;
  dm_state = 1;

  cnt_pause = TICRATE;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          for (j = 0; j < MAXPLAYERS; j++)
            {
              if (playeringame[j]) dm_frags[i][j] = 0;
            }

          dm_totals[i] = 0;
        }
    }

  wi_init_animate_back();
}

static void wi_update_deathmatch_stats(void)
{
  int i;
  int j;

  boolean stillticking;

  wi_update_animated_block();

  if (acceleratestage && dm_state != 4)
    {
      acceleratestage = 0;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (playeringame[i])
            {
              for (j = 0; j < MAXPLAYERS; j++)
                {
                  if (playeringame[j]) dm_frags[i][j] = plrs[i].frags[j];
                }

              dm_totals[i] = wi_frag_sum(i);
            }
        }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(0, SFX_BAREXP);
#endif
      dm_state = 4;
    }

  if (dm_state == 2)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      stillticking = false;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (playeringame[i])
            {
              for (j = 0; j < MAXPLAYERS; j++)
                {
                  if (playeringame[j] && dm_frags[i][j] != plrs[i].frags[j])
                    {
                      if (plrs[i].frags[j] < 0)
                        dm_frags[i][j]--;
                      else
                        dm_frags[i][j]++;

                      if (dm_frags[i][j] > 99) dm_frags[i][j] = 99;

                      if (dm_frags[i][j] < -99) dm_frags[i][j] = -99;

                      stillticking = true;
                    }
                }

              dm_totals[i] = wi_frag_sum(i);

              if (dm_totals[i] > 99) dm_totals[i] = 99;

              if (dm_totals[i] < -99) dm_totals[i] = -99;
            }
        }

      if (!stillticking)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          dm_state++;
        }
    }
  else if (dm_state == 4)
    {
      if (acceleratestage)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_SLOP);
#endif

          if (gamemode == commercial)
            wi_init_nostate();
          else
            wi_init_show_next_loc();
        }
    }
  else if (dm_state & 1)
    {
      if (!--cnt_pause)
        {
          dm_state++;
          cnt_pause = TICRATE;
        }
    }
}

static void wi_draw_deathmatch_stats(void)
{
  int i;
  int j;
  int x;
  int y;
  int w;

  wi_slam_background();

  /* draw animated background */

  wi_draw_animated_back();
  wi_draw_lf();

  /* draw stat titles (top line) */

  v_draw_patch(DM_TOTALSX - SHORT(total->width) / 2,
               DM_MATRIXY - WI_SPACINGY + 10, total);

  v_draw_patch(DM_KILLERSX, DM_KILLERSY, killers);
  v_draw_patch(DM_VICTIMSX, DM_VICTIMSY, victims);

  /* draw P? */

  x = DM_MATRIXX + DM_SPACINGX;
  y = DM_MATRIXY;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (playeringame[i])
        {
          v_draw_patch(x - SHORT(p[i]->width) / 2, DM_MATRIXY - WI_SPACINGY,
                       p[i]);

          v_draw_patch(DM_MATRIXX - SHORT(p[i]->width) / 2, y, p[i]);

          if (i == me)
            {
              v_draw_patch(x - SHORT(p[i]->width) / 2,
                           DM_MATRIXY - WI_SPACINGY, bstar);

              v_draw_patch(DM_MATRIXX - SHORT(p[i]->width) / 2, y, star);
            }
        }
      else
        {
          /* v_draw_patch(x-SHORT(bp[i]->width)/2,
           *   DM_MATRIXY - WI_SPACINGY, bp[i]);
           * v_draw_patch(DM_MATRIXX-SHORT(bp[i]->width)/2,
           *   y, bp[i]);
           */
        }

      x += DM_SPACINGX;
      y += WI_SPACINGY;
    }

  /* draw stats */

  y = DM_MATRIXY + 10;
  w = SHORT(num[0]->width);

  for (i = 0; i < MAXPLAYERS; i++)
    {
      x = DM_MATRIXX + DM_SPACINGX;

      if (playeringame[i])
        {
          for (j = 0; j < MAXPLAYERS; j++)
            {
              if (playeringame[j]) wi_draw_num(x + w, y, dm_frags[i][j], 2);

              x += DM_SPACINGX;
            }

          wi_draw_num(DM_TOTALSX + w, y, dm_totals[i], 2);
        }

      y += WI_SPACINGY;
    }
}

static void wi_init_netgame_stats(void)
{
  int i;

  state = STAT_COUNT;
  acceleratestage = 0;
  ng_state = 1;

  cnt_pause = TICRATE;

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (!playeringame[i]) continue;

      cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

      dofrags += wi_frag_sum(i);
    }

  dofrags = !!dofrags;

  wi_init_animate_back();
}

static void wi_update_netgame_stats(void)
{
  int i;
  int fsum;

  boolean stillticking;

  wi_update_animated_block();

  if (acceleratestage && ng_state != 10)
    {
      acceleratestage = 0;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (!playeringame[i]) continue;

          cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
          cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
          cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;

          if (dofrags) cnt_frags[i] = wi_frag_sum(i);
        }

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(0, SFX_BAREXP);
#endif
      ng_state = 10;
    }

  if (ng_state == 2)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      stillticking = false;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (!playeringame[i]) continue;

          cnt_kills[i] += 2;

          if (cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
            cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
          else
            stillticking = true;
        }

      if (!stillticking)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          ng_state++;
        }
    }
  else if (ng_state == 4)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      stillticking = false;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (!playeringame[i]) continue;

          cnt_items[i] += 2;
          if (cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
            cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
          else
            stillticking = true;
        }

      if (!stillticking)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          ng_state++;
        }
    }
  else if (ng_state == 6)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      stillticking = false;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (!playeringame[i]) continue;

          cnt_secret[i] += 2;

          if (cnt_secret[i] >= (plrs[i].ssecret * 100) / wbs->maxsecret)
            cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;
          else
            stillticking = true;
        }

      if (!stillticking)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          ng_state += 1 + 2 * !dofrags;
        }
    }
  else if (ng_state == 8)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      stillticking = false;

      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (!playeringame[i]) continue;

          cnt_frags[i] += 1;

          if (cnt_frags[i] >= (fsum = wi_frag_sum(i)))
            cnt_frags[i] = fsum;
          else
            stillticking = true;
        }

      if (!stillticking)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_PLDETH);
#endif
          ng_state++;
        }
    }
  else if (ng_state == 10)
    {
      if (acceleratestage)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_SGCOCK);
#endif
          if (gamemode == commercial)
            wi_init_nostate();
          else
            wi_init_show_next_loc();
        }
    }
  else if (ng_state & 1)
    {
      if (!--cnt_pause)
        {
          ng_state++;
          cnt_pause = TICRATE;
        }
    }
}

static void wi_init_stats(void)
{
  state = STAT_COUNT;
  acceleratestage = 0;
  sp_state = 1;
  cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
  cnt_time = cnt_par = -1;
  cnt_pause = TICRATE;

  wi_init_animate_back();
}

static void wi_update_stats(void)
{
  wi_update_animated_block();

  if (acceleratestage && sp_state != 10)
    {
      acceleratestage = 0;
      cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
      cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
      cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
      cnt_time = plrs[me].stime / TICRATE;
      cnt_par = wbs->partime / TICRATE;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      s_start_sound(0, SFX_BAREXP);
#endif
      sp_state = 10;
    }

  if (sp_state == 2)
    {
      cnt_kills[0] += 2;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
        {
          cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          sp_state++;
        }
    }
  else if (sp_state == 4)
    {
      cnt_items[0] += 2;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
        {
          cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          sp_state++;
        }
    }
  else if (sp_state == 6)
    {
      cnt_secret[0] += 2;

#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
        {
          cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_BAREXP);
#endif
          sp_state++;
        }
    }

  else if (sp_state == 8)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      if (!(bcnt & 3)) s_start_sound(0, SFX_PISTOL);
#endif

      cnt_time += 3;

      if (cnt_time >= plrs[me].stime / TICRATE)
        cnt_time = plrs[me].stime / TICRATE;

      cnt_par += 3;

      if (cnt_par >= wbs->partime / TICRATE)
        {
          cnt_par = wbs->partime / TICRATE;

          if (cnt_time >= plrs[me].stime / TICRATE)
            {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
              s_start_sound(0, SFX_BAREXP);
#endif
              sp_state++;
            }
        }
    }
  else if (sp_state == 10)
    {
      if (acceleratestage)
        {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
          s_start_sound(0, SFX_SGCOCK);
#endif

          if (gamemode == commercial)
            wi_init_nostate();
          else
            wi_init_show_next_loc();
        }
    }
  else if (sp_state & 1)
    {
      if (!--cnt_pause)
        {
          sp_state++;
          cnt_pause = TICRATE;
        }
    }
}

static void wi_draw_netgame_stats(void)
{
  int i;
  int x;
  int y;
  int pwidth = SHORT(percent->width);

  wi_slam_background();

  /* draw animated background */

  wi_draw_animated_back();

  wi_draw_lf();

  /* draw stat titles (top line) */

  v_draw_patch(NG_STATSX + NG_SPACINGX - SHORT(kills->width), NG_STATSY,
               kills);

  v_draw_patch(NG_STATSX + 2 * NG_SPACINGX - SHORT(items->width), NG_STATSY,
               items);

  v_draw_patch(NG_STATSX + 3 * NG_SPACINGX - SHORT(secret->width), NG_STATSY,
               secret);

  if (dofrags)
    {
      v_draw_patch(NG_STATSX + 4 * NG_SPACINGX - SHORT(frags->width),
                   NG_STATSY, frags);
    }

  /* draw stats */

  y = NG_STATSY + SHORT(kills->height);

  for (i = 0; i < MAXPLAYERS; i++)
    {
      if (!playeringame[i]) continue;

      x = NG_STATSX;
      v_draw_patch(x - SHORT(p[i]->width), y, p[i]);

      if (i == me) v_draw_patch(x - SHORT(p[i]->width), y, star);

      x += NG_SPACINGX;
      wi_draw_percent(x - pwidth, y + 10, cnt_kills[i]);
      x += NG_SPACINGX;
      wi_draw_percent(x - pwidth, y + 10, cnt_items[i]);
      x += NG_SPACINGX;
      wi_draw_percent(x - pwidth, y + 10, cnt_secret[i]);
      x += NG_SPACINGX;

      if (dofrags) wi_draw_num(x, y + 10, cnt_frags[i], -1);

      y += WI_SPACINGY;
    }
}

static void wi_draw_stats(void)
{
  int lh; /* line height */

  lh = (3 * SHORT(num[0]->height)) / 2;

  wi_slam_background();

  /* draw animated background */

  wi_draw_animated_back();

  wi_draw_lf();

  v_draw_patch(SP_STATSX, SP_STATSY, kills);
  wi_draw_percent(SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

  v_draw_patch(SP_STATSX, SP_STATSY + lh, items);
  wi_draw_percent(SCREENWIDTH - SP_STATSX, SP_STATSY + lh, cnt_items[0]);

  v_draw_patch(SP_STATSX, SP_STATSY + 2 * lh, sp_secret);
  wi_draw_percent(SCREENWIDTH - SP_STATSX, SP_STATSY + 2 * lh,
          cnt_secret[0]);

  v_draw_patch(SP_TIMEX, SP_TIMEY, timepatch);
  wi_draw_time(SCREENWIDTH / 2 - SP_TIMEX, SP_TIMEY, cnt_time);

  if (wbs->epsd < 3)
    {
      v_draw_patch(SCREENWIDTH / 2 + SP_TIMEX, SP_TIMEY, par);
      wi_draw_time(SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
    }
}

static void wi_check_for_accelerate(void)
{
  int i;
  player_t *player;

  /* check for button presses to skip delays */

  for (i = 0, player = players; i < MAXPLAYERS; i++, player++)
    {
      if (playeringame[i])
        {
          if (player->cmd.buttons & BT_ATTACK)
            {
              if (!player->attackdown) acceleratestage = 1;
              player->attackdown = true;
            }
          else
            player->attackdown = false;
          if (player->cmd.buttons & BT_USE)
            {
              if (!player->usedown) acceleratestage = 1;
              player->usedown = true;
            }
          else
            player->usedown = false;
        }
    }
}

static void wi_init_variables(wbstartstruct_t *wbstartstruct)
{
  wbs = wbstartstruct;

  acceleratestage = 0;
  cnt = bcnt = 0;
  firstrefresh = 1;
  me = wbs->pnum;
  plrs = wbs->plyr;

  if (!wbs->maxkills) wbs->maxkills = 1;

  if (!wbs->maxitems) wbs->maxitems = 1;

  if (!wbs->maxsecret) wbs->maxsecret = 1;

  if (gameversion < exe_ultimate)
    {
      if (wbs->epsd > 2) wbs->epsd -= 3;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void wi_end(void)
{
  wi_unload_data();
}

/* Updates stuff each tick */

void wi_ticker(void)
{
  /* counter for general background animation */

  bcnt++;

  if (bcnt == 1)
    {
#ifdef CONFIG_GAMES_NXDOOM_SOUND
      /* intermission music */

      if (gamemode == commercial)
        s_change_music(MUS_DM2INT, true);
      else
        s_change_music(MUS_INTER, true);
#endif
    }

  wi_check_for_accelerate();

  switch (state)
    {
    case STAT_COUNT:
      if (deathmatch)
        wi_update_deathmatch_stats();
      else if (netgame)
        wi_update_netgame_stats();
      else
        wi_update_stats();
      break;

    case SHOW_NEXT_LOC:
      wi_update_show_next_loc();
      break;

    case NO_STATE:
      wi_update_no_state();
      break;
    }
}

void wi_drawer(void)
{
  switch (state)
    {
    case STAT_COUNT:
      if (deathmatch)
        wi_draw_deathmatch_stats();
      else if (netgame)
        wi_draw_netgame_stats();
      else
        wi_draw_stats();
      break;

    case SHOW_NEXT_LOC:
      wi_draw_show_next_loc();
      break;

    case NO_STATE:
      wi_draw_no_state();
      break;
    }
}

void wi_start(wbstartstruct_t *wbstartstruct)
{
  wi_init_variables(wbstartstruct);
  wi_load_data();

  if (deathmatch)
    wi_init_deathmatch_stats();
  else if (netgame)
    wi_init_netgame_stats();
  else
    wi_init_stats();
}
