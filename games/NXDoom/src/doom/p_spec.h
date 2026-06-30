/****************************************************************************
 * apps/games/NXDoom/src/doom/p_spec.h
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
 *  Implements special effects:
 *  Texture animation, height or lighting changes according to adjacent
 *  sectors, respective utility functions, etc.
 *
 ****************************************************************************/

#ifndef __P_SPEC__
#define __P_SPEC__

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Define values for map objects */

#define MO_TELEPORTMAN 14

#define GLOWSPEED 8
#define STROBEBRIGHT 5
#define FASTDARK 15
#define SLOWDARK 35

/* max # of wall switches in a level */

#define MAXSWITCHES 50

/* 4 players, 4 buttons each at once, max. */

#define MAXBUTTONS 16

/* 1 second, in ticks. */

#define BUTTONTIME 35

#define PLATWAIT 3
#define PLATSPEED FRACUNIT
#define MAXPLATS 30

#define FLOORSPEED FRACUNIT

#define VDOORSPEED FRACUNIT * 2
#define VDOORWAIT 150

#define CEILSPEED FRACUNIT
#define CEILWAIT 150
#define MAXCEILINGS 30

#if 0 /* UNUSED */

/* how many frames of animation */

#define SNUMFRAMES 4

#define SDOORWAIT 35 * 3
#define SWAITTICS 4

/* how many diff. types of anims */

#define MAXSLIDEDOORS 5
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* P_LIGHTS */

typedef struct
{
  thinker_t thinker;
  sector_t *sector;
  int count;
  int maxlight;
  int minlight;
} fireflicker_t;

typedef struct
{
  thinker_t thinker;
  sector_t *sector;
  int count;
  int maxlight;
  int minlight;
  int maxtime;
  int mintime;
} lightflash_t;

typedef struct
{
  thinker_t thinker;
  sector_t *sector;
  int count;
  int minlight;
  int maxlight;
  int darktime;
  int brighttime;
} strobe_t;

typedef struct
{
  thinker_t thinker;
  sector_t *sector;
  int minlight;
  int maxlight;
  int direction;
} glow_t;

/* P_SWITCH */

typedef struct
{
  char name1[9];
  char name2[9];
  short episode;
} switchlist_t;

typedef enum
{
  top,
  middle,
  bottom
} bwhere_e;

typedef struct
{
  line_t *line;
  bwhere_e where;
  int btexture;
  int btimer;
  degenmobj_t *soundorg;
} button_t;

/* P_PLATS */

typedef enum
{
  up,
  down,
  waiting,
  in_stasis
} plat_e;

typedef enum
{
  PLAT_PERPETUALRAISE,
  PLAT_DOWNWAITUPSTAY,
  PLAT_RAISEANDCHANGE,
  PLAT_RAISETONEARESTANDCHANGE,
  PLAT_BLAZEDWUS
} plattype_e;

typedef struct
{
  thinker_t thinker;
  sector_t *sector;
  fixed_t speed;
  fixed_t low;
  fixed_t high;
  int wait;
  int count;
  plat_e status;
  plat_e oldstatus;
  boolean crush;
  int tag;
  plattype_e type;
} plat_t;

/* P_FLOOR */

typedef enum
{
  /* lower floor to highest surrounding floor */

  FLOOR_LOWERFLOOR,

  /* lower floor to lowest surrounding floor */

  FLOOR_LOWERFLOORTOLOWEST,

  /* lower floor to highest surrounding floor VERY FAST */

  FLOOR_TURBOLOWER,

  /* raise floor to lowest surrounding CEILING */

  FLOOR_RAISEFLOOR,

  /* raise floor to next highest surrounding floor */

  FLOOR_RAISEFLOORTONEAREST,

  /* raise floor to shortest height texture around it */

  FLOOR_RAISETOTEXTURE,

  /* lower floor to lowest surrounding floor and change floorpic */

  FLOOR_LOWERANDCHANGE,

  FLOOR_RAISEFLOOR24,
  FLOOR_RAISEFLOOR24ANDCHANGE,
  FLOOR_RAISEFLOORCRUSH,

  /* raise to next highest floor, turbo-speed */

  FLOOR_RAISEFLOORTURBO,
  FLOOR_DONUTRAISE,
  FLOOR_RAISEFLOOR512
} floor_e;

typedef enum
{
  STAIR_BUILD8, /* slowly build by 8 */
  STAIR_TURBO16 /* quickly build by 16 */
} stair_e;

typedef struct
{
  thinker_t thinker;
  floor_e type;
  boolean crush;
  sector_t *sector;
  int direction;
  int newspecial;
  short texture;
  fixed_t floordestheight;
  fixed_t speed;
} floormove_t;

typedef enum
{
  ok,
  crushed,
  pastdest
} result_e;

typedef enum
{
  VLD_NORMAL,
  VLD_CLOSE30THENOPEN,
  VLD_CLOSE,
  VLD_OPEN,
  VLD_RAISEIN5MINS,
  VLD_BLAZERAISE,
  VLD_BLAZEOPEN,
  VLD_BLAZECLOSE
} vldoor_e;

typedef struct
{
  thinker_t thinker;
  vldoor_e type;
  sector_t *sector;
  fixed_t topheight;
  fixed_t speed;

  /* 1 = up, 0 = waiting at top, -1 = down */

  int direction;

  /* tics to wait at the top */

  int topwait;

  /* (keep in case a door going down is reset)
   * when it reaches 0, start going down
   */

  int topcountdown;
} vldoor_t;

