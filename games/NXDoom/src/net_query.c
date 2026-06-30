/****************************************************************************
 * apps/games/NXDoom/src/net_query.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_system.h"
#include "i_timer.h"
#include "m_misc.h"

#include "net_common.h"
#include "net_defs.h"
#include "net_io.h"
#include "net_packet.h"
#include "net_query.h"
#include "net_sdl.h"
#include "net_structrw.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DNS address of the Internet master server. */

#define MASTER_SERVER_ADDRESS "master.chocolate-doom.org:2342"

/* Time to wait for a response before declaring a timeout. */

#define QUERY_TIMEOUT_SECS 2

/* Time to wait for secure demo signatures before declaring a timeout. */

#define SIGNATURE_TIMEOUT_SECS 5

/* Number of query attempts to make before giving up on a server. */

#define QUERY_MAX_ATTEMPTS 3

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  QUERY_TARGET_SERVER,   /* Normal server target. */
  QUERY_TARGET_MASTER,   /* The master server. */
  QUERY_TARGET_BROADCAST /* Send a broadcast query */
} query_target_type_t;

typedef enum
{
  QUERY_TARGET_QUEUED,    /* Query not yet sent */
  QUERY_TARGET_QUERIED,   /* Query sent, waiting response */
  QUERY_TARGET_RESPONDED, /* Response received */
  QUERY_TARGET_NO_RESPONSE
} query_target_state_t;

typedef struct
{
  query_target_type_t type;
  query_target_state_t state;
  net_addr_t *addr;
  net_querydata_t data;
  unsigned int ping_time;
  unsigned int query_time;
  unsigned int query_attempts;
  boolean printed;
} query_target_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static boolean g_registered_with_master = false;
static boolean g_got_master_response = false;

static net_context_t *g_query_context;
static query_target_t *g_targets;
static int g_num_targets;

static boolean g_query_loop_running = false;
static boolean g_printed_header = false;
static int g_last_query_time = 0;

#if 0
static char *g_securedemo_start_message = NULL;
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void formatted_printf(int wide, const char *s, ...) PRINTF_ATTR(2, 3);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Send a query to the master server. */

static void net_query_send_master_query(net_addr_t *addr)
{
  net_packet_t *packet;

  packet = net_new_packet(4);
  net_write_int16(packet, NET_MASTER_PACKET_TYPE_QUERY);
  net_send_packet(addr, packet);
  net_free_packet(packet);

  /* We also send a NAT_HOLE_PUNCH_ALL packet so that servers behind
   * NAT gateways will open themselves up to us.
   */

  packet = net_new_packet(4);
  net_write_int16(packet, NET_MASTER_PACKET_TYPE_NAT_HOLE_PUNCH_ALL);
  net_send_packet(addr, packet);
  net_free_packet(packet);
}

/* Given the specified address, find the target associated.  If no
 * target is found, and 'create' is true, a new target is created.
 */

static query_target_t *get_target_for_addr(net_addr_t *addr, boolean create)
{
  query_target_t *target;
  int i;

  for (i = 0; i < g_num_targets; ++i)
    {
      if (g_targets[i].addr == addr)
        {
          return &g_targets[i];
        }
    }

  if (!create)
    {
      return NULL;
    }

  g_targets =
      i_realloc(g_targets, sizeof(query_target_t) * (g_num_targets + 1));

  target = &g_targets[g_num_targets];
  target->type = QUERY_TARGET_SERVER;
  target->state = QUERY_TARGET_QUEUED;
  target->printed = false;
  target->query_attempts = 0;
  target->addr = addr;
  net_reference_address(addr);
  ++g_num_targets;

  return target;
}

static void free_targets(void)
{
  int i;

  for (i = 0; i < g_num_targets; ++i)
    {
      net_release_address(g_targets[i].addr);
    }

  free(g_targets);
  g_targets = NULL;
  g_num_targets = 0;
}

/* Transmit a query packet */

static void net_query_send_query(net_addr_t *addr)
{
  net_packet_t *request;

  request = net_new_packet(10);
  net_write_int16(request, NET_PACKET_TYPE_QUERY);

  if (addr == NULL)
    {
      net_send_broadcast(g_query_context, request);
    }
  else
    {
      net_send_packet(addr, request);
    }

  net_free_packet(request);
}

