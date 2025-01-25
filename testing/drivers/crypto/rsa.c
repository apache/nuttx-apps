/****************************************************************************
 * apps/testing/drivers/crypto/rsa.c
 *
 * SPDX-License-Identifier: ISC
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ****************************************************************************/

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
#include <sys/endian.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOD_LEN        128
#define PRIVATE_KEYLEN 128
#define PUBLIC_KEYLEN  5
#define PLAIN_LEN      24
#define PADDING_LEN    (MOD_LEN - PLAIN_LEN)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct rsa_data_s
{
  unsigned char e[PUBLIC_KEYLEN];
  unsigned char d[PRIVATE_KEYLEN];
  unsigned char n[MOD_LEN];
}
rsa_data_t;

typedef struct crypto_context_s
{
  int fd;
  int cryptodev_fd;
}
crypto_context_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Example RSA-1024 keypair, for test purposes */

static const unsigned char g_rsa_n[] =
"9292758453063D803DD603D5E777D788"
"8ED1D5BF35786190FA2F23EBC0848AEA"
"DDA92CA6C3D80B32C4D109BE0F36D6AE"
"7130B9CED7ACDF54CFC7555AC14EEBAB"
"93A89813FBF3C4F8066D2D800F7C38A8"
"1AE31942917403FF4946B0A83D3D3E05"
"EE57C6F5F5606FB5D4BC6CD34EE0801A"
"5E94BB77B07507233A0BC7BAC8F90F79";

static const unsigned char g_rsa_e[] = "10001";

static const unsigned char g_rsa_d[] =
"24BF6185468786FDD303083D25E64EFC"
"66CA472BC44D253102F8B4A9D3BFA750"
"91386C0077937FE33FA3252D28855837"
"AE1B484A8A9A45F7EE8C0C634F99E8CD"
"DF79C5CE07EE72C7F123142198164234"
"CABB724CF78B8173B9F880FC86322407"
"AF1FEDFDDE2BEB674CA15F3E81A1521E"
"071513A1E85B5DFA031F21ECAE91A34D";

static const unsigned char g_message[] =
"\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF"
"\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pkcs1_v15_padding(FAR unsigned char *padding,
                              size_t pad_len)
{
  int i;

  /* this padding is only for rsa sign */

  *padding++ = 0;
  *padding++ = 0x02;
  for (i = 0; i < pad_len - 3; i++)
    {
      *padding++ = rand() % 256;
    }

  *padding++ = 0;
}

static void rsa_be32toh(FAR uint8_t *dst, size_t len, FAR const uint8_t *src)
{
  int i;
  size_t rlen = len / 4;
  uint32_t *dst32 = (uint32_t *)dst;
  uint32_t *src32 = (uint32_t *)src;

  for (i = 0; i < rlen; i++)
    {
      dst32[i] = be32toh(src32[rlen - i - 1]);
    }
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

static void mpi_from_string(FAR unsigned char *buf,
                            FAR const unsigned char *s,
                            size_t slen)
{
  int i;
  int j;
  uint8_t d = 0;

  for (i = slen, j = 0; i > 0; i--, j++)
    {
      mpi_get_digit(&d, s[i - 1]);
      buf[j / 2] |= d << ((j % 2) << 2);
    }
}

static int rsa_data_init(FAR rsa_data_t *data,
                         FAR const unsigned char *e, size_t elen,
                         FAR const unsigned char *d, size_t dlen,
                         FAR const unsigned char *n, size_t nlen)
{
  memset(data, 0, sizeof(rsa_data_t));
  mpi_from_string(data->e, e, elen);
  mpi_from_string(data->d, d, dlen);
  mpi_from_string(data->n, n, nlen);
  return 0;
}

static void cryptodev_free(FAR crypto_context_t *ctx)
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

static int cryptodev_init(FAR crypto_context_t *ctx)
{
  memset(ctx, 0, sizeof(crypto_context_t));
  if ((ctx->fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("CRIOGET");
      cryptodev_free(ctx);
      return 1;
    }

  if (ioctl(ctx->fd, CRIOGET, &ctx->cryptodev_fd) == -1)
    {
      perror("CRIOGET");
      cryptodev_free(ctx);
      return 1;
    }

  return 0;
}

static int cryptodev_rsa_calc(FAR crypto_context_t *ctx,
                              FAR rsa_data_t *data,
                              FAR const unsigned char *input, size_t ilen,
                              FAR const unsigned char *output, size_t olen,
                              bool is_priv)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_MOD_EXP;
  cryptk.crk_iparams = 3;
  cryptk.crk_oparams = 1;
  cryptk.crk_param[0].crp_p = (caddr_t)input;
  cryptk.crk_param[0].crp_nbits = ilen * 8;
  if (is_priv)
    {
      cryptk.crk_param[1].crp_p = (caddr_t)data->d;
      cryptk.crk_param[1].crp_nbits = sizeof(data->d) * 8;
    }
  else
    {
      cryptk.crk_param[1].crp_p = (caddr_t)data->e;
      cryptk.crk_param[1].crp_nbits = sizeof(data->e) * 8;
    }

  cryptk.crk_param[2].crp_p = (caddr_t)data->n;
  cryptk.crk_param[2].crp_nbits = sizeof(data->n) * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)output;
  cryptk.crk_param[3].crp_nbits = olen * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("rsa calculate failed");
      return -1;
    }

  return cryptk.crk_status;
}

