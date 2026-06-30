/****************************************************************************
 * apps/games/NXDoom/src/doom/p_setup.c
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
 *  Do all the WAD I/O, get map description, set up initial state and misc.
 * LUTs.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "z_zone.h"

#include "deh_main.h"
#include "i_swap.h"
#include "m_argv.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"
#include "p_rejectpad.h"

#ifdef CONFIG_GAMES_NXDOOM_SOUND
#include "s_sound.h"
#endif

#include "doomstat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Maintain single and multi player starting spots. */

#define MAX_DEATHMATCH_STARTS 10

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void p_spawn_map_thing(mapthing_t *mthing);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int totallines;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* MAP related Lookup tables.
 * Store VERTICES, LINEDEFS, SIDEDEFS, etc.
 */

int numvertices;
vertex_t *vertices;

int numsegs;
seg_t *segs;

int numsectors;
sector_t *sectors;

int numsubsectors;
subsector_t *subsectors;

int numnodes;
node_t *nodes;

int numlines;
line_t *lines;

int numsides;
side_t *sides;

/* BLOCKMAP
 * Created from axis aligned bounding box
 * of the map, a rectangular array of
 * blocks of size ...
 * Used to speed up collision detection
 * by spatial subdivision in 2D.
 */

/* Blockmap size. */

int bmapwidth;
int bmapheight;  /* size in mapblocks */
short *blockmap; /* int for larger maps */

/* offsets in blockmap are from here */

short *blockmaplump;

/* origin of block map */

fixed_t bmaporgx;
fixed_t bmaporgy;

/* for thing chains */

mobj_t **blocklinks;

/* REJECT
 * For fast sight rejection.
 * Speeds up enemy AI by skipping detailed LineOf Sight calculation.
 * Without special effect, this could be used as a PVS lookup as well.
 */

byte *rejectmatrix;

mapthing_t deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t *deathmatch_p;
mapthing_t playerstarts[MAXPLAYERS];
boolean playerstartsingame[MAXPLAYERS];

/* pointer to the current map lump info struct */

lumpinfo_t *maplumpinfo;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void p_load_vertices(int lump)
{
  byte *data;
  int i;
  mapvertex_t *ml;
  vertex_t *li;

  /* Determine number of lumps: total lump length / vertex record length. */

  numvertices = w_lump_length(lump) / sizeof(mapvertex_t);

  /* Allocate zone memory for buffer. */

  vertices = z_malloc(numvertices * sizeof(vertex_t), PU_LEVEL, 0);

  /* Load data into cache. */

  data = w_cache_lump_num(lump, PU_STATIC);

  ml = (mapvertex_t *)data;
  li = vertices;

  /* Copy and convert vertex coordinates, internal representation as fixed. */

  for (i = 0; i < numvertices; i++, li++, ml++)
    {
      li->x = SHORT(ml->x) << FRACBITS;
      li->y = SHORT(ml->y) << FRACBITS;
    }

  /* Free buffer memory. */

  w_release_lump_num(lump);
}

static sector_t *get_sector_at_null_address(void)
{
  static boolean null_sector_is_initialized = false;
  static sector_t null_sector;

  if (!null_sector_is_initialized)
    {
      memset(&null_sector, 0, sizeof(null_sector));
      i_get_memory_value(0, &null_sector.floorheight, 4);
      i_get_memory_value(4, &null_sector.ceilingheight, 4);
      null_sector_is_initialized = true;
    }

  return &null_sector;
}

