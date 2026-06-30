/****************************************************************************
 * apps/games/NXDoom/src/net_packet.h
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
 *     Definitions for use in networking code.
 *
 ****************************************************************************/

#ifndef NET_PACKET_H
#define NET_PACKET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "net_defs.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

net_packet_t *net_new_packet(int initial_size);
net_packet_t *net_packet_dup(net_packet_t *packet);
void net_free_packet(net_packet_t *packet);

boolean net_read_int8(net_packet_t *packet, unsigned int *data);
boolean net_read_int16(net_packet_t *packet, unsigned int *data);
boolean net_read_int32(net_packet_t *packet, unsigned int *data);

boolean net_read_sint8(net_packet_t *packet, signed int *data);
boolean net_read_sint16(net_packet_t *packet, signed int *data);

char *net_read_string(net_packet_t *packet);
char *net_read_safe_string(net_packet_t *packet);

void net_write_int8(net_packet_t *packet, unsigned int i);
void net_write_int16(net_packet_t *packet, unsigned int i);
void net_write_int32(net_packet_t *packet, unsigned int i);

void net_write_string(net_packet_t *packet, const char *string);

#endif /* NET_PACKET_H */
