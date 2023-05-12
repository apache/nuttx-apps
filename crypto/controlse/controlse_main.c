/****************************************************************************
 * apps/crypto/controlse/controlse_main.c
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

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <nuttx/config.h>
#include <nuttx/crypto/se05x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef CONFIG_STACK_COLORATION
#include <nuttx/arch.h>
#include <nuttx/sched.h>
#endif

#include "x509_utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define EOT 0x04

#define DEFAULT_SETTINGS                                                     \
  {                                                                          \
    .se05x_dev_filename = default_se05x_device, 0                            \
  }

#define TBS_HASH_BUFFER_SIZE 32
#define SIGNATURE_BUFFER_SIZE 300
#define SYMM_KEY_BUFFER_SIZE 300
#define RAW_KEY_BUFFER_SIZE 600
#define DEFAULT_BUFFER_SIZE 1000

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum
{
  KEYSTORE_NO_ACTION = 0,
  KEYSTORE_READ,
  KEYSTORE_WRITE,
  KEYSTORE_GENERATE,
  KEYSTORE_DELETE,
  KEYSTORE_DERIVE_SYMM_KEY,
  KEYSTORE_CREATE_SIGNATURE,
  KEYSTORE_VERIFY_SIGNATURE,
  KEYSTORE_SIGN_CSR,
  KEYSTORE_VERIFY_CERTIFICATE,
  KEYSTORE_GET_INFO,
  KEYSTORE_GET_UID,
} keystore_operation;

struct settings_t
{
  FAR const char *se05x_dev_filename;
  FAR char *input_filename;
  FAR char *signature_filename;
  bool skip_process;
  keystore_operation operation;
  bool raw_data_in_device;
  bool interface_with_pem;
  uint32_t key_id;
  uint32_t private_key_id;
  bool show_stack_used;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char default_se05x_device[] = "/dev/se05x";
static const char enter_key_hex[] = "enter key(hex)";
static const char enter_data_pem[] = "enter data(pem)";
static const char enter_data_raw[] = "enter data(raw)";

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void print_usage(FAR FILE *f, FAR char *prg)
{
  fprintf(f, "%s - Control SE05x Secure Element\n", prg);
  fprintf(f, "\nUsage: %s [options] <secure element device>\n", prg);
  fprintf(f, "Options:\n");
  fprintf(f, "         -r <id>     (read item from keystore at <id>)\n");
  fprintf(f, "         -w <id>     (set item in keystore at <id>)\n");
  fprintf(f, "         -g <id>     (generate keypair at <id>)\n");
  fprintf(f, "         -d <id>     (delete key at <id>)\n");
  fprintf(f, "         -s <id>     (create signature for data\n");
  fprintf(f, "                      with key at <id>)\n");
  fprintf(f, "         -S <id>     (Sign CSR with key at <id>)\n");
  fprintf(f, "         -v <id>     (verify signature for data\n");
  fprintf(f, "                      with key at <id>)\n");
  fprintf(f, "         -V <id>     (verify CRT with key at <id>\n");
  fprintf(f, "         -a <id>     (derive symm key\n");
  fprintf(f, "                      from public key <id>\n");
  fprintf(f, "                      and private key)\n");
  fprintf(f, "         -t          (interface using raw data\n");
  fprintf(f, "                      use with -r, -w)\n");
  fprintf(f, "         -c          (interface with PEM format\n");
  fprintf(f, "                      internally using DER\n");
  fprintf(f, "                      use with -r, -w)\n");
  fprintf(f, "         -p <id>     (select private key\n");
  fprintf(f, "                      use with -a)\n");
  fprintf(f, "         -n <file>   (Read input from file)\n");
  fprintf(f, "         -N <file>   (Read signature from file)\n");
  fprintf(f, "         -i          (show generic information)\n");
  fprintf(f, "         -u          (show UID)\n");
  fprintf(f, "         -m          (show used stack memory space)\n");
  fprintf(f, "         -h          (show this help)\n");
  fprintf(f, "\n");
}

static int set_operation(FAR struct settings_t *settings,
                         keystore_operation operation, FAR char *key_id_text)
{
  int result = -EPERM;
  if (settings->operation == KEYSTORE_NO_ACTION)
    {
      settings->operation = operation;
      settings->key_id = 0;
      if (key_id_text != NULL)
        {
          settings->key_id = (uint32_t)strtoul(key_id_text, NULL, 0);
        }

      result = 0;
    }

  return result;
}

static int parse_arguments(int argc, FAR char *argv[],
                           FAR struct settings_t *settings)
{
  int result = 0;
  int opt;
  FAR char *prg = basename(argv[0]);
  while (
      ((opt = getopt(argc, argv, "iug:w:r:d:s:v:S:V:tca:p:n:N:mh")) != -1) &&
      (result == 0))
    {
      switch (opt)
        {
        case 'i':
          result = set_operation(settings, KEYSTORE_GET_INFO, optarg);
          break;
        case 'u':
          result = set_operation(settings, KEYSTORE_GET_UID, optarg);
          break;
        case 'g':
          result = set_operation(settings, KEYSTORE_GENERATE, optarg);
          break;
        case 'r':
          result = set_operation(settings, KEYSTORE_READ, optarg);
          break;
        case 'w':
          result = set_operation(settings, KEYSTORE_WRITE, optarg);
          break;
        case 't':
          settings->raw_data_in_device = TRUE;
          break;
        case 'c':
          settings->interface_with_pem = TRUE;
          settings->raw_data_in_device = TRUE;
          break;
        case 'd':
          result = set_operation(settings, KEYSTORE_DELETE, optarg);
          break;
        case 's':
          result =
            set_operation(settings, KEYSTORE_CREATE_SIGNATURE, optarg);
          break;
        case 'v':
          result =
            set_operation(settings, KEYSTORE_VERIFY_SIGNATURE, optarg);
          break;
        case 'S':
          result = set_operation(settings, KEYSTORE_SIGN_CSR, optarg);
          break;
        case 'V':
          result =
              set_operation(settings, KEYSTORE_VERIFY_CERTIFICATE, optarg);
          break;
        case 'a':
          result = set_operation(settings, KEYSTORE_DERIVE_SYMM_KEY, optarg);
          break;
        case 'p':
          settings->private_key_id = (uint32_t)strtoul(optarg, NULL, 0);
          break;
        case 'n':
          settings->input_filename = optarg;
          break;
        case 'N':
          settings->signature_filename = optarg;
          break;
        case 'm':
          settings->show_stack_used = TRUE;
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
      if (settings->operation == KEYSTORE_NO_ACTION)
        {
          print_usage(stderr, prg);
          result = -EINVAL;
        }

      /* if device is specified as positional argument */

      if (optind != argc)
        {
          settings->se05x_dev_filename = argv[optind];
        }
    }

  return result;
}

