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
 * MERCHANTABILITY. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN USE OF THIS CONNECTION WITH THIS SOFTWARE.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MOD_1024_LEN              128
#define PLAIN_LEN                 24
#define PADDING_LEN               (MOD_1024_LEN - PLAIN_LEN)
#define MOD_2048_LEN              256
#define SHA256_LEN                32
#define DIGESTINFO_LEN            19
#define EM_2048_TLEN              (DIGESTINFO_LEN + SHA256_LEN)
#define EM_2048_PREFIX_LEN        (MOD_2048_LEN - EM_2048_TLEN)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* A key as MPI hex strings, the way cryptodev tests carry them.  The 1024
 * bit key carries n, e and d only, so every private operation in this test
 * runs without the CRT components.
 */

struct rsa_vector_s
{
  FAR const char *n;
  FAR const char *e;
  FAR const char *d;
  size_t modlen;
};

typedef struct rsa_data_s
{
  FAR unsigned char *e;
  FAR unsigned char *d;
  FAR unsigned char *n;
  size_t elen;
  size_t dlen;
  size_t nlen;
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

/* RSA-2048 keypair given as n, e and d only */

static const char g_rsa_n_2048[] =
"B11CD52A2175035B3AE01EAD94670442"
"65E521123F2E66ECE0458F4C21ACDA52"
"CEFE57E1D5F03F85B6973C9385FF5309"
"9EB35D760B3B321C8953B4C86B3525FD"
"327464C0C4A7E60D5729FC2B7837D44F"
"EC082DB712E77A7FFCAFED1ABD3F1D0C"
"E1FD31EB57A082CA43D1AE182A705AE5"
"F16E3C4BDF6D959B42F331020B5D0113"
"2271A9468C2D87DA73FEB828053909CC"
"355C1536D0131E7BE88FB3173CEBB402"
"68A706D3F7CA17F38CE56402B304D775"
"562779240BE2DBF07D91AB68A6D17A61"
"DE9019630F3D9DA8A2438D21D0EEF282"
"8D5BF379BCD5C70B3043C510140F0C48"
"D6254F0F1A71BC1C43E1F75D7A55FC4B"
"83D269E9336FE91E3F648153FDBD4CAF";

static const char g_rsa_e_2048[] = "010001";

static const char g_rsa_d_2048[] =
"1572685DB1AF79B89268914E73AA05DA"
"047CBA847B2223548BD3E086578D96E2"
"702E5C75FC721B3F15DD3A78A2C1484C"
"E1EAD66674F5B61BBE85645EE09C8EFD"
"1A1EFE1459BBFB1FA26E731AD073F241"
"BB534235E9141EC160D7ECDE500C5478"
"3AE56E2611765E601FD6443EA8E4F21C"
"E61F732DBCE0883D44DE2E11114F31D1"
"EBB01624BF9C4D2C22F2239E9A475305"
"17E2D870002872449D2680D9D729F1FE"
"573D8FA6304FE503922193323367B012"
"031CE97B1F0C103AB446543D23438BBF"
"8F09C142AD9105FE1F55E984C4C79220"
"6C5EDD7FF4CF9EEF4DA98A05C1258821"
"AE9917C76F6343F2865125B18E1CC827"
"C4DECC980B2DCCB405D68D869C8ED801";

static const struct rsa_vector_s g_rsa_1024 =
{
  g_rsa_n_1024, g_rsa_e_1024, g_rsa_d_1024, MOD_1024_LEN
};

static const struct rsa_vector_s g_rsa_2048 =
{
  g_rsa_n_2048, g_rsa_e_2048, g_rsa_d_2048, MOD_2048_LEN
};

static const unsigned char g_message[] =
"\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF"
"\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD";

/* SHA-256 digest of "abc" (FIPS 180 test vector) */

static const char g_sha256_abc[] =
"CDC76E5C9914FB9281A1C7E284D73E67"
"F1809A48A497200E046D39CCC7112CD0";

/* RSASSA-PKCS1-v1_5 signature of g_sha256_abc under the 2048 bit key,
 * made with a reference implementation of RFC 8017, so the backend is
 * checked against the standard encoding and not only against itself.
 */

static const char g_rsa_pkcs15_sig_2048[] =
"7DCE3F23611DBEED4E4F7206FECB3E9A"
"5D4DA5F906FC8132B2F0BB6114F95384"
"362001E097F6B92B6BA7C28395552F98"
"1C784E3DE9C024D03190A30EB6DF406B"
"DE8FF311B681F46D584ACA46EDDD8786"
"D9CB8A5A07C200F9BF7B2000E97ABD11"
"4E0525BD8CB8959855BED92B5A721B07"
"7DB512FFB1D5960EB785510E810F7E55"
"156E03D2EAD9313E517774935C96EEA2"
"5393E6BB6DA45AAED28CBB26D0AADFBA"
"332E786D61652642B9BB544150FE0212"
"994FE3849353E80583C4B03069314190"
"3C3CB91E599C29E5A6068EFBF05A0EF1"
"2C0CF5708DA6DDFA8B059EDD0962689E"
"0FF805515820F3471543352CE9B44657"
"746A088F63D6C67C482F0BCC5532EDC4";

/* DER header of the SHA-256 DigestInfo in EMSA-PKCS1-v1_5 */

static const unsigned char g_digestinfo[DIGESTINFO_LEN] =
{
  0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
  0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20
};

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
  for (i = 0; i < (int)pad_len - 3; i++)
    {
      *padding++ = rand() % 256;
    }

  *padding++ = 0;
}