/* P_CEILNG */

typedef enum
{
  CEIL_LOWERTOFLOOR,
  CEIL_RAISETOHIGHEST,
  CEIL_LOWERANDCRUSH,
  CEIL_CRUSHANDRAISE,
  CEIL_FASTCRUSHANDRAISE,
  CEIL_SILENTCRUSHANDRAISE
} ceiling_e;

typedef struct
{
  thinker_t thinker;
  ceiling_e type;
  sector_t *sector;
  fixed_t bottomheight;
  fixed_t topheight;
  fixed_t speed;
  boolean crush;

  /* 1 = up, 0 = waiting, -1 = down */

  int direction;

  /* ID */

  int tag;
  int olddirection;
} ceiling_t;

#if 0 /* UNUSED */

/* Sliding doors... */

typedef enum
{
  sd_opening,
  sd_waiting,
  sd_closing
} sd_e;

typedef enum
{
  SDT_OPENONLY,
  SDT_CLOSEONLY,
  SDT_OPENANDCLOSE
} sdt_e;

typedef struct
{
  thinker_t thinker;
  sdt_e type;
  line_t *line;
  int frame;
  int which_door_index;
  int timer;
  sector_t *frontsector;
  sector_t *backsector;
  sd_e status;
} slidedoor_t;

typedef struct
{
  char front_frame1[9];
  char front_frame2[9];
  char front_frame3[9];
  char front_frame4[9];
  char back_frame1[9];
  char back_frame2[9];
  char back_frame3[9];
  char back_frame4[9];
} slidename_t;

typedef struct
{
  int front_frames[4];
  int back_frames[4];
} slideframe_t;
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* End-level timer (-TIMER option) */

extern button_t buttonlist[MAXBUTTONS];

extern plat_t *activeplats[MAXPLATS];

extern ceiling_t *activeceilings[MAXCEILINGS];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* at game start */

void p_init_pic_anims(void);

/* at map load */

void p_spawn_specials(void);

/* every tic */

void p_update_specials(void);

/* when needed */

boolean p_use_special_line(mobj_t *thing, line_t *line, int side);

void p_shoot_special_line(mobj_t *thing, line_t *line);

void p_cross_special_line(int linenum, int side, mobj_t *thing);

void p_player_in_special_sector(player_t *player);

int two_sided(int sector, int line);

sector_t *get_sector(int current_sector, int line, int side);

side_t *get_side(int current_sector, int line, int side);

fixed_t p_find_lowest_floor_surrounding(sector_t *sec);
fixed_t p_find_highest_floor_surrounding(sector_t *sec);

fixed_t p_find_next_highest_floor(sector_t *sec, int currentheight);

fixed_t p_find_lowest_ceiling_surrounding(sector_t *sec);
fixed_t p_find_heighest_ceiling_surrounding(sector_t *sec);

int p_find_sector_from_line_tag(line_t *line, int start);

int p_find_min_surrounding(sector_t *sector, int max);

sector_t *get_next_sector(line_t *line, sector_t *sec);

/* SPECIAL */

int ev_do_donut(line_t *line);

void p_spawn_fire_flicker(sector_t *sector);
void t_light_flash(lightflash_t *flash);
void p_spawn_light_flash(sector_t *sector);
void t_strobe_flash(strobe_t *flash);

void p_spawn_strobe_flash(sector_t *sector, int fast_or_slow, int in_sync);

void ev_start_light_strobing(line_t *line);
void ev_turn_tag_lights_off(line_t *line);

void ev_light_turn_on(line_t *line, int bright);

void t_glow(glow_t *g);
void p_spawn_glowing_light(sector_t *sector);

void p_change_switch_texture(line_t *line, int use_again);

void p_init_switch_list(void);

void t_plat_raise(plat_t *plat);

int ev_do_plat(line_t *line, plattype_e type, int amount);

void p_add_active_plat(plat_t *plat);
void p_remove_active_plat(plat_t *plat);
void ev_stop_plat(line_t *line);
void p_activate_in_stasis(int tag);

/* P_DOORS */

void ev_vertical_door(line_t *line, mobj_t *thing);

int ev_do_door(line_t *line, vldoor_e type);

int ev_do_locked_door(line_t *line, vldoor_e type, mobj_t *thing);

void t_vertical_door(vldoor_t *door);
void p_spawn_door_close_in30(sector_t *sec);

void p_spawn_door_raise_in_5min(sector_t *sec, int secnum);

int ev_do_ceiling(line_t *line, ceiling_e type);

void t_move_ceiling(ceiling_t *ceiling);
void p_add_active_ceiling(ceiling_t *c);
void p_remove_active_ceiling(ceiling_t *c);
int ev_ceiling_crush_stop(line_t *line);
void p_activate_in_stasis_ceiling(line_t *line);

result_e t_move_plane(sector_t *sector, fixed_t speed, fixed_t dest,
                     boolean crush, int floor_or_ceiling, int direction);

int ev_build_stairs(line_t *line, stair_e type);

int ev_do_floor(line_t *line, floor_e floortype);

void t_move_floor(floormove_t *floor);

/* P_TELEPT */

int ev_teleport(line_t *line, int side, mobj_t *thing);

#if 0 /* UNUSED */
void p_init_sliding_door_frames(void);

void ev_sliding_door(line_t *line, mobj_t *thing);
#endif

#endif /* __P_SPEC__ */