static void net_query_parse_response(net_addr_t *addr, net_packet_t *packet,
                                     net_query_callback_t callback,
                                     void *user_data)
{
  unsigned int packet_type;
  net_querydata_t querydata;
  query_target_t *target;

  /* Read the header */

  if (!net_read_int16(packet, &packet_type) ||
      packet_type != NET_PACKET_TYPE_QUERY_RESPONSE)
    {
      return;
    }

  /* Read query data */

  if (!net_read_query_data(packet, &querydata))
    {
      return;
    }

  /* Find the target that responded. */

  target = get_target_for_addr(addr, false);

  /* If the target is not found, it may be because we are doing
   * a LAN broadcast search, in which case we need to create a
   * target for the new responder.
   */

  if (target == NULL)
    {
      query_target_t *broadcast_target;

      broadcast_target = get_target_for_addr(NULL, false);

      /* Not in broadcast mode, unexpected response that came out
       * of nowhere. Ignore.
       */

      if (broadcast_target == NULL ||
          broadcast_target->state != QUERY_TARGET_QUERIED)
        {
          return;
        }

      /* Create new target. */

      target = get_target_for_addr(addr, true);
      broadcast_target = get_target_for_addr(NULL, false);
      target->state = QUERY_TARGET_QUERIED;
      target->query_time = broadcast_target->query_time;
    }

  if (target->state != QUERY_TARGET_RESPONDED)
    {
      target->state = QUERY_TARGET_RESPONDED;
      memcpy(&target->data, &querydata, sizeof(net_querydata_t));

      /* Calculate RTT. */

      target->ping_time = i_get_time_ms() - target->query_time;

      /* Invoke callback to signal that we have a new address. */

      callback(addr, &target->data, target->ping_time, user_data);
    }
}

/* Parse a response packet from the master server. */

static void net_query_parse_master_response(net_addr_t *master_addr,
                                            net_packet_t *packet)
{
  unsigned int packet_type;
  query_target_t *target;
  char *addr_str;
  net_addr_t *addr;

  /* Read the header.  We are only interested in query responses. */

  if (!net_read_int16(packet, &packet_type) ||
      packet_type != NET_MASTER_PACKET_TYPE_QUERY_RESPONSE)
    {
      return;
    }

  /* Read a list of strings containing the addresses of servers
   * that the master knows about.
   */

  for (; ; )
    {
      addr_str = net_read_string(packet);

      if (addr_str == NULL)
        {
          break;
        }

      /* Resolve address and add to targets list if it is not already
       * there.
       */

      addr = net_resolve_address(g_query_context, addr_str);
      if (addr != NULL)
        {
          get_target_for_addr(addr, true);
          net_release_address(addr);
        }
    }

  /* Mark the master as having responded. */

  target = get_target_for_addr(master_addr, true);
  target->state = QUERY_TARGET_RESPONDED;
}

static void net_query_parse_packet(net_addr_t *addr, net_packet_t *packet,
                                   net_query_callback_t callback,
                                   void *user_data)
{
  query_target_t *target;

  /* This might be the master server responding. */

  target = get_target_for_addr(addr, false);

  if (target != NULL && target->type == QUERY_TARGET_MASTER)
    {
      net_query_parse_master_response(addr, packet);
    }
  else
    {
      net_query_parse_response(addr, packet, callback, user_data);
    }
}

static void net_query_get_response(net_query_callback_t callback,
                                   void *user_data)
{
  net_addr_t *addr;
  net_packet_t *packet;

  if (net_recv_packet(g_query_context, &addr, &packet))
    {
      net_query_parse_packet(addr, packet, callback, user_data);
      net_release_address(addr);
      net_free_packet(packet);
    }
}

/* Find a target we have not yet queried and send a query. */

static void send_one_query(void)
{
  unsigned int now;
  unsigned int i;

  now = i_get_time_ms();

  /* Rate limit - only send one query every 50ms. */

  if (now - g_last_query_time < 50)
    {
      return;
    }

  for (i = 0; i < g_num_targets; ++i)
    {
      /* Not queried yet?
       * Or last query timed out without a response?
       */

      if (g_targets[i].state == QUERY_TARGET_QUEUED ||
          (g_targets[i].state == QUERY_TARGET_QUERIED &&
           now - g_targets[i].query_time > QUERY_TIMEOUT_SECS * 1000))
        {
          break;
        }
    }

  if (i >= g_num_targets)
    {
      return;
    }

  /* Found a target to query.  Send a query; how to do this depends on
   * the target type.
   */

  switch (g_targets[i].type)
    {
    case QUERY_TARGET_SERVER:
      net_query_send_query(g_targets[i].addr);
      break;

    case QUERY_TARGET_BROADCAST:
      net_query_send_query(NULL);
      break;

    case QUERY_TARGET_MASTER:
      net_query_send_master_query(g_targets[i].addr);
      break;
    }

  /* printf("Queried %s\n", net_addr_to_string(targets[i].addr)); */

  g_targets[i].state = QUERY_TARGET_QUERIED;
  g_targets[i].query_time = now;
  ++g_targets[i].query_attempts;

  g_last_query_time = now;
}