static int convert_array_hex_to_bin(FAR char *hex_buffer,
                                    size_t hex_buffer_size,
                                    FAR uint8_t *bin_buffer,
                                    size_t bin_buffer_size,
                                    FAR size_t *bin_buffer_content_size)
{
  hex_buffer_size = strcspn(hex_buffer, " \r\n");
  if (hex_buffer_size & 1)
    {
      return -1;
    }

  *bin_buffer_content_size = 0;
  size_t hex_buffer_pos;
  for (hex_buffer_pos = 0; (hex_buffer_pos < hex_buffer_size) &&
                           (*bin_buffer_content_size < bin_buffer_size);
       hex_buffer_pos += 2)
    {
      sscanf(&hex_buffer[hex_buffer_pos], "%2hhx",
             &bin_buffer[*bin_buffer_content_size]);
      (*bin_buffer_content_size)++;
    }

  return hex_buffer_pos == hex_buffer_size ? 0 : -1;
}

static int read_from_file(FAR const char *prompt, FAR char *filename,
                          FAR char *buffer, size_t buffer_size)
{
  FAR FILE *f = stdin;
  if (filename != NULL)
    {
      f = fopen(filename, "r");
    }
  else
    {
      puts(prompt);
    }

  size_t buffer_content_size = 0;
  if (f != NULL)
    {
      FAR char *c = buffer;
      int result = fgetc(f);
      while ((result != EOF) && (result != EOT) &&
             (buffer_content_size < buffer_size))
        {
          *c = result & 0xff;
          c++;
          buffer_content_size++;
          result = fgetc(f);
        }
    }

  if (filename != NULL)
    {
      fclose(f);
    }

  return buffer_content_size;
}

