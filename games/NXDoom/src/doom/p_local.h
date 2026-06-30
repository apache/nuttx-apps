/****************************************************************************
 * apps/games/NXDoom/src/doom/p_local.h
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
 *  Play functions, animation, global header.
 *
 ****************************************************************************/

#ifndef __P_LOCAL__
#define __P_LOCAL__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifndef __R_LOCAL__
#include "r_local.h"
#endif

/* P_SPEC */

#include "p_spec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define FLOATSPEED (FRACUNIT * 4)

#define MAXHEALTH 100
#define VIEWHEIGHT (41 * FRACUNIT)

/* mapblocks are used to check movement against lines and things */

#define MAPBLOCKUNITS 128
#define MAPBLOCKSIZE (MAPBLOCKUNITS * FRACUNIT)
#define MAPBLOCKSHIFT (FRACBITS + 7)
#define MAPBMASK (MAPBLOCKSIZE - 1)
#define MAPBTOFRAC (MAPBLOCKSHIFT - FRACBITS)

/* player radius for movement checking */

#define PLAYERRADIUS 16 * FRACUNIT

/* MAXRADIUS is for precalculated sector block boxes the spider demon is
 * larger, but we do not have any moving sectors nearby
 */

#define MAXRADIUS 32 * FRACUNIT

#define GRAVITY FRACUNIT
#define MAXMOVE (30 * FRACUNIT)

#define USERANGE (64 * FRACUNIT)
#define MELEERANGE (64 * FRACUNIT)
#define MISSILERANGE (32 * 64 * FRACUNIT)

/* follow a player exclusively for 3 seconds */

#define BASETHRESHOLD 100

#define ONFLOORZ INT_MIN
#define ONCEILINGZ INT_MAX

/* Time interval for item respawning. */

#define ITEMQUESIZE 128

/* Extended MAXINTERCEPTS, to allow for intercepts overrun emulation. */

#define MAXINTERCEPTS_ORIGINAL 128
#define MAXINTERCEPTS (MAXINTERCEPTS_ORIGINAL + 61)

#define PT_ADDLINES 1
#define PT_ADDTHINGS 2
#define PT_EARLYOUT 4

/* fraggle: I have increased the size of this buffer.  In the original Doom,
 * overrunning past this limit caused other bits of memory to be overwritten,
 * affecting demo playback.  However, in doing so, the limit was still
 * exceeded.  So we have to support more than 8 specials.
 *
 * We keep the original limit, to detect what variables in memory were
 * overwritten (see SpechitOverrun())
 */

#define MAXSPECIALCROSS 20
#define MAXSPECIALCROSS_ORIGINAL 8

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* P_MAPUTL */

typedef struct
{
  fixed_t x;
  fixed_t y;
  fixed_t dx;
  fixed_t dy;
} divline_t;

typedef struct
{
  fixed_t frac; /* along trace line */
  boolean isaline;
  union
  {
    mobj_t *thing;
    line_t *line;
  } d;
} intercept_t;

typedef boolean (*traverser_t)(intercept_t *in);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* P_TICK */

/* both the head and tail of the thinker list */

extern thinker_t thinkercap;

extern mapthing_t itemrespawnque[ITEMQUESIZE];
extern int itemrespawntime[ITEMQUESIZE];
extern int iquehead;
extern int iquetail;

extern intercept_t intercepts[MAXINTERCEPTS];
extern intercept_t *intercept_p;

extern fixed_t opentop;
extern fixed_t openbottom;
extern fixed_t openrange;
extern fixed_t lowfloor;

extern divline_t trace;

/* If "floatok" true, move would be ok if within "tmfloorz - tmceilingz". */

extern boolean floatok;
extern fixed_t tmfloorz;
extern fixed_t tmceilingz;

extern line_t *ceilingline;

extern line_t *spechit[MAXSPECIALCROSS];
extern int numspechit;

extern mobj_t *linetarget; /* who got hit (or NULL) */

extern fixed_t attackrange;

/* slopes to top and bottom of target */

extern fixed_t topslope;
extern fixed_t bottomslope;

extern byte *rejectmatrix;  /* for fast sight rejection */
extern short *blockmaplump; /* offsets in blockmap are from here */
extern short *blockmap;
extern int bmapwidth;
extern int bmapheight; /* in mapblocks */
extern fixed_t bmaporgx;
extern fixed_t bmaporgy;    /* origin of block map */
extern mobj_t **blocklinks; /* for thing chains */

extern int maxammo[NUMAMMO];
extern int clipammo[NUMAMMO];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void p_init_thinkers(void);
void p_add_thinker(thinker_t *thinker);
void p_remove_thinker(thinker_t *thinker);

/* P_PSPR */

void p_setup_psprites(player_t *curplayer);
void p_move_psprites(player_t *curplayer);
void p_drop_weapon(player_t *player);

/* P_USER */

void p_player_think(player_t *player);

/* P_MOBJ */

void p_respawn_specials(void);

mobj_t *p_spawn_mobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);

void p_remove_mobj(mobj_t *th);
mobj_t *p_subst_null_mobj(mobj_t *th);
boolean p_set_mobj_state(mobj_t *mobj, statenum_t state);
void p_mobj_thinker(mobj_t *mobj);

void p_spawn_puff(fixed_t x, fixed_t y, fixed_t z);
void p_spawn_blood(fixed_t x, fixed_t y, fixed_t z, int damage);
mobj_t *p_spawn_missile(mobj_t *source, mobj_t *dest, mobjtype_t type);
void p_spawn_player_missile(mobj_t *source, mobjtype_t type);

/* P_ENEMY */

void p_noise_alert(mobj_t *target, mobj_t *emmiter);

fixed_t p_approx_distance(fixed_t dx, fixed_t dy);
int p_point_on_line_side(fixed_t x, fixed_t y, line_t *line);
int p_point_on_divline_side(fixed_t x, fixed_t y, divline_t *line);
int p_box_on_line_side(fixed_t *tmbox, line_t *ld);

void p_line_opening(line_t *linedef);

boolean p_block_lines_iterator(int x, int y, boolean (*func)(line_t *));
boolean p_block_things_iterator(int x, int y, boolean (*func)(mobj_t *));

boolean p_path_traverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
                        int flags, boolean (*trav)(intercept_t *));

void p_unset_thing_position(mobj_t *thing);
void p_set_thing_position(mobj_t *thing);

/* P_MAP */

boolean p_check_position(mobj_t *thing, fixed_t x, fixed_t y);
boolean p_try_move(mobj_t *thing, fixed_t x, fixed_t y);
boolean p_teleport_move(mobj_t *thing, fixed_t x, fixed_t y);
void p_slide_move(mobj_t *mo);
boolean p_check_sight(mobj_t *t1, mobj_t *t2);
void p_use_lines(player_t *player);

boolean p_change_sector(sector_t *sector, boolean crunch);

fixed_t p_aim_line_attack(mobj_t *t1, angle_t angle, fixed_t distance);

void p_line_attack(mobj_t *t1, angle_t angle, fixed_t distance,
        fixed_t slope, int damage);

void p_radius_attack(mobj_t *spot, mobj_t *source, int damage);

/* P_INTER */

void p_touch_special_thing(mobj_t *special, mobj_t *toucher);

void p_damage_mobj(mobj_t *target, mobj_t *inflictor, mobj_t *source,
                   int damage);

#endif /* __P_LOCAL__ */
