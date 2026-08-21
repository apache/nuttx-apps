/****************************************************************************
 * apps/games/NXDoom/src/doom/r_things.c
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
 *  Refresh of things, i.e. objects represented by sprites.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "deh_main.h"
#include "doomdef.h"

#include "i_swap.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_local.h"

#include "doomstat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MINZ (FRACUNIT * 4)
#define BASEYCENTER (SCREENHEIGHT / 2)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  int x1;
  int x2;

  int column;
  int topclip;
  int bottomclip;
} maskdraw_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Sprite rotation 0 is facing the viewer, rotation 1 is one angle turn
 * CLOCKWISE around the axis.
 *
 * This is not the same as the angle, which increases counter clockwise
 * (protractor). There was a lot of stuff grabbed wrong, so I changed it...
 */

fixed_t pspritescale;
fixed_t pspriteiscale;

lighttable_t **spritelights;

/* constant arrays used for psprite clipping and initializing clipping */

short negonearray[SCREENWIDTH];
short screenheightarray[SCREENWIDTH];

/* INITIALIZATION FUNCTIONS */

/* variables used to look up and range check thing_t sprites patches */

spritedef_t *sprites;
int numsprites;

spriteframe_t sprtemp[29];
int maxframe;
const char *spritename;

#ifdef CONFIG_GAMES_NXDOOM_HEAP_BUFFERS
vissprite_t *vissprites;
#else
vissprite_t vissprites[CONFIG_GAMES_NXDOOM_MAXVISSPRITES];
#endif
vissprite_t *vissprite_p;
int newvissprite;

vissprite_t overflowsprite;

short *mfloorclip;
short *mceilingclip;

fixed_t spryscale;
fixed_t sprtopscreen;

vissprite_t vsprsortedhead;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* R_InstallSpriteLump
 * Local function for r_init_sprites.
 */

static void r_install_sprite_lump(int lump, unsigned frame,
        unsigned rotation, boolean flipped)
{
  int r;

  if (frame >= 29 || rotation > 8)
    i_error("R_InstallSpriteLump: "
            "Bad frame characters in lump %i",
            lump);

  if ((int)frame > maxframe) maxframe = frame;

  if (rotation == 0)
    {
      /* the lump should be used for all rotations */

      if (sprtemp[frame].rotate == false)
        i_error("r_init_sprites: Sprite %s frame %c has "
                "multip rot=0 lump",
                spritename, 'A' + frame);

      if (sprtemp[frame].rotate == true)
        i_error("r_init_sprites: Sprite %s frame %c has rotations "
                "and a rot=0 lump",
                spritename, 'A' + frame);

      sprtemp[frame].rotate = false;
      for (r = 0; r < 8; r++)
        {
          sprtemp[frame].lump[r] = lump - firstspritelump;
          sprtemp[frame].flip[r] = (byte)flipped;
        }

      return;
    }

  /* the lump is only used for one rotation */

  if (sprtemp[frame].rotate == false)
    i_error("r_init_sprites: Sprite %s frame %c has rotations "
            "and a rot=0 lump",
            spritename, 'A' + frame);

  sprtemp[frame].rotate = true;

  /* make 0 based */

  rotation--;
  if (sprtemp[frame].lump[rotation] != -1)
    i_error("r_init_sprites: Sprite %s : %c : %c "
            "has two lumps mapped to it",
            spritename, 'A' + frame, '1' + rotation);

  sprtemp[frame].lump[rotation] = lump - firstspritelump;
  sprtemp[frame].flip[rotation] = (byte)flipped;
}

/* r_init_sprite_defs
 *
 * Pass a null terminated list of sprite names (4 chars exactly) to be used.
 *
 * Builds the sprite rotation matrixes to account for horizontally flipped
 * sprites.
 *
 * Will report an error if the lumps are inconsistent. Only called at
 * startup.
 *
 * Sprite lump names are 4 characters for the actor, a letter for the frame,
 * and a number for the rotation.
 *
 * A sprite that is flippable will have an additional letter/number appended.
 *
 * The rotation character can be 0 to signify no rotations.
 */

