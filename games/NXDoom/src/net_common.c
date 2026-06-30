/****************************************************************************
 * apps/games/NXDoom/src/net_common.c
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
 * Common code shared between the client and server
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "d_mode.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_timer.h"
#include "m_argv.h"
#include "m_misc.h"

#include "net_common.h"
#include "net_io.h"
#include "net_packet.h"
#include "net_structrw.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* connections time out after 30 seconds */

#define CONNECTION_TIMEOUT_LEN 30

/* maximum time between sending packets */

#define KEEPALIVE_PERIOD 1

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* reliable packet that is guaranteed to reach its destination */

struct net_reliable_packet_s
{
  net_packet_t *packet;
  int last_send_time;
  int seq;
  net_reliable_packet_t *next;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void net_conn_init(net_connection_t *conn, net_addr_t *addr,
                          net_protocol_t protocol)
{
  conn->last_send_time = -1;
  conn->num_retries = 0;
  conn->addr = addr;
  conn->protocol = protocol;
  conn->reliable_packets = NULL;
  conn->reliable_send_seq = 0;
  conn->reliable_recv_seq = 0;
  conn->keepalive_recv_time = i_get_time_ms();
}

static void net_conn_parse_disconnect(net_connection_t *conn,
                                      net_packet_t *packet)
{
  net_packet_t *reply;

  /* Other end wants to disconnect
   * Send a DISCONNECT_ACK reply.
   */

  reply = net_new_packet(10);
  net_write_int16(reply, NET_PACKET_TYPE_DISCONNECT_ACK);
  net_conn_send_packet(conn, reply);
  net_free_packet(reply);

  conn->last_send_time = i_get_time_ms();

  conn->state = NET_CONN_STATE_DISCONNECTED_SLEEP;
  conn->disconnect_reason = NET_DISCONNECT_REMOTE;
}

/* Parse a DISCONNECT_ACK packet */

static void net_conn_parse_disconnect_ack(net_connection_t *conn,
                                          net_packet_t *packet)
{
  if (conn->state == NET_CONN_STATE_DISCONNECTING)
    {
      /* We have received an acknowledgement to our disconnect
       * request. We have been disconnected successfully.
       */

      conn->state = NET_CONN_STATE_DISCONNECTED;
      conn->disconnect_reason = NET_DISCONNECT_LOCAL;
      conn->last_send_time = -1;
    }
}

static void net_conn_parse_reliable_ack(net_connection_t *conn,
                                        net_packet_t *packet)
{
  unsigned int seq;

  if (!net_read_int8(packet, &seq))
    {
      return;
    }

  if (conn->reliable_packets == NULL)
    {
      return;
    }

  /* Is this an acknowledgement for the first packet in the list? */

  if (seq == (unsigned int)((conn->reliable_packets->seq + 1) & 0xff))
    {
      net_reliable_packet_t *rp;

      /* Discard it, then.
       * Unlink from the list.
       */

      rp = conn->reliable_packets;
      conn->reliable_packets = rp->next;

      net_free_packet(rp->packet);
      free(rp);
    }
}

/* Process the header of a reliable packet
 *
 * Returns true if the packet should be discarded (incorrect sequence)
 */

static boolean net_connect_reliable_packet(net_connection_t *conn,
                                           net_packet_t *packet)
{
  unsigned int seq;
  net_packet_t *reply;
  boolean result;

  /* Read the sequence number */

  if (!net_read_int8(packet, &seq))
    {
      return true;
    }

  if (seq != (unsigned int)(conn->reliable_recv_seq & 0xff))
    {
      /* This is not the next expected packet in the sequence!
       *
       * Discard the packet.  If we were smart, we would use a proper
       * sliding window protocol to do this, but I'm lazy.
       */

      result = true;
    }
  else
    {
      /* Now we can receive the next packet in the sequence. */

      conn->reliable_recv_seq = (conn->reliable_recv_seq + 1) & 0xff;

      result = false;
    }

  /* Send an acknowledgement */

  /* Note: this is braindead.  It would be much more sensible to
   * include this in the next packet, rather than the overhead of
   * sending a complete packet just for one byte of information.
   */

  reply = net_new_packet(10);

  net_write_int16(reply, NET_PACKET_TYPE_RELIABLE_ACK);
  net_write_int8(reply, conn->reliable_recv_seq & 0xff);

  net_conn_send_packet(conn, reply);

  net_free_packet(reply);

  return result;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Initialize as a client connection */

void net_conn_init_client(net_connection_t *conn, net_addr_t *addr,
                          net_protocol_t protocol)
{
  net_conn_init(conn, addr, protocol);
  conn->state = NET_CONN_STATE_CONNECTING;
}

/* Initialize as a server connection */

void net_conn_init_server(net_connection_t *conn, net_addr_t *addr,
                         net_protocol_t protocol)
{
  net_conn_init(conn, addr, protocol);
  conn->state = NET_CONN_STATE_CONNECTED;
}

/* Send a packet to a connection
 * All packets should be sent through this interface, as it maintains the
 * keepalive_send_time counter.
 */

void net_conn_send_packet(net_connection_t *conn, net_packet_t *packet)
{
  conn->keepalive_send_time = i_get_time_ms();
  net_send_packet(conn->addr, packet);
}

/* Process a packet received by the server
 *
 * Returns true if eaten by common code
 */

boolean net_conn_packet(net_connection_t *conn, net_packet_t *packet,
                        unsigned int *packet_type)
{
  conn->keepalive_recv_time = i_get_time_ms();

  /* Is this a reliable packet? */

  if (*packet_type & NET_RELIABLE_PACKET)
    {
      if (net_connect_reliable_packet(conn, packet))
        {
          /* Invalid packet: eat it. */

          return true;
        }

      /* Remove the reliable bit */

      *packet_type &= ~NET_RELIABLE_PACKET;
    }

  switch (*packet_type)
    {
    case NET_PACKET_TYPE_DISCONNECT:
      net_conn_parse_disconnect(conn, packet);
      break;
    case NET_PACKET_TYPE_DISCONNECT_ACK:
      net_conn_parse_disconnect_ack(conn, packet);
      break;
    case NET_PACKET_TYPE_KEEPALIVE:
      break; /* No special action needed. */
    case NET_PACKET_TYPE_RELIABLE_ACK:
      net_conn_parse_reliable_ack(conn, packet);
      break;
    default:
      return false; /* Not a common packet */
    }

  /* We found a packet that we found interesting, and ate it. */

  return true;
}

void net_conn_disconnect(net_connection_t *conn)
{
  if (conn->state != NET_CONN_STATE_DISCONNECTED &&
      conn->state != NET_CONN_STATE_DISCONNECTING &&
      conn->state != NET_CONN_STATE_DISCONNECTED_SLEEP)
    {
      conn->state = NET_CONN_STATE_DISCONNECTING;
      conn->disconnect_reason = NET_DISCONNECT_LOCAL;
      conn->last_send_time = -1;
      conn->num_retries = 0;
    }
}

void net_conn_run(net_connection_t *conn)
{
  net_packet_t *packet;
  unsigned int nowtime;

  nowtime = i_get_time_ms();

  if (conn->state == NET_CONN_STATE_CONNECTED)
    {
      /* Check the keepalive counters */

      if (nowtime - conn->keepalive_recv_time >
              CONNECTION_TIMEOUT_LEN * 1000)
        {
          /* Haven't received any packets from the other end in a long
           * time.  Assume disconnected.
           */

          conn->state = NET_CONN_STATE_DISCONNECTED;
          conn->disconnect_reason = NET_DISCONNECT_TIMEOUT;
        }

      if (nowtime - conn->keepalive_send_time > KEEPALIVE_PERIOD * 1000)
        {
          /* We have not sent anything in a long time.
           * Send a keepalive.
           */

          packet = net_new_packet(10);
          net_write_int16(packet, NET_PACKET_TYPE_KEEPALIVE);
          net_conn_send_packet(conn, packet);
          net_free_packet(packet);
        }

      /* Check the reliable packet list. Has the first packet in the
       * list timed out?
       *
       * NB.  This is braindead, we have a fixed time of one second.
       */

      if (conn->reliable_packets != NULL &&
          (conn->reliable_packets->last_send_time < 0 ||
           nowtime - conn->reliable_packets->last_send_time > 1000))
        {
          /* Packet timed out, time to resend */

          net_conn_send_packet(conn, conn->reliable_packets->packet);
          conn->reliable_packets->last_send_time = nowtime;
        }
    }
  else if (conn->state == NET_CONN_STATE_DISCONNECTING)
    {
      /* Waiting for a reply to our DISCONNECT request. */

      if (conn->last_send_time < 0 || nowtime - conn->last_send_time > 1000)
        {
          /* it has been a second since the last disconnect packet
           * was sent, and still no reply.
           */

          if (conn->num_retries < MAX_RETRIES)
            {
              /* send another disconnect */

              packet = net_new_packet(10);
              net_write_int16(packet, NET_PACKET_TYPE_DISCONNECT);
              net_conn_send_packet(conn, packet);
              net_free_packet(packet);
              conn->last_send_time = nowtime;

              ++conn->num_retries;
            }
          else
            {
              /* No more retries allowed. Force disconnect. */

              conn->state = NET_CONN_STATE_DISCONNECTED;
              conn->disconnect_reason = NET_DISCONNECT_LOCAL;
            }
        }
    }
  else if (conn->state == NET_CONN_STATE_DISCONNECTED_SLEEP)
    {
      /* We are disconnected, waiting in case we need to send
       * a DISCONNECT_ACK to the server again.
       */

      if (nowtime - conn->last_send_time > 5000)
        {
          /* Idle for 5 seconds, switch state */

          conn->state = NET_CONN_STATE_DISCONNECTED;
          conn->disconnect_reason = NET_DISCONNECT_REMOTE;
        }
    }
}

net_packet_t *net_conn_new_reliable(net_connection_t *conn, int packet_type)
{
  net_packet_t *packet;
  net_reliable_packet_t *rp;
  net_reliable_packet_t **listend;

  /* Generate a packet with the right header */

  packet = net_new_packet(100);

  net_write_int16(packet, packet_type | NET_RELIABLE_PACKET);

  /* write the low byte of the send sequence number */

  net_write_int8(packet, conn->reliable_send_seq & 0xff);

  /* Add to the list of reliable packets */

  rp = malloc(sizeof(net_reliable_packet_t));
  rp->packet = packet;
  rp->next = NULL;
  rp->seq = conn->reliable_send_seq;
  rp->last_send_time = -1;

  for (listend = &conn->reliable_packets; *listend != NULL;
       listend = &((*listend)->next))
    ;

  *listend = rp;

  /* Count along the sequence */

  conn->reliable_send_seq = (conn->reliable_send_seq + 1) & 0xff;

  /* Finished */

  return packet;
}

/* Used to expand the least significant byte of a tic number into
 * the full tic number, from the current tic number
 */

unsigned int net_expand_tic_num(unsigned int relative, unsigned int b)
{
  unsigned int l;
  unsigned int h;
  unsigned int result;

  h = relative & ~0xff;
  l = relative & 0xff;

  result = h | b;

  if (l < 0x40 && b > 0xb0) result -= 0x100;
  if (l > 0xb0 && b < 0x40) result += 0x100;

  return result;
}

/* Check that game settings are valid */

boolean net_valid_game_settings(game_mode_t mode, gamemission_t mission,
                              net_gamesettings_t *settings)
{
  if (settings->ticdup <= 0)
    {
      return false;
    }

  if (settings->extratics < 0)
    {
      return false;
    }

  if (settings->deathmatch < 0 || settings->deathmatch > 2)
    {
      return false;
    }

  if (settings->skill < sk_noitems || settings->skill > sk_nightmare)
    {
      return false;
    }

  if (!d_valid_game_version(mission, settings->gameversion))
    {
      return false;
    }

  if (!d_valid_episode_map(mission, mode, settings->episode, settings->map))
    {
      return false;
    }

  return true;
}

#ifdef CONFIG_GAMES_NXDOOM_NET_LOGS
void net_log_packet(net_packet_t *packet)
{
  int i;
  int bytes;

  bytes = packet->len - packet->pos;
  if (bytes == 0)
    {
      return;
    }

  fprintf(stderr, "\t%02x", packet->data[packet->pos]);

  for (i = 1; i < bytes; ++i)
    {
      if ((i % 16) == 0)
        {
          fprintf(stderr, "\n\t");
        }
      else
        {
          fprintf(stderr, " ");
        }

      fprintf(stderr, "%02x", packet->data[packet->pos + i]);
    }

  fprintf(stderr, "\n");
}
#endif /* CONFIG_GAMES_NXDOOM_NET_LOGS */
