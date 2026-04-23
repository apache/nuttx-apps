/****************************************************************************
 * apps/system/vncviewer/rfb_protocol.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "rfb_protocol.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Data - DES tables for VNC authentication
 ****************************************************************************/

/* Initial permutation */

static const uint8_t g_ip[64] =
{
  58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
  62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
  57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
  61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7
};

/* Final permutation (IP inverse) */

static const uint8_t g_fp[64] =
{
  40,  8, 48, 16, 56, 24, 64, 32, 39,  7, 47, 15, 55, 23, 63, 31,
  38,  6, 46, 14, 54, 22, 62, 30, 37,  5, 45, 13, 53, 21, 61, 29,
  36,  4, 44, 12, 52, 20, 60, 28, 35,  3, 43, 11, 51, 19, 59, 27,
  34,  2, 42, 10, 50, 18, 58, 26, 33,  1, 41,  9, 49, 17, 57, 25
};

/* Expansion permutation */

static const uint8_t g_expand[48] =
{
  32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
  8,   9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
  16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
  24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
};

/* Permuted choice 1 */

static const uint8_t g_pc1[56] =
{
  57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
  10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
  63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
  14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};

/* Permuted choice 2 */

static const uint8_t g_pc2[48] =
{
  14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
  23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
  41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
  44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};

/* P-box permutation */

static const uint8_t g_pbox[32] =
{
  16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,
  5,  18, 31, 10,  2,  8, 24, 14, 32, 27,  3,  9,
  19, 13, 30,  6, 22, 11,  4, 25
};

/* S-boxes */

static const uint8_t g_sbox[8][64] =
{
  {
    14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
     0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
     4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
    15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
  },
  {
    15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
     3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
     0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
    13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
  },
  {
    10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
    13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
    13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
     1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
  },
  {
     7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
    13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
     3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
  },
  {
     2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
    14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
     4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
    11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
  },
  {
    12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
    10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
     9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
     4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
  },
  {
     4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
    13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
     1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
     6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
  },
  {
    13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
     1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
     7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
     2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
  }
};

/* Key schedule left shifts */

