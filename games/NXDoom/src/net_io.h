/****************************************************************************
 * apps/games/NXDoom/src/net_io.h
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
 *      Network packet manipulation (net_packet_t)
 *
 ****************************************************************************/

#ifndef NET_IO_H
#define NET_IO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "net_defs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern net_addr_t net_broadcast_addr;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Create a new network context. */

net_context_t *net_new_context(void);

/* Add a network module to a context. */

void net_add_module(net_context_t *context, net_module_t *module);

/* Send a packet to the given address. */

void net_send_packet(net_addr_t *addr, net_packet_t *packet);

/* Send a broadcast using all modules in the given context. */

void net_send_broadcast(net_context_t *context, net_packet_t *packet);

/* Check all modules in the given context and receive a packet, returning
 * true if a packet was received. The result is stored in *packet and the
 * source is stored in *addr, with an implicit reference added. The packet
 * must be freed by the caller and the reference released.
 */

boolean net_recv_packet(net_context_t *context, net_addr_t **addr,
                        net_packet_t **packet);

/* Return a string representation of the given address. The result points
 * to a static buffer and will become invalid with the next call.
 */

char *net_addr_to_string(net_addr_t *addr);

/* Add a reference to the given address. */

void net_reference_address(net_addr_t *addr);

/* Release a reference to the given address. When there are no more
 * references, the address will be freed.
 */

void net_release_address(net_addr_t *addr);

/* Resolve a string representation of an address. If successful, a net_addr_t
 * pointer is received with an implicit reference that must be freed by the
 * caller when it is no longer needed.
 */

net_addr_t *net_resolve_address(net_context_t *context, const char *address);

#endif /* NET_IO_H */