/* Time out servers that have been queried and not responded. */

static void check_target_timeouts(void)
{
  unsigned int i;
  unsigned int now;

  now = i_get_time_ms();

  for (i = 0; i < g_num_targets; ++i)
    {
      /* printf("target %i: state %i, queries %i, query time %i\n",
       *      i, targets[i].state, targets[i].query_attempts,
       *      now - targets[i].query_time);
       */

      /* We declare a target to be "no response" when we've sent
       * multiple query packets to it (QUERY_MAX_ATTEMPTS) and
       * received no response to any of them.
       */

      if (g_targets[i].state == QUERY_TARGET_QUERIED &&
          g_targets[i].query_attempts >= QUERY_MAX_ATTEMPTS &&
          now - g_targets[i].query_time > QUERY_TIMEOUT_SECS * 1000)
        {
          g_targets[i].state = QUERY_TARGET_NO_RESPONSE;

          if (g_targets[i].type == QUERY_TARGET_MASTER)
            {
              fprintf(stderr, "net_master_query: no response "
                              "from master server.\n");
            }
        }
    }
}

/* If all targets have responded or timed out, returns true. */

static boolean all_targets_done(void)
{
  unsigned int i;

  for (i = 0; i < g_num_targets; ++i)
    {
      if (g_targets[i].state != QUERY_TARGET_RESPONDED &&
          g_targets[i].state != QUERY_TARGET_NO_RESPONSE)
        {
          return false;
        }
    }

  return true;
}

/* Stop the query loop */

static void net_query_exit_loop(void)
{
  g_query_loop_running = false;
}

/* Loop waiting for responses.
 * The specified callback is invoked when a new server responds.
 */

static void net_query_query_loop(net_query_callback_t callback,
                                 void *user_data)
{
  g_query_loop_running = true;

  while (g_query_loop_running && net_query_poll(callback, user_data))
    {
      /* Don't thrash the CPU */

      usleep(1000);
    }
}

static void net_query_init(void)
{
  if (g_query_context == NULL)
    {
      g_query_context = net_new_context();
      net_add_module(g_query_context, &net_sdl_module);
      net_sdl_module.init_client();
    }

  free(g_targets);
  g_targets = NULL;
  g_num_targets = 0;

  g_printed_header = false;
}

/* Callback that exits the query loop when the first server is found. */

static void net_query_exit_callback(net_addr_t *addr, net_querydata_t *data,
                                    unsigned int ping_time, void *user_data)
{
  net_query_exit_loop();
}

/* Search the targets list and find a target that has responded.
 * If none have responded, returns NULL.
 */

static query_target_t *find_first_responder(void)
{
  unsigned int i;

  for (i = 0; i < g_num_targets; ++i)
    {
      if (g_targets[i].type == QUERY_TARGET_SERVER &&
          g_targets[i].state == QUERY_TARGET_RESPONDED)
        {
          return &g_targets[i];
        }
    }

  return NULL;
}

/* Return a count of the number of responses. */

static int get_num_responses(void)
{
  unsigned int i;
  int result;

  result = 0;

  for (i = 0; i < g_num_targets; ++i)
    {
      if (g_targets[i].type == QUERY_TARGET_SERVER &&
          g_targets[i].state == QUERY_TARGET_RESPONDED)
        {
          ++result;
        }
    }

  return result;
}

static void formatted_printf(int wide, const char *s, ...)
{
  va_list args;
  int i;

  va_start(args, s);
  i = vprintf(s, args);
  va_end(args);

  while (i < wide)
    {
      putchar(' ');
      ++i;
    }
}

static const char *game_description(game_mode_t mode, gamemission_t mission)
{
  switch (mission)
    {
    case doom:
      if (mode == shareware)
        return "swdoom";
      else if (mode == registered)
        return "regdoom";
      else if (mode == retail)
        return "ultdoom";
      else
        return "doom";
    case doom2:
      return "doom2";
    case pack_tnt:
      return "tnt";
    case pack_plut:
      return "plutonia";
    case pack_chex:
      return "chex";
    case pack_hacx:
      return "hacx";
    case heretic:
      return "heretic";
    case hexen:
      return "hexen";
    case strife:
      return "strife";
    default:
      return "?";
    }
}

static void print_header(void)
{
  int i;

  putchar('\n');
  formatted_printf(5, "Ping");
  formatted_printf(18, "Address");
  formatted_printf(8, "Players");
  puts("Description");

  for (i = 0; i < 70; ++i)
    putchar('=');
  putchar('\n');
}

