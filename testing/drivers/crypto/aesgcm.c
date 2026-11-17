/****************************************************************************
 * apps/testing/drivers/crypto/aesgcm.c
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

#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

/* AES-GCM test data from the GCM spec (McGrew/Viega), appendix B,
 * test case 4.
 */

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const unsigned char g_gcm_aes_key[16] =
{
  0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
  0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08
};

static const unsigned char g_gcm_iv[12] =
{
  0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
  0xde, 0xca, 0xf8, 0x88
};

/* A 60-byte message (the 64-byte spec message without the last block),
 * so that a request boundary also lands mid-block.
 */

static const unsigned char g_gcm_aad[20] =
{
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
  0xab, 0xad, 0xda, 0xd2
};

static const unsigned char g_gcm_plain[60] =
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

/* Expected ciphertext and tag, GCM spec test case 4 parameters.
 * The 8-byte IV exercises the GHASH-derived pre-counter block.
 */

static const unsigned char g_gcm_ct_iv12[60] =
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

static const unsigned char g_gcm_tag_iv12[16] =
{
  0x5b, 0xc9, 0x4f, 0xbc, 0x32, 0x21, 0xa5, 0xdb,
  0x94, 0xfa, 0xe9, 0x5a, 0xe7, 0x12, 0x1a, 0x47
};

static const unsigned char g_gcm_ct_iv8[60] =
{
  0x61, 0x35, 0x3b, 0x4c, 0x28, 0x06, 0x93, 0x4a,
  0x77, 0x7f, 0xf5, 0x1f, 0xa2, 0x2a, 0x47, 0x55,
  0x69, 0x9b, 0x2a, 0x71, 0x4f, 0xcd, 0xc6, 0xf8,
  0x37, 0x66, 0xe5, 0xf9, 0x7b, 0x6c, 0x74, 0x23,
  0x73, 0x80, 0x69, 0x00, 0xe4, 0x9f, 0x24, 0xb2,
  0x2b, 0x09, 0x75, 0x44, 0xd4, 0x89, 0x6b, 0x42,
  0x49, 0x89, 0xb5, 0xe1, 0xeb, 0xac, 0x0f, 0x07,
  0xc2, 0x3f, 0x45, 0x98
};

static const unsigned char g_gcm_tag_iv8[16] =
{
  0x36, 0x12, 0xd2, 0xe7, 0x9e, 0x3b, 0x07, 0x85,
  0x56, 0x1b, 0xe1, 0x4a, 0xac, 0xa2, 0xfc, 0xcb
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Run one AEAD operation, split into three CIOCCRYPT requests: the IV and
 * the AAD first (COP_FLAG_UPDATE_AAD), the payload next
 * (COP_FLAG_UPDATE), and the tag last.
 */

static int syscrypt_gcm(int op, FAR const unsigned char *key, int key_len,
                        FAR const unsigned char *iv, size_t iv_len,
                        FAR const unsigned char *aad, size_t aad_len,
                        FAR const unsigned char *input,
                        FAR unsigned char *output, size_t len,
                        FAR unsigned char *tag, size_t tag_len)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;

  if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      perror("/dev/crypto");
      goto err;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) == -1)
    {
      perror("CRIOGET");
      goto err;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = CRYPTO_AES_GCM_16;
  session.key = (caddr_t)key;
  session.keylen = key_len;
  session.mac = CRYPTO_AES_128_GMAC;
  session.mackey = (caddr_t)key;
  session.mackeylen = key_len;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      perror("CIOCGSESSION");
      goto err;
    }

  /* UPDATE_AAD - IV + AAD, no data */

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = op;
  cryp.flags = COP_FLAG_UPDATE_AAD;
  cryp.iv = (caddr_t)iv;
  cryp.ivlen = iv_len;
  cryp.aad = (caddr_t)aad;
  cryp.aadlen = aad_len;
  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT UPDATE_AAD");
      goto err;
    }

  /* UPDATE - data only, no IV, no AAD */

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = op;
  cryp.flags = COP_FLAG_UPDATE;
  cryp.len = len;
  cryp.src = (caddr_t)input;
  cryp.dst = (caddr_t)output;
  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT UPDATE");
      goto err;
    }

  /* FINAL - get tag */

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = op;
  cryp.mac = (caddr_t)tag;
  cryp.olen = tag_len;
  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      perror("CIOCCRYPT FINAL");
      goto err;
    }

  if (ioctl(cryptodev_fd, CIOCFSESSION, &session.ses) == -1)
    {
      perror("CIOCFSESSION");
      goto err;
    }

  close(cryptodev_fd);
  close(fd);
  return 0;

