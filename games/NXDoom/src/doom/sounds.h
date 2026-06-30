/****************************************************************************
 * apps/games/NXDoom/src/doom/sounds.h
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
 *  Created by the sound utility written by Dave Taylor.
 *  Kept as a sample, DOOM2  sounds. Frozen.
 *
 ****************************************************************************/

#ifndef __SOUNDS__
#define __SOUNDS__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "i_sound.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Identifiers for all music in game. */

typedef enum
{
  MUS_NONE,
  MUS_E1M1,
  MUS_E1M2,
  MUS_E1M3,
  MUS_E1M4,
  MUS_E1M5,
  MUS_E1M6,
  MUS_E1M7,
  MUS_E1M8,
  MUS_E1M9,
  MUS_E2M1,
  MUS_E2M2,
  MUS_E2M3,
  MUS_E2M4,
  MUS_E2M5,
  MUS_E2M6,
  MUS_E2M7,
  MUS_E2M8,
  MUS_E2M9,
  MUS_E3M1,
  MUS_E3M2,
  MUS_E3M3,
  MUS_E3M4,
  MUS_E3M5,
  MUS_E3M6,
  MUS_E3M7,
  MUS_E3M8,
  MUS_E3M9,
  MUS_INTER,
  MUS_INTRO,
  MUS_BUNNY,
  MUS_VICTOR,
  MUS_INTROA,
  MUS_RUNNIN,
  MUS_STALKS,
  MUS_COUNTD,
  MUS_BETWEE,
  MUS_DOOM,
  MUS_THE_DA,
  MUS_SHAWN,
  MUS_DDTBLU,
  MUS_IN_CIT,
  MUS_DEAD,
  MUS_STLKS2,
  MUS_THEDA2,
  MUS_DOOM2,
  MUS_DDTBL2,
  MUS_RUNNI2,
  MUS_DEAD2,
  MUS_STLKS3,
  MUS_ROMERO,
  MUS_SHAWN2,
  MUS_MESSAG,
  MUS_COUNT2,
  MUS_DDTBL3,
  MUS_AMPIE,
  MUS_THEDA3,
  MUS_ADRIAN,
  MUS_MESSG2,
  MUS_ROMER2,
  MUS_TENSE,
  MUS_SHAWN3,
  MUS_OPENIN,
  MUS_EVIL,
  MUS_ULTIMA,
  MUS_READ_M,
  MUS_DM2TTL,
  MUS_DM2INT,
  MUS_NUMMUSIC
} musicenum_t;

/* Identifiers for all sfx in game. */

typedef enum
{
  SFX_NONE,
  SFX_PISTOL,
  SFX_SHOTGN,
  SFX_SGCOCK,
  SFX_DSHTGN,
  SFX_DBOPN,
  SFX_DBCLS,
  SFX_DBLOAD,
  SFX_PLASMA,
  SFX_BFG,
  SFX_SAWUP,
  SFX_SAWIDL,
  SFX_SAWFUL,
  SFX_SAWHIT,
  SFX_RLAUNC,
  SFX_RXPLOD,
  SFX_FIRSHT,
  SFX_FIRXPL,
  SFX_PSTART,
  SFX_PSTOP,
  SFX_DOROPN,
  SFX_DORCLS,
  SFX_STNMOV,
  SFX_SWTCHN,
  SFX_SWTCHX,
  SFX_PLPAIN,
  SFX_DMPAIN,
  SFX_POPAIN,
  SFX_VIPAIN,
  SFX_MNPAIN,
  SFX_PEPAIN,
  SFX_SLOP,
  SFX_ITEMUP,
  SFX_WPNUP,
  SFX_OOF,
  SFX_TELEPT,
  SFX_POSIT1,
  SFX_POSIT2,
  SFX_POSIT3,
  SFX_BGSIT1,
  SFX_BGSIT2,
  SFX_SGTSIT,
  SFX_CACSIT,
  SFX_BRSSIT,
  SFX_CYBSIT,
  SFX_SPISIT,
  SFX_BSPSIT,
  SFX_KNTSIT,
  SFX_VILSIT,
  SFX_MANSIT,
  SFX_PESIT,
  SFX_SKLATK,
  SFX_SGTATK,
  SFX_SKEPCH,
  SFX_VILATK,
  SFX_CLAW,
  SFX_SKESWG,
  SFX_PLDETH,
  SFX_PDIEHI,
  SFX_PODTH1,
  SFX_PODTH2,
  SFX_PODTH3,
  SFX_BGDTH1,
  SFX_BGDTH2,
  SFX_SGTDTH,
  SFX_CACDTH,
  SFX_SKLDTH,
  SFX_BRSDTH,
  SFX_CYBDTH,
  SFX_SPIDTH,
  SFX_BSPDTH,
  SFX_VILDTH,
  SFX_KNTDTH,
  SFX_PEDTH,
  SFX_SKEDTH,
  SFX_POSACT,
  SFX_BGACT,
  SFX_DMACT,
  SFX_BSPACT,
  SFX_BSPWLK,
  SFX_VILACT,
  SFX_NOWAY,
  SFX_BAREXP,
  SFX_PUNCH,
  SFX_HOOF,
  SFX_METAL,
  SFX_CHGUN,
  SFX_TINK,
  SFX_BDOPN,
  SFX_BDCLS,
  SFX_ITMBK,
  SFX_FLAME,
  SFX_FLAMST,
  SFX_GETPOW,
  SFX_BOSPIT,
  SFX_BOSCUB,
  SFX_BOSSIT,
  SFX_BOSPN,
  SFX_BOSDTH,
  SFX_MANATK,
  SFX_MANDTH,
  SFX_SSSIT,
  SFX_SSDTH,
  SFX_KEENPN,
  SFX_KEENDT,
  SFX_SKEACT,
  SFX_SKESIT,
  SFX_SKEATK,
  SFX_RADIO,
  SFX_NUMSFX
} sfxenum_t;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* the complete set of sound effects */

extern sfxinfo_t s_sfx[];

/* the complete set of music */

extern musicinfo_t s_music[];

#endif /* __SOUNDS__ */
