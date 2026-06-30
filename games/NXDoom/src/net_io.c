/****************************************************************************
 * apps/games/NXDoom/src/net_io.c
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
 *   Network packet I/O.  Base layer for sending/receiving packets,
 *   through the network module system
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>

#include "i_system.h"
#include "net_defs.h"
#include "net_io.h"
#include "z_zone.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_MODULES 16

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct _net_context_s
{
  net_module_t *modules[MAX_MODULES];
  int num_modules;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

net_addr_t net_broadcast_addr;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

net_context_t *net_new_context(void)
{
  net_context_t *context;

  context = z_malloc(sizeof(net_context_t), PU_STATIC, 0);
  context->num_modules = 0;

  return context;
}

void net_add_module(net_context_t *context, net_module_t *module)
{
  if (context->num_modules >= MAX_MODULES)
    {
      i_error("net_add_module: No more modules for context");
    }

  context->modules[context->num_modules] = module;
  ++context->num_modules;
}

net_addr_t *net_resolve_address(net_context_t *context, const char *addr)
{
  int i;
  net_addr_t *result;

  for (i = 0; i < context->num_modules; ++i)
    {
      result = context->modules[i]->resolve_address(addr);

      if (result != NULL)
        {
          net_reference_address(result);
          return result;
        }
    }

  return NULL;
}

void net_send_packet(net_addr_t *addr, net_packet_t *packet)
{
  addr->module->send_packet(addr, packet);
}

void net_send_broadcast(net_context_t *context, net_packet_t *packet)
{
  int i;

  for (i = 0; i < context->num_modules; ++i)
    {
      context->modules[i]->send_packet(&net_broadcast_addr, packet);
    }
}

boolean net_recv_packet(net_context_t *context, net_addr_t **addr,
                       net_packet_t **packet)
{
  int i;

  /* check all modules for new packets */

  for (i = 0; i < context->num_modules; ++i)
    {
      if (context->modules[i]->recv_packet(addr, packet))
        {
          net_reference_address(*addr);
          return true;
        }
    }

  return false;
}

/* Note: this prints into a static buffer, calling again overwrites
 * the first result
 */

char *net_addr_to_string(net_addr_t *addr)
{
  static char buf[128];

  addr->module->addr_to_string(addr, buf, sizeof(buf) - 1);

  return buf;
}

void net_reference_address(net_addr_t *addr)
{
  if (addr == NULL)
    {
      return;
    }

  ++addr->refcount;
}

void net_release_address(net_addr_t *addr)
{
  if (addr == NULL)
    {
      return;
    }

  --addr->refcount;
  if (addr->refcount <= 0)
    {
      addr->module->free_address(addr);
    }
}