static void p_load_segs(int lump)
{
  byte *data;
  int i;
  mapseg_t *ml;
  seg_t *li;
  line_t *ldef;
  int linedef_int;
  int side;
  int sidenum;

  numsegs = w_lump_length(lump) / sizeof(mapseg_t);
  segs = z_malloc(numsegs * sizeof(seg_t), PU_LEVEL, 0);
  memset(segs, 0, numsegs * sizeof(seg_t));
  data = w_cache_lump_num(lump, PU_STATIC);

  ml = (mapseg_t *)data;
  li = segs;
  for (i = 0; i < numsegs; i++, li++, ml++)
    {
      li->v1 = &vertices[SHORT(ml->v1)];
      li->v2 = &vertices[SHORT(ml->v2)];

      li->angle = (SHORT(ml->angle)) << FRACBITS;
      li->offset = (SHORT(ml->offset)) << FRACBITS;
      linedef_int = SHORT(ml->linedef);
      ldef = &lines[linedef_int];
      li->linedef = ldef;
      side = SHORT(ml->side);

      /* e6y: check for wrong indexes */

      if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
        {
          i_error("p_load_segs: linedef %d for seg %d references a "
                  "non-existent sidedef %d",
                  linedef_int, i, (unsigned)ldef->sidenum[side]);
        }

      li->sidedef = &sides[ldef->sidenum[side]];
      li->frontsector = sides[ldef->sidenum[side]].sector;

      if (ldef->flags & ML_TWOSIDED)
        {
          sidenum = ldef->sidenum[side ^ 1];

          /* If the sidenum is out of range, this may be a "glass hack"
           * impassable window. Point at side #0 (this may not be
           * the correct Vanilla behavior; however, it seems to work for
           * OTTAWAU.WAD, which is the one place I've seen this trick
           * used).
           */

          if (sidenum < 0 || sidenum >= numsides)
            {
              li->backsector = get_sector_at_null_address();
            }
          else
            {
              li->backsector = sides[sidenum].sector;
            }
        }
      else
        {
          li->backsector = 0;
        }
    }

  w_release_lump_num(lump);
}

static void p_load_subsectors(int lump)
{
  byte *data;
  int i;
  mapsubsector_t *ms;
  subsector_t *ss;

  numsubsectors = w_lump_length(lump) / sizeof(mapsubsector_t);
  subsectors = z_malloc(numsubsectors * sizeof(subsector_t), PU_LEVEL, 0);
  data = w_cache_lump_num(lump, PU_STATIC);

  ms = (mapsubsector_t *)data;
  memset(subsectors, 0, numsubsectors * sizeof(subsector_t));
  ss = subsectors;

  for (i = 0; i < numsubsectors; i++, ss++, ms++)
    {
      ss->numlines = SHORT(ms->numsegs);
      ss->firstline = SHORT(ms->firstseg);
    }

  w_release_lump_num(lump);
}

static void p_load_sectors(int lump)
{
  byte *data;
  int i;
  mapsector_t *ms;
  sector_t *ss;

  numsectors = w_lump_length(lump) / sizeof(mapsector_t);
  sectors = z_malloc(numsectors * sizeof(sector_t), PU_LEVEL, 0);
  memset(sectors, 0, numsectors * sizeof(sector_t));
  data = w_cache_lump_num(lump, PU_STATIC);

  ms = (mapsector_t *)data;
  ss = sectors;
  for (i = 0; i < numsectors; i++, ss++, ms++)
    {
      ss->floorheight = SHORT(ms->floorheight) << FRACBITS;
      ss->ceilingheight = SHORT(ms->ceilingheight) << FRACBITS;
      ss->floorpic = r_flat_num_for_name(ms->floorpic);
      ss->ceilingpic = r_flat_num_for_name(ms->ceilingpic);
      ss->lightlevel = SHORT(ms->lightlevel);
      ss->special = SHORT(ms->special);
      ss->tag = SHORT(ms->tag);
      ss->thinglist = NULL;
    }

  w_release_lump_num(lump);
}

static void p_load_nodes(int lump)
{
  byte *data;
  int i;
  int j;
  int k;
  mapnode_t *mn;
  node_t *no;

  numnodes = w_lump_length(lump) / sizeof(mapnode_t);
  nodes = z_malloc(numnodes * sizeof(node_t), PU_LEVEL, 0);
  data = w_cache_lump_num(lump, PU_STATIC);

  mn = (mapnode_t *)data;
  no = nodes;

  for (i = 0; i < numnodes; i++, no++, mn++)
    {
      no->x = SHORT(mn->x) << FRACBITS;
      no->y = SHORT(mn->y) << FRACBITS;
      no->dx = SHORT(mn->dx) << FRACBITS;
      no->dy = SHORT(mn->dy) << FRACBITS;
      for (j = 0; j < 2; j++)
        {
          no->children[j] = SHORT(mn->children[j]);
          for (k = 0; k < 4; k++)
            no->bbox[j][k] = SHORT(mn->bbox[j][k]) << FRACBITS;
        }
    }

  w_release_lump_num(lump);
}

