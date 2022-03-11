/////////////////////////////////////////////////////////////////////////////
// apps/graphics/slcd/cslcd.cxx
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

/////////////////////////////////////////////////////////////////////////////
// Included Files
/////////////////////////////////////////////////////////////////////////////

#include <fixedmath.h>
#include <debug.h>

#include <nuttx/nx/nx.h>

#include "graphics/nxwidgets/inxwindow.hxx"
#include "graphics/slcd.hxx"
#include "slcd.hxx"

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

/////////////////////////////////////////////////////////////////////////////
// CSLcd Method implementations
/////////////////////////////////////////////////////////////////////////////

using namespace SLcd;

/**
 * CSLcd Constructor
 *
 * @param wnd Identifies the window to draw into
 * @param height The initial height of the SLCD image
 */

CSLcd::CSLcd(NXWidgets::INxWindow *wnd, nxgl_coord_t height)
{
  m_window = wnd;
  scale(height);
}

/**
 * Created scaled trapezoids for the specified height
 *
 * @param height The height of the SLCD image
 */

void CSLcd::scale(nxgl_coord_t height)
{
  // Save the new height

  m_height = height;

  // Scale each segment

  scaleSegment(GTop_Runs, m_top, NTOP_TRAPEZOIDS + 1);
  scaleSegment(GTopLeft_Runs, m_topLeft, NTOPLEFT_TRAPEZOIDS + 1);
  scaleSegment(GTopRight_Runs, m_topRight, NTOPRIGHT_TRAPEZOIDS + 1);
  scaleSegment(GMiddle_Runs, m_middle, NMIDDLE_TRAPEZOIDS + 1);
  scaleSegment(GBottomLeft_Runs, m_bottomLeft, NBOTTOMLEFT_TRAPEZOIDS + 1);
  scaleSegment(GBottomRight_Runs, m_bottomRight, NBOTTOMRIGHT_TRAPEZOIDS + 1);
  scaleSegment(GBottom_Runs, m_bottom, NBOTTOM_TRAPEZOIDS + 1);
}

/**
 * Get the SLcd image width.  No since the images are
 * slight slanted, this includes some minimal intra-image
 * spacing
 *
 * @return The width of the SLCD image (in pixels)
 */

nxgl_coord_t CSLcd::getWidth(void)
{
  return b16toi(m_height * SLCD_ASPECT_B16);
}

/**
 * Erase the SLCD image at this position
 *
 * @param pos The upper left position of the SLCD image
 */

void CSLcd::erase(FAR const nxgl_point_s &pos)
{
  struct nxgl_rect_s rect;
  rect.pt1.x = pos.x;
  rect.pt1.y = pos.y;
  rect.pt2.x = pos.x + getWidth() - 1;
  rect.pt2.y = pos.y + getHeight() - 1;

  bool success = m_window->fill(&rect, SLCD_BACKGROUND);
  if (!success)
    {
      gerr("ERROR: fill() failed\n");
    }
}

/**
 * Return a the code associated with this ASCII character
 *
 * @param ch The ASCII encoded character to be converted
 * @param code The location to return the code
 * @return True if the character can be represented
 */

bool CSLcd::convert(char ch, FAR uint8_t &code)
{
  if (ch >= '0' && ch <= '9')
    {
      code = GSLcdDigits[ch - '0'];
      return true;
    }
  else if (ch >= 'A' && ch <= 'Z')
    {
      code = GSLcdUpperCase[ch - 'A'];
      return true;
    }
  else if (ch >= 'a' && ch <= 'z')
    {
      code = GSLcdLowerCase[ch - 'a'];
      return true;
    }
  else
    {
      for (int i = 0; i < NMISC_MAPPINGS; i++)
        {
          if (SSLcdMisc[i].ch == ch)
            {
              code = SSLcdMisc[i].segments;
              return true;
            }
        }
    }

  return false;
}