static int cryptodev_rsa_verify(
            FAR crypto_context_t *ctx, FAR rsa_data_t *data,
            FAR const unsigned char *sig, size_t sig_len,
            FAR const unsigned char *hash, size_t hash_len,
            FAR const unsigned char *padding, size_t pad_len)
{
  struct crypt_kop cryptk;

  memset(&cryptk, 0, sizeof(struct crypt_kop));
  cryptk.crk_op = CRK_RSA_PKCS15_VERIFY;
  cryptk.crk_iparams = 5;
  cryptk.crk_oparams = 0;

  cryptk.crk_param[0].crp_p = (caddr_t)data->e;
  cryptk.crk_param[0].crp_nbits = sizeof(data->e) * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)data->n;
  cryptk.crk_param[1].crp_nbits = sizeof(data->n) * 8;
  cryptk.crk_param[2].crp_p = (caddr_t)sig;
  cryptk.crk_param[2].crp_nbits = sig_len * 8;
  cryptk.crk_param[3].crp_p = (caddr_t)hash;
  cryptk.crk_param[3].crp_nbits = hash_len * 8;
  cryptk.crk_param[4].crp_p = (caddr_t)padding;
  cryptk.crk_param[4].crp_nbits = pad_len * 8;

  if (ioctl(ctx->cryptodev_fd, CIOCKEY, &cryptk) == -1)
    {
      perror("CIOCKEY");
      return -1;
    }

  return cryptk.crk_status;
}

static int rsa_test(void)
{
  crypto_context_t ctx;
  rsa_data_t rsa_data;
  unsigned char cipher[MOD_LEN];
  unsigned char plain[PLAIN_LEN];
  unsigned char data[MOD_LEN];
  unsigned char padding[PADDING_LEN];
  unsigned char sig[MOD_LEN];
  unsigned char data_he[MOD_LEN];
  unsigned char pad_he[PADDING_LEN];
  unsigned char hash_he[MOD_LEN];
  int ret = 0;

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  ret = rsa_data_init(&rsa_data, g_rsa_e, sizeof(g_rsa_e) - 1,
                      g_rsa_d, sizeof(g_rsa_d) - 1,
                      g_rsa_n, sizeof(g_rsa_n) - 1);
  if (ret != 0)
    {
      goto free;
    }

  /* test 1: encrypt with private key and decrypt with public key */

  memset(cipher, 0, MOD_LEN);
  memset(plain, 0, PLAIN_LEN);

  /* encrypt with private key: g_message ^ ctx.data.d % n = cipher */

  ret = cryptodev_rsa_calc(&ctx, &rsa_data, g_message, PLAIN_LEN,
                           cipher, MOD_LEN, TRUE);
  if (ret != 0)
    {
      printf("rsa encrypt with private key failed\n");
      goto free;
    }

  /* dencrypt with public key: cipher ^ ctx.data.e % n = plain */

  ret = cryptodev_rsa_calc(&ctx, &rsa_data, cipher, MOD_LEN, plain,
                           PLAIN_LEN, FALSE);
  if (ret != 0)
    {
      printf("rsa decrypt with pulic key failed\n");
      goto free;
    }

  ret = memcmp(g_message, plain, PLAIN_LEN);
  if (ret != 0)
    {
      printf("rsa test case 1 failed\n");
      goto free;
    }
  else
    {
      printf("rsa test case 1 success\n");
    }

  /* test 2: rsa sign and verify */

  memset(data, 0, MOD_LEN);
  memset(padding, 0, PADDING_LEN);
  memset(sig, 0, MOD_LEN);

  /* pkcs15 v1 padding for sign: 0x00 0x02 (... random) 0x00 */

  pkcs1_v15_padding(padding, PADDING_LEN);
  memcpy(data, padding, PADDING_LEN);
  memcpy(data + PADDING_LEN, g_message, PLAIN_LEN);

  rsa_be32toh(data_he, MOD_LEN, data);
  rsa_be32toh(pad_he, PADDING_LEN, padding);
  rsa_be32toh(hash_he, PLAIN_LEN, g_message);

  /* sign with private key: (hash + padding) ^ d % n = sig */

  ret = cryptodev_rsa_calc(&ctx, &rsa_data, data_he, MOD_LEN, sig,
                           MOD_LEN, TRUE);
  if (ret != 0)
    {
      printf("rsa sign failed\n");
      goto free;
    }

  /* verify with public key: sig ^ e % n = (hash + padding) */

  ret = cryptodev_rsa_verify(&ctx, &rsa_data, sig, MOD_LEN, hash_he,
                             PLAIN_LEN, pad_he, PADDING_LEN);
  if (ret != 0)
    {
      printf("rsa verify failed\n");
    }
  else
    {
      printf("rsa test case 2 success\n");
    }

free:
  cryptodev_free(&ctx);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  if (rsa_test() == 0)
    {
      printf("rsa test success\n");
    }
  else
    {
      printf("rsa test failed\n");
    }

  return 0;
}