err:
  if (cryptodev_fd != -1)
    {
      close(cryptodev_fd);
    }

  if (fd != -1)
    {
      close(fd);
    }

  return -1;
}

static int match(FAR const unsigned char *a, FAR const unsigned char *b,
                 size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  warnx("data mismatch");

  for (i = 0; i < (int)len; i++)
    {
      printf("%2.2x", a[i]);
    }

  printf("\n");
  for (i = 0; i < (int)len; i++)
    {
      printf("%2.2x", b[i]);
    }

  printf("\n");

  return -1;
}

static int gcm_roundtrip(FAR const unsigned char *iv, size_t iv_len)
{
  FAR const unsigned char *exp_ct;
  FAR const unsigned char *exp_tag;
  unsigned char ciphertext[sizeof(g_gcm_plain)];
  unsigned char plaintext[sizeof(g_gcm_plain)];
  unsigned char tag[16];
  unsigned char tag_check[16];

  if (syscrypt_gcm(COP_ENCRYPT, g_gcm_aes_key, sizeof(g_gcm_aes_key),
                   iv, iv_len, g_gcm_aad, sizeof(g_gcm_aad),
                   g_gcm_plain, ciphertext, sizeof(g_gcm_plain),
                   tag, sizeof(tag)) != 0)
    {
      printf("aesgcm encrypt failed (iv length %d)\n", (int)iv_len);
      return -1;
    }

  if (iv_len == sizeof(g_gcm_iv))
    {
      exp_ct = g_gcm_ct_iv12;
      exp_tag = g_gcm_tag_iv12;
    }
  else
    {
      exp_ct = g_gcm_ct_iv8;
      exp_tag = g_gcm_tag_iv8;
    }

  if (match(ciphertext, exp_ct, sizeof(g_gcm_plain)) != 0 ||
      match(tag, exp_tag, sizeof(tag)) != 0)
    {
      printf("aesgcm result mismatch (iv length %d)\n", (int)iv_len);
      return -1;
    }

  if (syscrypt_gcm(COP_DECRYPT, g_gcm_aes_key, sizeof(g_gcm_aes_key),
                   iv, iv_len, g_gcm_aad, sizeof(g_gcm_aad),
                   ciphertext, plaintext, sizeof(plaintext),
                   tag_check, sizeof(tag_check)) != 0)
    {
      printf("aesgcm decrypt failed (iv length %d)\n", (int)iv_len);
      return -1;
    }

  if (match(plaintext, g_gcm_plain, sizeof(g_gcm_plain)) != 0)
    {
      printf("aesgcm plaintext mismatch (iv length %d)\n", (int)iv_len);
      return -1;
    }

  if (match(tag_check, tag, sizeof(tag)) != 0)
    {
      printf("aesgcm tag mismatch (iv length %d)\n", (int)iv_len);
      return -1;
    }

  printf("aesgcm test ok (iv length %d)\n", (int)iv_len);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  setbuf(stdout, NULL);

  /* Standard 96-bit IV, and a 60-bit IV that exercises the GHASH-based
   * pre-counter block derivation for non-96-bit IVs.
   */

  if (gcm_roundtrip(g_gcm_iv, sizeof(g_gcm_iv)) != 0)
    {
      return -1;
    }

  return gcm_roundtrip(g_gcm_iv, 8);
}