/* Reverse the byte order, between the big-endian integers of the test
 * vectors and the little-endian ones cryptodev carries.
 */

static void rsa_reverse(FAR unsigned char *dst,
                        FAR const unsigned char *src, size_t len)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      dst[i] = src[len - 1 - i];
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

/* Decode a hex string into bytes, keeping the written order */

static void hex_decode(FAR unsigned char *dst, FAR const char *s,
                       size_t slen)
{
  size_t i;
  uint8_t hi = 0;
  uint8_t lo = 0;

  for (i = 0; i < slen / 2; i++)
    {
      mpi_get_digit(&hi, s[2 * i]);
      mpi_get_digit(&lo, s[2 * i + 1]);
      dst[i] = (hi << 4) | lo;
    }
}

static int rsa_data_init(FAR rsa_data_t *data,
                         FAR const struct rsa_vector_s *key)
{
  memset(data, 0, sizeof(rsa_data_t));
  data->elen = strlen(key->e);
  data->dlen = key->modlen;
  data->nlen = key->modlen;
  data->e = zalloc(data->elen);
  data->d = zalloc(data->dlen);
  data->n = zalloc(data->nlen);
  if (data->e == NULL || data->d == NULL || data->n == NULL)
    {
      return -1;
    }

  mpi_from_string(data->e, (FAR const unsigned char *)key->e,
                  data->elen);
  mpi_from_string(data->d, (FAR const unsigned char *)key->d,
                  strlen(key->d));
  mpi_from_string(data->n, (FAR const unsigned char *)key->n,
                  strlen(key->n));
  return 0;
}

static void rsa_data_free(FAR rsa_data_t *data)
{
  free(data->e);
  free(data->d);
  free(data->n);
  memset(data, 0, sizeof(rsa_data_t));
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
      cryptk.crk_param[1].crp_nbits = data->dlen * 8;
    }
  else
    {
      cryptk.crk_param[1].crp_p = (caddr_t)data->e;
      cryptk.crk_param[1].crp_nbits = data->elen * 8;
    }

  cryptk.crk_param[2].crp_p = (caddr_t)data->n;
  cryptk.crk_param[2].crp_nbits = data->nlen * 8;
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
  cryptk.crk_param[0].crp_nbits = data->elen * 8;
  cryptk.crk_param[1].crp_p = (caddr_t)data->n;
  cryptk.crk_param[1].crp_nbits = data->nlen * 8;
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

/* Test 1: encrypt with the private key and decrypt with the public key,
 * and test 2: sign a caller-built block and verify it.  The key carries
 * n, e and d only, so the private operations run without the CRT
 * components.
 */

