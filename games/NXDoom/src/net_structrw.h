/****************************************************************************
 * apps/games/NXDoom/src/net_structrw.h
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
 ****************************************************************************/

#ifndef NET_STRUCTRW_H
#define NET_STRUCTRW_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "aes_prng.h"
#include "net_defs.h"
#include "net_packet.h"
#include "sha1.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void net_write_connect_data(net_packet_t *packet, net_connect_data_t *data);
boolean net_read_connect_data(net_packet_t *packet,
        net_connect_data_t *data);

extern void net_write_settings(net_packet_t *packet,
                              net_gamesettings_t *settings);
extern boolean net_read_settings(net_packet_t *packet,
                                net_gamesettings_t *settings);

extern void net_write_query_data(net_packet_t *packet,
                               net_querydata_t *querydata);
extern boolean net_read_query_data(net_packet_t *packet,
                                 net_querydata_t *querydata);

extern void net_write_ticcmd_diff(net_packet_t *packet, net_ticdiff_t *diff,
                                boolean lowres_turn);
extern boolean net_read_ticcmd_diff(net_packet_t *packet,
        net_ticdiff_t *diff, boolean lowres_turn);
extern void net_ticcmd_diff(ticcmd_t *tic1, ticcmd_t *tic2,
                           net_ticdiff_t *diff);
extern void net_ticcmd_patch(ticcmd_t *src, net_ticdiff_t *diff,
                            ticcmd_t *dest);

boolean net_read_full_ticcmd(net_packet_t *packet, net_full_ticcmd_t *cmd,
                           boolean lowres_turn);
void net_write_full_ticcmd(net_packet_t *packet, net_full_ticcmd_t *cmd,
                         boolean lowres_turn);

boolean net_read_sha1_sum(net_packet_t *packet, sha1_digest_t digest);
void net_write_sha1_sum(net_packet_t *packet, sha1_digest_t digest);

void net_write_wait_data(net_packet_t *packet, net_waitdata_t *data);
boolean net_read_wait_data(net_packet_t *packet, net_waitdata_t *data);

boolean net_read_prng_seed(net_packet_t *packet, prng_seed_t seed);
void net_write_prng_seed(net_packet_t *packet, prng_seed_t seed);

/* Protocol list exchange. */

net_protocol_t net_read_protocol(net_packet_t *packet);
void net_write_protocol(net_packet_t *packet, net_protocol_t protocol);
net_protocol_t net_read_protocol_list(net_packet_t *packet);
void net_write_protocol_list(net_packet_t *packet);

#endif /* NET_STRUCTRW_H */
