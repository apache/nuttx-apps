/****************************************************************************
 * apps/testing/drivers/crypto/keymanagement.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* Exercise the cryptodev key id interface: with SOP_F_KEYID the key
 * fields of a session, and the key params of an asymmetric operation,
 * carry a key id of the software key store in place of the key itself.
 * Every case runs the full lifecycle of the ids it uses: allocate,
 * import, operate, delete; a failure also deletes the ids the case
 * holds, so nothing is left behind in the store.
 *
 * Requires a config-data device at the path configured by
 * CONFIG_CRYPTO_CRYPTODEV_SOFTWARE_KEYMGMT_DEVICE (default /dev/nvs).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOD_1024_LEN  128
#define PLAIN_LEN     24
#define PADDING_LEN   (MOD_1024_LEN - PLAIN_LEN)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct crypto_context_s
{
  int fd;
  int cryptodev_fd;
}
crypto_context_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* AES-CBC vector from RFC 3602 */

static const char g_aescbc_key[16] =
  "\x06\xa9\x21\x40\x36\xb8\xa1\x5b\x51\x2e\x03\xd5\x34\x12\x00\x06";
static const char g_aescbc_iv[16] =
  "\x3d\xaf\xba\x42\x9d\x9e\xb4\x30\xb4\x22\xda\x80\x2c\x9f\xac\x41";
static const char g_aescbc_plain[] = "Single block msg";
static const char g_aescbc_cipher[16] =
  "\xe3\x53\x77\x9c\x10\x79\xae\xb8\x27\x08\x94\x2d\xbe\x77\x18\x1a";

/* HMAC-SHA256 vector as carried by the hmac test */

static const char g_hmac_key[16] =
  "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b";
static const char g_hmac_data[] = "Hi There";
static const char g_hmac_mac[32] =
  "\x49\x2c\xe0\x20\xfe\x25\x34\xa5\x78\x9d\xc3\x84\x88\x06\xc7\x8f"
  "\x4f\x67\x11\x39\x7f\x08\xe7\xe7\xa1\x2c\xa5\xa4\x48\x3c\x8a\xa6";

/* AES-128-CMAC vector from the NIST SP 800-38B examples */

static const unsigned char g_cmac_key[16] =
{
    0x2b, 0x7e, 0x15, 0x16,     0x28, 0xae, 0xd2, 0xa6,
    0xab, 0xf7, 0x15, 0x88,     0x09, 0xcf, 0x4f, 0x3c
};
static const unsigned char g_cmac_message[64] =
{
    0x6b, 0xc1, 0xbe, 0xe2,     0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11,     0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57,     0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac,     0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46,     0xa3, 0x5c, 0xe4, 0x11,
    0xe5, 0xfb, 0xc1, 0x19,     0x1a, 0x0a, 0x52, 0xef,
    0xf6, 0x9f, 0x24, 0x45,     0xdf, 0x4f, 0x9b, 0x17,
    0xad, 0x2b, 0x41, 0x7b,     0xe6, 0x6c, 0x37, 0x10
};
static const unsigned char g_cmac_mac[16] =
{
    0x51, 0xf0, 0xbe, 0xbf,     0x7e, 0x3b, 0x9d, 0x92,
    0xfc, 0x49, 0x74, 0x17,     0x79, 0x36, 0x3c, 0xfe
};

/* Example RSA-1024 keypair, as in the rsa test */

static const char g_rsa_n_1024[] =
"9292758453063D803DD603D5E777D788"
"8ED1D5BF35786190FA2F23EBC0848AEA"
"DDA92CA6C3D80B32C4D109BE0F36D6AE"
"7130B9CED7ACDF54CFC7555AC14EEBAB"
"93A89813FBF3C4F8066D2D800F7C38A8"
"1AE31942917403FF4946B0A83D3D3E05"
"EE57C6F5F5606FB5D4BC6CD34EE0801A"
"5E94BB77B07507233A0BC7BAC8F90F79";

static const char g_rsa_e_1024[] = "10001";

static const char g_rsa_d_1024[] =
"24BF6185468786FDD303083D25E64EFC"
"66CA472BC44D253102F8B4A9D3BFA750"
"91386C0077937FE33FA3252D28855837"
"AE1B484A8A9A45F7EE8C0C634F99E8CD"
"DF79C5CE07EE72C7F123142198164234"
"CABB724CF78B8173B9F880FC86322407"
"AF1FEDFDDE2BEB674CA15F3E81A1521E"
"071513A1E85B5DFA031F21ECAE91A34D";

