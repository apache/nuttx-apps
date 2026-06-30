/****************************************************************************
 * apps/games/NXDoom/src/doom/r_data.c
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
 *  Preparation of data for rendering,
 *  generation of lookups, caching, retrieval by name.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "deh_main.h"
#include "i_swap.h"
#include "i_system.h"
#include "z_zone.h"

#include "w_wad.h"

#include "doomdef.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_local.h"

#include "doomstat.h"
#include "r_sky.h"

#include "r_data.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Graphics.
 * DOOM graphics for walls and sprites
 * is stored in vertical runs of opaque pixels (posts).
 * A column is composed of zero or more posts,
 * a patch or sprite is composed of zero or more columns.
 */

/* Texture definition.
 * Each texture is composed of one or more patches,
 * with patches being lumps stored in the WAD.
 * The lumps are referenced by number, and patched
 * into the rectangular texture space using origin
 * and possibly other attributes.
 */

begin_packed_struct struct mappatch_t
{
  short originx;
  short originy;
  short patch;
  short stepdir;
  short colormap;
} end_packed_struct;

typedef struct mappatch_t mappatch_t;

/* Texture definition.
 * A DOOM wall texture is a list of patches
 * which are to be combined in a predefined order.
 */

begin_packed_struct struct maptexture_t
{
  char name[8];
  int masked;
  short width;
  short height;
  int obsolete;
  short patchcount;
  mappatch_t patches[1];
} end_packed_struct;

typedef struct maptexture_t maptexture_t;

/* A single patch from a texture definition, basically a rectangular area
 * within the texture rectangle.
 */

typedef struct
{
  /* Block origin (always UL), which has already accounted for the internal
   * origin of the patch.
   */

  short originx;
  short originy;
  int patch;
} texpatch_t;

/* A maptexturedef_t describes a rectangular texture, which is composed of
 * one or more mappatch_t structures that arrange graphic patches.
 */

typedef struct texture_s texture_t;

struct texture_s
{
  /* Keep name for switch changing, etc. */

  char name[8];
  short width;
  short height;

  /* Index in textures list */

  int index;

  /* Next in hash table chain */

  texture_t *next;

  /* All the patches[patchcount] are drawn back to front into the cached
   * texture.
   */

