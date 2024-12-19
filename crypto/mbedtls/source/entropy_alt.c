/****************************************************************************
 * apps/crypto/mbedtls/source/entropy_alt.c
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <psa/crypto.h>
#include <psa/crypto_platform.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_MBEDTLS_ENTROPY_HARDWARE_ALT
int mbedtls_hardware_poll(FAR void *data,
                          FAR unsigned char *output,
                          size_t len,
                          FAR size_t *olen)
{
  int fd;
  size_t read_len;
  *olen = 0;

  fd = open("/dev/random", O_RDONLY, 0);
  if (fd < 0)
    {
      return -errno;
    }

  read_len = read(fd, output, len);
  if (read_len != len)
    {
      close(fd);
      return -errno;
    }

  close(fd);
  *olen = len;

  return 0;
}
#endif /* CONFIG_MBEDTLS_ENTROPY_HARDWARE_ALT */

#ifdef CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG
psa_status_t mbedtls_psa_external_get_random(
  FAR mbedtls_psa_external_random_context_t *context,
  FAR uint8_t *output, size_t output_size, FAR size_t *output_length)
{
  int fd;
  size_t read_len;
  *output_length = 0;

  (void)context;

  fd = open("/dev/random", O_RDONLY, 0);
  if (fd < 0)
    {
      return -errno;
    }

  read_len = read(fd, output, output_size);
  if (read_len != output_size)
    {
      close(fd);
      return -errno;
    }

  close(fd);
  *output_length = read_len;

  return 0;
}
#endif /* CONFIG_MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG */
