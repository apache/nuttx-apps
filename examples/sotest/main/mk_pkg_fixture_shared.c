/****************************************************************************
 * apps/examples/sotest/main/mk_pkg_fixture_shared.c
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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct sha256_state_s
{
  uint32_t state[8];
  uint64_t bitcount;
  uint8_t buffer[SHA256_BLOCK_SIZE];
  size_t buffer_len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint32_t g_sha256_init[8] =
{
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
  0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint32_t g_sha256_k[64] =
{
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
  0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
  0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
  0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
  0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
  0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static uint32_t sha256_rotr(uint32_t value, unsigned int bits)
{
  return (value >> bits) | (value << (32 - bits));
}

static uint32_t sha256_ch(uint32_t x, uint32_t y, uint32_t z)
{
  return (x & y) ^ (~x & z);
}

static uint32_t sha256_maj(uint32_t x, uint32_t y, uint32_t z)
{
  return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sha256_bs0(uint32_t x)
{
  return sha256_rotr(x, 2) ^ sha256_rotr(x, 13) ^ sha256_rotr(x, 22);
}

static uint32_t sha256_bs1(uint32_t x)
{
  return sha256_rotr(x, 6) ^ sha256_rotr(x, 11) ^ sha256_rotr(x, 25);
}

static uint32_t sha256_ss0(uint32_t x)
{
  return sha256_rotr(x, 7) ^ sha256_rotr(x, 18) ^ (x >> 3);
}

static uint32_t sha256_ss1(uint32_t x)
{
  return sha256_rotr(x, 17) ^ sha256_rotr(x, 19) ^ (x >> 10);
}

static uint32_t sha256_load32(const uint8_t *src)
{
  return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) |
         ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}

static void sha256_store32(uint8_t *dst, uint32_t value)
{
  dst[0] = (uint8_t)(value >> 24);
  dst[1] = (uint8_t)(value >> 16);
  dst[2] = (uint8_t)(value >> 8);
  dst[3] = (uint8_t)value;
}

static void sha256_transform(struct sha256_state_s *ctx,
                             const uint8_t block[SHA256_BLOCK_SIZE])
{
  uint32_t schedule[64];
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint32_t f;
  uint32_t g;
  uint32_t h;
  int i;

  for (i = 0; i < 16; i++)
    {
      schedule[i] = sha256_load32(block + i * 4);
    }

  for (; i < 64; i++)
    {
      schedule[i] = sha256_ss1(schedule[i - 2]) + schedule[i - 7] +
                    sha256_ss0(schedule[i - 15]) + schedule[i - 16];
    }

  a = ctx->state[0];
  b = ctx->state[1];
  c = ctx->state[2];
  d = ctx->state[3];
  e = ctx->state[4];
  f = ctx->state[5];
  g = ctx->state[6];
  h = ctx->state[7];

  for (i = 0; i < 64; i++)
    {
      uint32_t t1 = h + sha256_bs1(e) + sha256_ch(e, f, g) + g_sha256_k[i] +
                    schedule[i];
      uint32_t t2 = sha256_bs0(a) + sha256_maj(a, b, c);

      h = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

static void sha256_init(struct sha256_state_s *ctx)
{
  memcpy(ctx->state, g_sha256_init, sizeof(g_sha256_init));
  ctx->bitcount = 0;
  ctx->buffer_len = 0;
}

static void sha256_update(struct sha256_state_s *ctx, const uint8_t *data,
                          size_t len)
{
  ctx->bitcount += (uint64_t)len * 8;

  while (len > 0)
    {
      size_t space = SHA256_BLOCK_SIZE - ctx->buffer_len;
      size_t chunk = len < space ? len : space;

      memcpy(ctx->buffer + ctx->buffer_len, data, chunk);
      ctx->buffer_len += chunk;
      data += chunk;
      len -= chunk;

      if (ctx->buffer_len == SHA256_BLOCK_SIZE)
        {
          sha256_transform(ctx, ctx->buffer);
          ctx->buffer_len = 0;
        }
    }
}

static void sha256_final(struct sha256_state_s *ctx,
                         uint8_t digest[SHA256_DIGEST_SIZE])
{
  uint8_t length_bytes[8];
  uint8_t padding[SHA256_BLOCK_SIZE] =
  {
    0x80
  };

  int i;

  for (i = 0; i < 8; i++)
    {
      length_bytes[7 - i] = (uint8_t)(ctx->bitcount >> (i * 8));
    }

  if (ctx->buffer_len < 56)
    {
      sha256_update(ctx, padding, 56 - ctx->buffer_len);
    }
  else
    {
      sha256_update(ctx, padding, SHA256_BLOCK_SIZE - ctx->buffer_len);
      sha256_update(ctx, padding + 1, 56);
    }

  sha256_update(ctx, length_bytes, sizeof(length_bytes));

  for (i = 0; i < 8; i++)
    {
      sha256_store32(digest + i * 4, ctx->state[i]);
    }
}

static int sha256_file(const char *path, char *digest_hex, size_t digest_len)
{
  struct sha256_state_s ctx;
  uint8_t digest[SHA256_DIGEST_SIZE];
  uint8_t buffer[4096];
  static const char g_hex[] = "0123456789abcdef";
  FILE *stream;
  size_t nread;
  int i;

  if (digest_len < SHA256_DIGEST_SIZE * 2 + 1)
    {
      fprintf(stderr, "digest buffer is too small\n");
      return -1;
    }

  stream = fopen(path, "rb");
  if (stream == NULL)
    {
      fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
      return -1;
    }

  sha256_init(&ctx);

  while ((nread = fread(buffer, 1, sizeof(buffer), stream)) > 0)
    {
      sha256_update(&ctx, buffer, nread);
    }

  if (ferror(stream) != 0)
    {
      fprintf(stderr, "failed to read %s: %s\n", path, strerror(errno));
      fclose(stream);
      return -1;
    }

  fclose(stream);
  sha256_final(&ctx, digest);

  for (i = 0; i < SHA256_DIGEST_SIZE; i++)
    {
      digest_hex[i * 2] = g_hex[digest[i] >> 4];
      digest_hex[i * 2 + 1] = g_hex[digest[i] & 0x0f];
    }

  digest_hex[SHA256_DIGEST_SIZE * 2] = '\0';
  return 0;
}

static int write_text_file(const char *path, const char *content)
{
  FILE *stream = fopen(path, "w");

  if (stream == NULL)
    {
      fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
      return -1;
    }

  if (fputs(content, stream) == EOF)
    {
      fprintf(stderr, "failed to write %s: %s\n", path, strerror(errno));
      fclose(stream);
      return -1;
    }

  if (fclose(stream) != 0)
    {
      fprintf(stderr, "failed to close %s: %s\n", path, strerror(errno));
      return -1;
    }

  return 0;
}

static int write_index_file(const char *path, const char *arch,
                            const char *compat, const char *modprint_digest,
                            const char *sotest_digest)
{
  FILE *stream = fopen(path, "w");

  if (stream == NULL)
    {
      fprintf(stderr, "failed to open %s: %s\n", path, strerror(errno));
      return -1;
    }

  if (fprintf(stream,
              "{\"_nxpkg_source\":\"/etc/nxpkg/index.json\","
              "\"packages\":["
              "{\"name\":\"modprint\",\"version\":\"1.0.0\",\"arch\":\"%s\","
              "\"compat\":\"%s\","
              "\"artifact\":\"modprint\","
              "\"sha256\":\"%s\",\"type\":\"shared-lib\"},"
              "{\"name\":\"modprint\",\"version\":\"9.9.9\","
              "\"arch\":\"arm\","
              "\"compat\":\"stm32f4discovery\","
              "\"artifact\":\"modprint\","
              "\"sha256\":\"%s\",\"type\":\"shared-lib\"},"
              "{\"name\":\"sotest\",\"version\":\"1.0.0\",\"arch\":\"%s\","
              "\"compat\":\"%s\",\"artifact\":\"sotest\","
              "\"sha256\":\"%s\",\"type\":\"shared-lib\"},"
              "{\"name\":\"sotest\",\"version\":\"9.9.9\",\"arch\":\"arm\","
              "\"compat\":\"stm32f4discovery\","
              "\"artifact\":\"sotest\","
              "\"sha256\":\"%s\",\"type\":\"shared-lib\"}"
              "]}\n",
              arch, compat, modprint_digest, modprint_digest, arch, compat,
              sotest_digest, sotest_digest) < 0)
    {
      fprintf(stderr, "failed to write %s: %s\n", path, strerror(errno));
      fclose(stream);
      return -1;
    }

  if (fclose(stream) != 0)
    {
      fprintf(stderr, "failed to close %s: %s\n", path, strerror(errno));
      return -1;
    }

  return 0;
}

static void usage(const char *progname)
{
  fprintf(stderr,
          "usage: %s <modprint-bin> <sotest-bin> <shared-index-json> "
          "<pkgsotest-nsh> <arch> <compat>\n",
          progname);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  char modprint_digest[SHA256_DIGEST_SIZE * 2 + 1];
  char sotest_digest[SHA256_DIGEST_SIZE * 2 + 1];
  static const char g_pkgsotest[] =
    "mount -t tmpfs /etc\n"
    "mount -t tmpfs /var\n"
    "mkdir /etc/nxpkg\n"
    "cp /mnt/sotest/romfs/shared-index.json /etc/nxpkg/index.json\n"
    "cp /mnt/sotest/romfs/modprint /etc/nxpkg/modprint\n"
    "cp /mnt/sotest/romfs/sotest /etc/nxpkg/sotest\n"
    "mkdir /var/lib\n"
    "mkdir /var/lib/nxpkg\n"
    "cp /mnt/sotest/romfs/shared-index.json /var/lib/nxpkg/index.jsn\n"
    "nxpkg install modprint\n"
    "nxpkg install sotest\n"
    "cp /var/lib/nxpkg/pkgs/modprint/1.0.0/modprint /etc/m\n"
    "cp /var/lib/nxpkg/pkgs/sotest/1.0.0/sotest /etc/s\n"
    "sotest /etc/m /etc/s\n"
    "nxpkg list\n";

  if (argc != 7)
    {
      usage(argv[0]);
      return EXIT_FAILURE;
    }

  if (sha256_file(argv[1], modprint_digest, sizeof(modprint_digest)) < 0 ||
      sha256_file(argv[2], sotest_digest, sizeof(sotest_digest)) < 0 ||
      write_index_file(argv[3], argv[5], argv[6], modprint_digest,
                       sotest_digest) < 0 ||
      write_text_file(argv[4], g_pkgsotest) < 0)
    {
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
