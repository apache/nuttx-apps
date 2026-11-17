/****************************************************************************
 * apps/testing/drivers/crypto/clone.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <crypto/cryptodev.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DATA_LEN  64
#define HALF_LEN  32
#define MAC_MAX   32

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct clone_alg_s
{
  uint32_t cipher;
  uint32_t mac;
  size_t keylen;
  size_t maclen;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_key[32] =
{
  0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
  0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
  0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
};

static const uint8_t g_iv[16] =
{
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
};

static uint8_t g_data[DATA_LEN];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int sysclone_init(void)
{
  int cfd = -1;
  int fd;

  fd = open("/dev/crypto", O_RDWR, 0);
  if (fd < 0)
    {
      warn("/dev/crypto");
      return -1;
    }

  if (ioctl(fd, CRIOGET, &cfd) == -1)
    {
      warn("CRIOGET");
      close(fd);
      return -1;
    }

  close(fd);
  return cfd;
}

static int sysclone_start(int cfd, FAR const struct clone_alg_s *alg)
{
  struct session_op sop;

  memset(&sop, 0, sizeof(sop));
  sop.cipher = alg->cipher;
  sop.mac = alg->mac;
  if (alg->cipher != 0)
    {
      sop.key = (caddr_t)g_key;
      sop.keylen = alg->keylen;
    }

  if (alg->mac != 0)
    {
      sop.mackey = (caddr_t)g_key;
      sop.mackeylen = alg->keylen;
    }

  if (ioctl(cfd, CIOCGSESSION, &sop) == -1)
    {
      warn("CIOCGSESSION");
      return -1;
    }

  return sop.ses;
}

static int sysclone_clone(int cfd)
{
  int nfd = -1;

  if (ioctl(cfd, CIOCCLONE, &nfd) == -1)
    {
      warn("CIOCCLONE");
      return -1;
    }

  return nfd;
}

static int sysclone_update(int cfd, uint32_t ses,
                           FAR const uint8_t *src, size_t len)
{
  struct crypt_op cop;

  memset(&cop, 0, sizeof(cop));
  cop.ses = ses;
  cop.op = COP_ENCRYPT;
  cop.flags = COP_FLAG_UPDATE;
  cop.src = (caddr_t)src;
  cop.len = len;
  if (ioctl(cfd, CIOCCRYPT, &cop) == -1)
    {
      warn("CIOCCRYPT update");
      return -1;
    }

  return 0;
}

static int sysclone_finish(int cfd, uint32_t ses,
                           FAR uint8_t *mac, size_t maclen)
{
  struct crypt_op cop;

  memset(&cop, 0, sizeof(cop));
  cop.ses = ses;
  cop.op = COP_ENCRYPT;
  cop.olen = maclen;
  cop.mac = (caddr_t)mac;
  if (ioctl(cfd, CIOCCRYPT, &cop) == -1)
    {
      warn("CIOCCRYPT finish");
      return -1;
    }

  return 0;
}

/* One-shot encryption; the driver loads the IV from the request. */

static int sysclone_crypt(int cfd, uint32_t ses, FAR const uint8_t *src,
                          FAR uint8_t *dst, size_t len)
{
  struct crypt_op cop;
  uint8_t iv[sizeof(g_iv)];

  memcpy(iv, g_iv, sizeof(iv));
  memset(&cop, 0, sizeof(cop));
  cop.ses = ses;
  cop.op = COP_ENCRYPT;
  cop.src = (caddr_t)src;
  cop.dst = (caddr_t)dst;
  cop.len = len;
  cop.iv = (caddr_t)iv;
  cop.ivlen = sizeof(iv);
  if (ioctl(cfd, CIOCCRYPT, &cop) == -1)
    {
      warn("CIOCCRYPT");
      return -1;
    }

  return 0;
}

static int match(FAR const uint8_t *a, FAR const uint8_t *b, size_t len)
{
  size_t i;

  if (memcmp(a, b, len) == 0)
    {
      return 0;
    }

  warnx("clone mismatch");

  for (i = 0; i < len; i++)
    {
      printf("%02x", a[i]);
    }

  printf("\n");
  for (i = 0; i < len; i++)
    {
      printf("%02x", b[i]);
    }

  printf("\n");
  return 1;
}

/* Hash the first half, clone, then finish the source and the clone
 * separately. Both must match a one-shot digest of the whole input.
 */