static void p_load_things(int lump)
{
  byte *data;
  int i;
  mapthing_t *mt;
  mapthing_t spawnthing;
  int numthings;
  boolean spawn;

  data = w_cache_lump_num(lump, PU_STATIC);
  numthings = w_lump_length(lump) / sizeof(mapthing_t);

  mt = (mapthing_t *)data;
  for (i = 0; i < numthings; i++, mt++)
    {
      spawn = true;

      /* Do not spawn cool, new monsters if !commercial */

      if (gamemode != commercial)
        {
          switch (SHORT(mt->type))
            {
            case 68: /* Arachnotron */
            case 64: /* Archvile */
            case 88: /* Boss Brain */
            case 89: /* Boss Shooter */
            case 69: /* Hell Knight */
            case 67: /* Mancubus */
            case 71: /* Pain Elemental */
            case 65: /* Former Human Commando */
            case 66: /* Revenant */
            case 84: /* Wolf SS */
              spawn = false;
              break;
            }
        }

      if (spawn == false) break;

      /* Do spawn all other stuff. */

      spawnthing.x = SHORT(mt->x);
      spawnthing.y = SHORT(mt->y);
      spawnthing.angle = SHORT(mt->angle);
      spawnthing.type = SHORT(mt->type);
      spawnthing.options = SHORT(mt->options);

      p_spawn_map_thing(&spawnthing);
    }

  if (!deathmatch)
    {
      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (playeringame[i] && !playerstartsingame[i])
            {
              i_error("p_load_things: Player %d start missing (vanilla "
                      "crashes here)",
                      i + 1);
            }

          playerstartsingame[i] = false;
        }
    }

  w_release_lump_num(lump);
}

/* p_load_linedefs
 * Also counts secret lines for intermissions.
 */

static void p_load_linedefs(int lump)
{
  byte *data;
  int i;
  maplinedef_t *mld;
  line_t *ld;
  vertex_t *v1;
  vertex_t *v2;

  numlines = w_lump_length(lump) / sizeof(maplinedef_t);
  lines = z_malloc(numlines * sizeof(line_t), PU_LEVEL, 0);
  memset(lines, 0, numlines * sizeof(line_t));
  data = w_cache_lump_num(lump, PU_STATIC);

  mld = (maplinedef_t *)data;
  ld = lines;
  for (i = 0; i < numlines; i++, mld++, ld++)
    {
      ld->flags = SHORT(mld->flags);
      ld->special = SHORT(mld->special);
      ld->tag = SHORT(mld->tag);
      v1 = ld->v1 = &vertices[SHORT(mld->v1)];
      v2 = ld->v2 = &vertices[SHORT(mld->v2)];
      ld->dx = v2->x - v1->x;
      ld->dy = v2->y - v1->y;

      if (!ld->dx)
        ld->slopetype = ST_VERTICAL;
      else if (!ld->dy)
        ld->slopetype = ST_HORIZONTAL;
      else
        {
          if (fixed_div(ld->dy, ld->dx) > 0)
            ld->slopetype = ST_POSITIVE;
          else
            ld->slopetype = ST_NEGATIVE;
        }

      if (v1->x < v2->x)
        {
          ld->bbox[BOXLEFT] = v1->x;
          ld->bbox[BOXRIGHT] = v2->x;
        }
      else
        {
          ld->bbox[BOXLEFT] = v2->x;
          ld->bbox[BOXRIGHT] = v1->x;
        }

      if (v1->y < v2->y)
        {
          ld->bbox[BOXBOTTOM] = v1->y;
          ld->bbox[BOXTOP] = v2->y;
        }
      else
        {
          ld->bbox[BOXBOTTOM] = v2->y;
          ld->bbox[BOXTOP] = v1->y;
        }

      ld->sidenum[0] = SHORT(mld->sidenum[0]);
      ld->sidenum[1] = SHORT(mld->sidenum[1]);

      if (ld->sidenum[0] != -1)
        ld->frontsector = sides[ld->sidenum[0]].sector;
      else
        ld->frontsector = 0;

      if (ld->sidenum[1] != -1)
        ld->backsector = sides[ld->sidenum[1]].sector;
      else
        ld->backsector = 0;
    }

  w_release_lump_num(lump);
}

