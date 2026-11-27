/****************************************************************************
 * apps/testing/drivers/crypto/async.c
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

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include <crypto/cryptodev.h>
#include <crypto/sha2.h>

/* Every result is checked against a known-answer vector instead of only a
 * self round trip, so a backend that is wrong in both directions still
 * fails.  The vectors are RFC 3602 (AES-CBC), FIPS 180-2 (SHA-256),
 * NIST SP 800-38B (CMAC) and NIST SP 800-38D test case 4 (GCM/GMAC).
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AES_MULTIBLOCK_LEN      64  /* 4 AES blocks */
#define SHA_LARGE_TOTAL         256 /* 256 bytes total for multi-update */
#define SHA_LARGE_CHUNK         64  /* 64 bytes per update */
#define CMAC_TAG_LEN            16
#define GCM_IV_LEN              12
#define GCM_AAD_LEN             20
#define GCM_DATA_LEN            60
#define GCM_TAG_LEN             16
#define ASYNC_POLL_TIMEOUT_MS   10000
#define CONCURRENT_ROUNDS       10

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct async_ctx_s
{
  int fd;                /* /dev/crypto */
  int cryptodev_fd;      /* cloned descriptor */
};

struct worker_arg_s
{
  int rounds;
  int result;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* RFC 3602 AES-CBC single-block test vector */

static const unsigned char g_aes_key[16] =
{
  0x06, 0xa9, 0x21, 0x40, 0x36, 0xb8, 0xa1, 0x5b,
  0x51, 0x2e, 0x03, 0xd5, 0x34, 0x12, 0x00, 0x06
};

static const unsigned char g_aes_iv[16] =
{
  0x3d, 0xaf, 0xba, 0x42, 0x9d, 0x9e, 0xb4, 0x30,
  0xb4, 0x22, 0xda, 0x80, 0x2c, 0x9f, 0xac, 0x41
};

static const unsigned char g_aes_plain[16] = "Single block msg";

static const unsigned char g_aes_cipher[16] =
{
  0xe3, 0x53, 0x77, 0x9c, 0x10, 0x79, 0xae, 0xb8,
  0x27, 0x08, 0x94, 0x2d, 0xbe, 0x77, 0x18, 0x1a
};

/* AES-128-CBC of 64 incrementing bytes under g_aes_key / g_aes_iv */

static const unsigned char g_aes_plain_64[AES_MULTIBLOCK_LEN] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
};

static const unsigned char g_aes_cipher_64[AES_MULTIBLOCK_LEN] =
{
  0x71, 0x43, 0x73, 0xe9, 0x99, 0x1e, 0x8a, 0x58,
  0xf7, 0x9e, 0xfa, 0x62, 0xb4, 0x6f, 0x76, 0x52,
  0xfb, 0xfa, 0x5d, 0xe5, 0x96, 0xb9, 0x3a, 0xca,
  0xaf, 0xbd, 0xb2, 0x41, 0x23, 0x11, 0xac, 0x13,
  0xbd, 0xa9, 0x6d, 0x03, 0x0b, 0xa5, 0xe6, 0x07,
  0x61, 0x35, 0x78, 0xcc, 0xb2, 0xad, 0x23, 0x26,
  0x68, 0x3b, 0x3b, 0x10, 0x41, 0x28, 0x94, 0x35,
  0xa3, 0x82, 0x42, 0xd6, 0x4e, 0x96, 0x83, 0x75
};

/* FIPS 180-2 "abc" SHA-256 digest, and the digest of 256 bytes of 0xaa
 * fed as four 64-byte updates.
 */

static const unsigned char g_sha_msg[] = "abc";