static const uint8_t g_shifts[] =
{
  1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: des_getbit / des_setbit
 *
 * Description:
 *   Get/set a single bit in a byte array (1-indexed, MSB first).
 *
 ****************************************************************************/

static int des_getbit(const uint8_t *data, int bitnum)
{
  return (data[(bitnum - 1) / 8] >> (7 - ((bitnum - 1) % 8))) & 1;
}

static void des_setbit(uint8_t *data, int bitnum, int val)
{
  int byte_idx = (bitnum - 1) / 8;
  int bit_idx  = 7 - ((bitnum - 1) % 8);

  if (val)
    {
      data[byte_idx] |= (uint8_t)(1 << bit_idx);
    }
  else
    {
      data[byte_idx] &= (uint8_t)~(1 << bit_idx);
    }
}

/****************************************************************************
 * Name: des_permute
 *
 * Description:
 *   Apply a permutation table to a bit array.
 *
 ****************************************************************************/

static void des_permute(const uint8_t *in, uint8_t *out,
                        const uint8_t *table, int n)
{
  int i;

  memset(out, 0, (n + 7) / 8);
  for (i = 0; i < n; i++)
    {
      des_setbit(out, i + 1, des_getbit(in, table[i]));
    }
}

/****************************************************************************
 * Name: des_encrypt_block
 *
 * Description:
 *   DES ECB encrypt a single 8-byte block in place.
 *   Straightforward implementation using standard tables.
 *
 ****************************************************************************/

static void des_encrypt_block(uint8_t *block, const uint8_t *key)
{
  uint8_t state[8];
  uint8_t cd[7];
  uint8_t l[4];
  uint8_t r[4];
  uint8_t er[6];
  uint8_t subkey[6];
  uint8_t sout[4];
  uint8_t f[4];
  uint8_t newl[4];
  uint8_t combined[8];
  uint8_t result[8];
  uint32_t c;
  uint32_t d;
  int round;
  int i;
  int s;

  /* Initial permutation */

  des_permute(block, state, g_ip, 64);
  memcpy(l, state, 4);
  memcpy(r, state + 4, 4);

  /* Key schedule: apply PC1 to key */

  des_permute(key, cd, g_pc1, 56);

  /* Extract C (bits 1-28) and D (bits 29-56) as 28-bit values */

  for (round = 0; round < 16; round++)
    {
      int shift = g_shifts[round];

      c = ((uint32_t)cd[0] << 20) | ((uint32_t)cd[1] << 12) |
          ((uint32_t)cd[2] << 4)  | ((uint32_t)cd[3] >> 4);
      d = (((uint32_t)cd[3] & 0x0f) << 24) | ((uint32_t)cd[4] << 16) |
          ((uint32_t)cd[5] << 8)  | (uint32_t)cd[6];

      /* Left rotate C and D by shift positions */

      c = ((c << shift) | (c >> (28 - shift))) & 0x0fffffff;
      d = ((d << shift) | (d >> (28 - shift))) & 0x0fffffff;

      /* Pack back into cd[] */

      cd[0] = (uint8_t)(c >> 20);
      cd[1] = (uint8_t)(c >> 12);
      cd[2] = (uint8_t)(c >> 4);
      cd[3] = (uint8_t)((c << 4) | (d >> 24));
      cd[4] = (uint8_t)(d >> 16);
      cd[5] = (uint8_t)(d >> 8);
      cd[6] = (uint8_t)d;

      /* Apply PC2 to get 48-bit subkey */

      des_permute(cd, subkey, g_pc2, 48);

      /* Expand R from 32 to 48 bits */

      des_permute(r, er, g_expand, 48);

      /* XOR with subkey */

      for (i = 0; i < 6; i++)
        {
          er[i] ^= subkey[i];
        }

      /* S-box substitution: 48 bits -> 32 bits */

      memset(sout, 0, 4);
      for (s = 0; s < 8; s++)
        {
          int off = s * 6;
          int b0  = des_getbit(er, off + 1);
          int b1  = des_getbit(er, off + 2);
          int b2  = des_getbit(er, off + 3);
          int b3  = des_getbit(er, off + 4);
          int b4  = des_getbit(er, off + 5);
          int b5  = des_getbit(er, off + 6);
          int row = (b0 << 1) | b5;
          int col = (b1 << 3) | (b2 << 2) | (b3 << 1) | b4;
          int val = g_sbox[s][row * 16 + col];

          sout[s / 2] |= (uint8_t)(val << (((s & 1) == 0) ? 4 : 0));
        }

      /* P-box permutation */

      des_permute(sout, f, g_pbox, 32);

      /* newR = L XOR f(R, K); L = old R */

      for (i = 0; i < 4; i++)
        {
          newl[i] = l[i] ^ f[i];
        }

      memcpy(l, r, 4);
      memcpy(r, newl, 4);
    }

  /* Final: combine R16 | L16 (note the swap) and apply FP */

  memcpy(combined, r, 4);
  memcpy(combined + 4, l, 4);
  des_permute(combined, result, g_fp, 64);
  memcpy(block, result, 8);
}

/****************************************************************************
 * Name: vnc_encrypt_challenge
 *
 * Description:
 *   Encrypt 16-byte VNC challenge using password as DES key.
 *   VNC reverses bits in each byte of the password.
 *
 ****************************************************************************/

static void vnc_encrypt_challenge(uint8_t *challenge,
                                  const char *password)
{
  uint8_t key[8];
  int i;

  memset(key, 0, 8);
  for (i = 0; i < 8 && password[i] != '\0'; i++)
    {
      uint8_t c = (uint8_t)password[i];

      /* Reverse bits in byte (VNC quirk) */

      key[i] = (uint8_t)(((c & 0x01) << 7) | ((c & 0x02) << 5) |
                          ((c & 0x04) << 3) | ((c & 0x08) << 1) |
                          ((c & 0x10) >> 1) | ((c & 0x20) >> 3) |
                          ((c & 0x40) >> 5) | ((c & 0x80) >> 7));
    }

  /* Encrypt two 8-byte blocks */

  des_encrypt_block(challenge, key);
  des_encrypt_block(challenge + 8, key);
}

/****************************************************************************
 * Name: rfb_recv_exact
 *
 * Description:
 *   Receive exactly 'len' bytes from socket.
 *
 ****************************************************************************/

static int rfb_recv_exact(int sockfd, void *buf, size_t len)
{
  uint8_t *p = (uint8_t *)buf;
  ssize_t n;

  while (len > 0)
    {
      n = recv(sockfd, p, len, 0);
      if (n <= 0)
        {
          if (n == 0)
            {
              return -ECONNRESET;
            }

          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      p   += n;
      len -= n;
    }

  return OK;
}

/****************************************************************************
 * Name: rfb_send_exact
 *
 * Description:
 *   Send exactly 'len' bytes to socket.
 *
 ****************************************************************************/

static int rfb_send_exact(int sockfd, const void *buf, size_t len)
{
  const uint8_t *p = (const uint8_t *)buf;
  ssize_t n;

  while (len > 0)
    {
      n = send(sockfd, p, len, 0);
      if (n <= 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          return -errno;
        }

      p   += n;
      len -= n;
    }

  return OK;
}

/****************************************************************************
 * Name: rfb_negotiate_version
 *
 * Description:
 *   Exchange RFB protocol version strings.
 *
 ****************************************************************************/

static int rfb_negotiate_version(int sockfd)
{
  char buf[RFB_VERSION_LEN + 1];
  int ret;

  /* Receive server version */

  ret = rfb_recv_exact(sockfd, buf, RFB_VERSION_LEN);
  if (ret < 0)
    {
      printf("vncviewer: failed to receive server version\n");
      return ret;
    }

  buf[RFB_VERSION_LEN] = '\0';
  printf("vncviewer: server version: %s", buf);

  /* Send our version (3.8) */

  ret = rfb_send_exact(sockfd, RFB_VERSION_STRING, RFB_VERSION_LEN);
  if (ret < 0)
    {
      printf("vncviewer: failed to send client version\n");
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: rfb_negotiate_security
 *
 * Description:
 *   Negotiate security type. Supports None and VNC Authentication.
 *
 ****************************************************************************/

static int rfb_negotiate_security(int sockfd, const char *password)
{
  uint8_t num_types;
  uint8_t types[16];
  uint8_t chosen;
  uint32_t result;
  int ret;
  int i;

  /* Receive number of security types */

  ret = rfb_recv_exact(sockfd, &num_types, 1);
  if (ret < 0)
    {
      return ret;
    }

  if (num_types == 0)
    {
      /* Connection failed - read reason string */

      uint32_t reason_len;
      rfb_recv_exact(sockfd, &reason_len, 4);
      reason_len = ntohl(reason_len);
      printf("vncviewer: connection refused by server\n");
      return -ECONNREFUSED;
    }

  /* Receive security type list */

  ret = rfb_recv_exact(sockfd, types, num_types);
  if (ret < 0)
    {
      return ret;
    }

  /* Choose security type: prefer None, fallback to VNC Auth */

  chosen = RFB_SEC_INVALID;
  for (i = 0; i < num_types; i++)
    {
      if (types[i] == RFB_SEC_NONE)
        {
          chosen = RFB_SEC_NONE;
          break;
        }

      if (types[i] == RFB_SEC_VNC_AUTH)
        {
          chosen = RFB_SEC_VNC_AUTH;
        }
    }

  if (chosen == RFB_SEC_INVALID)
    {
      printf("vncviewer: no supported security type\n");
      return -ENOTSUP;
    }

  /* Send chosen type */

  ret = rfb_send_exact(sockfd, &chosen, 1);
  if (ret < 0)
    {
      return ret;
    }

  printf("vncviewer: security type: %s\n",
         chosen == RFB_SEC_NONE ? "None" : "VNC Auth");

  if (chosen == RFB_SEC_VNC_AUTH)
    {
      /* VNC Authentication: receive 16-byte challenge */

      uint8_t challenge[16];

      ret = rfb_recv_exact(sockfd, challenge, 16);
      if (ret < 0)
        {
          return ret;
        }

      /* Encrypt challenge with password using DES */

      if (password == NULL || password[0] == '\0')
        {
          printf("vncviewer: WARNING: VNC Auth required but no password\n");
        }
      else
        {
          vnc_encrypt_challenge(challenge, password);
        }

      ret = rfb_send_exact(sockfd, challenge, 16);
      if (ret < 0)
        {
          return ret;
        }
    }

  /* Receive SecurityResult (for both None and VNC Auth in RFB 3.8) */

  ret = rfb_recv_exact(sockfd, &result, 4);
  if (ret < 0)
    {
      return ret;
    }

  result = ntohl(result);
  if (result != 0)
    {
      printf("vncviewer: authentication failed (result=%u)\n",
             (unsigned)result);
      return -EACCES;
    }

  printf("vncviewer: authentication OK\n");
  return OK;
}

/****************************************************************************
 * Name: rfb_client_init
 *
 * Description:
 *   Send ClientInit and receive ServerInit.
 *
 ****************************************************************************/

static int rfb_client_init(int sockfd, struct rfb_conn_s *conn)
{
  uint8_t shared = 1;  /* shared session */
  uint8_t buf[24];
  uint32_t name_len;
  int ret;

  /* Send ClientInit */

  ret = rfb_send_exact(sockfd, &shared, 1);
  if (ret < 0)
    {
      return ret;
    }

  /* Receive ServerInit: 2+2+16+4 = 24 bytes header + name */

  ret = rfb_recv_exact(sockfd, buf, 24);
  if (ret < 0)
    {
      return ret;
    }

  conn->fb_width  = ntohs(*(uint16_t *)&buf[0]);
  conn->fb_height = ntohs(*(uint16_t *)&buf[2]);

  /* bytes 4..19 = server pixel format (we'll override it) */

  name_len = ntohl(*(uint32_t *)&buf[20]);
  if (name_len > sizeof(conn->name) - 1)
    {
      name_len = sizeof(conn->name) - 1;
    }

  if (name_len > 0)
    {
      ret = rfb_recv_exact(sockfd, conn->name, name_len);
      if (ret < 0)
        {
          return ret;
        }
    }

  conn->name[name_len] = '\0';

  printf("vncviewer: server desktop: \"%s\" %ux%u\n",
         conn->name, conn->fb_width, conn->fb_height);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rfb_connect
 ****************************************************************************/

int rfb_connect(const char *host, uint16_t port)
{
  struct sockaddr_in addr;
  int sockfd;
  int ret;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    {
      printf("vncviewer: socket() failed: %d\n", errno);
      return -errno;
    }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(port);

  ret = inet_pton(AF_INET, host, &addr.sin_addr);
  if (ret <= 0)
    {
      printf("vncviewer: invalid address: %s\n", host);
      close(sockfd);
      return -EINVAL;
    }

  printf("vncviewer: connecting to %s:%u...\n", host, port);

  ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
    {
      printf("vncviewer: connect() failed: %d\n", errno);
      close(sockfd);
      return -errno;
    }

  printf("vncviewer: connected\n");
  return sockfd;
}

/****************************************************************************
 * Name: rfb_handshake
 ****************************************************************************/

int rfb_handshake(struct rfb_conn_s *conn, const char *password)
{
  int ret;

  ret = rfb_negotiate_version(conn->sockfd);
  if (ret < 0)
    {
      return ret;
    }

  ret = rfb_negotiate_security(conn->sockfd, password);
  if (ret < 0)
    {
      return ret;
    }

  ret = rfb_client_init(conn->sockfd, conn);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: rfb_set_pixel_format
 ****************************************************************************/

int rfb_set_pixel_format(struct rfb_conn_s *conn, uint8_t fmt)
{
  uint8_t msg[20];
  uint8_t bits;
  uint8_t depth;
  uint16_t rmax;
  uint16_t gmax;
  uint16_t bmax;
  uint8_t rshift;
  uint8_t gshift;
  uint8_t bshift;

  switch (fmt)
    {
      case 11: /* FB_FMT_RGB16_565 */
        bits   = 16; depth  = 16;
        rmax   = 31; gmax   = 63; bmax   = 31;
        rshift = 11; gshift = 5;  bshift = 0;
        conn->bpp = 2;
        break;

      case 10: /* FB_FMT_RGB16_555 */
        bits   = 16; depth  = 15;
        rmax   = 31; gmax   = 31; bmax   = 31;
        rshift = 10; gshift = 5;  bshift = 0;
        conn->bpp = 2;
        break;

      case 12: /* FB_FMT_RGB24 */
        bits   = 32; depth  = 24;
        rmax   = 255; gmax  = 255; bmax  = 255;
        rshift = 16; gshift = 8;  bshift = 0;
        conn->bpp = 4;
        break;

      case 13: /* FB_FMT_RGB32 */
        bits   = 32; depth  = 24;
        rmax   = 255; gmax  = 255; bmax  = 255;
        rshift = 16; gshift = 8;  bshift = 0;
        conn->bpp = 4;
        break;

      default:
        printf("vncviewer: unsupported LCD format %d\n", fmt);
        return -ENOTSUP;
    }

  memset(msg, 0, sizeof(msg));

  /* Message type + 3 bytes padding */

  msg[0] = RFB_SET_PIXEL_FORMAT;

  /* Pixel format at offset 4 */

  msg[4]  = bits;          /* bits-per-pixel */
  msg[5]  = depth;         /* depth */
  msg[6]  = 0;             /* big-endian = false */
  msg[7]  = 1;             /* true-color = true */

  msg[8]  = (rmax >> 8) & 0xff;
  msg[9]  = rmax & 0xff;
  msg[10] = (gmax >> 8) & 0xff;
  msg[11] = gmax & 0xff;
  msg[12] = (bmax >> 8) & 0xff;
  msg[13] = bmax & 0xff;

  msg[14] = rshift;
  msg[15] = gshift;
  msg[16] = bshift;

  return rfb_send_exact(conn->sockfd, msg, sizeof(msg));
}

/****************************************************************************
 * Name: rfb_set_encodings
 ****************************************************************************/

int rfb_set_encodings(struct rfb_conn_s *conn)
{
  uint8_t msg[8];

  msg[0] = RFB_SET_ENCODINGS;
  msg[1] = 0;  /* padding */

  /* number-of-encodings = 1 */

  msg[2] = 0;
  msg[3] = 1;

  /* encoding: Raw = 0 */

  msg[4] = 0;
  msg[5] = 0;
  msg[6] = 0;
  msg[7] = 0;

  return rfb_send_exact(conn->sockfd, msg, sizeof(msg));
}

/****************************************************************************
 * Name: rfb_request_update
 ****************************************************************************/

int rfb_request_update(struct rfb_conn_s *conn, bool incremental,
                       uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint8_t msg[10];

  msg[0] = RFB_FB_UPDATE_REQUEST;
  msg[1] = incremental ? 1 : 0;
  *(uint16_t *)&msg[2] = htons(x);
  *(uint16_t *)&msg[4] = htons(y);
  *(uint16_t *)&msg[6] = htons(w);
  *(uint16_t *)&msg[8] = htons(h);

  return rfb_send_exact(conn->sockfd, msg, sizeof(msg));
}

/****************************************************************************
 * Name: rfb_recv_update
 ****************************************************************************/

int rfb_recv_update(struct rfb_conn_s *conn, rfb_rect_cb_t rect_cb,
                    void *arg)
{
  uint8_t msg_type;
  int ret;

  /* Read message type */

  ret = rfb_recv_exact(conn->sockfd, &msg_type, 1);
  if (ret < 0)
    {
      return ret;
    }

  switch (msg_type)
    {
      case RFB_FB_UPDATE:
        {
          uint8_t hdr[3];
          uint16_t num_rects;
          uint16_t i;

          /* Read padding(1) + number-of-rectangles(2) */

          ret = rfb_recv_exact(conn->sockfd, hdr, 3);
          if (ret < 0)
            {
              return ret;
            }

          num_rects = ntohs(*(uint16_t *)&hdr[1]);

          for (i = 0; i < num_rects; i++)
            {
              struct rfb_rect_hdr_s rect;
              uint8_t rect_buf[12];
              uint32_t row_bytes;
              uint32_t row;

              /* Read rectangle header: x(2)+y(2)+w(2)+h(2)+encoding(4) */

              ret = rfb_recv_exact(conn->sockfd, rect_buf, 12);
              if (ret < 0)
                {
                  return ret;
                }

              rect.x        = ntohs(*(uint16_t *)&rect_buf[0]);
              rect.y        = ntohs(*(uint16_t *)&rect_buf[2]);
              rect.w        = ntohs(*(uint16_t *)&rect_buf[4]);
              rect.h        = ntohs(*(uint16_t *)&rect_buf[6]);
              rect.encoding = (int32_t)ntohl(*(uint32_t *)&rect_buf[8]);

              if (rect.encoding != RFB_ENCODING_RAW)
                {
                  printf("vncviewer: unsupported encoding %d\n",
                         (int)rect.encoding);
                  return -ENOTSUP;
                }

              /* Read pixel data row by row to limit memory usage */

              row_bytes = rect.w * conn->bpp;

              FAR uint8_t *rowbuf = malloc(row_bytes);
              if (rowbuf == NULL)
                {
                  return -ENOMEM;
                }

              for (row = 0; row < rect.h; row++)
                {
                  struct rfb_rect_hdr_s row_rect;

                  ret = rfb_recv_exact(conn->sockfd, rowbuf, row_bytes);
                  if (ret < 0)
                    {
                      free(rowbuf);
                      return ret;
                    }

                  /* Callback with single-row rectangle */

                  row_rect.x = rect.x;
                  row_rect.y = rect.y + row;
                  row_rect.w = rect.w;
                  row_rect.h = 1;
                  row_rect.encoding = RFB_ENCODING_RAW;

                  if (rect_cb)
                    {
                      rect_cb(&row_rect, rowbuf, arg);
                    }
                }

              free(rowbuf);
            }
        }
        break;

      case RFB_SET_COLORMAP:
        {
          /* Skip: padding(1) + first-color(2) + num-colors(2) */

          uint8_t cm_hdr[5];
          uint16_t num_colors;

          ret = rfb_recv_exact(conn->sockfd, cm_hdr, 5);
          if (ret < 0)
            {
              return ret;
            }

          num_colors = ntohs(*(uint16_t *)&cm_hdr[3]);

          /* Skip color entries: num_colors * 6 bytes (R+G+B each 2B) */

          uint32_t skip = num_colors * 6;
          uint8_t tmp[256];

          while (skip > 0)
            {
              uint32_t chunk = skip > sizeof(tmp) ? sizeof(tmp) : skip;
              ret = rfb_recv_exact(conn->sockfd, tmp, chunk);
              if (ret < 0)
                {
                  return ret;
                }

              skip -= chunk;
            }
        }
        break;

      case RFB_BELL:

        /* No payload */

        break;

      case RFB_SERVER_CUT_TEXT:
        {
          uint8_t ct_hdr[7];
          uint32_t text_len;

          ret = rfb_recv_exact(conn->sockfd, ct_hdr, 7);
          if (ret < 0)
            {
              return ret;
            }

          text_len = ntohl(*(uint32_t *)&ct_hdr[3]);

          /* Skip text content */

          uint8_t tmp[256];

          while (text_len > 0)
            {
              uint32_t chunk = text_len > sizeof(tmp)
                               ? sizeof(tmp) : text_len;
              ret = rfb_recv_exact(conn->sockfd, tmp, chunk);
              if (ret < 0)
                {
                  return ret;
                }

              text_len -= chunk;
            }
        }
        break;

      default:
        printf("vncviewer: unknown message type %u\n", msg_type);
        return -EPROTO;
    }

  return OK;
}
