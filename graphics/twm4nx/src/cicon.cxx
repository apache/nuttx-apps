/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/src/cicon.cxx
// Icon releated routines
//
//   Copyright (C) 2019 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Largely an original work but derives from TWM 1.0.10 in many ways:
//
//   Copyright 1989,1998  The Open Group
//
// Please refer to apps/twm4nx/COPYING for detailed copyright information.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

#include "nuttx/nx/nxglib.h"

#include "graphics/twm4nx/ctwm4nx.hxx"
#include "graphics/twm4nx/cwindow.hxx"
#include "graphics/twm4nx/cwindowevent.hxx"
#include "graphics/twm4nx/cicon.hxx"

/////////////////////////////////////////////////////////////////////////////
// CTwm4Nx Implementation
/////////////////////////////////////////////////////////////////////////////

using namespace Twm4Nx;

/**
 * CIcon Constructor
 */

CIcon::CIcon(CTwm4Nx *twm4nx)
{
  m_twm4nx      = twm4nx;                       // Cached the Twm4Nx session
  m_icons       = (FAR struct SNameList *)0;    // List of icon images
  m_regionHead  = (FAR struct SIconRegion *)0;  // Pointer to icon regions
  m_regionTail  = (FAR struct SIconRegion *)0;  // Pointer to the last icon region
}

/**
 * CIcon Destructor
 */

CIcon::~CIcon(void)
{
  freeIconRegions();
}

/**
 * Create a new icon region and add it to the list of icon regions.
 *
 * @param pos  The position of the region on the background
 * @param size The size of the region
 * @param step
 */

void CIcon::addIconRegion(FAR nxgl_point_s *pos, FAR nxgl_size_s *size,
                          FAR struct nxgl_point_s *step)
{
  FAR struct SIconRegion *ir;

  // Allocate the new region

  ir = (FAR struct SIconRegion *)std::malloc(sizeof(struct SIconRegion));

  // Add the new region to the list of regions

  ir->flink = (FAR struct SIconRegion *)0;

  if (m_regionTail != (FAR struct SIconRegion *)0)
    {
      m_regionTail->flink = ir;
    }

  m_regionTail = ir;

  if (m_regionHead != (FAR struct SIconRegion *)0)
    {
      m_regionHead = ir;
    }

  // Initiliaze the region

  ir->entries = NULL;

  if (step->x <= 0)
    {
      step->x = 1;
    }

  if (step->y <= 0)
    {
      step->y = 1;
    }

  ir->step.x = step->x;
  ir->step.y = step->y;
  ir->pos.x  = pos->x;
  ir->pos.y  = pos->y;
  ir->size.w = size->w;
  ir->size.h = size->h;

  struct nxgl_size_s displaySize;
  m_twm4nx->getDisplaySize(&displaySize);

  if (ir->pos.x < 0)
    {
      ir->pos.x += displaySize.w - ir->size.w;
    }

  if (ir->pos.y < 0)
    {
      ir->pos.y += displaySize.h - ir->size.h;
    }

  // Allocate one region entry

  ir->entries = (FAR struct SIconEntry *)malloc(sizeof(struct SIconRegion));
  if (ir->entries != (FAR struct SIconEntry *)0)
    {
      ir->entries->flink   = (FAR struct SIconEntry *)0;
      ir->entries->pos.x   = ir->pos.x;
      ir->entries->pos.y   = ir->pos.y;
      ir->entries->size.w  = ir->size.w;
      ir->entries->size.h  = ir->size.h;
      ir->entries->cwin    = (FAR CWindow *)0;
      ir->entries->used    = false;
    }
}

/**
 * Position an icon on the background
 *
 * @param cwin The Window whose icon will be placed
 * @param pos  An backup position to use is there are no available regions
 * @param final A location in which to return the selection icon position
 */

void CIcon::place(FAR CWindow *cwin, FAR const struct nxgl_point_s *pos,
                  FAR struct nxgl_point_s *final)
{
  // Try each region

  FAR struct SIconEntry *ie = (FAR struct SIconEntry *)0;
  for (FAR struct SIconRegion *ir = m_regionHead; ir; ir = ir->flink)
    {
      struct nxgl_size_s iconWindowSize;
      cwin->getIconWidgetSize(iconWindowSize);

      struct nxgl_size_s tmpsize;
      tmpsize.w = roundUp(iconWindowSize.w, ir->step.x);
      tmpsize.h = roundUp(iconWindowSize.h, ir->step.y);

      // Try each entry in the regions

      for (ie = ir->entries; ie; ie = ie->flink)
        {
          // Look for an unused entry

          if (ie->used)
            {
              continue;
            }

          // Does the entry describe a region that is of sufficient size?

          if (ie->size.w >= tmpsize.w && ie->size.h >= tmpsize.h)
            {
              // Yes.. We have it.  Break out with ie non-NULL

              break;
            }
        }

      // Break out of the outer loop if the the region entry was found by
      // the inner loop

      if (ie != (FAR struct SIconEntry *)0)
        {
          break;
        }
    }

  // Did we find an entry?

  if (ie != (FAR struct SIconEntry *)0)
    {
      // Yes.. place the icon in this region

      ie->used = true;
      ie->cwin = cwin;

      struct nxgl_size_s iconWindowSize;
      cwin->getIconWidgetSize(iconWindowSize);

      final->x = ie->pos.x + (ie->size.w - iconWindowSize.w) / 2;
      final->y = ie->pos.y + (ie->size.h - iconWindowSize.h) / 2;
    }
  else
    {
      // No.. place it.. wherever

      final->x = pos->x;
      final->y = pos->y;
    }
}