static int clone_mac_check(FAR const struct clone_alg_s *alg,
                           bool closesrc)
{
  uint8_t expect[MAC_MAX];
  uint8_t mac[MAC_MAX];
  uint32_t ses;
  int cfd;
  int nfd;
  int ret = 0;

  cfd = sysclone_init();
  if (cfd < 0)
    {
      return 1;
    }

  ses = sysclone_start(cfd, alg);
  if (ses == (uint32_t)-1)
    {
      goto err_cfd;
    }

  if (sysclone_update(cfd, ses, g_data, DATA_LEN) < 0 ||
      sysclone_finish(cfd, ses, expect, alg->maclen) < 0)
    {
      goto err_cfd;
    }

  close(cfd);

  cfd = sysclone_init();
  if (cfd < 0)
    {
      return 1;
    }

  ses = sysclone_start(cfd, alg);
  if (ses == (uint32_t)-1)
    {
      goto err_cfd;
    }

  if (sysclone_update(cfd, ses, g_data, HALF_LEN) < 0)
    {
      goto err_cfd;
    }

  nfd = sysclone_clone(cfd);
  if (nfd < 0)
    {
      goto err_cfd;
    }

  if (!closesrc)
    {
      if (sysclone_update(cfd, ses, g_data + HALF_LEN,
                          DATA_LEN - HALF_LEN) < 0 ||
          sysclone_finish(cfd, ses, mac, alg->maclen) < 0)
        {
          goto err_nfd;
        }

      ret = match(expect, mac, alg->maclen);
    }

  close(cfd);

  if (sysclone_update(nfd, ses, g_data + HALF_LEN,
                      DATA_LEN - HALF_LEN) < 0 ||
      sysclone_finish(nfd, ses, mac, alg->maclen) < 0)
    {
      goto err_nfd;
    }

  ret |= match(expect, mac, alg->maclen);
  close(nfd);
  return ret;

err_nfd:
  close(nfd);

err_cfd:
  close(cfd);
  return 1;
}

/* Clone a cipher session, close the source descriptor, then encrypt with
 * the clone. The result must match a one-shot encryption with a fresh
 * session, so the clone owns an independent copy of the key schedule.
 */

static int clone_cipher_check(FAR const struct clone_alg_s *alg)
{
  uint8_t expect[DATA_LEN];
  uint8_t out[DATA_LEN];
  uint32_t ses;
  int cfd;
  int nfd;

  cfd = sysclone_init();
  if (cfd < 0)
    {
      return 1;
    }

  ses = sysclone_start(cfd, alg);
  if (ses == (uint32_t)-1)
    {
      goto err_cfd;
    }

  if (sysclone_crypt(cfd, ses, g_data, expect, DATA_LEN) < 0)
    {
      goto err_cfd;
    }

  close(cfd);

  cfd = sysclone_init();
  if (cfd < 0)
    {
      return 1;
    }

  ses = sysclone_start(cfd, alg);
  if (ses == (uint32_t)-1)
    {
      goto err_cfd;
    }

  nfd = sysclone_clone(cfd);
  if (nfd < 0)
    {
      goto err_cfd;
    }

  close(cfd);

  if (sysclone_crypt(nfd, ses, g_data, out, DATA_LEN) < 0)
    {
      goto err_nfd;
    }

  close(nfd);
  return match(expect, out, DATA_LEN);

err_nfd:
  close(nfd);

err_cfd:
  close(cfd);
  return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  static const struct clone_alg_s sha256 =
    {
      0, CRYPTO_SHA2_256, 0, 32
    };

  static const struct clone_alg_s hmac_sha256 =
    {
      0, CRYPTO_SHA2_256_HMAC, 32, 32
    };

  static const struct clone_alg_s aes_cmac =
    {
      CRYPTO_AES_CMAC, CRYPTO_AES_128_CMAC, 16, 16
    };

  static const struct clone_alg_s aes_cbc =
    {
      CRYPTO_AES_CBC, 0, 16, 0
    };

  size_t i;
  int ret;
  int err = 0;

  setbuf(stdout, NULL);

  for (i = 0; i < DATA_LEN; i++)
    {
      g_data[i] = (uint8_t)i;
    }

  ret = clone_mac_check(&sha256, false);
  printf("clone sha256 %s\n", ret ? "failed" : "success");
  err |= ret;

  ret = clone_mac_check(&hmac_sha256, false);
  printf("clone hmac-sha256 %s\n", ret ? "failed" : "success");
  err |= ret;

  ret = clone_mac_check(&aes_cmac, false);
  printf("clone aes-cmac %s\n", ret ? "failed" : "success");
  err |= ret;

  ret = clone_mac_check(&sha256, true);
  printf("clone close-source %s\n", ret ? "failed" : "success");
  err |= ret;

  ret = clone_cipher_check(&aes_cbc);
  printf("clone aes-cbc %s\n", ret ? "failed" : "success");
  err |= ret;

  return err ? 1 : 0;
}
