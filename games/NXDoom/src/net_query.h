/****************************************************************************
 * apps/games/NXDoom/src/net_query.h
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
 *     Querying servers to find their current status.
 *
 ****************************************************************************/

#ifndef NET_QUERY_H
#define NET_QUERY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "net_defs.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef void (*net_query_callback_t)(net_addr_t *addr,
                                     net_querydata_t *querydata,
                                     unsigned int ping_time,
                                     void *user_data);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

extern int net_start_lan_query(void);
extern int net_start_master_query(void);

extern void net_lan_query(void);
extern void net_master_query(void);
extern void net_query_address(const char *addr);
extern net_addr_t *net_find_lan_server(void);

extern int net_query_poll(net_query_callback_t callback, void *user_data);

extern net_addr_t *net_query_resolve_master(net_context_t *context);
extern void net_query_add_to_master(net_addr_t *master_addr);
extern boolean net_query_check_added_to_master(boolean *result);
extern void net_query_add_response(net_packet_t *packet);
extern void net_request_hole_punch(net_context_t *context, net_addr_t *addr);

#endif /* NET_QUERY_H */