static void p_load_side_defs(int lump)
{
  byte *data;
  int i;
  mapsidedef_t *msd;
  side_t *sd;

  numsides = w_lump_length(lump) / sizeof(mapsidedef_t);
  sides = z_malloc(numsides * sizeof(side_t), PU_LEVEL, 0);
  memset(sides, 0, numsides * sizeof(side_t));
  data = w_cache_lump_num(lump, PU_STATIC);

  msd = (mapsidedef_t *)data;
  sd = sides;
  for (i = 0; i < numsides; i++, msd++, sd++)
    {
      sd->textureoffset = SHORT(msd->textureoffset) << FRACBITS;
      sd->rowoffset = SHORT(msd->rowoffset) << FRACBITS;
      sd->toptexture = r_texture_num_for_name(msd->toptexture);
      sd->bottomtexture = r_texture_num_for_name(msd->bottomtexture);
      sd->midtexture = r_texture_num_for_name(msd->midtexture);
      sd->sector = &sectors[SHORT(msd->sector)];
    }

  w_release_lump_num(lump);
}

static void p_load_block_map(int lump)
{
  int i;
  int count;
  int lumplen;

  lumplen = w_lump_length(lump);
  count = lumplen / 2;

  blockmaplump = z_malloc(lumplen, PU_LEVEL, NULL);
  w_read_lump(lump, blockmaplump);
  blockmap = blockmaplump + 4;

  /* Swap all short integers to native byte ordering. */

  for (i = 0; i < count; i++)
    {
      blockmaplump[i] = SHORT(blockmaplump[i]);
    }

  /* Read the header */

  bmaporgx = blockmaplump[0] << FRACBITS;
  bmaporgy = blockmaplump[1] << FRACBITS;
  bmapwidth = blockmaplump[2];
  bmapheight = blockmaplump[3];

  /* Clear out mobj chains */

  count = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = z_malloc(count, PU_LEVEL, 0);
  memset(blocklinks, 0, count);
}

/* p_group_lines
 * Builds sector line lists and subsector sector numbers.
 * Finds block bounding boxes for sectors.
 */

static void p_group_lines(void)
{
  line_t **linebuffer;
  int i;
  int j;
  line_t *li;
  sector_t *sector;
  subsector_t *ss;
  seg_t *seg;
  fixed_t bbox[4];
  int block;

  /* look up sector number for each subsector */

  ss = subsectors;
  for (i = 0; i < numsubsectors; i++, ss++)
    {
      seg = &segs[ss->firstline];
      ss->sector = seg->sidedef->sector;
    }

  /* count number of lines in each sector */

  li = lines;
  totallines = 0;
  for (i = 0; i < numlines; i++, li++)
    {
      totallines++;
      li->frontsector->linecount++;

      if (li->backsector && li->backsector != li->frontsector)
        {
          li->backsector->linecount++;
          totallines++;
        }
    }

  /* build line tables for each sector */

  linebuffer = z_malloc(totallines * sizeof(line_t *), PU_LEVEL, 0);

  for (i = 0; i < numsectors; ++i)
    {
      /* Assign the line buffer for this sector */

      sectors[i].lines = linebuffer;
      linebuffer += sectors[i].linecount;

      /* Reset linecount to zero so in the next stage we can count
       * lines into the list.
       */

      sectors[i].linecount = 0;
    }

  /* Assign lines to sectors */

  for (i = 0; i < numlines; ++i)
    {
      li = &lines[i];

      if (li->frontsector != NULL)
        {
          sector = li->frontsector;

          sector->lines[sector->linecount] = li;
          ++sector->linecount;
        }

      if (li->backsector != NULL && li->frontsector != li->backsector)
        {
          sector = li->backsector;

          sector->lines[sector->linecount] = li;
          ++sector->linecount;
        }
    }

  /* Generate bounding boxes for sectors */

  sector = sectors;
  for (i = 0; i < numsectors; i++, sector++)
    {
      m_clear_box(bbox);

      for (j = 0; j < sector->linecount; j++)
        {
          li = sector->lines[j];

          m_add_to_box(bbox, li->v1->x, li->v1->y);
          m_add_to_box(bbox, li->v2->x, li->v2->y);
        }

      /* set the degenmobj_t to the middle of the bounding box */

      sector->soundorg.x = (bbox[BOXRIGHT] + bbox[BOXLEFT]) / 2;
      sector->soundorg.y = (bbox[BOXTOP] + bbox[BOXBOTTOM]) / 2;

      /* adjust bounding box to map blocks */

      block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
      block = block >= bmapheight ? bmapheight - 1 : block;
      sector->blockbox[BOXTOP] = block;

      block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXBOTTOM] = block;

      block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
      block = block >= bmapwidth ? bmapwidth - 1 : block;
      sector->blockbox[BOXRIGHT] = block;

      block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
      block = block < 0 ? 0 : block;
      sector->blockbox[BOXLEFT] = block;
    }
}