static const unsigned char g_message[PLAIN_LEN] =
"\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF"
"\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD";

static const unsigned char g_sha256_message[32] =
{
  0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
  0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
  0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
  0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
};

static const unsigned char g_lifecycle_data[32] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void crypto_free(FAR crypto_context_t *ctx)
{
  if (ctx->cryptodev_fd != 0)
    {
      close(ctx->cryptodev_fd);
      ctx->cryptodev_fd = 0;
    }

  if (ctx->fd != 0)
    {
      close(ctx->fd);
      ctx->fd = 0;
    }
}

static int crypto_init(FAR crypto_context_t *ctx)
{
  memset(ctx, 0, sizeof(crypto_context_t));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("/dev/crypto");
      crypto_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) == -1)
    {
      perror("CRIOGET");
      crypto_free(ctx);
      return 1;
    }

  return 0;
}

/* Run one key management operation.  The key id is always param[0];
 * import and export also use buf as param[1].
 */

static int keyid_op(FAR crypto_context_t *ctx, int op,
                    FAR uint32_t *keyid, FAR void *buf, size_t buflen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = op;
  cryptk.crk_param[0].crp_p = (caddr_t)keyid;
  cryptk.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;

  switch (op)
    {
      case CRK_ALLOCATE_KEY:
        cryptk.crk_iparams = 0;
        cryptk.crk_oparams = 1;
        break;
      case CRK_IMPORT_KEY:
        cryptk.crk_iparams = 2;
        cryptk.crk_oparams = 0;
        cryptk.crk_param[1].crp_p = (caddr_t)buf;
        cryptk.crk_param[1].crp_nbits = buflen * 8;
        break;
      case CRK_EXPORT_KEY:
        cryptk.crk_iparams = 1;
        cryptk.crk_oparams = 1;
        cryptk.crk_param[1].crp_p = (caddr_t)buf;
        cryptk.crk_param[1].crp_nbits = buflen * 8;
        break;
      case CRK_DELETE_KEY:
      case CRK_SAVE_KEY:
      case CRK_LOAD_KEY:
      case CRK_UNLOAD_KEY:
        cryptk.crk_iparams = 1;
        cryptk.crk_oparams = 0;
        break;
      default:
        return -EINVAL;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static void mpi_get_digit(FAR uint8_t *d, char c)
{
  if (c >= '0' && c <= '9')
    {
      *d = c - '0';
    }

  if (c >= 'A' && c <= 'F')
    {
      *d = c - 'A' + 10;
    }

  if (c >= 'a' && c <= 'f')
    {
      *d = c - 'a' + 10;
    }
}

/* Decode a big-endian hex string into the little-endian byte order
 * cryptodev carries, and return the length in bytes.
 */

static size_t mpi_from_string(FAR unsigned char *buf, FAR const char *s)
{
  size_t slen = strlen(s);
  size_t i;
  int j;
  uint8_t d = 0;

  for (i = slen, j = 0; i > 0; i--, j++)
    {
      mpi_get_digit(&d, s[i - 1]);
      buf[j / 2] |= d << ((j % 2) << 2);
    }

  return (slen + 1) / 2;
}

/* Reverse the byte order, between the little-endian integers of the
 * cryptodev interface and the big-endian ones of the test vectors.
 */

static void mpi_reverse(FAR unsigned char *dst,
                        FAR const unsigned char *src, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      dst[i] = src[len - 1 - i];
    }
}

static int match(FAR const void *a, FAR const void *b, size_t len)
{
  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  printf("mismatch:\n");
  return -1;
}

/* Case 1: the key id lifecycle, allocate to delete, with the import
 * and export in between.
 */

static int test_lifecycle(FAR crypto_context_t *ctx)
{
  unsigned char buf[sizeof(g_lifecycle_data)];
  uint32_t keyid = 0;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_IMPORT_KEY, &keyid,
                 (FAR void *)g_lifecycle_data, sizeof(g_lifecycle_data));
  if (ret != 0)
    {
      printf("keyid import failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_EXPORT_KEY, &keyid, buf, sizeof(buf));
  if (ret != 0)
    {
      printf("keyid export failed\n");
      goto err;
    }

  if (match(g_lifecycle_data, buf, sizeof(g_lifecycle_data)) != 0)
    {
      printf("keyid export mismatch\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_SAVE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid save failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_LOAD_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid load failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_UNLOAD_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid unload failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      return -1;
    }

  printf("keyid lifecycle success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  return -1;
}

/* Case 2: AES-CBC with the cipher key referenced by id */

static int aescbc_crypt(FAR crypto_context_t *ctx, uint32_t keyid,
                        FAR const char *iv, FAR const char *in,
                        FAR unsigned char *out, size_t len, int encrypt)
{
  struct session_op session;
  struct crypt_op cryp;

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CBC;
  session.key = (caddr_t)&keyid;
  session.keylen = sizeof(keyid);
  session.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  session.flags = SOP_F_KEYID;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      perror("CIOCGSESSION");
      return -1;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = encrypt ? COP_ENCRYPT : COP_DECRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = len;
  cryp.ivlen = 16;
  cryp.src = (caddr_t)in;
  cryp.dst = (caddr_t)out;
  cryp.iv = (caddr_t)iv;
  cryp.mac = 0;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT");
      return -1;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      perror("CIOCFSESSION");
      return -1;
    }

  return 0;
}

static int test_aescbc(FAR crypto_context_t *ctx)
{
  unsigned char buf[16];
  uint32_t keyid = 0;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_IMPORT_KEY, &keyid, (FAR void *)g_aescbc_key,
                 sizeof(g_aescbc_key));
  if (ret != 0)
    {
      printf("keyid import failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_SAVE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid save failed\n");
      goto err;
    }

  ret = aescbc_crypt(ctx, keyid, g_aescbc_iv, g_aescbc_plain, buf,
                     sizeof(buf), 1);
  if (ret != 0)
    {
      printf("aescbc encrypt failed\n");
      goto err;
    }

  if (match(g_aescbc_cipher, buf, sizeof(buf)) != 0)
    {
      printf("aescbc encrypt mismatch\n");
      goto err;
    }

  ret = aescbc_crypt(ctx, keyid, g_aescbc_iv, g_aescbc_cipher, buf,
                     sizeof(buf), 0);
  if (ret != 0)
    {
      printf("aescbc decrypt failed\n");
      goto err;
    }

  if (match(g_aescbc_plain, buf, sizeof(buf)) != 0)
    {
      printf("aescbc decrypt mismatch\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      return -1;
    }

  printf("aescbc keyid success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  return -1;
}

/* Case 3: HMAC-SHA256 with the mac key referenced by id */

static int hmac_keyid(FAR crypto_context_t *ctx, uint32_t keyid,
                      FAR const char *data, size_t len,
                      FAR unsigned char *out, size_t outlen)
{
  struct session_op session;
  struct crypt_op cryp;

  memset(&session, 0, sizeof(session));
  session.mac = CRYPTO_SHA2_256_HMAC;
  session.mackey = (caddr_t)&keyid;
  session.mackeylen = sizeof(keyid);
  session.flags = SOP_F_KEYID;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      perror("CIOCGSESSION");
      return -1;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = outlen;
  cryp.src = (caddr_t)data;
  cryp.mac = (caddr_t)out;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT");
      return -1;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      perror("CIOCFSESSION");
      return -1;
    }

  return 0;
}

static int test_hmac(FAR crypto_context_t *ctx)
{
  unsigned char buf[32];
  uint32_t keyid = 0;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_IMPORT_KEY, &keyid, (FAR void *)g_hmac_key,
                 sizeof(g_hmac_key));
  if (ret != 0)
    {
      printf("keyid import failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_SAVE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid save failed\n");
      goto err;
    }

  ret = hmac_keyid(ctx, keyid, g_hmac_data, strlen(g_hmac_data),
                   buf, sizeof(buf));
  if (ret != 0)
    {
      printf("hmac failed\n");
      goto err;
    }

  if (match(g_hmac_mac, buf, sizeof(buf)) != 0)
    {
      printf("hmac mismatch\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      return -1;
    }

  printf("hmac keyid success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  return -1;
}

/* Case 4: AES-CMAC with the mac key referenced by id */

static int aescmac_keyid(FAR crypto_context_t *ctx, uint32_t keyid,
                         FAR const unsigned char *message, size_t len,
                         FAR unsigned char *output, size_t outlen)
{
  struct session_op session;
  struct crypt_op cryp;

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_CMAC;
  session.key = (caddr_t)&keyid;
  session.keylen = sizeof(keyid);
  session.mac = CRYPTO_AES_128_CMAC;
  session.mackey = (caddr_t)&keyid;
  session.mackeylen = sizeof(keyid);
  session.flags = SOP_F_KEYID;
  if (ioctl(ctx->cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      perror("CIOCGSESSION");
      return -1;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.flags = 0;
  cryp.len = len;
  cryp.olen = outlen;
  cryp.src = (caddr_t)message;
  cryp.mac = (caddr_t)output;
  if (ioctl(ctx->cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT");
      return -1;
    }

  if (ioctl(ctx->cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      perror("CIOCFSESSION");
      return -1;
    }

  return 0;
}

static int test_aescmac(FAR crypto_context_t *ctx)
{
  unsigned char buf[16];
  uint32_t keyid = 0;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_IMPORT_KEY, &keyid, (FAR void *)g_cmac_key,
                 sizeof(g_cmac_key));
  if (ret != 0)
    {
      printf("keyid import failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_SAVE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid save failed\n");
      goto err;
    }

  ret = aescmac_keyid(ctx, keyid, g_cmac_message, sizeof(g_cmac_message),
                      buf, sizeof(buf));
  if (ret != 0)
    {
      printf("aescmac failed\n");
      goto err;
    }

  if (match(g_cmac_mac, buf, sizeof(buf)) != 0)
    {
      printf("aescmac mismatch\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      return -1;
    }

  printf("aescmac keyid success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &keyid, NULL, 0);
  return -1;
}

/* Case 5: ECDSA over a generated key pair, both keys referenced by id.
 * The in-params of CRK_GENERATE_SECP256R1_KEY are key ids by
 * definition of the operation: the backend writes the generated
 * pair into them, so no SOP_F_KEYID is involved here.
 */

static int ecdsa_genkey_keyid(FAR crypto_context_t *ctx,
                              FAR uint32_t *prikeyid, FAR uint32_t *pubkeyid)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_GENERATE_SECP256R1_KEY;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 0;
  cryptk.crk_param[0].crp_p = (caddr_t)prikeyid;
  cryptk.crk_param[0].crp_nbits = sizeof(uint32_t) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)pubkeyid;
  cryptk.crk_param[1].crp_nbits = sizeof(uint32_t) * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int ecdsa_sign_keyid(FAR crypto_context_t *ctx, uint32_t prikeyid,
                            FAR const unsigned char *hash, size_t hashlen,
                            FAR unsigned char *r, size_t rlen,
                            FAR unsigned char *s, size_t slen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_ECDSA_SECP256R1_SIGN;
  cryptk.crk_iparams = 2;
  cryptk.crk_oparams = 2;
  cryptk.crk_flags = SOP_F_KEYID;
  cryptk.crk_param[0].crp_p = (caddr_t)&prikeyid;
  cryptk.crk_param[0].crp_nbits = sizeof(prikeyid) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)hash;
  cryptk.crk_param[1].crp_nbits = hashlen * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)r;
  cryptk.crk_param[2].crp_nbits = rlen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)s;
  cryptk.crk_param[3].crp_nbits = slen * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int ecdsa_verify_keyid(FAR crypto_context_t *ctx, uint32_t pubkeyid,
                              FAR const unsigned char *hash, size_t hashlen,
                              FAR const unsigned char *r, size_t rlen,
                              FAR const unsigned char *s, size_t slen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_ECDSA_SECP256R1_VERIFY;
  cryptk.crk_iparams = 6;
  cryptk.crk_oparams = 0;
  cryptk.crk_flags = SOP_F_KEYID;

  /* The public key is x||y under the first key id; the second param
   * exists for the plain x, y interface and is not used.
   */

  cryptk.crk_param[0].crp_p = (caddr_t)&pubkeyid;
  cryptk.crk_param[0].crp_nbits = sizeof(pubkeyid) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)&pubkeyid;
  cryptk.crk_param[1].crp_nbits = sizeof(pubkeyid) * 8;
  cryptk.crk_param[2].crp_p = NULL;
  cryptk.crk_param[2].crp_nbits = 0;
  cryptk.crk_param[3].crp_p = (caddr_t)r;
  cryptk.crk_param[3].crp_nbits = rlen * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)s;
  cryptk.crk_param[4].crp_nbits = slen * 8;
  cryptk.crk_param[5].crp_p = (caddr_t)hash;
  cryptk.crk_param[5].crp_nbits = hashlen * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int test_ecdsa(FAR crypto_context_t *ctx)
{
  unsigned char r[32];
  unsigned char s[32];
  uint32_t prikeyid = 0;
  uint32_t pubkeyid = 0;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &prikeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, &pubkeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      goto err;
    }

  ret = ecdsa_genkey_keyid(ctx, &prikeyid, &pubkeyid);
  if (ret != 0)
    {
      printf("ecdsa generate key failed\n");
      goto err;
    }

  ret = ecdsa_sign_keyid(ctx, prikeyid, g_sha256_message,
                         sizeof(g_sha256_message), r, sizeof(r),
                         s, sizeof(s));
  if (ret != 0)
    {
      printf("ecdsa sign failed\n");
      goto err;
    }

  ret = ecdsa_verify_keyid(ctx, pubkeyid, g_sha256_message,
                           sizeof(g_sha256_message), r, sizeof(r),
                           s, sizeof(s));
  if (ret != 0)
    {
      printf("ecdsa verify failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &prikeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &pubkeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      goto err;
    }

  printf("ecdsa keyid success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &prikeyid, NULL, 0);
  keyid_op(ctx, CRK_DELETE_KEY, &pubkeyid, NULL, 0);
  return -1;
}

/* Case 6: RSA with n, e and d referenced by id: a public encrypt
 * followed by a private decrypt, and a raw private-key operation
 * with type-2 padding, recovered and checked with the public key.
 */

static int rsa_mod_exp_keyid(FAR crypto_context_t *ctx,
                             FAR const unsigned char *in, size_t ilen,
                             uint32_t keyid, uint32_t nkeyid,
                             FAR unsigned char *out, size_t olen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_MOD_EXP;
  cryptk.crk_iparams = 3;
  cryptk.crk_oparams = 1;
  cryptk.crk_flags = SOP_F_KEYID;
  cryptk.crk_param[0].crp_p = (caddr_t)in;
  cryptk.crk_param[0].crp_nbits = ilen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)&keyid;
  cryptk.crk_param[1].crp_nbits = sizeof(keyid) * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)&nkeyid;
  cryptk.crk_param[2].crp_nbits = sizeof(nkeyid) * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)out;
  cryptk.crk_param[3].crp_nbits = olen * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int rsa_verify_keyid(FAR crypto_context_t *ctx,
                            uint32_t ekeyid, uint32_t nkeyid,
                            FAR const unsigned char *sig, size_t siglen,
                            FAR const unsigned char *hash, size_t hashlen,
                            FAR const unsigned char *padding,
                            size_t padlen)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(cryptk));
  cryptk.crk_op = CRK_RSA_PKCS15_VERIFY;
  cryptk.crk_iparams = 5;
  cryptk.crk_oparams = 0;
  cryptk.crk_flags = SOP_F_KEYID;
  cryptk.crk_param[0].crp_p = (caddr_t)&ekeyid;
  cryptk.crk_param[0].crp_nbits = sizeof(ekeyid) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)&nkeyid;
  cryptk.crk_param[1].crp_nbits = sizeof(nkeyid) * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)sig;
  cryptk.crk_param[2].crp_nbits = siglen * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)hash;
  cryptk.crk_param[3].crp_nbits = hashlen * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)padding;
  cryptk.crk_param[4].crp_nbits = padlen * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int rsa_import_keyid(FAR crypto_context_t *ctx,
                            FAR uint32_t *keyid, FAR const char *hex)
{
  unsigned char buf[MOD_1024_LEN];
  size_t len;
  int ret;

  ret = keyid_op(ctx, CRK_ALLOCATE_KEY, keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid allocate failed\n");
      return -1;
    }

  memset(buf, 0, sizeof(buf));
  len = mpi_from_string(buf, hex);

  ret = keyid_op(ctx, CRK_IMPORT_KEY, keyid, buf, len);
  if (ret != 0)
    {
      printf("keyid import failed\n");
      return -1;
    }

  ret = keyid_op(ctx, CRK_SAVE_KEY, keyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid save failed\n");
      return -1;
    }

  return 0;
}