/* Callback function that just prints information in a table. */

static void net_query_print_callback(net_addr_t *addr, net_querydata_t *data,
                                     unsigned int ping_time, void *user_data)
{
  /* If this is the first server, print the header. */

  if (!g_printed_header)
    {
      print_header();
      g_printed_header = true;
    }

  formatted_printf(5, "%4i", ping_time);
  formatted_printf(22, "%s", net_addr_to_string(addr));
  formatted_printf(4, "%i/%i ", data->num_players, data->max_players);

  if (data->gamemode != indetermined)
    {
      printf("(%s) ", game_description(data->gamemode, data->gamemission));
    }

  if (data->server_state)
    {
      printf("(game running) ");
    }

  printf("%s\n", data->description);
}

#if 0 /* UNUSED */

/* Block until a packet of the given type is received from the given
 * address.
 */

static net_packet_t *block_for_packet(net_addr_t *addr,
                                      unsigned int packet_type,
                                      unsigned int timeout_ms)
{
  net_packet_t *packet;
  net_addr_t *packet_src;
  unsigned int read_packet_type;
  unsigned int start_time;

  start_time = i_get_time_ms();

  while (i_get_time_ms() < start_time + timeout_ms)
    {
      if (!net_recv_packet(g_query_context, &packet_src, &packet))
        {
          usleep(20000);
          continue;
        }

      /* Caller doesn't need additional reference. */

      net_release_address(packet_src);

      if (packet_src == addr && net_read_int16(packet, &read_packet_type) &&
          packet_type == read_packet_type)
        {
          return packet;
        }

      net_free_packet(packet);
    }

  /* Timeout - no response. */

  return NULL;
}

/* Query master server for secure demo start seed value. */

static boolean net_start_secure_demo(prng_seed_t seed)
{
  net_packet_t *request, *response;
  net_addr_t *master_addr;
  char *signature;
  boolean result;

  net_query_init();
  master_addr = net_query_resolve_master(g_query_context);

  /* Send request packet to master server. */

  request = net_new_packet(10);
  net_write_int16(request, NET_MASTER_PACKET_TYPE_SIGN_START);
  net_send_packet(master_addr, request);
  net_free_packet(request);

  /* Block for response and read contents.
   * The signed start message will be saved for later.
   */

  response = block_for_packet(master_addr,
                              NET_MASTER_PACKET_TYPE_SIGN_START_RESPONSE,
                              SIGNATURE_TIMEOUT_SECS * 1000);

  result = false;

  if (response != NULL)
    {
      if (net_read_prng_seed(response, seed))
        {
          signature = net_read_string(response);

          if (signature != NULL)
            {
              g_securedemo_start_message = m_string_duplicate(signature);
              result = true;
            }
        }

      net_free_packet(response);
    }

  return result;
}

/* Query master server for secure demo end signature. */