static int read_from_file_and_convert(FAR const char *prompt,
                                      FAR char *filename,
                                      FAR uint8_t *buffer,
                                      size_t buffer_size,
                                      FAR size_t *buffer_content_size)
{
  char file_buffer[DEFAULT_BUFFER_SIZE];
  size_t file_buffer_content_size;
  int result;

  file_buffer_content_size = read_from_file(
      prompt, filename, (FAR char *)file_buffer, sizeof(file_buffer));
  result = convert_array_hex_to_bin(file_buffer, file_buffer_content_size,
                                    buffer, buffer_size, buffer_content_size
                                    );

  return result;
}

static size_t add_zero_termination_character(char *data, size_t content_size,
                                             size_t buf_size)
{
  size_t zero_termination_char_position = content_size - 1;
  if ((content_size + 1) <= buf_size)
    {
      zero_termination_char_position = content_size;
      content_size++;
    }

  data[zero_termination_char_position] = 0;
  return content_size;
}

static int process(int se05x_fd, FAR struct settings_t *settings)
{
  int result = 0;
  if (settings->operation == KEYSTORE_GET_INFO)
    {
      struct se05x_info_s info;
      result = ioctl(se05x_fd, SEIOC_GET_INFO, &info);

      if (result == 0)
        {
          printf("OEF ID: %04x\n", info.oef_id);
        }
    }

  else if (settings->operation == KEYSTORE_GET_UID)
    {
      struct se05x_uid_s uid;
      result = ioctl(se05x_fd, SEIOC_GET_UID, &uid);

      if (result == 0)
        {
          printf("UID: ");
          for (size_t i = 0; i < SE05X_MODULE_UNIQUE_ID_LEN; i++)
            {
              printf("%02x", uid.uid[i]);
            }

          printf("\n");
        }
    }
  else if (settings->operation == KEYSTORE_GENERATE)
    {
      struct se05x_generate_keypair_s args = {
          .id = settings->key_id, .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256
      };

      result = ioctl(se05x_fd, SEIOC_GENERATE_KEYPAIR, &args);

      if (result == 0)
        {
          printf("Keypair generated successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_WRITE)
    {
      char pem_buf[DEFAULT_BUFFER_SIZE];
      FAR const char *prompt = enter_key_hex;
      if (settings->raw_data_in_device)
        {
          prompt =
              settings->interface_with_pem ? enter_data_pem : enter_data_raw;
        }

      size_t pem_content_size = read_from_file(
          prompt, settings->input_filename, pem_buf, sizeof(pem_buf));

      uint8_t rawkey[RAW_KEY_BUFFER_SIZE];
      size_t rawkey_size = sizeof(rawkey);

      FAR uint8_t *data = (FAR uint8_t *)pem_buf;
      size_t data_size = pem_content_size;
      if (!settings->raw_data_in_device)
        {
          result = convert_public_key_pem_to_raw(rawkey, rawkey_size,
                                                 &rawkey_size, pem_buf);
          if (result == 0)
            {
              data = rawkey;
              data_size = rawkey_size;
            }
        }

      if (settings->interface_with_pem)
        {
          pem_content_size = add_zero_termination_character(
              pem_buf, pem_content_size, sizeof(pem_buf));
          result = convert_pem_certificate_or_csr_to_der(
              rawkey, rawkey_size, &rawkey_size, pem_buf, pem_content_size);
          if (result == 0)
            {
              data = rawkey;
              data_size = rawkey_size;
            }
        }

      if (result == 0)
        {
          struct se05x_key_transmission_s args = {
              .entry = {.id = settings->key_id,
                        .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256},
              .content = {.buffer = data,
                          .buffer_size = data_size,
                          .buffer_content_size = data_size}
          };

          result = ioctl(se05x_fd,
                         settings->raw_data_in_device ? SEIOC_SET_DATA
                                                      : SEIOC_SET_KEY,
                         &args);
        }

      if (result == 0)
        {
          printf("Data stored successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_READ)
    {
      uint8_t buffer[DEFAULT_BUFFER_SIZE];
      struct se05x_key_transmission_s args = {
          .entry = {.id = settings->key_id},
          .content = {.buffer = buffer, .buffer_size = sizeof(buffer)}
      };

      result =
        ioctl(se05x_fd,
              settings->raw_data_in_device ? SEIOC_GET_DATA : SEIOC_GET_KEY,
              &args);

      FAR char *data = (FAR char *)args.content.buffer;
      if ((result == 0)
              && settings->raw_data_in_device
              && !settings->interface_with_pem)
        {
          args.content.buffer_content_size = add_zero_termination_character(
              data, args.content.buffer_content_size,
              args.content.buffer_size);
        }

      char pem_buf[DEFAULT_BUFFER_SIZE];
      if ((result == 0) && !settings->raw_data_in_device)
        {
          result = convert_public_key_raw_to_pem(
              pem_buf, sizeof(pem_buf), args.content.buffer,
              args.content.buffer_content_size);

          data = pem_buf;
        }

      if ((result == 0) && settings->interface_with_pem)
        {
          size_t pem_content_size;
          result = convert_der_certificate_or_csr_to_pem(
              pem_buf, sizeof(pem_buf), &pem_content_size,
              args.content.buffer, args.content.buffer_content_size);

          data = pem_buf;
        }

      if (result == 0)
        {
          puts(data);
        }
    }
  else if (settings->operation == KEYSTORE_DELETE)
    {
      result = ioctl(se05x_fd, SEIOC_DELETE_KEY, settings->key_id);

      if (result == 0)
        {
          printf("Deleted key successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_DERIVE_SYMM_KEY)
    {
      uint8_t buffer[SYMM_KEY_BUFFER_SIZE];
      struct se05x_derive_key_s args = {
          .private_key_id = settings->private_key_id,
          .public_key_id = settings->key_id,
          .content = {.buffer = buffer, .buffer_size = sizeof(buffer)},
      };

      result = ioctl(se05x_fd, SEIOC_DERIVE_SYMM_KEY, &args);

      if (result == 0)
        {
          for (size_t i = 0; i < args.content.buffer_content_size; i++)
            {
              printf("%02x", args.content.buffer[i]);
            }

          printf("\n");
        }
    }
  else if (settings->operation == KEYSTORE_CREATE_SIGNATURE)
    {
      uint8_t tbs_buffer[TBS_HASH_BUFFER_SIZE];
      size_t tbs_content_size;
      read_from_file_and_convert("Enter tbs(hex):", settings->input_filename,
                                 tbs_buffer, sizeof(tbs_buffer),
                                 &tbs_content_size);
      uint8_t signature_buffer[SIGNATURE_BUFFER_SIZE];
      size_t signature_content_len = 0;
      if (result == 0)
        {
          struct se05x_signature_s args = {
              .key_id = settings->key_id,
              .algorithm = SE05X_ALGORITHM_SHA256,
              .tbs = {.buffer = tbs_buffer,
                      .buffer_size = tbs_content_size,
                      .buffer_content_size = tbs_content_size},
              .signature = {.buffer = signature_buffer,
                            .buffer_size = sizeof(signature_buffer)},
          };

          result = ioctl(se05x_fd, SEIOC_CREATE_SIGNATURE, &args);
          signature_content_len = args.signature.buffer_content_size;
        }

      if (result == 0)
        {
          for (size_t i = 0; i < signature_content_len; i++)
            {
              printf("%02x", signature_buffer[i]);
            }

          printf("\n");
        }
    }
  else if (settings->operation == KEYSTORE_VERIFY_SIGNATURE)
    {
      uint8_t tbs_buffer[TBS_HASH_BUFFER_SIZE];
      size_t tbs_content_size;
      result = read_from_file_and_convert(
          "Enter tbs(hex):", settings->input_filename, tbs_buffer,
          sizeof(tbs_buffer), &tbs_content_size);

      uint8_t signature_buffer[SIGNATURE_BUFFER_SIZE];
      size_t signature_content_size;
      if (result == 0)
        {
          result = read_from_file_and_convert(
              "Enter signature(hex):", settings->signature_filename,
              signature_buffer, sizeof(signature_buffer),
              &signature_content_size);
        }

      if (result == 0)
        {
          struct se05x_signature_s args = {
              .key_id = settings->key_id,
              .algorithm = SE05X_ALGORITHM_SHA256,
              .tbs = {.buffer = tbs_buffer,
                      .buffer_size = tbs_content_size,
                      .buffer_content_size = tbs_content_size},
              .signature = {.buffer = signature_buffer,
                            .buffer_size = signature_content_size,
                            .buffer_content_size = signature_content_size},
          };

          result = ioctl(se05x_fd, SEIOC_VERIFY_SIGNATURE, &args);
        }

      if (result == 0)
        {
          printf("Signature verified successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_SIGN_CSR)
    {
      char csr_pem_buf[DEFAULT_BUFFER_SIZE];
      size_t csr_pem_buf_content_size =
          read_from_file("enter csr(pem)", settings->input_filename,
                         csr_pem_buf, sizeof(csr_pem_buf));

      csr_pem_buf_content_size = add_zero_termination_character(csr_pem_buf,
              csr_pem_buf_content_size, sizeof(csr_pem_buf));

      char crt_pem_buf[DEFAULT_BUFFER_SIZE];
      result = sign_csr(se05x_fd, settings->key_id, crt_pem_buf,
                        sizeof(crt_pem_buf), csr_pem_buf,
                        csr_pem_buf_content_size);
      if (result == 0)
        {
          puts(crt_pem_buf);
        }
    }
  else if (settings->operation == KEYSTORE_VERIFY_CERTIFICATE)
    {
      char pem_buf[DEFAULT_BUFFER_SIZE];
      size_t pem_content_size =
          read_from_file("enter crt(pem)", settings->input_filename, pem_buf,
                         sizeof(pem_buf));

      pem_content_size = add_zero_termination_character(pem_buf,
                                  pem_content_size, sizeof(pem_buf));

      mbedtls_x509_crt crt;
      mbedtls_x509_crt_init(&crt);
      result = mbedtls_x509_crt_parse(&crt, (FAR uint8_t *)pem_buf,
                                      pem_content_size);

      uint8_t tbs_buffer[TBS_HASH_BUFFER_SIZE];

      if (result == 0)
        {
          result = mbedtls_sha256(crt.tbs.p,
                                  crt.tbs.len, tbs_buffer, 0
                                  );
        }

      if (result == 0)
        {
          struct se05x_signature_s args = {
              .key_id = settings->key_id,
              .algorithm = SE05X_ALGORITHM_SHA256,
              .tbs = {.buffer = tbs_buffer,
                      .buffer_size = sizeof(tbs_buffer),
                      .buffer_content_size = sizeof(tbs_buffer)},
              .signature = {.buffer = crt.sig.p,
                            .buffer_size = crt.sig.len,
                            .buffer_content_size =
                                crt.sig.len},
          };

          result = ioctl(se05x_fd, SEIOC_VERIFY_SIGNATURE, &args);
        }

      if (result == 0)
        {
          printf("Signature verified successfully\n");
        }

      mbedtls_x509_crt_free(&crt);
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

  if ((result == 0) && (!settings.skip_process))
    {
      int fd = open(settings.se05x_dev_filename, O_RDONLY);
      if (fd == -1)
        {
          result = -ENODEV;
        }
      else
        {
          result = process(fd, &settings);
          close(fd);
        }
    }

  if (result != 0)
    {
      fprintf(stderr, "err %i: %s\n", -result, strerror(-result));
    }

  if (settings.show_stack_used)
    {
#ifdef CONFIG_STACK_COLORATION
      FAR struct tcb_s *tcb;
      tcb = nxsched_get_tcb(getpid());
      fprintf(stderr, "\nStack used: %zu / %zu\n",
              up_check_tcbstack(tcb), tcb->adj_stack_size);
#else
      fprintf(stderr, "\nStack used: unknown"
             " (STACK_COLORATION must be enabled)\n");
#endif
    }

  return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