static void p_load_reject(int lumpnum)
{
  int minlength;
  int lumplen;

  /* Calculate the size that the REJECT lump *should* be. */

  minlength = (numsectors * numsectors + 7) / 8;

  /* If the lump meets the minimum length, it can be loaded directly.
   * Otherwise, we need to allocate a buffer of the correct size
   * and pad it with appropriate data.
   */

  lumplen = w_lump_length(lumpnum);

  if (lumplen >= minlength)
    {
      rejectmatrix = w_cache_lump_num(lumpnum, PU_LEVEL);
    }
  else
    {
      rejectmatrix = z_malloc(minlength, PU_LEVEL, &rejectmatrix);
      w_read_lump(lumpnum, rejectmatrix);

      pad_reject_array(rejectmatrix + lumplen, minlength - lumplen,
                       totallines);
    }
}

/* p_setup_level */

void p_setup_level(int episode, int map, int playermask, skill_t skill)
{
  int i;
  char lumpname[9];
  int lumpnum;

  totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
  wminfo.partime = 180;
  for (i = 0; i < MAXPLAYERS; i++)
    {
      players[i].killcount = players[i].secretcount = players[i].itemcount =
          0;
    }

  /* Initial height of PointOfView will be set by player think. */

  players[consoleplayer].viewz = 1;

  /* Make sure all sounds are stopped before z_free_tags. */

#ifdef CONFIG_GAMES_NXDOOM_SOUND
  s_start();
#endif

  z_free_tags(PU_LEVEL, PU_PURGELEVEL - 1);

  /* UNUSED w_profile (); */

  p_init_thinkers();

  /* if working with a development map, reload it */

  w_reload();

  /* find map name */

  if (gamemode == commercial)
    {
      if (map < 10)
        snprintf(lumpname, 9, "map0%d", (int8_t)map);
      else
        snprintf(lumpname, 9, "map%d", (int8_t)map);
    }
  else
    {
      lumpname[0] = 'E';
      lumpname[1] = '0' + episode;
      lumpname[2] = 'M';
      lumpname[3] = '0' + map;
      lumpname[4] = 0;
    }

  lumpnum = w_get_num_for_name(lumpname);

  maplumpinfo = lumpinfo[lumpnum];

  leveltime = 0;

  /* note: most of this ordering is important */

  p_load_block_map(lumpnum + ML_BLOCKMAP);
  p_load_vertices(lumpnum + ML_VERTICES);
  p_load_sectors(lumpnum + ML_SECTORS);
  p_load_side_defs(lumpnum + ML_SIDEDEFS);

  p_load_linedefs(lumpnum + ML_LINEDEFS);
  p_load_subsectors(lumpnum + ML_SSECTORS);
  p_load_nodes(lumpnum + ML_NODES);
  p_load_segs(lumpnum + ML_SEGS);

  p_group_lines();
  p_load_reject(lumpnum + ML_REJECT);

  bodyqueslot = 0;
  deathmatch_p = deathmatchstarts;
  p_load_things(lumpnum + ML_THINGS);

  /* if deathmatch, randomly spawn the active players */

  if (deathmatch)
    {
      for (i = 0; i < MAXPLAYERS; i++)
        {
          if (playeringame[i])
            {
              players[i].mo = NULL;
              g_death_match_spawn_player(i);
            }
        }
    }

  /* clear special respawning que */

  iquehead = iquetail = 0;

  /* set up world state */

  p_spawn_specials();

  /* preload graphics */

  if (precache) r_precache_level();
}

void p_init(void)
{
  p_init_switch_list();
  p_init_pic_anims();
  r_init_sprites(sprnames);
}