static const unsigned char g_sha_digest[SHA256_DIGEST_LENGTH] =
{
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
  0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
  0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

static const unsigned char g_sha_large_digest[SHA256_DIGEST_LENGTH] =
{
  0xfd, 0x4c, 0x55, 0xf0, 0xc4, 0x80, 0x8b, 0x05,
  0x02, 0xe8, 0xd8, 0x8b, 0x84, 0xc8, 0x4f, 0x80,
  0xe3, 0x8b, 0x4c, 0x8c, 0xd3, 0x54, 0x1c, 0x5a,
  0x7a, 0x32, 0x8c, 0x41, 0xb9, 0x24, 0xf9, 0x45
};

/* NIST SP 800-38B AES-128 CMAC example 2 (len=16) */

static const unsigned char g_cmac_key[16] =
{
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
};

static const unsigned char g_cmac_msg[16] =
{
  0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
  0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
};

static const unsigned char g_cmac_tag[CMAC_TAG_LEN] =
{
  0x07, 0x0a, 0x16, 0xb4, 0x6b, 0x4d, 0x41, 0x44,
  0xf7, 0x9b, 0xdd, 0x9d, 0xd0, 0x4a, 0x28, 0x7c
};

/* AES-128-GCM, NIST SP 800-38D test case 4.  The data phase runs
 * synchronously; the final request that produces the tag is submitted
 * asynchronously and retrieved through CIOCCRYPTRET.  The GMAC tag is
 * the same key, IV and AAD with no plaintext.
 */

static const unsigned char g_gcm_key[16] =
{
  0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
  0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};

static const unsigned char g_gcm_iv[GCM_IV_LEN] =
{
  0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
  0xde, 0xca, 0xf8, 0x88
};

static const unsigned char g_gcm_aad[GCM_AAD_LEN] =
{
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xab, 0xad, 0xda, 0xd2
};

static const unsigned char g_gcm_plain[GCM_DATA_LEN] =
{
  0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
  0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
  0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
  0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
  0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
  0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
  0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
  0xba, 0x63, 0x7b, 0x39
};

static const unsigned char g_gcm_cipher[GCM_DATA_LEN] =
{
  0x42, 0x83, 0x1e, 0xc2, 0x21, 0x77, 0x74, 0x24,
  0x4b, 0x72, 0x21, 0xb7, 0x84, 0xd0, 0xd4, 0x9c,
  0xe3, 0xaa, 0x21, 0x2f, 0x2c, 0x02, 0xa4, 0xe0,
  0x35, 0xc1, 0x7e, 0x23, 0x29, 0xac, 0xa1, 0x2e,
  0x21, 0xd5, 0x14, 0xb2, 0x54, 0x66, 0x93, 0x1c,
  0x7d, 0x8f, 0x6a, 0x5a, 0xac, 0x84, 0xaa, 0x05,
  0x1b, 0xa3, 0x0b, 0x39, 0x6a, 0x0a, 0xac, 0x97,
  0x3d, 0x58, 0xe0, 0x91
};

static const unsigned char g_gcm_tag[GCM_TAG_LEN] =
{
  0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
  0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47
};

static const unsigned char g_gmac_tag[GCM_TAG_LEN] =
{
  0x34, 0x64, 0x34, 0xfd, 0x51, 0xd5, 0xcd, 0x0c,
  0x58, 0x87, 0xec, 0x63, 0xe3, 0x9b, 0x90, 0x7a
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int async_open(FAR struct async_ctx_s *ctx)
{
  ctx->fd = -1;
  ctx->cryptodev_fd = -1;

  ctx->fd = open("/dev/crypto", O_RDWR, 0);
  if (ctx->fd < 0)
    {
      warn("/dev/crypto");
      return -1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) < 0)
    {
      warn("CRIOGET");
      close(ctx->fd);
      ctx->fd = -1;
      return -1;
    }

  return 0;
}

static void async_close(FAR struct async_ctx_s *ctx)
{
  if (ctx->cryptodev_fd >= 0)
    {
      close(ctx->cryptodev_fd);
    }

  if (ctx->fd >= 0)
    {
      close(ctx->fd);
    }

  ctx->fd = -1;
  ctx->cryptodev_fd = -1;
}

/* Block on poll(POLLIN) until the cryptodev has at least one completed
 * asynchronous request ready to retrieve.
 */

static int async_wait_ready(int cryptodev_fd)
{
  struct pollfd pfd;
  int ret;

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = cryptodev_fd;
  pfd.events = POLLIN;

  ret = poll(&pfd, 1, ASYNC_POLL_TIMEOUT_MS);
  if (ret < 0)
    {
      warn("poll");
      return -1;
    }

  if (ret == 0)
    {
      warnx("asynchronous completion timeout");
      return -1;
    }

  if (!(pfd.revents & POLLIN))
    {
      warnx("poll revents=0x%x", pfd.revents);
      return -1;
    }

  return 0;
}

/* Open a session, submit CIOCCRYPT with COP_FLAG_CBIMM, poll for
 * completion, retrieve the result through CIOCCRYPTRET and compare it
 * with the expected output.
 */

static int async_cbc(FAR struct async_ctx_s *ctx, uint32_t reqid,
                     FAR const unsigned char *plain, size_t len,
                     FAR const unsigned char *expect,
                     FAR unsigned char *out)
{
  struct session_op session;
  struct crypt_op cop;
  struct crypt_op cret;
  unsigned char iv[16];

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CBC;
  session.key = (caddr_t)g_aes_key;
  session.keylen = sizeof(g_aes_key);
  session.op = COP_ENCRYPT;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      warn("CIOCGSESSION");
      return -1;
    }

  memcpy(iv, g_aes_iv, 16);

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.op = COP_ENCRYPT;
  cop.flags = COP_FLAG_CBIMM;
  cop.reqid = reqid;
  cop.len = len;
  cop.olen = len;
  cop.ivlen = 16;
  cop.src = (caddr_t)plain;
  cop.dst = (caddr_t)out;
  cop.iv = (caddr_t)iv;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT (async)");
      goto err;
    }

  if (async_wait_ready(ctx->cryptodev_fd) < 0)
    {
      goto err;
    }

  memset(&cret, 0, sizeof(cret));
  cret.reqid = reqid;
  cret.dst = (caddr_t)out;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPTRET, &cret) < 0)
    {
      warn("CIOCCRYPTRET");
      goto err;
    }

  if (memcmp(out, expect, len) != 0)
    {
      warnx("ciphertext mismatch");
      goto err;
    }

  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return 0;

