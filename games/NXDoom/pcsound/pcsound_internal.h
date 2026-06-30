/****************************************************************************
 * apps/games/NXDoom/pcsound/pcsound_internal.h
 *
 * SPDX-License-Identifer: GPLv2
 *
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
 *    PC speaker interface.
 *
 ****************************************************************************/

#ifndef PCSOUND_INTERNAL_H
#define PCSOUND_INTERNAL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "pcsound.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PCSOUND_8253_FREQUENCY (1193280)

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct pcsound_driver_s pcsound_driver_t;
typedef int (*pcsound_init_func)(pcsound_callback_func callback);
typedef void (*pcsound_shutdown_func)(void);

struct pcsound_driver_s
{
  const char *name;
  pcsound_init_func init_func;
  pcsound_shutdown_func shutdown_func;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern int pcsound_sample_rate;

#ifdef HAVE_LINUX_KD_H
extern pcsound_driver_t pcsound_linux_driver;
#endif

extern pcsound_driver_t pcsound_sdl_driver;

#endif /* PCSOUND_INTERNAL_H */