static void r_init_sprite_defs(const char **namelist)
{
  const char **check;
  int i;
  int l;
  int frame;
  int rotation;
  int start;
  int end;
  int patched;

  /* count the number of sprite names */

  check = namelist;
  while (*check != NULL)
    check++;

  numsprites = check - namelist;

  if (!numsprites) return;

  sprites = z_malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);

  start = firstspritelump - 1;
  end = lastspritelump + 1;

  /* scan all the lump names for each of the names, noting the highest frame
   * letter. Just compare 4 characters as ints
   */

  for (i = 0; i < numsprites; i++)
    {
      spritename = (namelist[i]);
      memset(sprtemp, -1, sizeof(sprtemp));

      maxframe = -1;

      /* scan the lumps, filling in the frames for whatever is found
       */

      for (l = start + 1; l < end; l++)
        {
          if (!strncasecmp(lumpinfo[l]->name, spritename, 4))
            {
              frame = lumpinfo[l]->name[4] - 'A';
              rotation = lumpinfo[l]->name[5] - '0';

              if (modifiedgame)
                patched = w_get_num_for_name(lumpinfo[l]->name);
              else
                patched = l;

              r_install_sprite_lump(patched, frame, rotation, false);

              if (lumpinfo[l]->name[6])
                {
                  frame = lumpinfo[l]->name[6] - 'A';
                  rotation = lumpinfo[l]->name[7] - '0';
                  r_install_sprite_lump(l, frame, rotation, true);
                }
            }
        }

      /* check the frames that were found for completeness */

      if (maxframe == -1)
        {
          sprites[i].numframes = 0;
          continue;
        }

      maxframe++;

      for (frame = 0; frame < maxframe; frame++)
        {
          switch ((int)sprtemp[frame].rotate)
            {
            case -1:

              /* no rotations were found for that frame at all */

              i_error("r_init_sprites: No patches found "
                      "for %s frame %c",
                      spritename, frame + 'A');
              break;

            case 0:
              break; /* only the first rotation is needed */

            case 1:

              /* must have all 8 frames */

              for (rotation = 0; rotation < 8; rotation++)
                if (sprtemp[frame].lump[rotation] == -1)
                  i_error("r_init_sprites: Sprite %s frame %c "
                          "is missing rotations",
                          spritename, frame + 'A');
              break;
            }
        }

      /* allocate space for the frames present and copy sprtemp to it */

      sprites[i].numframes = maxframe;
      sprites[i].spriteframes =
          z_malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
      memcpy(sprites[i].spriteframes, sprtemp,
             maxframe * sizeof(spriteframe_t));
    }
}

static vissprite_t *r_new_vis_sprite(void)
{
  if (vissprite_p == &vissprites[CONFIG_GAMES_NXDOOM_MAXVISSPRITES])
    {
      return &overflowsprite;
    }

  vissprite_p++;
  return vissprite_p - 1;
}

/* R_DrawVisSprite
 *  mfloorclip and mceilingclip should also be set.
 */

static void r_draw_vis_sprite(vissprite_t *vis, int x1, int x2)
{
  column_t *column;
  int texturecolumn;
  fixed_t frac;
  patch_t *patch;

  patch = w_cache_lump_num(vis->patch + firstspritelump, PU_CACHE);

  dc_colormap = vis->colormap;

  if (!dc_colormap)
    {
      /* NULL colormap = shadow draw */

      colfunc = fuzzcolfunc;
    }
  else if (vis->mobjflags & MF_TRANSLATION)
    {
      colfunc = transcolfunc;
      dc_translation =
          translationtables - 256 +
          ((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));
    }

  dc_iscale = abs(vis->xiscale) >> detailshift;
  dc_texturemid = vis->texturemid;
  frac = vis->startfrac;
  spryscale = vis->scale;
  sprtopscreen = centeryfrac - fixed_mul(dc_texturemid, spryscale);

  for (dc_x = vis->x1; dc_x <= vis->x2; dc_x++, frac += vis->xiscale)
    {
      texturecolumn = frac >> FRACBITS;

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
      if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
        {
          i_error("R_DrawSpriteRange: bad texturecolumn");
        }

#endif

      column = (column_t *)((byte *)patch +
               LONG(patch->columnofs[texturecolumn]));
      r_draw_masked_column(column);
    }

  colfunc = basecolfunc;
}

/* R_ProjectSprite
 * Generates a vissprite for a thing if it might be visible.
 */

