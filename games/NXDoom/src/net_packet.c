/****************************************************************************
 * apps/games/NXDoom/src/net_packet.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "net_packet.h"
#include "m_misc.h"
#include "z_zone.h"
#include <ctype.h>
#include <string.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_total_packet_memory = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Dynamically increases the size of a packet */

static void net_increase_packet(net_packet_t *packet)
{
  byte *newdata;

  g_total_packet_memory -= packet->alloced;

  packet->alloced *= 2;

  newdata = z_malloc(packet->alloced, PU_STATIC, 0);

  memcpy(newdata, packet->data, packet->len);

  z_free(packet->data);
  packet->data = newdata;

  g_total_packet_memory += packet->alloced;
}

#if 0 /* Unused */
static boolean net_read_sint32(net_packet_t *packet, signed int *data)
{
  if (net_read_int32(packet, (unsigned int *)data))
    {
      if (*data & (1U << 31))
        {
          *data &= ~(1U << 31);
          *data -= (1U << 31);
        }

      return true;
    }
  else
    {
      return false;
    }
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

net_packet_t *net_new_packet(int initial_size)
{
  net_packet_t *packet;

  packet = (net_packet_t *)z_malloc(sizeof(net_packet_t), PU_STATIC, 0);

  if (initial_size == 0) initial_size = 256;

  packet->alloced = initial_size;
  packet->data = z_malloc(initial_size, PU_STATIC, 0);
  packet->len = 0;
  packet->pos = 0;

  g_total_packet_memory += sizeof(net_packet_t) + initial_size;

  /* printf("total packet memory: %i bytes\n", total_packet_memory);
   * printf("%p: allocated\n", packet);
   */

  return packet;
}

/* duplicates an existing packet */

net_packet_t *net_packet_dup(net_packet_t *packet)
{
  net_packet_t *newpacket;

  newpacket = net_new_packet(packet->len);
  memcpy(newpacket->data, packet->data, packet->len);
  newpacket->len = packet->len;

  return newpacket;
}

void net_free_packet(net_packet_t *packet)
{
  /* printf("%p: destroyed\n", packet); */

  g_total_packet_memory -= sizeof(net_packet_t) + packet->alloced;
  z_free(packet->data);
  z_free(packet);
}

/* Read a byte from the packet, returning true if read
 * successfully
 */

boolean net_read_int8(net_packet_t *packet, unsigned int *data)
{
  if (packet->pos + 1 > packet->len) return false;

  *data = packet->data[packet->pos];

  packet->pos += 1;

  return true;
}

/* Read a 16-bit integer from the packet, returning true if read
 * successfully
 */

boolean net_read_int16(net_packet_t *packet, unsigned int *data)
{
  byte *p;

  if (packet->pos + 2 > packet->len) return false;

  p = packet->data + packet->pos;

  *data = (p[0] << 8) | p[1];
  packet->pos += 2;

  return true;
}

/* Read a 32-bit integer from the packet, returning true if read
 * successfully
 */

boolean net_read_int32(net_packet_t *packet, unsigned int *data)
{
  byte *p;

  if (packet->pos + 4 > packet->len) return false;

  p = packet->data + packet->pos;

  *data = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
  packet->pos += 4;

  return true;
}

/* Signed read functions */

boolean net_read_sint8(net_packet_t *packet, signed int *data)
{
  if (net_read_int8(packet, (unsigned int *)data))
    {
      if (*data & (1 << 7))
        {
          *data &= ~(1 << 7);
          *data -= (1 << 7);
        }

      return true;
    }
  else
    {
      return false;
    }
}

boolean net_read_sint16(net_packet_t *packet, signed int *data)
{
  if (net_read_int16(packet, (unsigned int *)data))
    {
      if (*data & (1 << 15))
        {
          *data &= ~(1 << 15);
          *data -= (1 << 15);
        }

      return true;
    }
  else
    {
      return false;
    }
}

/* Read a string from the packet.  Returns NULL if a terminating
 * NUL character was not found before the end of the packet.
 */

char *net_read_string(net_packet_t *packet)
{
  char *start;

  start = (char *)packet->data + packet->pos;

  /* Search forward for a NUL character */

  while (packet->pos < packet->len && packet->data[packet->pos] != '\0')
    {
      ++packet->pos;
    }

  if (packet->pos >= packet->len)
    {
      /* Reached the end of the packet */

      return NULL;
    }

  /* packet->data[packet->pos] == '\0': We have reached a terminating
   * NULL.  Skip past this NULL and continue reading immediately
   * after it.
   */

  ++packet->pos;

  return start;
}

/* Read a string from the packet, but (potentially) modify it to strip
 * out any unprintable characters which could be malicious control codes.
 * Note that this may modify the original packet contents.
 */

char *net_read_safe_string(net_packet_t *packet)
{
  char *r;
  char *w;
  char *result;

  result = net_read_string(packet);
  if (result == NULL)
    {
      return NULL;
    }

  /* w is always <= r, so we never produce a longer string than the original.
   */

  w = result;
  for (r = result; *r != '\0'; ++r)
    {
      /* TODO: This is a very naive way of producing a safe string; only
       * ASCII characters are allowed. Probably this should really support
       * UTF-8 characters as well.
       */

      if (isprint(*r) || *r == '\n')
        {
          *w = *r;
          ++w;
        }
    }

  *w = '\0';

  return result;
}

/* Write a single byte to the packet */

void net_write_int8(net_packet_t *packet, unsigned int i)
{
  if (packet->len + 1 > packet->alloced) net_increase_packet(packet);

  packet->data[packet->len] = i;
  packet->len += 1;
}

/* Write a 16-bit integer to the packet */

void net_write_int16(net_packet_t *packet, unsigned int i)
{
  byte *p;

  if (packet->len + 2 > packet->alloced) net_increase_packet(packet);

  p = packet->data + packet->len;

  p[0] = (i >> 8) & 0xff;
  p[1] = i & 0xff;

  packet->len += 2;
}

/* Write a single byte to the packet */

void net_write_int32(net_packet_t *packet, unsigned int i)
{
  byte *p;

  if (packet->len + 4 > packet->alloced) net_increase_packet(packet);

  p = packet->data + packet->len;

  p[0] = (i >> 24) & 0xff;
  p[1] = (i >> 16) & 0xff;
  p[2] = (i >> 8) & 0xff;
  p[3] = i & 0xff;

  packet->len += 4;
}

void net_write_string(net_packet_t *packet, const char *string)
{
  byte *p;
  size_t string_size;

  string_size = strlen(string) + 1;

  /* Increase the packet size until large enough to hold the string */

  while (packet->len + string_size > packet->alloced)
    {
      net_increase_packet(packet);
    }

  p = packet->data + packet->len;

  m_str_copy((char *)p, string, string_size);

  packet->len += string_size;
}
