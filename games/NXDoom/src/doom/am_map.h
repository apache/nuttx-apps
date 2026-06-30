/****************************************************************************
 * apps/games/NXDoom/src/doom/am_map.h
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
 *   AutoMap module.
 *
 ****************************************************************************/

#ifndef __AMMAP_H__
#define __AMMAP_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_event.h"
#include "m_cheat.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Used by ST StatusBar stuff. */

#define AM_MSGHEADER (('a' << 24) + ('m' << 16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e' << 8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x' << 8))

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern cheatseq_t cheat_amap;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: am_responder
 *
 * Description:
 *  Called by main loop.
 *
 ****************************************************************************/

boolean am_responder(event_t *ev);

/****************************************************************************
 * Name: am_ticker
 *
 * Description:
 *  Called by main loop.
 *
 ****************************************************************************/

void am_ticker(void);

/****************************************************************************
 * Name: am_ticker
 *
 * Description:
 *  Called by main loop. Called instead of view drawer if automap active.
 *
 ****************************************************************************/

void am_drawer(void);

/****************************************************************************
 * Name: am_stop
 *
 * Description:
 *  Called to force the automap to quit if the level is completed while it is
 *  up.
 *
 ****************************************************************************/

void am_stop(void);

#endif /* __AMMAP_H__ */