static void r_project_sprite(mobj_t *thing)
{
  fixed_t tr_x;
  fixed_t tr_y;

  fixed_t gxt;
  fixed_t gyt;

  fixed_t tx;
  fixed_t tz;

  fixed_t xscale;

  int x1;
  int x2;

  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  int lump;

  unsigned rot;
  boolean flip;

  int index;

  vissprite_t *vis;

  angle_t ang;
  fixed_t iscale;

  /* transform the origin point */

  tr_x = thing->x - viewx;
  tr_y = thing->y - viewy;

  gxt = fixed_mul(tr_x, viewcos);
  gyt = -fixed_mul(tr_y, viewsin);

  tz = gxt - gyt;

  /* thing is behind view plane? */

  if (tz < MINZ) return;

  xscale = fixed_div(projection, tz);

  gxt = -fixed_mul(tr_x, viewsin);
  gyt = fixed_mul(tr_y, viewcos);
  tx = -(gyt + gxt);

  /* too far off the side? */

  if (abs(tx) > (tz << 2)) return;

  /* decide which patch to use for sprite relative to player */

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if ((unsigned int)thing->sprite >= (unsigned int)numsprites)
    i_error("R_ProjectSprite: invalid sprite number %i ", thing->sprite);
#endif
  sprdef = &sprites[thing->sprite];
#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
    i_error("R_ProjectSprite: invalid sprite frame %i : %i ", thing->sprite,
            thing->frame);
#endif
  sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
    {
      /* choose a different rotation based on player view */

      ang = r_point_to_angle(thing->x, thing->y);
      rot = (ang - thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
      lump = sprframe->lump[rot];
      flip = (boolean)sprframe->flip[rot];
    }
  else
    {
      /* use single rotation for all views */

      lump = sprframe->lump[0];
      flip = (boolean)sprframe->flip[0];
    }

  /* calculate edges of the shape */

  tx -= spriteoffset[lump];
  x1 = (centerxfrac + fixed_mul(tx, xscale)) >> FRACBITS;

  /* off the right side? */

  if (x1 > viewwidth) return;

  tx += spritewidth[lump];
  x2 = ((centerxfrac + fixed_mul(tx, xscale)) >> FRACBITS) - 1;

  /* off the left side */

  if (x2 < 0) return;

  /* store information in a vissprite */

  vis = r_new_vis_sprite();
  vis->mobjflags = thing->flags;
  vis->scale = xscale << detailshift;
  vis->gx = thing->x;
  vis->gy = thing->y;
  vis->gz = thing->z;
  vis->gzt = thing->z + spritetopoffset[lump];
  vis->texturemid = vis->gzt - viewz;
  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
  iscale = fixed_div(FRACUNIT, xscale);

  if (flip)
    {
      vis->startfrac = spritewidth[lump] - 1;
      vis->xiscale = -iscale;
    }
  else
    {
      vis->startfrac = 0;
      vis->xiscale = iscale;
    }

  if (vis->x1 > x1) vis->startfrac += vis->xiscale * (vis->x1 - x1);
  vis->patch = lump;

  /* get light level */

  if (thing->flags & MF_SHADOW)
    {
      /* shadow draw */

      vis->colormap = NULL;
    }
  else if (fixedcolormap)
    {
      /* fixed map */

      vis->colormap = fixedcolormap;
    }
  else if (thing->frame & FF_FULLBRIGHT)
    {
      /* full bright */

      vis->colormap = colormaps;
    }

  else
    {
      /* diminished light */

      index = xscale >> (LIGHTSCALESHIFT - detailshift);

      if (index >= MAXLIGHTSCALE) index = MAXLIGHTSCALE - 1;

      vis->colormap = spritelights[index];
    }
}

/* R_DrawPSprite */

static void r_draw_psprite(pspdef_t *psp)
{
  fixed_t tx;
  int x1;
  int x2;
  spritedef_t *sprdef;
  spriteframe_t *sprframe;
  int lump;
  boolean flip;
  vissprite_t *vis;
  vissprite_t avis;

  /* decide which patch to use */

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if ((unsigned)psp->state->sprite >= (unsigned int)numsprites)
    {
      i_error("R_ProjectSprite: invalid sprite number %i ",
              psp->state->sprite);
    }

#endif

  sprdef = &sprites[psp->state->sprite];

#ifdef CONFIG_GAMES_NXDOOM_RANGECHECK
  if ((psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
    {
      i_error("R_ProjectSprite: invalid sprite frame %i : %i ",
              psp->state->sprite, psp->state->frame);
    }

#endif
  sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];

  lump = sprframe->lump[0];
  flip = (boolean)sprframe->flip[0];

  /* calculate edges of the shape */

  tx = psp->sx - (SCREENWIDTH / 2) * FRACUNIT;

  tx -= spriteoffset[lump];
  x1 = (centerxfrac + fixed_mul(tx, pspritescale)) >> FRACBITS;

  /* off the right side */

  if (x1 > viewwidth) return;

  tx += spritewidth[lump];
  x2 = ((centerxfrac + fixed_mul(tx, pspritescale)) >> FRACBITS) - 1;

  /* off the left side */

  if (x2 < 0) return;

  /* store information in a vissprite */

  vis = &avis;
  vis->mobjflags = 0;
  vis->texturemid = (BASEYCENTER << FRACBITS) + FRACUNIT / 2 -
                    (psp->sy - spritetopoffset[lump]);
  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
  vis->scale = pspritescale << detailshift;

  if (flip)
    {
      vis->xiscale = -pspriteiscale;
      vis->startfrac = spritewidth[lump] - 1;
    }
  else
    {
      vis->xiscale = pspriteiscale;
      vis->startfrac = 0;
    }

  if (vis->x1 > x1) vis->startfrac += vis->xiscale * (vis->x1 - x1);

  vis->patch = lump;

  if (viewplayer->powers[pw_invisibility] > 4 * 32 ||
      viewplayer->powers[pw_invisibility] & 8)
    {
      vis->colormap = NULL; /* shadow draw */
    }
  else if (fixedcolormap)
    {
      vis->colormap = fixedcolormap; /* fixed color */
    }
  else if (psp->state->frame & FF_FULLBRIGHT)
    {
      vis->colormap = colormaps; /* full bright */
    }
  else
    {
      vis->colormap = spritelights[MAXLIGHTSCALE - 1]; /* local light */
    }

  r_draw_vis_sprite(vis, vis->x1, vis->x2);
}

static void r_draw_player_sprites(void)
{
  int i;
  int lightnum;
  pspdef_t *psp;

  /* get light level */

  lightnum =
      (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) +
      extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS - 1];
  else
    spritelights = scalelight[lightnum];

  /* clip to screen bounds */

  mfloorclip = screenheightarray;
  mceilingclip = negonearray;

  /* add all active psprites */

  for (i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
    {
      if (psp->state) r_draw_psprite(psp);
    }
}

static void r_sort_vis_sprites(void)
{
  int i;
  int count;
  vissprite_t *ds;
  vissprite_t *best;
  static vissprite_t unsorted;
  fixed_t bestscale;

  count = vissprite_p - vissprites;

  unsorted.next = unsorted.prev = &unsorted;

  if (!count) return;

  for (ds = vissprites; ds < vissprite_p; ds++)
    {
      ds->next = ds + 1;
      ds->prev = ds - 1;
    }

  vissprites[0].prev = &unsorted;
  unsorted.next = &vissprites[0];
  (vissprite_p - 1)->next = &unsorted;
  unsorted.prev = vissprite_p - 1;

  /* pull the vissprites out by scale */

  vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
  for (i = 0; i < count; i++)
    {
      bestscale = INT_MAX;
      best = unsorted.next;

      for (ds = unsorted.next; ds != &unsorted; ds = ds->next)
        {
          if (ds->scale < bestscale)
            {
              bestscale = ds->scale;
              best = ds;
            }
        }

      best->next->prev = best->prev;
      best->prev->next = best->next;
      best->next = &vsprsortedhead;
      best->prev = vsprsortedhead.prev;
      vsprsortedhead.prev->next = best;
      vsprsortedhead.prev = best;
    }
}

static void r_draw_sprite(vissprite_t *spr)
{
  drawseg_t *ds;
  short clipbot[SCREENWIDTH];
  short cliptop[SCREENWIDTH];
  int x;
  int r1;
  int r2;
  fixed_t scale;
  fixed_t lowscale;
  int silhouette;

  for (x = spr->x1; x <= spr->x2; x++)
    clipbot[x] = cliptop[x] = -2;

  /* Scan drawsegs from end to start for obscuring segs.
   * The first drawseg that has a greater scale is the clip seg.
   */

  for (ds = ds_p - 1; ds >= drawsegs; ds--)
    {
      /* determine if the drawseg obscures the sprite */

      if (ds->x1 > spr->x2 || ds->x2 < spr->x1 ||
          (!ds->silhouette && !ds->maskedtexturecol))
        {
          continue; /* does not cover sprite */
        }

      r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
      r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

      if (ds->scale1 > ds->scale2)
        {
          lowscale = ds->scale2;
          scale = ds->scale1;
        }
      else
        {
          lowscale = ds->scale1;
          scale = ds->scale2;
        }

      if (scale < spr->scale ||
          (lowscale < spr->scale &&
           !r_point_on_seg_side(spr->gx, spr->gy, ds->curline)))
        {
          /* masked mid texture? */

          if (ds->maskedtexturecol) r_render_masked_seg_range(ds, r1, r2);
          continue; /* seg is behind sprite */
        }

      /* clip this piece of the sprite */

      silhouette = ds->silhouette;

      if (spr->gz >= ds->bsilheight) silhouette &= ~SIL_BOTTOM;

      if (spr->gzt <= ds->tsilheight) silhouette &= ~SIL_TOP;

      if (silhouette == 1)
        {
          /* bottom sil */

          for (x = r1; x <= r2; x++)
            {
              if (clipbot[x] == -2) clipbot[x] = ds->sprbottomclip[x];
            }
        }
      else if (silhouette == 2)
        {
          /* top sil */

          for (x = r1; x <= r2; x++)
            {
              if (cliptop[x] == -2) cliptop[x] = ds->sprtopclip[x];
            }
        }
      else if (silhouette == 3)
        {
          /* both */

          for (x = r1; x <= r2; x++)
            {
              if (clipbot[x] == -2) clipbot[x] = ds->sprbottomclip[x];
              if (cliptop[x] == -2) cliptop[x] = ds->sprtopclip[x];
            }
        }
    }

  /* all clipping has been performed, so draw the sprite */

  /* check for unclipped columns */

  for (x = spr->x1; x <= spr->x2; x++)
    {
      if (clipbot[x] == -2) clipbot[x] = viewheight;

      if (cliptop[x] == -2) cliptop[x] = -1;
    }

  mfloorclip = clipbot;
  mceilingclip = cliptop;
  r_draw_vis_sprite(spr, spr->x1, spr->x2);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* r_init_sprites
 * Called at program start.
 */

void r_init_sprites(const char **namelist)
{
  int i;

  for (i = 0; i < SCREENWIDTH; i++)
    {
      negonearray[i] = -1;
    }

  r_init_sprite_defs(namelist);
}

/* Called at frame start. */

void r_clear_sprites(void)
{
  vissprite_p = vissprites;
}

/* r_draw_masked_column
 * Used for sprites and masked mid textures.
 * Masked means: partly transparent, i.e. stored in posts/runs of opaque
 * pixels.
 */

void r_draw_masked_column(column_t *column)
{
  int topscreen;
  int bottomscreen;
  fixed_t basetexturemid;

  basetexturemid = dc_texturemid;

  for (; column->topdelta != 0xff; )
    {
      /* calculate unclipped screen coordinates for post */

      topscreen = sprtopscreen + spryscale * column->topdelta;
      bottomscreen = topscreen + spryscale * column->length;

      dc_yl = (topscreen + FRACUNIT - 1) >> FRACBITS;
      dc_yh = (bottomscreen - 1) >> FRACBITS;

      if (dc_yh >= mfloorclip[dc_x]) dc_yh = mfloorclip[dc_x] - 1;
      if (dc_yl <= mceilingclip[dc_x]) dc_yl = mceilingclip[dc_x] + 1;

      if (dc_yl <= dc_yh)
        {
          dc_source = (byte *)column + 3;
          dc_texturemid = basetexturemid - (column->topdelta << FRACBITS);

          /* Drawn by either r_draw_column or (SHADOW) r_draw_fuzz_column.
           */

          colfunc();
        }

      column = (column_t *)((byte *)column + column->length + 4);
    }

  dc_texturemid = basetexturemid;
}

/* r_add_sprites
 * During BSP traversal, this adds sprites by sector.
 */

void r_add_sprites(sector_t *sec)
{
  mobj_t *thing;
  int lightnum;

  /* BSP is traversed by subsector.
   * A sector might have been split into several subsectors during BSP
   * building. Thus we check whether its already added.
   */

  if (sec->validcount == validcount) return;

  /* Well, now it will be done. */

  sec->validcount = validcount;

  lightnum = (sec->lightlevel >> LIGHTSEGSHIFT) + extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS - 1];
  else
    spritelights = scalelight[lightnum];

  /* Handle all things in sector. */

  for (thing = sec->thinglist; thing; thing = thing->snext)
    r_project_sprite(thing);
}

void r_draw_masked(void)
{
  vissprite_t *spr;
  drawseg_t *ds;

  r_sort_vis_sprites();

  if (vissprite_p > vissprites)
    {
      /* draw all vissprites back to front */

      for (spr = vsprsortedhead.next;
           spr != &vsprsortedhead;
           spr = spr->next)
        {
          r_draw_sprite(spr);
        }
    }

  /* render any remaining masked mid textures */

  for (ds = ds_p - 1; ds >= drawsegs; ds--)
    {
      if (ds->maskedtexturecol)
        {
          r_render_masked_seg_range(ds, ds->x1, ds->x2);
        }
    }

  /* draw the psprites on top of everything but does not draw on side views
   */

  if (!viewangleoffset)
    {
      r_draw_player_sprites();
    }
}
