/****************************************************************************
 * apps/games/NXDoom/src/net_sdl.c
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
 *     Networking module which uses SDL_net
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "net_defs.h"
#include "net_io.h"
#include "net_packet.h"
#include "net_sdl.h"
#include "z_zone.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static boolean net_null_init_client(void);
static boolean net_null_init_server(void);
static void net_null_send_packet(net_addr_t *addr, net_packet_t *packet);
static boolean net_null_recv_packet(net_addr_t **addr,
        net_packet_t **packet);
static void net_null_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len);
static void net_null_free_address(net_addr_t *addr);
static net_addr_t *net_null_resolve_address(const char *address);

/****************************************************************************
 * Public Data
 ****************************************************************************/

net_module_t net_sdl_module =
{
  net_null_init_client,     net_null_init_server,  net_null_send_packet,
  net_null_recv_packet,      net_null_addr_to_string, net_null_free_address,
  net_null_resolve_address,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static net_addr_t *net_null_resolve_address(const char *address)
{
  return NULL;
}

static boolean net_null_init_client(void)
{
  return false;
}

static boolean net_null_init_server(void)
{
  return false;
}

static void net_null_send_packet(net_addr_t *addr, net_packet_t *packet)
{
}

static boolean net_null_recv_packet(net_addr_t **addr,
        net_packet_t **packet)
{
  return false;
}

static void net_null_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len)
{
}

static void net_null_free_address(net_addr_t *addr)
{
}