/* Show the SCLD image at the provided position on the
 * display.
 *
 * @param code The encode value that describes the SLCD image
 * @param pos The location to show the SLCD image in the window
 * @return True if the display update was successfully queued
 */

bool CSLcd::show(uint8_t code, FAR const struct nxgl_point_s &pos)
{
  // Show each segment in the encoded segment set

  if ((code & SEGMENT_1) != 0)
    {
      showSegment(m_top, pos, NTOP_TRAPEZOIDS);
    }

  if ((code & SEGMENT_2) != 0)
    {
      showSegment(m_topLeft, pos, NTOPLEFT_TRAPEZOIDS);
    }

  if ((code & SEGMENT_3) != 0)
    {
      showSegment(m_topRight, pos, NTOPRIGHT_TRAPEZOIDS);
    }

  if ((code & SEGMENT_4) != 0)
    {
      showSegment(m_middle, pos, NMIDDLE_TRAPEZOIDS);
    }

  if ((code & SEGMENT_5) != 0)
    {
      showSegment(m_bottomLeft, pos, NBOTTOMLEFT_TRAPEZOIDS);
    }

  if ((code & SEGMENT_6) != 0)
    {
      showSegment(m_bottomRight, pos, NBOTTOMRIGHT_TRAPEZOIDS);
    }

  if ((code & SEGMENT_7) != 0)
    {
      showSegment(m_bottom, pos, NBOTTOM_TRAPEZOIDS);
    }

  return true;
}

/**
 * Created scaled trapezoids for the specified height
 *
 * @param run A pointer to the beginning of an array of runs
 * @param trapezoid A pointer to a trapezoid array to catch the
 *        output
 * @param nRuns The number of runs in the array.  The number of
 *        trapezoids will be be nRuns - 1;
 */

void CSLcd::scaleSegment(FAR const struct SLcdTrapezoidRun *run,
                         FAR struct nxgl_trapezoid_s *trapezoid,
                         int nRuns)
{
  // Get the top of the first trapezoid

  trapezoid[0].top.x1 = m_height * run[0].leftx;
  trapezoid[0].top.x2 = m_height * run[0].rightx;
  trapezoid[0].top.y  = b16toi(m_height * run[0].y);

  int nTraps = nRuns - 1;
  for (int i = 1; i < nRuns; i++)
    {
      // Get the bottom of the previous trapezoid

      trapezoid[i - 1].bot.x1 = m_height * run[i].leftx;
      trapezoid[i - 1].bot.x2 = m_height * run[i].rightx;
      trapezoid[i - 1].bot.y  = b16toi(m_height * run[i].y);

      if (i < nTraps)
        {
          // Get the top of the current trapezoid

          trapezoid[i].top.x1 = trapezoid[i - 1].bot.x1;
          trapezoid[i].top.x2 = trapezoid[i - 1].bot.x2;
          trapezoid[i].top.y  = trapezoid[i - 1].bot.y;
        }
    }
}

/**
 * Rend one segment of the SLCD image in the window at the provided
 * position.
 *
 * @param trapezoid A pointer to the trapezoid array to show.
 * @param pos The offset position of the SLCD image in the window
 * @param nTraps The number of trapezoids in the the array
 */

void CSLcd::showSegment(FAR const struct nxgl_trapezoid_s *trapezoid,
                        FAR const struct nxgl_point_s &pos,
                        int nTraps)
{
  for (int i = 0; i < nTraps; i++)
    {
      // Translate the trapezoid by the requested offset

      struct nxgl_trapezoid_s trap;
      nxgl_trapoffset(&trap, &trapezoid[i], pos.x, pos.y);

      bool success =
        m_window->fillTrapezoid((FAR const struct nxgl_rect_s *)0,
                               &trap, SLCD_FOREGROUND);
      if (!success)
        {
          gerr("ERROR: fillTapezoid failed\n");
        }
    }
}
