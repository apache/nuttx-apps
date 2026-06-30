/****************************************************************************
 * apps/games/NXDoom/src/net_loop.h
 *
 * SPDX-License-Identifier: GPLv2
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
 *  Loopback network module for server compiled into the client
 *
 ****************************************************************************/

#ifndef NET_LOOP_H
#define NET_LOOP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "net_defs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern net_module_t net_loop_client_module;
extern net_module_t net_loop_server_module;

#endif /* NET_LOOP_H */