static int test_rsa(FAR crypto_context_t *ctx)
{
  unsigned char cipher[MOD_1024_LEN];
  unsigned char plain[PLAIN_LEN];
  unsigned char data[MOD_1024_LEN];
  unsigned char padding[PADDING_LEN];
  unsigned char sig[MOD_1024_LEN];
  unsigned char data_le[MOD_1024_LEN];
  unsigned char pad_le[PADDING_LEN];
  unsigned char hash_le[PLAIN_LEN];
  uint32_t nkeyid = 0;
  uint32_t ekeyid = 0;
  uint32_t dkeyid = 0;
  int i;
  int ret;

  ret = rsa_import_keyid(ctx, &nkeyid, g_rsa_n_1024);
  if (ret != 0)
    {
      goto err;
    }

  ret = rsa_import_keyid(ctx, &ekeyid, g_rsa_e_1024);
  if (ret != 0)
    {
      goto err;
    }

  ret = rsa_import_keyid(ctx, &dkeyid, g_rsa_d_1024);
  if (ret != 0)
    {
      goto err;
    }

  /* encrypt with the public key and decrypt with the private one */

  ret = rsa_mod_exp_keyid(ctx, g_message, PLAIN_LEN, ekeyid, nkeyid,
                          cipher, MOD_1024_LEN);
  if (ret != 0)
    {
      printf("rsa encrypt failed\n");
      goto err;
    }

  ret = rsa_mod_exp_keyid(ctx, cipher, MOD_1024_LEN, dkeyid, nkeyid,
                          plain, PLAIN_LEN);
  if (ret != 0)
    {
      printf("rsa decrypt failed\n");
      goto err;
    }

  if (match(g_message, plain, PLAIN_LEN) != 0)
    {
      printf("rsa decrypt mismatch\n");
      goto err;
    }

  /* type-2 padding (0x00 0x02 ... random ... 0x00) around the raw
   * message for the private-key operation below.
   */

  memset(data, 0, sizeof(data));
  memset(padding, 0, sizeof(padding));
  padding[0] = 0;
  padding[1] = 2;
  for (i = 2; i < PADDING_LEN - 1; i++)
    {
      padding[i] = rand() % 256;
    }

  padding[PADDING_LEN - 1] = 0;
  memcpy(data, padding, PADDING_LEN);
  memcpy(data + PADDING_LEN, g_message, PLAIN_LEN);
  mpi_reverse(data_le, data, MOD_1024_LEN);
  mpi_reverse(pad_le, padding, PADDING_LEN);
  mpi_reverse(hash_le, g_message, PLAIN_LEN);

  /* raw private-key operation: (padding || message) ^ d mod n */

  ret = rsa_mod_exp_keyid(ctx, data_le, MOD_1024_LEN, dkeyid, nkeyid,
                          sig, MOD_1024_LEN);
  if (ret != 0)
    {
      printf("rsa sign failed\n");
      goto err;
    }

  /* recover with the public key: sig ^ e mod n = padding || message */

  ret = rsa_verify_keyid(ctx, ekeyid, nkeyid, sig, MOD_1024_LEN,
                         hash_le, PLAIN_LEN, pad_le, PADDING_LEN);
  if (ret != 0)
    {
      printf("rsa verify failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &nkeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &ekeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      goto err;
    }

  ret = keyid_op(ctx, CRK_DELETE_KEY, &dkeyid, NULL, 0);
  if (ret != 0)
    {
      printf("keyid delete failed\n");
      goto err;
    }

  printf("rsa keyid success\n");
  return 0;

err:
  keyid_op(ctx, CRK_DELETE_KEY, &nkeyid, NULL, 0);
  keyid_op(ctx, CRK_DELETE_KEY, &ekeyid, NULL, 0);
  keyid_op(ctx, CRK_DELETE_KEY, &dkeyid, NULL, 0);
  return -1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  crypto_context_t ctx;
  int ret;

  setbuf(stdout, NULL);

  ret = crypto_init(&ctx);
  if (ret != 0)
    {
      return 1;
    }

  ret = test_lifecycle(&ctx);
  if (ret != 0)
    {
      goto out;
    }

  ret = test_aescbc(&ctx);
  if (ret != 0)
    {
      goto out;
    }

  ret = test_hmac(&ctx);
  if (ret != 0)
    {
      goto out;
    }

  ret = test_aescmac(&ctx);
  if (ret != 0)
    {
      goto out;
    }

  ret = test_ecdsa(&ctx);
  if (ret != 0)
    {
      goto out;
    }

  ret = test_rsa(&ctx);

out:
  crypto_free(&ctx);
  return ret == 0 ? 0 : 1;
}
