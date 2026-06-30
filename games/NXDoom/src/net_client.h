/****************************************************************************
 * apps/games/NXDoom/src/net_client.h
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
 * Network client code
 *
 ****************************************************************************/

#ifndef NET_CLIENT_H
#define NET_CLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "d_ticcmd.h"
#include "doomtype.h"
#include "net_defs.h"
#include "sha1.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern boolean net_client_connected;
extern boolean net_client_received_wait_data;
extern net_waitdata_t net_client_wait_data;
extern char *net_client_reject_reason;
extern boolean net_waiting_for_launch;
extern char *net_player_name;

extern sha1_digest_t net_server_wad_sha1sum;
extern sha1_digest_t net_server_deh_sha1sum;
extern unsigned int net_server_is_freedoom;
extern sha1_digest_t net_local_wad_sha1sum;
extern sha1_digest_t net_local_deh_sha1sum;
extern unsigned int net_local_is_freedoom;

extern boolean drone;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

boolean net_cl_connect(net_addr_t *addr, net_connect_data_t *data);
void net_cl_disconnect(void);
void net_cl_run(void);
void net_cl_launch_game(void);
void net_cl_start_game(net_gamesettings_t *settings);
void net_cl_send_ticcmd(ticcmd_t *ticcmd, int maketic);
boolean net_cl_get_settings(net_gamesettings_t *_settings);
void net_init(void);

void net_bind_variables(void);

#endif /* NET_CLIENT_H */