static int rsa_test_1024(FAR crypto_context_t *ctx)
{
  unsigned char cipher[MOD_1024_LEN];
  unsigned char plain[PLAIN_LEN];
  unsigned char data[MOD_1024_LEN];
  unsigned char padding[PADDING_LEN];
  unsigned char sig[MOD_1024_LEN];
  unsigned char data_he[MOD_1024_LEN];
  unsigned char pad_he[PADDING_LEN];
  unsigned char hash_he[PLAIN_LEN];
  rsa_data_t rsa_data;
  int ret = 0;

  ret = rsa_data_init(&rsa_data, &g_rsa_1024);
  if (ret != 0)
    {
      return ret;
    }

  /* test 1: encrypt with private key and decrypt with public key */

  memset(cipher, 0, MOD_1024_LEN);
  memset(plain, 0, PLAIN_LEN);

  /* encrypt with private key: g_message ^ ctx.data.d % n = cipher */

  ret = cryptodev_rsa_calc(ctx, &rsa_data, g_message, PLAIN_LEN,
                           cipher, MOD_1024_LEN, TRUE);
  if (ret != 0)
    {
      printf("rsa encrypt with private key failed\n");
      goto free;
    }

  /* dencrypt with public key: cipher ^ ctx.data.e % n = plain */

  ret = cryptodev_rsa_calc(ctx, &rsa_data, cipher, MOD_1024_LEN, plain,
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

  memset(data, 0, MOD_1024_LEN);
  memset(padding, 0, PADDING_LEN);
  memset(sig, 0, MOD_1024_LEN);

  /* pkcs15 v1 padding for sign: 0x00 0x02 (... random) 0x00 */

  pkcs1_v15_padding(padding, PADDING_LEN);
  memcpy(data, padding, PADDING_LEN);
  memcpy(data + PADDING_LEN, g_message, PLAIN_LEN);

  rsa_reverse(data_he, data, MOD_1024_LEN);
  rsa_reverse(pad_he, padding, PADDING_LEN);
  rsa_reverse(hash_he, g_message, PLAIN_LEN);

  /* sign with private key: (hash + padding) ^ d % n = sig */

  ret = cryptodev_rsa_calc(ctx, &rsa_data, data_he, MOD_1024_LEN, sig,
                           MOD_1024_LEN, TRUE);
  if (ret != 0)
    {
      printf("rsa sign failed\n");
      goto free;
    }

  /* verify with public key: sig ^ e % n = (hash + padding) */

  ret = cryptodev_rsa_verify(ctx, &rsa_data, sig, MOD_1024_LEN, hash_he,
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
  rsa_data_free(&rsa_data);
  return ret;
}

/* Test 3: sign the SHA-256 digest of "abc" under the 2048 bit key,
 * which is given as n, e and d only, and compare the signature with a
 * reference implementation of RSASSA-PKCS1-v1_5 byte for byte.
 */

static int rsa_test_2048_donly(FAR crypto_context_t *ctx)
{
  unsigned char em[MOD_2048_LEN];
  unsigned char em_le[MOD_2048_LEN];
  unsigned char sig[MOD_2048_LEN];
  unsigned char expect[MOD_2048_LEN];
  unsigned char hash_he[EM_2048_TLEN];
  unsigned char pad_he[EM_2048_PREFIX_LEN];
  rsa_data_t rsa_data;
  int ret;

  ret = rsa_data_init(&rsa_data, &g_rsa_2048);
  if (ret != 0)
    {
      return ret;
    }

  /* EMSA-PKCS1-v1_5 of the digest: 0x00 0x01 PS 0x00 DigestInfo Hash */

  memset(em, 0, sizeof(em));
  em[1] = 0x01;
  memset(em + 2, 0xff, EM_2048_PREFIX_LEN - 3);
  em[EM_2048_PREFIX_LEN - 1] = 0x00;
  memcpy(em + EM_2048_PREFIX_LEN, g_digestinfo, DIGESTINFO_LEN);
  hex_decode(em + EM_2048_PREFIX_LEN + DIGESTINFO_LEN, g_sha256_abc,
             strlen(g_sha256_abc));

  /* sign with the private key: EM ^ d % n = sig */

  rsa_reverse(em_le, em, MOD_2048_LEN);
  memset(sig, 0, sizeof(sig));
  ret = cryptodev_rsa_calc(ctx, &rsa_data, em_le, MOD_2048_LEN, sig,
                           MOD_2048_LEN, TRUE);
  if (ret != 0)
    {
      printf("rsa sign failed\n");
      goto free;
    }

  memset(expect, 0, sizeof(expect));
  mpi_from_string(expect, (FAR const unsigned char *)g_rsa_pkcs15_sig_2048,
                  strlen(g_rsa_pkcs15_sig_2048));
  ret = memcmp(sig, expect, MOD_2048_LEN);
  if (ret != 0)
    {
      printf("rsa test case 3 signature mismatch\n");
      goto free;
    }

  /* verify with the public key: the low bytes of sig ^ e % n are the
   * DigestInfo and hash, the high ones the padding.
   */

  rsa_reverse(hash_he, em + EM_2048_PREFIX_LEN, EM_2048_TLEN);
  rsa_reverse(pad_he, em, EM_2048_PREFIX_LEN);
  ret = cryptodev_rsa_verify(ctx, &rsa_data, sig, MOD_2048_LEN, hash_he,
                             EM_2048_TLEN, pad_he, EM_2048_PREFIX_LEN);
  if (ret != 0)
    {
      printf("rsa verify failed\n");
      goto free;
    }

  /* a tampered signature must not verify */

  sig[0] ^= 1;
  ret = cryptodev_rsa_verify(ctx, &rsa_data, sig, MOD_2048_LEN, hash_he,
                             EM_2048_TLEN, pad_he, EM_2048_PREFIX_LEN);
  if (ret == 0)
    {
      printf("rsa test case 3 tampered signature verified\n");
      ret = -1;
      goto free;
    }

  printf("rsa test case 3 success\n");
  ret = 0;

free:
  rsa_data_free(&rsa_data);
  return ret;
}

static int rsa_test(void)
{
  crypto_context_t ctx;
  int ret;

  ret = cryptodev_init(&ctx);
  if (ret != 0)
    {
      return ret;
    }

  ret = rsa_test_1024(&ctx);
  if (ret != 0)
    {
      goto free;
    }

  ret = rsa_test_2048_donly(&ctx);

free:
  cryptodev_free(&ctx);
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  setbuf(stdout, NULL);

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
