/****************************************************************************
 * apps/games/NXDoom/src/net_loop.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "i_system.h"
#include "m_misc.h"
#include "net_defs.h"
#include "net_loop.h"
#include "net_packet.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_QUEUE_SIZE 16

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
  net_packet_t *packets[MAX_QUEUE_SIZE];
  int head;
  int tail;
} packet_queue_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static packet_queue_t client_queue;
static packet_queue_t server_queue;
static net_addr_t client_addr;
static net_addr_t server_addr;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static boolean net_cl_init_client(void);
static boolean net_cl_init_server(void);
static void net_cl_send_packet(net_addr_t *addr, net_packet_t *packet);
static boolean net_cl_recv_packet(net_addr_t **addr, net_packet_t **packet);
static void net_cl_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len);
static void net_cl_free_address(net_addr_t *addr);
static net_addr_t *net_cl_resolve_address(const char *address);

static boolean net_sv_init_client(void);
static boolean net_sv_init_server(void);
static void net_sv_send_packet(net_addr_t *addr, net_packet_t *packet);
static boolean net_sv_recv_packet(net_addr_t **addr, net_packet_t **packet);
static void net_sv_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len);
static void net_sv_free_address(net_addr_t *addr);
static net_addr_t *net_sv_resolve_address(const char *address);

/****************************************************************************
 * Public Data
 ****************************************************************************/

net_module_t net_loop_client_module =
{
  net_cl_init_client,     net_cl_init_server,    net_cl_send_packet,
  net_cl_recv_packet,     net_cl_addr_to_string, net_cl_free_address,
  net_cl_resolve_address,
};

net_module_t net_loop_server_module =
{
  net_sv_init_client,     net_sv_init_server,    net_sv_send_packet,
  net_sv_recv_packet,     net_sv_addr_to_string, net_sv_free_address,
  net_sv_resolve_address,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void queue_init(packet_queue_t *queue)
{
  queue->head = queue->tail = 0;
}

static void queue_push(packet_queue_t *queue, net_packet_t *packet)
{
  int new_tail;

  new_tail = (queue->tail + 1) % MAX_QUEUE_SIZE;

  if (new_tail == queue->head)
    {
      /* queue is full */

      return;
    }

  queue->packets[queue->tail] = packet;
  queue->tail = new_tail;
}

static net_packet_t *queue_pop(packet_queue_t *queue)
{
  net_packet_t *packet;

  if (queue->tail == queue->head)
    {
      /* queue empty */

      return NULL;
    }

  packet = queue->packets[queue->head];
  queue->head = (queue->head + 1) % MAX_QUEUE_SIZE;

  return packet;
}

/****************************************************************************
 * Client-end code
 ****************************************************************************/

static boolean net_cl_init_client(void)
{
  queue_init(&client_queue);

  return true;
}

static boolean net_cl_init_server(void)
{
  i_error("NET_CL_init_server: attempted to initialize client pipe end as a "
          "server!");
  return false;
}

static void net_cl_send_packet(net_addr_t *addr, net_packet_t *packet)
{
  queue_push(&server_queue, net_packet_dup(packet));
}

static boolean net_cl_recv_packet(net_addr_t **addr, net_packet_t **packet)
{
  net_packet_t *popped;

  popped = queue_pop(&client_queue);

  if (popped != NULL)
    {
      *packet = popped;
      *addr = &client_addr;
      client_addr.module = &net_loop_client_module;

      return true;
    }

  return false;
}

static void net_cl_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len)
{
  snprintf(buffer, buffer_len, "local server");
}

static void net_cl_free_address(net_addr_t *addr)
{
}

static net_addr_t *net_cl_resolve_address(const char *address)
{
  if (address == NULL)
    {
      client_addr.module = &net_loop_client_module;

      return &client_addr;
    }
  else
    {
      return NULL;
    }
}

/****************************************************************************
 * Server-end code
 ****************************************************************************/

static boolean net_sv_init_client(void)
{
  i_error("net_sv_initClient: attempted to initialize server pipe end as a "
          "client!");
  return false;
}

static boolean net_sv_init_server(void)
{
  queue_init(&server_queue);
  return true;
}

static void net_sv_send_packet(net_addr_t *addr, net_packet_t *packet)
{
  queue_push(&client_queue, net_packet_dup(packet));
}

static boolean net_sv_recv_packet(net_addr_t **addr, net_packet_t **packet)
{
  net_packet_t *popped;

  popped = queue_pop(&server_queue);

  if (popped != NULL)
    {
      *packet = popped;
      *addr = &server_addr;
      server_addr.module = &net_loop_server_module;

      return true;
    }

  return false;
}

static void net_sv_addr_to_string(net_addr_t *addr, char *buffer,
                                  int buffer_len)
{
  snprintf(buffer, buffer_len, "local client");
}

static void net_sv_free_address(net_addr_t *addr)
{
}

static net_addr_t *net_sv_resolve_address(const char *address)
{
  if (address == NULL)
    {
      server_addr.module = &net_loop_server_module;
      return &server_addr;
    }
  else
    {
      return NULL;
    }
}
