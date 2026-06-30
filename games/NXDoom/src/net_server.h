/****************************************************************************
 * apps/games/NXDoom/src/net_server.h
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
 * Network server code
 *
 ****************************************************************************/

#ifndef NET_SERVER_H
#define NET_SERVER_H

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* initialize server and wait for connections */

void net_sv_init(void);

/* run server: check for new packets received etc. */

void net_sv_run(void);

/* Shut down the server
 * Blocks until all clients disconnect, or until a 5 second timeout
 */

void net_sv_shutdown(void);

/* Add a network module to the context used by the server */

void net_sv_add_module(net_module_t *module);

/* Register server with master server. */

void net_sv_register_with_master(void);

#endif /* NET_SERVER_H */
