/****************************************************************************
 * apps/fsutils/passwd/passwd_pbkdf2.c
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

#include <crypto/cryptodev.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "passwd_pbkdf2.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int passwd_pbkdf2_hmac_sha256(FAR const uint8_t *pass, size_t passlen,
                              FAR const uint8_t *salt, size_t saltlen,
                              uint32_t iterations,
                              FAR uint8_t *out, size_t outlen)
{
  struct session_op session;
  struct crypt_op cryp;
  int cryptodev_fd = -1;
  int fd = -1;
  int ret = 0;

  if (pass == NULL || salt == NULL || out == NULL || passlen == 0 ||
      saltlen == 0 || iterations == 0 || outlen == 0)
    {
      return -EINVAL;
    }

  fd = open("/dev/crypto", O_RDWR, 0);
  if (fd < 0)
    {
      return -errno;
    }

  if (ioctl(fd, CRIOGET, &cryptodev_fd) < 0)
    {
      ret = -errno;
      goto errout;
    }

  memset(&session, 0, sizeof(session));
  session.cipher = 0;
  session.mac = CRYPTO_PBKDF2_HMAC_SHA256;
  session.mackey = (caddr_t)pass;
  session.mackeylen = passlen;

  if (ioctl(cryptodev_fd, CIOCGSESSION, &session) < 0)
    {
      ret = -errno;
      goto errout;
    }

  memset(&cryp, 0, sizeof(cryp));
  cryp.ses = session.ses;
  cryp.op = COP_ENCRYPT;
  cryp.src = (caddr_t)salt;
  cryp.len = saltlen;
  cryp.mac = (caddr_t)out;
  cryp.iterations = iterations;
  cryp.olen = outlen;

  if (ioctl(cryptodev_fd, CIOCCRYPT, &cryp) < 0)
    {
      ret = -errno;
      goto errout_with_session;
    }

errout_with_session:
  ioctl(cryptodev_fd, CIOCFSESSION, &session.ses);

errout:
  if (cryptodev_fd >= 0)
    {
      close(cryptodev_fd);
    }

  if (fd >= 0)
    {
      close(fd);
    }

  return ret;
}