/**
 * Bring the window up.
 *
 * @param cwin The window to be brought up.
 */

void CIcon::up(FAR CWindow *cwin)
{
  struct nxgl_point_s newpos;
  struct nxgl_point_s oldpos;

  // Did the user move the icon?

  if (cwin->hasIconMoved())
    {
      struct nxgl_size_s oldsize;
      cwin->getIconWidgetSize(oldsize);
      cwin->getIconWidgetPosition(oldpos);

      newpos.x = oldpos.x + ((int)oldsize.w) / 2;
      newpos.y = oldpos.y + ((int)oldsize.h) / 2;

      FAR struct SIconRegion *ir;
      for (ir = m_regionHead; ir; ir = ir->flink)
        {
          if (newpos.x >= ir->pos.x &&
              newpos.x < (ir->pos.x + ir->size.w) &&
              newpos.y >= ir->pos.y &&
              newpos.y < (ir->pos.y + ir->size.h))
            {
              break;
            }
        }

      if (ir == NULL)
        {
          return;    // outside icon regions, leave alone
        }
    }

  oldpos.x = -100;
  oldpos.y = -100;

  place(cwin, &oldpos, &newpos);

  if (newpos.x != oldpos.x || newpos.y != oldpos.y)
    {
      cwin->getIconWidgetPosition(newpos);
      cwin->setIconMoved(false);    // since we've restored it
    }
}

/**
 * Take the window down.
 *
 * @param vwin The window to be taken down.
 */

void CIcon::down(FAR CWindow *cwin)
{
  FAR struct SIconRegion *ir;
  FAR struct SIconEntry *ie;

  ie = findEntry(cwin, &ir);
  if (ie != (FAR struct SIconEntry *)0)
    {
      ie->cwin = 0;
      ie->used = false;

      FAR struct SIconEntry *ip = prevEntry(ie, ir);
      FAR struct SIconEntry *in = ie->flink;

      for (; ; )
        {
          if (ip && ip->used == false &&
              ((ip->pos.x == ie->pos.x && ip->size.w == ie->size.w) ||
               (ip->pos.y == ie->pos.y && ip->size.h == ie->size.h)))
            {
              ip->flink = ie->flink;
              mergeEntries(ie, ip);
              free(ie);
              ie = ip;
              ip = prevEntry(ip, ir);
            }
          else if (in && in->used == false &&
                   ((in->pos.x == ie->pos.x && in->size.w == ie->size.w) ||
                    (in->pos.y == ie->pos.y && in->size.h == ie->size.h)))
            {
              ie->flink = in->flink;
              mergeEntries(in, ie);
              free(in);
              in = ie->flink;
            }
          else
            {
              break;
            }
        }
    }
}

/**
 * Find the icon region holding the window 'cwin'
 *
 * @param cwin The window whose icon region is sought
 * @param irp  A location in which to provide the region
 */

FAR struct SIconEntry *CIcon::findEntry(FAR CWindow *cwin,
                                        FAR struct SIconRegion **irp)
{
  FAR struct SIconRegion *ir;
  FAR struct SIconEntry *ie;

  for (ir = m_regionHead; ir; ir = ir->flink)
    {
      for (ie = ir->entries; ie; ie = ie->flink)
        if (ie->cwin == cwin)
          {
            if (irp)
              {
                *irp = ir;
              }

            return ie;
          }
    }

  return (FAR struct SIconEntry *)0;
}

/**
 * Given entry 'ie' in the list 'ir', return the entry just prior to 'ie'
 *
 * @param ie The entry whose predecessor is sought
 * @param ir The region containing the entry
 * @return The entry just before 'ie' in the list
 */

FAR struct SIconEntry *CIcon::prevEntry(FAR struct SIconEntry *ie,
                                        FAR struct SIconRegion *ir)
{
  FAR struct SIconEntry *ip;

  if (ie == ir->entries)
    {
      return (FAR struct SIconEntry *)0;
    }

  for (ip = ir->entries; ip->flink != ie; ip = ip->flink)
    {
    }

  return ip;
}

/**
 * 'old' is being freed; and is adjacent to ie.  Merge
 * regions together
 */

void CIcon::mergeEntries(FAR struct SIconEntry *old,
                         FAR struct SIconEntry *ie)
{
  if (old->pos.y == ie->pos.y)
    {
      ie->size.w = old->size.w + ie->size.w;
      if (old->pos.x < ie->pos.x)
        {
          ie->pos.x = old->pos.x;
        }
    }
  else
    {
      ie->size.h = old->size.h + ie->size.h;
      if (old->pos.y < ie->pos.y)
        {
          ie->pos.y = old->pos.y;
        }
    }
}

/**
 * Free all of the icon entries linked to a region
 *
 * @param ir The region whose entries will be freed
 */

void CIcon::freeIconEntries(FAR struct SIconRegion *ir)
{
  FAR struct SIconEntry *ie;
  FAR struct SIconEntry *tmp;

  for (ie = ir->entries; ie; ie = tmp)
    {
      tmp = ie->flink;
      std::free(ie);
    }
}

/**
 * Free all icon regions and all of the region entries linked into the
 * region
 */

void CIcon::freeIconRegions(void)
{
  struct SIconRegion *ir;
  struct SIconRegion *tmp;

  for (ir = m_regionHead; ir != NULL;)
    {
      tmp = ir;
      freeIconEntries(ir);
      ir = ir->flink;
      free(tmp);
    }

  m_regionHead = NULL;
  m_regionTail = NULL;
}
