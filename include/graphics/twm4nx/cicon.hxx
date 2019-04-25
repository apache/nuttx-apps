/////////////////////////////////////////////////////////////////////////////
// apps/graphics/twm4nx/include/cicon.hxx
// Icon related definitions
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

#ifndef __APPS_INCLUDE_GRAPHICS_TWM4NX_CICON_HXX
#define __APPS_INCLUDE_GRAPHICS_TWM4NX_CICON_HXX

/////////////////////////////////////////////////////////////////////////////
// Implementation Classes
/////////////////////////////////////////////////////////////////////////////

namespace Twm4Nx
{
  class CTwm4Nx;     /* Forward reference */
  struct SIconEntry; /* Forward reference */

  struct SIconRegion
  {
    FAR struct SIconRegion *flink;
    struct nxgl_point_s pos;
    struct nxgl_size_s size;
    struct nxgl_point_s step;    // allocation granularity
    FAR struct SIconEntry *entries;
  };

  struct SIconEntry
  {
    FAR struct SIconEntry *flink;
    struct nxgl_point_s pos;
    struct nxgl_size_s size;
    FAR CWindow *cwin;
    bool used;
  };

  /**
   * The CIcon class supports a database of icons.
   */

  class CIcon
  {
    private:

      FAR CTwm4Nx                 *m_twm4nx;       /**< The Twm4Nx session */
      FAR struct SNameList        *m_icons;        /**< List of icon images */
      FAR struct SIconRegion      *m_regionHead;   /**< Head of the icon region list */
      FAR struct SIconRegion      *m_regionTail;   /**< Tail of the icon region list */

      inline int roundUp(int v, int multiple)
      {
        return ((v + multiple - 1) / multiple) * multiple;
      }

      /**
       * Find the icon region holding the window 'cwin'
       *
       * @param cwin The window whose icon region is sought
       * @param irp  A location in which to provide the region
       */

      FAR struct SIconEntry *findEntry(FAR CWindow *cwin,
                                       FAR struct SIconRegion **irp);

      /**
       * Given entry 'ie' in the list 'ir', return the entry just prior to 'ie'
       *
       * @param ie The entry whose predecessor is sought
       * @param ir The region containing the entry
       * @return The entry just before 'ie' in the list
       */

      FAR struct SIconEntry *prevEntry(FAR struct SIconEntry *ie,
                                       FAR struct SIconRegion *ir);

      /**
       * 'old' is being freed; and is adjacent to ie.  Merge
       * regions together
       */

      void mergeEntries(FAR struct SIconEntry *old,
                        FAR struct SIconEntry *ie);

      /**
       * Free all of the icon entries linked to a region
       *
       * @param ir The region whose entries will be freed
       */

      void freeIconEntries(FAR struct SIconRegion *ir);

      /**
       * Free all icon regions and all of the region entries linked into the
       * region
       */

      void freeIconRegions(void);

    public:

      /**
       * CIcon Constructor
       */

      CIcon(FAR CTwm4Nx *twm4nx);

      /**
       * CIcon Destructor
       */

      ~CIcon(void);

      /**
       * Create a new icon region and add it to the list of icon regions.
       *
       * @param pos  The position of the region on the background
       * @param size The size of the region
       * @param step
       */

      void addIconRegion(FAR nxgl_point_s *pos, FAR nxgl_size_s *size,
                         FAR struct nxgl_point_s *step);

      /**
       * Position an icon on the background
       *
       * @param cwin The Window whose icon will be placed
       * @param pos  An backup position to use is there are no available regions
       * @param final A location in which to return the selection icon position
       */

      void place(FAR CWindow *cwin, FAR const struct nxgl_point_s *pos,
                 FAR struct nxgl_point_s *final);

      /**
       * Bring the window up.
       *
       * @param cwin.  The window to be brought up.
       */

      void up(FAR CWindow *cwin);

      /**
       * Take the window down.
       *
       * @param cwin.  The window to be taken down.
       */

      void down(FAR CWindow *cwin);
  };
}

#endif  // __APPS_INCLUDE_GRAPHICS_TWM4NX_CICON_HXX