err:
  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return -1;
}

/* Feed the message synchronously in 64-byte chunks with COP_FLAG_UPDATE,
 * then submit the final request asynchronously to get the digest.
 */

static int async_sha256(FAR struct async_ctx_s *ctx, uint32_t reqid,
                        FAR const unsigned char *msg, size_t len,
                        FAR const unsigned char *expect,
                        FAR unsigned char *digest)
{
  struct session_op session;
  struct crypt_op cop;
  struct crypt_op cret;
  size_t offset;

  memset(&session, 0, sizeof(session));
  session.mac = CRYPTO_SHA2_256;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      warn("CIOCGSESSION (sha)");
      return -1;
    }

  for (offset = 0; offset < len; offset += SHA_LARGE_CHUNK)
    {
      size_t chunk = MIN(SHA_LARGE_CHUNK, len - offset);

      memset(&cop, 0, sizeof(cop));
      cop.ses = session.ses;
      cop.op = COP_ENCRYPT;
      cop.flags = COP_FLAG_UPDATE;
      cop.len = chunk;
      cop.src = (caddr_t)(msg + offset);
      if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
        {
          warn("CIOCCRYPT (sha update)");
          goto err;
        }
    }

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.flags = COP_FLAG_CBIMM;
  cop.reqid = reqid;
  cop.olen = SHA256_DIGEST_LENGTH;
  cop.mac = (caddr_t)digest;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT (sha final async)");
      goto err;
    }

  if (async_wait_ready(ctx->cryptodev_fd) < 0)
    {
      goto err;
    }

  memset(&cret, 0, sizeof(cret));
  cret.reqid = reqid;
  cret.mac = (caddr_t)digest;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPTRET, &cret) < 0)
    {
      warn("CIOCCRYPTRET (sha)");
      goto err;
    }

  if (memcmp(digest, expect, SHA256_DIGEST_LENGTH) != 0)
    {
      warnx("digest mismatch");
      goto err;
    }

  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return 0;

err:
  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return -1;
}

/* Feed the message synchronously, then submit the final request
 * asynchronously to get the CMAC tag.
 */