static char *net_end_secure_demo(sha1_digest_t demo_hash)
{
  net_packet_t *request, *response;
  net_addr_t *master_addr;
  char *signature;

  master_addr = net_query_resolve_master(g_query_context);

  /* Construct end request and send to master server. */

  request = net_new_packet(10);
  net_write_int16(request, NET_MASTER_PACKET_TYPE_SIGN_END);
  net_write_sha1_sum(request, demo_hash);
  net_write_string(request, g_securedemo_start_message);
  net_send_packet(master_addr, request);
  net_free_packet(request);

  /* Block for response. The response packet simply contains a string
   * with the ASCII signature.
   */

  response =
      block_for_packet(master_addr, NET_MASTER_PACKET_TYPE_SIGN_END_RESPONSE,
                       SIGNATURE_TIMEOUT_SECS * 1000);

  if (response == NULL)
    {
      return NULL;
    }

  signature = net_read_string(response);

  net_free_packet(response);

  return signature;
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Resolve the master server address. */

net_addr_t *net_query_resolve_master(net_context_t *context)
{
  net_addr_t *addr;

  addr = net_resolve_address(context, MASTER_SERVER_ADDRESS);

  if (addr == NULL)
    {
      fprintf(stderr,
              "Warning: Failed to resolve address "
              "for master server: %s\n",
              MASTER_SERVER_ADDRESS);
    }

  return addr;
}

/* Send a registration packet to the master server to register
 * ourselves with the global list.
 */

void net_query_add_to_master(net_addr_t *master_addr)
{
  net_packet_t *packet;

  packet = net_new_packet(10);
  net_write_int16(packet, NET_MASTER_PACKET_TYPE_ADD);
  net_send_packet(master_addr, packet);
  net_free_packet(packet);
}

/* Process a packet received from the master server. */

void net_query_add_response(net_packet_t *packet)
{
  unsigned int result;

  if (!net_read_int16(packet, &result))
    {
      return;
    }

  if (result != 0)
    {
      /* Only show the message once. */

      if (!g_registered_with_master)
        {
          printf("Registered with master server at %s\n",
                 MASTER_SERVER_ADDRESS);
          g_registered_with_master = true;
        }
    }
  else
    {
      /* Always show rejections. */

      printf("Failed to register with master server at %s\n",
             MASTER_SERVER_ADDRESS);
    }

  g_got_master_response = true;
}

boolean net_query_check_added_to_master(boolean *result)
{
  /* Got response from master yet? */

  if (!g_got_master_response)
    {
      return false;
    }

  *result = g_registered_with_master;
  return true;
}

/* Send a hole punch request to the master server for the server at the
 * given address.
 */

void net_request_hole_punch(net_context_t *context, net_addr_t *addr)
{
  net_addr_t *master_addr;
  net_packet_t *packet;

  master_addr = net_query_resolve_master(context);
  if (master_addr == NULL)
    {
      return;
    }

  packet = net_new_packet(32);
  net_write_int16(packet, NET_MASTER_PACKET_TYPE_NAT_HOLE_PUNCH);
  net_write_string(packet, net_addr_to_string(addr));
  net_send_packet(master_addr, packet);

  net_free_packet(packet);
  net_release_address(master_addr);
}

/* Polling function, invoked periodically to send queries and
 * interpret new responses received from remote servers.
 * Returns zero when the query sequence has completed and all targets
 * have returned responses or timed out.
 */

int net_query_poll(net_query_callback_t callback, void *user_data)
{
  check_target_timeouts();

  /* Send a query.  This will only send a single query at once. */

  send_one_query();

  /* Check for a response */

  net_query_get_response(callback, user_data);

  return !all_targets_done();
}

int net_start_lan_query(void)
{
  query_target_t *target;

  net_query_init();

  /* Add a broadcast target to the list. */

  target = get_target_for_addr(NULL, true);
  target->type = QUERY_TARGET_BROADCAST;

  return 1;
}

int net_start_master_query(void)
{
  net_addr_t *master;
  query_target_t *target;

  net_query_init();

  /* Resolve master address and add to targets list. */

  master = net_query_resolve_master(g_query_context);

  if (master == NULL)
    {
      return 0;
    }

  target = get_target_for_addr(master, true);
  target->type = QUERY_TARGET_MASTER;
  net_release_address(master);

  return 1;
}

void net_lan_query(void)
{
  if (net_start_lan_query())
    {
      printf("\nSearching for servers on local LAN ...\n");

      net_query_query_loop(net_query_print_callback, NULL);

      printf("\n%i server(s) found.\n", get_num_responses());
      free_targets();
    }
}

void net_master_query(void)
{
  if (net_start_master_query())
    {
      printf("\nSearching for servers on Internet ...\n");

      net_query_query_loop(net_query_print_callback, NULL);

      printf("\n%i server(s) found.\n", get_num_responses());
      free_targets();
    }
}

void net_query_address(const char *addr_str)
{
  net_addr_t *addr;
  query_target_t *target;

  net_query_init();

  addr = net_resolve_address(g_query_context, addr_str);

  if (addr == NULL)
    {
      i_error("net_query_address: Host '%s' not found!", addr_str);
    }

  /* Add the address to the list of targets. */

  target = get_target_for_addr(addr, true);

  printf("\nQuerying '%s'...\n", addr_str);

  /* Run query loop. */

  net_query_query_loop(net_query_exit_callback, NULL);

  /* Check if the target responded. */

  if (target->state == QUERY_TARGET_RESPONDED)
    {
      net_query_print_callback(addr, &target->data, target->ping_time, NULL);
      net_release_address(addr);
      free_targets();
    }
  else
    {
      i_error("No response from '%s'", addr_str);
    }
}

net_addr_t *net_find_lan_server(void)
{
  query_target_t *target;
  query_target_t *responder;
  net_addr_t *result;

  net_query_init();

  /* Add a broadcast target to the list. */

  target = get_target_for_addr(NULL, true);
  target->type = QUERY_TARGET_BROADCAST;

  /* Run the query loop, and stop at the first target found. */

  net_query_query_loop(net_query_exit_callback, NULL);

  responder = find_first_responder();

  if (responder != NULL)
    {
      result = responder->addr;
      net_reference_address(result);
    }
  else
    {
      result = NULL;
    }

  free_targets();
  return result;
}
