/****************************************************************************
 * apps/graphics/traveler/include/trv_trigtbl.h
 * This file defines the fixed precision math environment and look-up tables
 * for trigonometric functions.
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H
#define __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "trv_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Angles *******************************************************************/

/* These are definitions of commonly used angles. */

#define TWOPI       1920
#define PI           960
#define HALFPI       480
#define QTRPI        240

/* Here are definitions for those who prefer degrees */
/* NOTE:  ANGLE_60 and ANGLE_30 are special values.  They were */
/* chosen to match the horizontal screen resolution of 320 pixels. */
/* These, in fact, drive the entire angular measurement system */

#define ANGLE_0        0
#define ANGLE_6       32
#define ANGLE_9       48
#define ANGLE_12      64
#define ANGLE_30     160
#define ANGLE_45     240
#define ANGLE_60     320
#define ANGLE_90     480
#define ANGLE_180    960
#define ANGLE_270   1440
#define ANGLE_360   1920

/* This is the angular change made with each column of the ray caster */
/* This is (2048/360 units/degree) * 59.94 (degrees) / (320 columns) */

#define VIDEO_COLUMN_ANGLE 1
#define VIDEO_ROW_ANGLE    1

/* Fixed precision definitions **********************************************/

/* SMALL precision (6 bits past binary point) */
/* This occurs frequently because each CELL is 64x64 */

#define sUNITY   64
#define sHALF    32
#define sQUARTER 16
#define sSHIFT    6
#define sMASK    63

#define sTRUNC(a) ((a) >> sSHIFT)
#define sROUND(a) (((a) + sHALF) >> sSHIFT)
#define sFIX(a)   ((a) << sSHIFT)
#define sSNAP(a)  ((a) & (~sMASK))
#define sFRAC(a)  ((a) & sMASK)

#define sMUL(a,b) (((int32_t)(a) * (int32_t)(b)) >> sSHIFT )
#define sDIV(a,b) (((int32_t)(a) << sSHIFT) / (b))
#define sFLOAT(a) ((float)(a) / (float)sUNITY)

/* DOUBLE precision (12 bits past binary point) */
/* This precision results when two SMALL precision numbers are multiplied */

#define dUNITY 4096
#define dHALF  2048
#define dSHIFT   12
#define dMASK  4095

#define dTRUNC(a) ((a) >> dSHIFT)
#define dROUND(a) (((a) + dHALF) >> dSHIFT)
#define dFIX(a)   ((a) << dSHIFT)
#define dSNAP(a)  ((a) & (~dMASK))
#define dFRAC(a)  ((a) & dMASK)

/* TRIPLE precision (18 bits past binary point) */
/* This precision results when a SMALL and a DOUBLE precision number
 * are multiplied
 */

#define tSHIFT     18

/* QUAD precision (24 bits past binary point) */
/* This precision results when two DOUBLE precision numbers are multiplied */

#define qSHIFT     24

/* BIG precision (16 bits past binary point) */
/* This is convenient precision because it is easy to extract the integer
 * portion without shifting or masking
 */

#define bUNITY 65536
#define bHALF  32768
#define bSHIFT    16
#define bMASK  65535

/* Conversions between SMALL, DOUBLE, TRIPLE and BIG precision */

#define sTOd(a)   ((a) << (dSHIFT-sSHIFT))
#define sTOb(a)   ((a) << (bSHIFT-sSHIFT))
#define dTOs(a)   ((a) >> (dSHIFT-sSHIFT))
#define dTOb(a)   ((a) << (bSHIFT-dSHIFT))
#define tTOs(a)   ((a) >> (tSHIFT-sSHIFT))
#define tTOd(a)   ((a) >> (tSHIFT-dSHIFT))
#define tTOb(a)   ((a) >> (tSHIFT-bSHIFT))
#define qTOd(a)   ((a) >> (qSHIFT-dSHIFT))
#define qTOb(a)   ((a) >> (qSHIFT-bSHIFT))
#define bTOs(a)   ((a) >> (bSHIFT-sSHIFT))

/* These are general math macros that have nothing to do with fixed precision */

#define ABS(a)    ((a) < 0 ? -(a) : (a))
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define MAX(a,b)  ((a) > (b) ? (a) : (b))

/* Trigonometry *************************************************************/
/* Because COS(x) = SIN(x + HALFPI) and COT(x) = TAN(90-x), the following
 * provide fast conversions to get cosines from the g_sin_table's and
 * cotangents form the g_tan_table.
 */

#define g_cot_table(x) g_tan_table[PI+HALFPI-(x)]
#define g_cos_table    ((int16_t*)&g_sin_table[HALFPI])
#define g_sec_table    ((int32_t*)&g_csc_table[HALFPI])

/* Here are some MACROs to make life easier */
/* The following extend the range of the table to all positive angles */

#define TAN(x) ((x)>=(PI+HALFPI) ? g_tan_table[(x)-PI] : g_tan_table[x])

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* This structure is useful for manipulating BIG precision types from
 * C code (NOTE: The following union assumes LITTLE ENDIAN!).
 */

struct trv_bigfp_s
{
  uint16_t f;
  int16_t i;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Here are declarations for the trig tables */

extern const int32_t g_tan_table[PI+HALFPI+1];
extern const int16_t g_sin_table[TWOPI+HALFPI+1];
extern const int32_t g_csc_table[TWOPI+HALFPI+1];

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_GRAPHICS_TRAVELER_INCLUDE_TRV_TRIGTBL_H */