static int async_cmac(FAR struct async_ctx_s *ctx, uint32_t reqid,
                      FAR const unsigned char *msg, size_t len,
                      FAR unsigned char *tag)
{
  struct session_op session;
  struct crypt_op cop;
  struct crypt_op cret;

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CMAC;
  session.key = (caddr_t)g_cmac_key;
  session.keylen = sizeof(g_cmac_key);
  session.mac = CRYPTO_AES_128_CMAC;
  session.mackey = (caddr_t)g_cmac_key;
  session.mackeylen = sizeof(g_cmac_key);
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      warn("CIOCGSESSION (cmac)");
      return -1;
    }

  if (len > 0)
    {
      memset(&cop, 0, sizeof(cop));
      cop.ses = session.ses;
      cop.flags = COP_FLAG_UPDATE;
      cop.len = len;
      cop.src = (caddr_t)msg;
      if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
        {
          warn("CIOCCRYPT (cmac update)");
          goto err;
        }
    }

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.flags = COP_FLAG_CBIMM;
  cop.reqid = reqid;
  cop.olen = CMAC_TAG_LEN;
  cop.mac = (caddr_t)tag;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT (cmac final async)");
      goto err;
    }

  if (async_wait_ready(ctx->cryptodev_fd) < 0)
    {
      goto err;
    }

  memset(&cret, 0, sizeof(cret));
  cret.reqid = reqid;
  cret.mac = (caddr_t)tag;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPTRET, &cret) < 0)
    {
      warn("CIOCCRYPTRET (cmac)");
      goto err;
    }

  if (memcmp(tag, g_cmac_tag, CMAC_TAG_LEN) != 0)
    {
      warnx("cmac mismatch");
      goto err;
    }

  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return 0;

err:
  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return -1;
}

/* Run the three-request AEAD sequence of aesgcm.c: the IV and the AAD
 * first, the payload next, both synchronous, and the tag last,
 * asynchronous.  With aad_only the payload phase is skipped and the tag
 * is a GMAC.
 */

static int async_aead(FAR struct async_ctx_s *ctx, uint32_t reqid,
                      bool aad_only, FAR unsigned char *cipher,
                      FAR unsigned char *tag)
{
  struct session_op session;
  struct crypt_op cop;
  struct crypt_op cret;
  unsigned char iv[GCM_IV_LEN];

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_GCM_16;
  session.key = (caddr_t)g_gcm_key;
  session.keylen = sizeof(g_gcm_key);
  session.mac = CRYPTO_AES_128_GMAC;
  session.mackey = (caddr_t)g_gcm_key;
  session.mackeylen = sizeof(g_gcm_key);
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      warn("CIOCGSESSION (aead)");
      return -1;
    }

  memcpy(iv, g_gcm_iv, GCM_IV_LEN);

  /* UPDATE_AAD - IV + AAD, no data */

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.op = COP_ENCRYPT;
  cop.flags = COP_FLAG_UPDATE_AAD;
  cop.iv = (caddr_t)iv;
  cop.ivlen = GCM_IV_LEN;
  cop.aad = (caddr_t)g_gcm_aad;
  cop.aadlen = GCM_AAD_LEN;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT UPDATE_AAD");
      goto err;
    }

  /* UPDATE - data only, no IV, no AAD */

  if (!aad_only)
    {
      memset(&cop, 0, sizeof(cop));
      cop.ses = session.ses;
      cop.op = COP_ENCRYPT;
      cop.flags = COP_FLAG_UPDATE;
      cop.len = GCM_DATA_LEN;
      cop.src = (caddr_t)g_gcm_plain;
      cop.dst = (caddr_t)cipher;
      if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
        {
          warn("CIOCCRYPT UPDATE");
          goto err;
        }
    }

  /* FINAL - asynchronous, produces the tag */

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.op = COP_ENCRYPT;
  cop.flags = COP_FLAG_CBIMM;
  cop.reqid = reqid;
  cop.olen = GCM_TAG_LEN;
  cop.mac = (caddr_t)tag;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT FINAL (async)");
      goto err;
    }

  if (async_wait_ready(ctx->cryptodev_fd) < 0)
    {
      goto err;
    }

  memset(&cret, 0, sizeof(cret));
  cret.reqid = reqid;
  cret.mac = (caddr_t)tag;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPTRET, &cret) < 0)
    {
      warn("CIOCCRYPTRET (aead)");
      goto err;
    }

  if (!aad_only && memcmp(cipher, g_gcm_cipher, GCM_DATA_LEN) != 0)
    {
      warnx("ciphertext mismatch");
      goto err;
    }

  if (memcmp(tag, aad_only ? g_gmac_tag : g_gcm_tag, GCM_TAG_LEN) != 0)
    {
      warnx("tag mismatch");
      goto err;
    }

  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return 0;

err:
  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return -1;
}

/* Submit an asynchronous AES-CBC op and cancel it by reqid before
 * collecting.  Whether the op was still in flight or already completed,
 * the result is dropped and a later retrieval reports EAGAIN.
 * Cancelling a reqid that was never submitted fails with ENOENT.
 */