  short patchcount;
  texpatch_t patches[1];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

int firstflat;
int lastflat;
int numflats;

int firstpatch;
int lastpatch;
int numpatches;

int firstspritelump;
int lastspritelump;
int numspritelumps;

int numtextures;
texture_t **textures;
texture_t **textures_hashtable;

int *texturewidthmask;

/* needed for texture pegging */

fixed_t *textureheight;
int *texturecompositesize;
short **texturecolumnlump;
unsigned short **texturecolumnofs;
byte **texturecomposite;

/* for global animation */

int *flattranslation;
int *texturetranslation;

/* needed for pre rendering */

fixed_t *spritewidth;
fixed_t *spriteoffset;
fixed_t *spritetopoffset;

lighttable_t *colormaps;

int flatmemory;
int texturememory;
int spritememory;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void generate_texture_hashtable(void)
{
  texture_t **rover;
  int i;
  int key;

  textures_hashtable =
      z_malloc(sizeof(texture_t *) * numtextures, PU_STATIC, 0);

  memset(textures_hashtable, 0, sizeof(texture_t *) * numtextures);

  /* Add all textures to hash table */

  for (i = 0; i < numtextures; ++i)
    {
      /* Store index */

      textures[i]->index = i;

      /* Vanilla Doom does a linear search of the textures array
       * and stops at the first entry it finds. If there are two
       * entries with the same name, the first one in the array
       * wins. The new entry must therefore be added at the end
       * of the hash chain, so that earlier entries win.
       */

      key = w_lump_name_hash(textures[i]->name) % numtextures;

      rover = &textures_hashtable[key];

      while (*rover != NULL)
        {
          rover = &(*rover)->next;
        }

      /* Hook into hash table */

      textures[i]->next = NULL;
      *rover = textures[i];
    }
}

/****************************************************************************
 * Name: r_generate_lookup
 ****************************************************************************/

static void r_generate_lookup(int texnum)
{
  texture_t *texture;
  byte *patchcount; /* patchcount[texture->width] */
  texpatch_t *patch;
  patch_t *realpatch;
  int x;
  int x1;
  int x2;
  int i;
  short *collump;
  unsigned short *colofs;

  texture = textures[texnum];

  /* Composited texture not created yet. */

  texturecomposite[texnum] = 0;

  texturecompositesize[texnum] = 0;
  collump = texturecolumnlump[texnum];
  colofs = texturecolumnofs[texnum];

  /* Now count the number of columns that are covered by more than one patch.
   *
   * Fill in the lump / offset, so columns with only a single patch are all
   * done.
   */

  patchcount = (byte *)z_malloc(texture->width, PU_STATIC, &patchcount);
  memset(patchcount, 0, texture->width);

  for (i = 0, patch = texture->patches;
       i < texture->patchcount;
       i++, patch++)
    {
      realpatch = w_cache_lump_num(patch->patch, PU_CACHE);
      x1 = patch->originx;
      x2 = x1 + SHORT(realpatch->width);

      if (x1 < 0)
        x = 0;
      else
        x = x1;

      if (x2 > texture->width) x2 = texture->width;
      for (; x < x2; x++)
        {
          patchcount[x]++;
          collump[x] = patch->patch;
          colofs[x] = LONG(realpatch->columnofs[x - x1]) + 3;
        }
    }

  for (x = 0; x < texture->width; x++)
    {
      if (!patchcount[x])
        {
          printf("r_generate_lookup: column without a patch (%s)\n",
                 texture->name);
          return;
        }

      if (patchcount[x] > 1)
        {
          /* Use the cached block. */

          collump[x] = -1;
          colofs[x] = texturecompositesize[texnum];

          if (texturecompositesize[texnum] > 0x10000 - texture->height)
            {
              i_error("r_generate_lookup: texture %i is >64k", texnum);
            }

          texturecompositesize[texnum] += texture->height;
        }
    }

  z_free(patchcount);
}

/* MAPTEXTURE_T CACHING
 *
 * When a texture is first needed, it counts the number of composite columns
 * required in the texture and allocates space for a column directory and any
 * new columns.
 *
 * The directory will simply point inside other patches if there is only one
 * patch in a given column, but any columns with multiple patches will have
 * new column_ts generated.
 */

/****************************************************************************
 * Name: r_init_textures
 *
 * Description:
 *  Initializes the texture list
 *   with the textures from the world map.
 *
 ****************************************************************************/

static void r_init_textures(void)
{
  maptexture_t *mtexture;
  texture_t *texture;
  mappatch_t *mpatch;
  texpatch_t *patch;

  int i;
  int j;

  int *maptex;
  int *maptex2;
  int *maptex1;

  char name[9];
  char *names;
  char *name_p;

  int *patchlookup;

  int nummappatches;
  int offset;
  int maxoff;
  int maxoff2;
  int numtextures1;
  int numtextures2;

  int *directory;

  int temp1;
  int temp2;
  int temp3;

  /* Load the patch names from pnames.lmp. */

  name[8] = 0;
  names = w_cache_lump_name(("PNAMES"), PU_STATIC);
  nummappatches = LONG(*((int *)names));
  name_p = names + 4;
  patchlookup =
      z_malloc(nummappatches * sizeof(*patchlookup), PU_STATIC, NULL);

  for (i = 0; i < nummappatches; i++)
    {
      m_str_copy(name, name_p + i * 8, sizeof(name));
      patchlookup[i] = w_check_num_for_name(name);
    }

  w_release_lump_name(("PNAMES"));

  /* Load the map texture definitions from textures.lmp.
   * The data is contained in one or two lumps,
   *  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
   */

  maptex = maptex1 = w_cache_lump_name(("TEXTURE1"), PU_STATIC);
  numtextures1 = LONG(*maptex);
  maxoff = w_lump_length(w_get_num_for_name(("TEXTURE1")));
  directory = maptex + 1;

  if (w_check_num_for_name(("TEXTURE2")) != -1)
    {
      maptex2 = w_cache_lump_name(("TEXTURE2"), PU_STATIC);
      numtextures2 = LONG(*maptex2);
      maxoff2 = w_lump_length(w_get_num_for_name(("TEXTURE2")));
    }
  else
    {
      maptex2 = NULL;
      numtextures2 = 0;
      maxoff2 = 0;
    }

  numtextures = numtextures1 + numtextures2;

  textures = z_malloc(numtextures * sizeof(*textures), PU_STATIC, 0);
  texturecolumnlump =
      z_malloc(numtextures * sizeof(*texturecolumnlump), PU_STATIC, 0);
  texturecolumnofs =
      z_malloc(numtextures * sizeof(*texturecolumnofs), PU_STATIC, 0);
  texturecomposite =
      z_malloc(numtextures * sizeof(*texturecomposite), PU_STATIC, 0);
  texturecompositesize =
      z_malloc(numtextures * sizeof(*texturecompositesize), PU_STATIC, 0);
  texturewidthmask =
      z_malloc(numtextures * sizeof(*texturewidthmask), PU_STATIC, 0);
  textureheight =
      z_malloc(numtextures * sizeof(*textureheight), PU_STATIC, 0);

  /* Really complex printing shit... */

  temp1 = w_get_num_for_name(("S_START")); /* P_??????? */
  temp2 = w_get_num_for_name(("S_END")) - 1;
  temp3 = ((temp2 - temp1 + 63) / 64) + ((numtextures + 63) / 64);

  /* If stdout is a real console, use the classic vanilla "filling
   * up the box" effect, which uses backspace to "step back" inside
   * the box.  If stdout is a file, don't draw the box.
   */

  if (i_console_stdout())
    {
      printf("[");
      for (i = 0; i < temp3 + 9; i++)
        printf(" ");
      printf("]");
      for (i = 0; i < temp3 + 10; i++)
        printf("\b");
    }

  for (i = 0; i < numtextures; i++, directory++)
    {
      if (!(i & 63)) printf(".");

      if (i == numtextures1)
        {
          /* Start looking in second texture file. */

          maptex = maptex2;
          maxoff = maxoff2;
          directory = maptex + 1;
        }

      offset = LONG(*directory);

      if (offset > maxoff) i_error("r_init_textures: bad texture directory");

      mtexture = (maptexture_t *)((byte *)maptex + offset);

      texture = textures[i] =
          z_malloc(sizeof(texture_t) +
                   sizeof(texpatch_t) * (SHORT(mtexture->patchcount) - 1),
                   PU_STATIC, 0);

      texture->width = SHORT(mtexture->width);
      texture->height = SHORT(mtexture->height);
      texture->patchcount = SHORT(mtexture->patchcount);

      memcpy(texture->name, mtexture->name, sizeof(texture->name));
      mpatch = &mtexture->patches[0];
      patch = &texture->patches[0];

      for (j = 0; j < texture->patchcount; j++, mpatch++, patch++)
        {
          patch->originx = SHORT(mpatch->originx);
          patch->originy = SHORT(mpatch->originy);
          patch->patch = patchlookup[SHORT(mpatch->patch)];
          if (patch->patch == -1)
            {
              i_error("r_init_textures: Missing patch in texture %s",
                      texture->name);
            }
        }

      texturecolumnlump[i] = z_malloc(
          texture->width * sizeof(**texturecolumnlump), PU_STATIC, 0);
      texturecolumnofs[i] =
          z_malloc(texture->width * sizeof(**texturecolumnofs),
                  PU_STATIC, 0);

      j = 1;
      while (j * 2 <= texture->width)
        j <<= 1;

      texturewidthmask[i] = j - 1;
      textureheight[i] = texture->height << FRACBITS;
    }

  z_free(patchlookup);

  w_release_lump_name(("TEXTURE1"));
  if (maptex2) w_release_lump_name(("TEXTURE2"));

  /* Precalculate whatever possible. */

  for (i = 0; i < numtextures; i++)
    r_generate_lookup(i);

  /* Create translation table for global animation. */

  texturetranslation =
      z_malloc((numtextures + 1) * sizeof(*texturetranslation),
              PU_STATIC, 0);

  for (i = 0; i < numtextures; i++)
    texturetranslation[i] = i;

  generate_texture_hashtable();
}

/****************************************************************************
 * Name: r_init_flats
 ****************************************************************************/

static void r_init_flats(void)
{
  int i;

  firstflat = w_get_num_for_name(("F_START")) + 1;
  lastflat = w_get_num_for_name(("F_END")) - 1;
  numflats = lastflat - firstflat + 1;

  /* Create translation table for global animation. */

  flattranslation =
      z_malloc((numflats + 1) * sizeof(*flattranslation), PU_STATIC, 0);

  for (i = 0; i < numflats; i++)
    flattranslation[i] = i;
}

/****************************************************************************
 * Name: r_init_sprite_lumps
 *
 * Description:
 *  Finds the width and hoffset of all sprites in the wad,
 *   so the sprite does not need to be cached completely
 *   just for having the header info ready during rendering.
 *
 ****************************************************************************/

static void r_init_sprite_lumps(void)
{
  int i;
  patch_t *patch;

  firstspritelump = w_get_num_for_name(("S_START")) + 1;
  lastspritelump = w_get_num_for_name(("S_END")) - 1;

  numspritelumps = lastspritelump - firstspritelump + 1;
  spritewidth =
      z_malloc(numspritelumps * sizeof(*spritewidth), PU_STATIC, 0);
  spriteoffset =
      z_malloc(numspritelumps * sizeof(*spriteoffset), PU_STATIC, 0);
  spritetopoffset =
      z_malloc(numspritelumps * sizeof(*spritetopoffset), PU_STATIC, 0);

  for (i = 0; i < numspritelumps; i++)
    {
      if (!(i & 63)) printf(".");

      patch = w_cache_lump_num(firstspritelump + i, PU_CACHE);
      spritewidth[i] = SHORT(patch->width) << FRACBITS;
      spriteoffset[i] = SHORT(patch->leftoffset) << FRACBITS;
      spritetopoffset[i] = SHORT(patch->topoffset) << FRACBITS;
    }
}

/****************************************************************************
 * Name: r_init_colourmaps
 ****************************************************************************/

static void r_init_colourmaps(void)
{
  int lump;

  /* Load in the light tables, 256 byte align tables. */

  lump = w_get_num_for_name(("COLORMAP"));
  colormaps = w_cache_lump_num(lump, PU_STATIC);
}

/****************************************************************************
 * Name: r_draw_column_in_cache
 *
 * Description:
 *  Clip and draw a column from a patch into a cached post.
 *
 ****************************************************************************/

static void r_draw_column_in_cache(column_t *patch, byte *cache, int originy,
                         int cacheheight)
{
  int count;
  int position;
  byte *source;

  while (patch->topdelta != 0xff)
    {
      source = (byte *)patch + 3;
      count = patch->length;
      position = originy + patch->topdelta;

      if (position < 0)
        {
          count += position;
          position = 0;
        }

      if (position + count > cacheheight) count = cacheheight - position;

      if (count > 0) memcpy(cache + position, source, count);

      patch = (column_t *)((byte *)patch + patch->length + 4);
    }
}

/****************************************************************************
 * Name: r_generate_composite
 *
 * Description:
 *  Using the texture definition, the composite texture is created from the
 *  patches, and each column is cached.
 *
 ****************************************************************************/

static void r_generate_composite(int texnum)
{
  byte *block;
  texture_t *texture;
  texpatch_t *patch;
  patch_t *realpatch;
  int x;
  int x1;
  int x2;
  int i;
  column_t *patchcol;
  short *collump;
  unsigned short *colofs;

  texture = textures[texnum];

  block = z_malloc(texturecompositesize[texnum], PU_STATIC,
                   &texturecomposite[texnum]);

  collump = texturecolumnlump[texnum];
  colofs = texturecolumnofs[texnum];

  /* Composite the columns together. */

  for (i = 0, patch = texture->patches;
       i < texture->patchcount;
       i++, patch++)
    {
      realpatch = w_cache_lump_num(patch->patch, PU_CACHE);
      x1 = patch->originx;
      x2 = x1 + SHORT(realpatch->width);

      if (x1 < 0)
        x = 0;
      else
        x = x1;

      if (x2 > texture->width) x2 = texture->width;

      for (; x < x2; x++)
        {
          /* Column does not have multiple patches? */

          if (collump[x] >= 0) continue;

          patchcol = (column_t *)((byte *)realpatch +
                                  LONG(realpatch->columnofs[x - x1]));
          r_draw_column_in_cache(patchcol, block + colofs[x],
                  patch->originy, texture->height);
        }
    }

  /* Now that the texture has been built in column cache,
   * it is purgeable from zone memory.
   */

  z_change_tag(block, PU_CACHE);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: r_get_column
 ****************************************************************************/

byte *r_get_column(int tex, int col)
{
  int lump;
  int ofs;

  col &= texturewidthmask[tex];
  lump = texturecolumnlump[tex][col];
  ofs = texturecolumnofs[tex][col];

  if (lump > 0) return (byte *)w_cache_lump_num(lump, PU_CACHE) + ofs;

  if (!texturecomposite[tex]) r_generate_composite(tex);

  return texturecomposite[tex] + ofs;
}

/****************************************************************************
 * Name: r_init_data
 *
 * Description:
 *  Locates all the lumps that will be used by all views
 *  Must be called after W_Init.
 *
 ****************************************************************************/

void r_init_data(void)
{
  r_init_textures();
  printf(".");
  r_init_flats();
  printf(".");
  r_init_sprite_lumps();
  printf(".");
  r_init_colourmaps();
}

/****************************************************************************
 * Name: r_flat_num_for_name
 *
 * Description:
 *  Retrieval, get a flat number for a flat name.
 *
 ****************************************************************************/

int r_flat_num_for_name(const char *name)
{
  int i;
  char namet[9];

  i = w_check_num_for_name(name);

  if (i == -1)
    {
      namet[8] = 0;
      memcpy(namet, name, 8);
      i_error("r_flat_num_for_name: %s not found", namet);
    }

  return i - firstflat;
}

/****************************************************************************
 * Name: r_check_texture_num_for_name
 *
 * Description:
 *  Check whether texture is available.
 *  Filter out NoTexture indicator.
 *
 ****************************************************************************/

int r_check_texture_num_for_name(const char *name)
{
  texture_t *texture;
  int key;

  /* "NoTexture" marker. */

  if (name[0] == '-') return 0;

  key = w_lump_name_hash(name) % numtextures;

  texture = textures_hashtable[key];

  while (texture != NULL)
    {
      if (!strncasecmp(texture->name, name, 8)) return texture->index;

      texture = texture->next;
    }

  return -1;
}

/****************************************************************************
 * Name: r_texture_num_for_name
 *
 * Description:
 *  Calls r_check_texture_num_for_name, aborts with error message.
 *
 ****************************************************************************/

int r_texture_num_for_name(const char *name)
{
  int i;

  i = r_check_texture_num_for_name(name);

  if (i == -1)
    {
      i_error("r_texture_num_for_name: %s not found", name);
    }

  return i;
}

/****************************************************************************
 * Name: r_precache_level
 *
 * Description:
 *  Preloads all relevant graphics for the level.
 *
 ****************************************************************************/

void r_precache_level(void)
{
  char *flatpresent;
  char *texturepresent;
  char *spritepresent;

  int i;
  int j;
  int k;
  int lump;

  texture_t *texture;
  thinker_t *th;
  spriteframe_t *sf;

  if (demoplayback) return;

  /* Precache flats. */

  flatpresent = z_malloc(numflats, PU_STATIC, NULL);
  memset(flatpresent, 0, numflats);

  for (i = 0; i < numsectors; i++)
    {
      flatpresent[sectors[i].floorpic] = 1;
      flatpresent[sectors[i].ceilingpic] = 1;
    }

  flatmemory = 0;

  for (i = 0; i < numflats; i++)
    {
      if (flatpresent[i])
        {
          lump = firstflat + i;
          flatmemory += lumpinfo[lump]->size;
          w_cache_lump_num(lump, PU_CACHE);
        }
    }

  z_free(flatpresent);

  /* Precache textures. */

  texturepresent = z_malloc(numtextures, PU_STATIC, NULL);
  memset(texturepresent, 0, numtextures);

  for (i = 0; i < numsides; i++)
    {
      texturepresent[sides[i].toptexture] = 1;
      texturepresent[sides[i].midtexture] = 1;
      texturepresent[sides[i].bottomtexture] = 1;
    }

  /* Sky texture is always present.
   * Note that F_SKY1 is the name used to indicate a sky floor/ceiling as a
   * flat, while the sky texture is stored like a wall texture, with an
   * episode dependent name.
   */

  texturepresent[skytexture] = 1;

  texturememory = 0;
  for (i = 0; i < numtextures; i++)
    {
      if (!texturepresent[i]) continue;

      texture = textures[i];

      for (j = 0; j < texture->patchcount; j++)
        {
          lump = texture->patches[j].patch;
          texturememory += lumpinfo[lump]->size;
          w_cache_lump_num(lump, PU_CACHE);
        }
    }

  z_free(texturepresent);

  /* Precache sprites. */

  spritepresent = z_malloc(numsprites, PU_STATIC, NULL);
  memset(spritepresent, 0, numsprites);

  for (th = thinkercap.next; th != &thinkercap; th = th->next)
    {
      if (th->function.acp1 == (actionf_p1)p_mobj_thinker)
        spritepresent[((mobj_t *)th)->sprite] = 1;
    }

  spritememory = 0;
  for (i = 0; i < numsprites; i++)
    {
      if (!spritepresent[i]) continue;

      for (j = 0; j < sprites[i].numframes; j++)
        {
          sf = &sprites[i].spriteframes[j];
          for (k = 0; k < 8; k++)
            {
              lump = firstspritelump + sf->lump[k];
              spritememory += lumpinfo[lump]->size;
              w_cache_lump_num(lump, PU_CACHE);
            }
        }
    }

  z_free(spritepresent);
}
