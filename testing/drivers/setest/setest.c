/****************************************************************************
 * apps/testing/drivers/setest/setest.c
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

/* Copyright 2023 NXP */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <fcntl.h>
#include <libgen.h>
#include <nuttx/crypto/se05x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFAULT_SETTINGS                                                     \
  {                                                                          \
    .se05x_dev = default_se05x_device, .skip_process = FALSE,                \
    .base_id = 0x230000                                                      \
  }

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct settings_t
{
  FAR char *se05x_dev;
  bool skip_process;
  uint32_t base_id;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char default_se05x_device[] = "/dev/se05x";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(FAR FILE *f, char *prg)
{
  fprintf(f, "%s - Secure Element Test\n", prg);
  fprintf(f, "\nUsage: %s [options] <secure element device>\n", prg);
  fprintf(f, "Options:\n");
  fprintf(f, "         -b <id>     (use base address <id>\n");
  fprintf(f, "                      default = 0x230000)\n");
}

static int parse_arguments(int argc, FAR char *argv[],
                           FAR struct settings_t *settings)
{
  int result = 0;
  int opt;
  FAR char *prg = basename(argv[0]);
  while (((opt = getopt(argc, argv, "b:h")) != -1) && (result == 0))
    {
      switch (opt)
        {
        case 'b':
          settings->base_id = (uint32_t)strtoul(optarg, NULL, 0);
          break;
        case 'h':
          print_usage(stdout, prg);
          settings->skip_process = TRUE;
          break;
        default:
          print_usage(stderr, prg);
          result = -EINVAL;
          break;
        }
    }

  if ((result == 0) && (!settings->skip_process))
    {
      settings->se05x_dev = default_se05x_device;

      /* if device is specified as positional argument */

      if (optind != argc)
        {
          settings->se05x_dev = argv[optind];
        }
    }

  return result;
}

static uint32_t get_se05x_id(FAR struct settings_t *settings, uint32_t id)
{
  return settings->base_id + id;
}

static void print_result(bool success)
{
  if (success)
    {
      puts(" SUCCESS");
    }
  else
    {
      puts(" FAIL");
    }
}

static int invert_result(int result)
{
  return result != 0 ? 0 : -1;
}

static int run_tests(int fd, FAR struct settings_t *settings)
{
  int result = 0;
  puts("Setup");
  for (uint32_t id = get_se05x_id(settings, 1);
       id < get_se05x_id(settings, 5); id++)
    {
      /* result is neglected because already empty entries result with
       * errors
       */

      (void)ioctl(fd, SEIOC_DELETE_KEY, id);
    }

  puts("Generate 3 keypairs");
  for (uint32_t id = get_se05x_id(settings, 1);
       (id < get_se05x_id(settings, 4)) && (result == 0); id++)
    {
      struct se05x_generate_keypair_s args = {
          .id = id, .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256
      };

      result = ioctl(fd, SEIOC_GENERATE_KEYPAIR, &args);
    }

  print_result(result == 0);

  uint8_t public_key[300];
  size_t public_key_size;
  if (result == 0)
    {
      puts("Read from pub #1");

      struct se05x_key_transmission_s args = {
          .entry = {.id = get_se05x_id(settings, 1),
                    .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256},
          .content = {.buffer = public_key,
                      .buffer_size = sizeof(public_key)}
      };

      result = ioctl(fd, SEIOC_GET_KEY, &args);

      public_key_size = args.content.buffer_content_size;

      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("Write to pub #4");

      struct se05x_key_transmission_s args = {
          .entry = {.id = get_se05x_id(settings, 4),
                    .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256},
          .content = {.buffer = public_key,
                      .buffer_size = public_key_size,
                      .buffer_content_size = public_key_size}
      };

      result = ioctl(fd, SEIOC_SET_KEY, &args);

      print_result(result == 0);
    }

  uint8_t hash[32] = {
          0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
          11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
          22, 23, 24, 25, 26, 27, 28, 29, 30, 31
  };

  uint8_t signature[300];
  size_t signature_content_size;
  if (result == 0)
    {
      puts("Create signature with priv #1");

      struct se05x_signature_s args = {
          .key_id = get_se05x_id(settings, 1),
          .algorithm = SE05X_ALGORITHM_SHA256,
          .tbs = {.buffer = hash,
                  .buffer_size = sizeof(hash),
                  .buffer_content_size = sizeof(hash)},
          .signature = {.buffer = signature,
                        .buffer_size = sizeof(signature)},
      };

      result = ioctl(fd, SEIOC_CREATE_SIGNATURE, &args);
      signature_content_size = args.signature.buffer_content_size;
      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("Verify signature with pub #4");

      struct se05x_signature_s args = {
          .key_id = get_se05x_id(settings, 4),
          .algorithm = SE05X_ALGORITHM_SHA256,
          .tbs = {.buffer = hash,
                  .buffer_size = sizeof(hash),
                  .buffer_content_size = sizeof(hash)},
          .signature = {.buffer = signature,
                        .buffer_content_size = signature_content_size},
      };

      result = ioctl(fd, SEIOC_VERIFY_SIGNATURE, &args);

      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("Verification of signature with pub #2 should be dissapproved");

      struct se05x_signature_s args = {
          .key_id = get_se05x_id(settings, 2),
          .algorithm = SE05X_ALGORITHM_SHA256,
          .tbs = {.buffer = hash, .buffer_size = sizeof(hash)},
          .signature = {.buffer = signature,
                        .buffer_content_size = signature_content_size},
      };

      result = invert_result(ioctl(fd, SEIOC_VERIFY_SIGNATURE, &args));

      print_result(result == 0);
    }

  uint8_t symm_key_a[32];
  if (result == 0)
    {
      puts("Derive symm key #a from priv #1 and pub #2");

      struct se05x_derive_key_s args = {
          .private_key_id = get_se05x_id(settings, 1),
          .public_key_id = get_se05x_id(settings, 2),
          .content = {.buffer = symm_key_a,
                      .buffer_size = sizeof(symm_key_a)},
      };

      result = ioctl(fd, SEIOC_DERIVE_SYMM_KEY, &args);

      print_result(result == 0);
    }

  uint8_t symm_key_b[32];
  if (result == 0)
    {
      puts("Derive symm key #b from priv #2 and pub #1");

      struct se05x_derive_key_s args = {
          .private_key_id = get_se05x_id(settings, 2),
          .public_key_id = get_se05x_id(settings, 1),
          .content = {.buffer = symm_key_b,
                      .buffer_size = sizeof(symm_key_b)},
      };

      result = ioctl(fd, SEIOC_DERIVE_SYMM_KEY, &args);

      print_result(result == 0);
    }

  uint8_t symm_key_c[32];
  if (result == 0)
    {
      puts("Derive symm key #c from priv #1 and pub #3");

      struct se05x_derive_key_s args = {
          .private_key_id = get_se05x_id(settings, 1),
          .public_key_id = get_se05x_id(settings, 3),
          .content = {.buffer = symm_key_c,
                      .buffer_size = sizeof(symm_key_c)},
      };

      result = ioctl(fd, SEIOC_DERIVE_SYMM_KEY, &args);

      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("Symm key #a and #b should be equal");

      result = memcmp(symm_key_a, symm_key_b, sizeof(symm_key_a));

      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("Symm key #a and #c should be different");

      result =
          invert_result(memcmp(symm_key_a, symm_key_c, sizeof(symm_key_a)));

      print_result(result == 0);
    }

  if (result == 0)
    {
      puts("All tests succeeded!\n");
    }

  return result;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct settings_t settings = DEFAULT_SETTINGS;
  int result = parse_arguments(argc, argv, &settings);

  int fd;
  if ((result == 0) && (!settings.skip_process))
    {
      fd = open(settings.se05x_dev, O_RDONLY);
      if (fd == -1)
        {
          result = -ENODEV;
        }
      else
        {
          run_tests(fd, &settings);
          close(fd);
        }
    }

  if (result != 0)
    {
      fprintf(stderr, "err %i: %s\n", -result, strerror(-result));
    }

  return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