static int async_cancel(FAR struct async_ctx_s *ctx)
{
  struct session_op session;
  struct crypt_op cop;
  unsigned char iv[16];
  unsigned char out[16];

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CBC;
  session.key = (caddr_t)g_aes_key;
  session.keylen = sizeof(g_aes_key);
  session.op = COP_ENCRYPT;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      warn("CIOCGSESSION");
      return -1;
    }

  memcpy(iv, g_aes_iv, 16);

  memset(&cop, 0, sizeof(cop));
  cop.ses = session.ses;
  cop.op = COP_ENCRYPT;
  cop.flags = COP_FLAG_CBIMM;
  cop.reqid = 0x7001;
  cop.len = 16;
  cop.olen = 16;
  cop.ivlen = 16;
  cop.src = (caddr_t)g_aes_plain;
  cop.dst = (caddr_t)out;
  cop.iv = (caddr_t)iv;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT (async)");
      goto err;
    }

  memset(&cop, 0, sizeof(cop));
  cop.flags = COP_FLAG_CANCEL;
  cop.reqid = 0x7001;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) < 0)
    {
      warn("CIOCCRYPT (cancel)");
      goto err;
    }

  memset(&cop, 0, sizeof(cop));
  cop.reqid = 0x7001;
  cop.dst = (caddr_t)out;
  cop.olen = 16;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPTRET, &cop) >= 0 ||
      errno != EAGAIN)
    {
      warnx("cancelled request still retrievable");
      goto err;
    }

  memset(&cop, 0, sizeof(cop));
  cop.flags = COP_FLAG_CANCEL;
  cop.reqid = 0xdead;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cop) >= 0 ||
      errno != ENOENT)
    {
      warnx("cancel of unknown reqid did not report ENOENT");
      goto err;
    }

  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return 0;

err:
  ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses);
  return -1;
}

/* Three workers with private descriptors submit asynchronous requests
 * concurrently; each round is checked against a known answer.
 */

static FAR void *worker_aes(FAR void *arg)
{
  FAR struct worker_arg_s *wa = arg;
  struct async_ctx_s ctx;
  unsigned char cipher[AES_MULTIBLOCK_LEN];
  int i;

  if (async_open(&ctx) < 0)
    {
      wa->result = -1;
      return NULL;
    }

  for (i = 0; i < wa->rounds; i++)
    {
      memset(cipher, 0, sizeof(cipher));
      if (async_cbc(&ctx, 0x10000 + i, g_aes_plain_64,
                    AES_MULTIBLOCK_LEN, g_aes_cipher_64,
                    cipher) != 0)
        {
          wa->result = -1;
          async_close(&ctx);
          return NULL;
        }
    }

  wa->result = 0;
  async_close(&ctx);
  return NULL;
}

static FAR void *worker_sha(FAR void *arg)
{
  FAR struct worker_arg_s *wa = arg;
  struct async_ctx_s ctx;
  unsigned char data[SHA_LARGE_TOTAL];
  unsigned char digest[SHA256_DIGEST_LENGTH];
  int i;

  if (async_open(&ctx) < 0)
    {
      wa->result = -1;
      return NULL;
    }

  memset(data, 0xaa, sizeof(data));

  for (i = 0; i < wa->rounds; i++)
    {
      memset(digest, 0, sizeof(digest));
      if (async_sha256(&ctx, 0x20000 + i, data, SHA_LARGE_TOTAL,
                       g_sha_large_digest, digest) != 0)
        {
          wa->result = -1;
          async_close(&ctx);
          return NULL;
        }
    }

  wa->result = 0;
  async_close(&ctx);
  return NULL;
}

static FAR void *worker_cmac(FAR void *arg)
{
  FAR struct worker_arg_s *wa = arg;
  struct async_ctx_s ctx;
  unsigned char tag[CMAC_TAG_LEN];
  int i;

  if (async_open(&ctx) < 0)
    {
      wa->result = -1;
      return NULL;
    }

  for (i = 0; i < wa->rounds; i++)
    {
      memset(tag, 0, sizeof(tag));
      if (async_cmac(&ctx, 0x40000 + i, g_cmac_msg, 16, tag) != 0)
        {
          wa->result = -1;
          async_close(&ctx);
          return NULL;
        }
    }

  wa->result = 0;
  async_close(&ctx);
  return NULL;
}

