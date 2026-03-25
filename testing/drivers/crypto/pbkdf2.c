/****************************************************************************
 * apps/testing/drivers/crypto/pbkdf2.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <crypto/cryptodev.h>

struct tb
{
  FAR char *key;
  int keylen;
  FAR char *data;
  int datalen;
  int iterations;
  int dklen;
}
pbkdf2_testcases[] =
{
    {
      "password",
      8,
      "salt",
      4,
      1,
      20,
    },
    {
      "password",
      8,
      "salt",
      4,
      2,
      20,
    },
    {
      "password",
      8,
      "salt",
      4,
      4096,
      20,
    },
    {
      "password",
      8,
      "salt",
      4,
      16777216,
      20,
    },
    {
      "passwordPASSWORDpassword",
      24,
      "saltSALTsaltSALTsaltSALTsaltSALTsalt",
      36,
      4096,
      25,
    },
    {
      "pass\0word",
      9,
      "sa\0lt",
      5,
      4096,
      16,
    },
};

FAR char *pbkdf2_sha1_result[] =
{
  "\x0c\x60\xc8\x0f\x96\x1f\x0e\x71\xf3\xa9\xb5\x24\xaf\x60\x12\x06"
  "\x2f\xe0\x37\xa6",
  "\xea\x6c\x01\x4d\xc7\x2d\x6f\x8c\xcd\x1e\xd9\x2a\xce\x1d\x41\xf0"
  "\xd8\xde\x89\x57",
  "\x4b\x00\x79\x01\xb7\x65\x48\x9a\xbe\xad\x49\xd9\x26\xf7\x21\xd0"
  "\x65\xa4\x29\xc1",
  "\xee\xfe\x3d\x61\xcd\x4d\xa4\xe4\xe9\x94\x5b\x3d\x6b\xa2\x15\x8c"
  "\x26\x34\xe9\x84",
  "\x3d\x2e\xec\x4f\xe4\x1c\x84\x9b\x80\xc8\xd8\x36\x62\xc0\xe4\x4a"
  "\x8b\x29\x1a\x96\x4c\xf2\xf0\x70\x38",
  "\x56\xfa\x6a\xa7\x55\x48\x09\x9d\xcc\x37\xd7\xf0\x34\x25\xe0\xc3",
};

FAR char *pbkdf2_sha256_result[] =
{
  "\x12\x0f\xb6\xcf\xfc\xf8\xb3\x2c\x43\xe7\x22\x52\x56\xc4\xf8\x37"
  "\xa8\x65\x48\xc9",
  "\xae\x4d\x0c\x95\xaf\x6b\x46\xd3\x2d\x0a\xdf\xf9\x28\xf0\x6d\xd0"
  "\x2a\x30\x3f\x8e",
  "\xc5\xe4\x78\xd5\x92\x88\xc8\x41\xaa\x53\x0d\xb6\x84\x5c\x4c\x8d"
  "\x96\x28\x93\xa0",
  "\xcf\x81\xc6\x6f\xe8\xcf\xc0\x4d\x1f\x31\xec\xb6\x5d\xab\x40\x89"
  "\xf7\xf1\x79\xe8",
  "\x34\x8c\x89\xdb\xcb\xd3\x2b\x2f\x32\xd8\x14\xb8\x11\x6e\x84\xcf"
  "\x2b\x17\x34\x7e\xbc\x18\x00\x18\x1c",
  "\x89\xb6\x9d\x05\x16\xf8\x29\x89\x3c\x69\x62\x26\x65\x0a\x86\x87",
};

int syspbkdf2(int mac, FAR const char *key, size_t keylen,
            FAR const char *s, size_t len, int iterations,
            size_t dklen, FAR char *out)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;

  if ((fd = open("/dev/crypto", O_RDWR, 0)) < 0)
    {
      warn("/dev/crypto");
      goto err;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) == -1)
    {
      warn("CRIOGET");
      goto err;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = 0;
  session.mac = mac;
  session.mackey = (caddr_t)key;
  session.mackeylen = keylen;
  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) == -1)
    {
      warn("CIOCGSESSION");
      goto err;
    }
  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.flags = 0;
  cryp.src = (caddr_t)s;
  cryp.len = len;
  cryp.dst = 0;
  cryp.mac = (caddr_t) out;
  cryp.iv = 0;
  cryp.iterations = iterations;
  cryp.olen = dklen;

  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) == -1)
    {
      warn("CIOCCRYPT");
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

  return 1;
}

static int match(unsigned char *a, unsigned char *b, size_t len)
{
  int i;

  if (memcmp(a, b, len) == 0)
    return (0);

  warnx("pbkdf2 mismatch\n");

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

  return (1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(void)
{
  char output[40];
  int ret = 0;
  for (int i = 0; i < 6; i++)
    {
      ret = syspbkdf2(CRYPTO_PBKDF2_HMAC_SHA1, pbkdf2_testcases[i].key,
                      pbkdf2_testcases[i].keylen,
                      pbkdf2_testcases[i].data,
                      pbkdf2_testcases[i].datalen,
                      pbkdf2_testcases[i].iterations,
                      pbkdf2_testcases[i].dklen, output);
      if (ret)
        {
          printf("PBKDF2 SHA1 failed\n");
        }

      ret += match((unsigned char *)pbkdf2_sha1_result[i],
                   (unsigned char *)output,
                   pbkdf2_testcases[i].dklen);
      if (ret)
        {
          printf("match PBKDF2 SHA1 failed\n");
        }
      else
        {
          printf("hmac PBKDF2 SHA1 success\n");
        }
    }

  for (int i = 0; i < 6; i++)
    {
      ret = syspbkdf2(CRYPTO_PBKDF2_HMAC_SHA256, pbkdf2_testcases[i].key,
                      pbkdf2_testcases[i].keylen,
                      pbkdf2_testcases[i].data,
                      pbkdf2_testcases[i].datalen,
                      pbkdf2_testcases[i].iterations,
                      pbkdf2_testcases[i].dklen, output);
      if (ret)
        {
          printf("PBKDF2 SHA256 failed\n");
        }

      ret += match((unsigned char *)pbkdf2_sha256_result[i],
                   (unsigned char *)output,
                   pbkdf2_testcases[i].dklen);
      if (ret)
        {
          printf("match PBKDF2 SHA256 failed\n");
        }
      else
        {
          printf("hmac PBKDF2 SHA256 success\n");
        }
    }

  return 0;
}
