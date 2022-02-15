/////////////////////////////////////////////////////////////////////////////
// apps/include/graphics/slcd.hxx
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APPS_INCLUDE_GRAPHICS_SLCD_HXX
#define __APPS_INCLUDE_GRAPHICS_SLCD_HXX

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <fixedmath.h>
#include <nuttx/nx/nxglib.h>

#include "graphics/nxwidgets/nxconfig.hxx"

/////////////////////////////////////////////////////////////////////////////
// Pre-processor definitions
/////////////////////////////////////////////////////////////////////////////

// Segment
//
//  11111111
// 2        3
// 2        3
// 2        3
//  44444444
// 5        6
// 5        6
// 5        6
//  77777777

// Number of trapezoids in each segment

#define NTOP_TRAPEZOIDS         3
#define NTOPLEFT_TRAPEZOIDS     5
#define NTOPRIGHT_TRAPEZOIDS    5
#define NMIDDLE_TRAPEZOIDS      2
#define NBOTTOMLEFT_TRAPEZOIDS  4
#define NBOTTOMRIGHT_TRAPEZOIDS 5
#define NBOTTOM_TRAPEZOIDS      2

// Clock colors:  Light grey-green background, greenish-black foreground.
// Similar to what you would see on a classic LCD.

#define SLCD_BACKGROUND MKRGB(128, 140, 128)
#define SLCD_FOREGROUND MKRGB(0, 16, 0)

/////////////////////////////////////////////////////////////////////////////
// CSLcd Implementation Class
/////////////////////////////////////////////////////////////////////////////

namespace NXWidgets
{
  class INxWindow;                     // Forward Reference
}

namespace SLcd
{
  class CSLcd
  {
    private:
      FAR NXWidgets::INxWindow *m_window;  /**< Drawing window */
      nxgl_coord_t              m_height;

      // Scaled trapezoids

      struct nxgl_trapezoid_s   m_top[NTOP_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_topLeft[NTOPLEFT_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_topRight[NTOPRIGHT_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_middle[NMIDDLE_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_bottomLeft[NBOTTOMLEFT_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_bottomRight[NBOTTOMRIGHT_TRAPEZOIDS];
      struct nxgl_trapezoid_s   m_bottom[NBOTTOM_TRAPEZOIDS];

      /**
       * Created scaled trapezoids for the specified height
       *
       * @param run A pointer to the beginning of an array of runs
       * @param trapezoid A pointer to a trapezoid array to catch the
       *        output
       * @param nRuns The number of runs in the array.  The number of
       *        trapezoids will be be nRuns - 1;
       */

      void scaleSegment(FAR const struct SLcdTrapezoidRun *run,
                        FAR struct nxgl_trapezoid_s *trapezoid,
                        int nRuns);

      /**
       * Rend one segment of the SLCD image in the window at the provided
       * position.
       *
       * @param trapezoid A pointer to the trapezoid array to show.
       * @param pos The offset position of the SLCD image in the window
       * @param nTraps The number of trapezoids in the the array
       */

      void showSegment(FAR const struct nxgl_trapezoid_s *trapezoid,
                       FAR const struct nxgl_point_s &pos,
                       int nTraps);

    public:

      /**
       * CSLcd Constructor
       *
       * @param wnd Identifies the window to draw into
       * @param height The initial height of the SLCD image
       */

      CSLcd(NXWidgets::INxWindow *wnd, nxgl_coord_t height);

      /**
       * CSLcd Destructor
       */

      inline ~CSLcd(void)
      {
      }

      /**
       * Created scaled trapezoids for the specified height
       *
       * @param height The height of the SLCD image
       */

      void scale(nxgl_coord_t height);

      /**
       * Get the SLcd image height
       *
       * @return The height of the SLCD image (in rows)
       */

      inline nxgl_coord_t getHeight(void)
      {
        return m_height;
      }

      /**
       * Get the SLcd image width.  No since the images are
       * slight slanted, this includes some minimal intra-image
       * spacing
       *
       * @return The width of the SLCD image (in pixels)
       */

      nxgl_coord_t getWidth(void);

      /**
       * Erase the SLCD image at this position
       *
       * @param pos The upper left position of the SLCD image
       */

      void erase(FAR const nxgl_point_s &pos);

      /**
       * Return a the code associated with this ASCII character
       *
       * @param ch The ASCII encoded character to be converted
       * @param code The location to return the code
       * @return True if the character can be represented
       */

      bool convert(char ch, FAR uint8_t &code);

      /* Show the SCLD image at the provided position on the
       * display.
       *
       * @param code The encode value that describes the SLCD image
       * @param pos The location to show the SLCD image in the window
       * @return True if the display update was successfully queued
       */

      bool show(uint8_t code, FAR const struct nxgl_point_s &pos);
  };
}

#endif /* __APPS_INCLUDE_GRAPHICS_SLCD_HXX */