static int check_concurrent(void)
{
  pthread_t t_aes;
  pthread_t t_sha;
  pthread_t t_cmac;
  struct worker_arg_s wa_aes;
  struct worker_arg_s wa_sha;
  struct worker_arg_s wa_cmac;

  wa_aes.rounds = CONCURRENT_ROUNDS;
  wa_aes.result = -1;
  wa_sha = wa_aes;
  wa_cmac = wa_aes;

  if (pthread_create(&t_aes, NULL, worker_aes, &wa_aes) != 0 ||
      pthread_create(&t_sha, NULL, worker_sha, &wa_sha) != 0 ||
      pthread_create(&t_cmac, NULL, worker_cmac, &wa_cmac) != 0)
    {
      warn("pthread_create");
      return -1;
    }

  pthread_join(t_aes, NULL);
  pthread_join(t_sha, NULL);
  pthread_join(t_cmac, NULL);

  if (wa_aes.result != 0 || wa_sha.result != 0 || wa_cmac.result != 0)
    {
      warnx("concurrent worker failed");
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  struct async_ctx_s ctx;
  unsigned char cipher[AES_MULTIBLOCK_LEN];
  unsigned char out[GCM_DATA_LEN];
  unsigned char tag[GCM_TAG_LEN];
  unsigned char digest[SHA256_DIGEST_LENGTH];
  unsigned char data[SHA_LARGE_TOTAL];
  int err = 0;
  int ret;

  setbuf(stdout, NULL);

  if (async_open(&ctx) < 0)
    {
      return 1;
    }

  /* AES-CBC, single block and 4 blocks, against RFC 3602 */

  memset(cipher, 0, sizeof(cipher));
  ret = async_cbc(&ctx, 0x1001, g_aes_plain, 16, g_aes_cipher, cipher);
  printf("async aes-cbc %s\n", ret ? "failed" : "success");
  err |= ret;

  ret = async_cbc(&ctx, 0x1002, g_aes_plain_64, AES_MULTIBLOCK_LEN,
                  g_aes_cipher_64, cipher);
  printf("async aes-cbc multiblock %s\n", ret ? "failed" : "success");
  err |= ret;

  /* SHA-256, one shot and 4 x 64-byte updates */

  memset(digest, 0, sizeof(digest));
  ret = async_sha256(&ctx, 0x2001, g_sha_msg, sizeof(g_sha_msg) - 1,
                     g_sha_digest, digest);
  printf("async sha256 %s\n", ret ? "failed" : "success");
  err |= ret;

  memset(data, 0xaa, sizeof(data));
  memset(digest, 0, sizeof(digest));
  ret = async_sha256(&ctx, 0x2002, data, SHA_LARGE_TOTAL,
                     g_sha_large_digest, digest);
  printf("async sha256 multi-update %s\n", ret ? "failed" : "success");
  err |= ret;

  /* AES-CMAC, update + asynchronous final, against SP 800-38B */

  memset(tag, 0, sizeof(tag));
  ret = async_cmac(&ctx, 0x3001, g_cmac_msg, 16, tag);
  printf("async aes-cmac %s\n", ret ? "failed" : "success");
  err |= ret;

  /* AES-GCM and GMAC, asynchronous final tag, against SP 800-38D */

  memset(out, 0, sizeof(out));
  memset(tag, 0, sizeof(tag));
  ret = async_aead(&ctx, 0x6001, false, out, tag);
  printf("async aes-gcm %s\n", ret ? "failed" : "success");
  err |= ret;

  memset(tag, 0, sizeof(tag));
  ret = async_aead(&ctx, 0x7001, true, NULL, tag);
  printf("async aes-gmac %s\n", ret ? "failed" : "success");
  err |= ret;

  /* Cancel a completed request and an unknown reqid */

  ret = async_cancel(&ctx);
  printf("async cancel %s\n", ret ? "failed" : "success");
  err |= ret;

  async_close(&ctx);

  /* Concurrent submitters, private descriptors */

  ret = check_concurrent();
  printf("async concurrent %s\n", ret ? "failed" : "success");
  err |= ret;

  if (err)
    {
      printf("async test failed\n");
      return 1;
    }

  printf("async test ok\n");
  return 0;
}
