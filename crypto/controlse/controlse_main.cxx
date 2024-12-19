//***************************************************************************
// apps/crypto/controlse/controlse_main.cxx
//
// SPDX-License-Identifier: Apache-2.0
//
// Licensed to the Apache Software Foundation (ASF) under one or more
// contributor license agreements.  See the NOTICE file distributed with
// this work for additional information regarding copyright ownership.  The
// ASF licenses this file to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance with the
// License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
//
//**************************************************************************

// Copyright 2023, 2024 NXP

//***************************************************************************
// Included Files
//***************************************************************************

#include "crypto/controlse/ccertificate.hxx"
#include "crypto/controlse/ccsr.hxx"
#include "crypto/controlse/chex_util.hxx"
#include "crypto/controlse/cpublic_key.hxx"
#include "crypto/controlse/csecure_element.hxx"
#include "crypto/controlse/cstring.hxx"
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#ifdef CONFIG_STACK_COLORATION
#include <nuttx/arch.h>
#include <nuttx/sched.h>
#endif

//***************************************************************************
// Pre-processor Definitions
//**************************************************************************

#define EOT 0x04

#define DEFAULT_SETTINGS                                                      \
  {                                                                           \
    .se05x_dev_filename = default_se05x_device, 0                             \
  }

#define TBS_HASH_BUFFER_SIZE 32
#define SIGNATURE_BUFFER_SIZE 300
#define SYMM_KEY_BUFFER_SIZE 300
#define RAW_KEY_BUFFER_SIZE 600
#define DEFAULT_BUFFER_SIZE 1000

//***************************************************************************
// Private Types
//**************************************************************************

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
} EKeystoreOperation;

typedef enum
{
  KEYSTORE_DATA_TYPE_KEY = 0,
  KEYSTORE_DATA_TYPE_CERT_OR_CSR,
  KEYSTORE_DATA_TYPE_STRING,
} EKeystoreDataType;

struct SSettings
{
  FAR const char *se05x_dev_filename;
  FAR char *input_filename;
  FAR char *signature_filename;
  bool skip_process;
  EKeystoreOperation operation;
  EKeystoreDataType data_type;
  uint32_t key_id;
  uint32_t private_key_id;
  bool show_stack_used;
};

//***************************************************************************
// Private Function Prototypes
//**************************************************************************

extern "C" int main(int argc, FAR char *argv[]);

//***************************************************************************
// Private Data
//**************************************************************************

static const char default_se05x_device[] = "/dev/se05x";
static const char enter_key_hex[] = "enter key(hex)";
static const char enter_data_pem[] = "enter data(pem)";
static const char enter_data_raw[] = "enter data(raw)";

//***************************************************************************
// Public Data
//**************************************************************************

//***************************************************************************
// Private Functions
//**************************************************************************

static void printUsage(FAR FILE *f, FAR char *prg)
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

static int setOperation(FAR struct SSettings *settings,
                        EKeystoreOperation operation, FAR char *key_id_text)
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

static int parseArguments(int argc, FAR char *argv[],
                          FAR struct SSettings *settings)
{
  int result = 0;
  int opt;
  FAR char *prg = basename(argv[0]);
  while (((opt = getopt(argc, argv, "iug:w:r:d:s:v:S:V:tca:p:n:N:mh")) != -1)
         && (result == 0))
    {
      switch (opt)
        {
        case 'i':
          result = setOperation(settings, KEYSTORE_GET_INFO, optarg);
          break;
        case 'u':
          result = setOperation(settings, KEYSTORE_GET_UID, optarg);
          break;
        case 'g':
          result = setOperation(settings, KEYSTORE_GENERATE, optarg);
          break;
        case 'r':
          result = setOperation(settings, KEYSTORE_READ, optarg);
          break;
        case 'w':
          result = setOperation(settings, KEYSTORE_WRITE, optarg);
          break;
        case 't':
          settings->data_type = KEYSTORE_DATA_TYPE_STRING;
          break;
        case 'c':
          settings->data_type = KEYSTORE_DATA_TYPE_CERT_OR_CSR;
          break;
        case 'd':
          result = setOperation(settings, KEYSTORE_DELETE, optarg);
          break;
        case 's':
          result = setOperation(settings, KEYSTORE_CREATE_SIGNATURE, optarg);
          break;
        case 'v':
          result = setOperation(settings, KEYSTORE_VERIFY_SIGNATURE, optarg);
          break;
        case 'S':
          result = setOperation(settings, KEYSTORE_SIGN_CSR, optarg);
          break;
        case 'V':
          result = setOperation(settings, KEYSTORE_VERIFY_CERTIFICATE, optarg);
          break;
        case 'a':
          result = setOperation(settings, KEYSTORE_DERIVE_SYMM_KEY, optarg);
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
          printUsage(stdout, prg);
          settings->skip_process = TRUE;
          break;
        default:
          printUsage(stderr, prg);
          result = -EINVAL;
          break;
        }
    }

  if ((result == 0) && (!settings->skip_process))
    {
      if (settings->operation == KEYSTORE_NO_ACTION)
        {
          printUsage(stderr, prg);
          result = -EINVAL;
        }

      // if device is specified as positional argument

      if (optind != argc)
        {
          settings->se05x_dev_filename = argv[optind];
        }
    }

  return result;
}

// TODO: use chexutil
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
  for (hex_buffer_pos = 0; (hex_buffer_pos < hex_buffer_size)
                           && (*bin_buffer_content_size < bin_buffer_size);
       hex_buffer_pos += 2)
    {
      sscanf(&hex_buffer[hex_buffer_pos], "%2hhx",
             &bin_buffer[*bin_buffer_content_size]);
      (*bin_buffer_content_size)++;
    }

  return hex_buffer_pos == hex_buffer_size ? 0 : -1;
}

static int readFromFile(FAR const char *prompt, FAR char *filename,
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
      // keep looping until EOF or EOT are received or the buffer has only 1
      // space left (needed for the zero termination character)
      while ((result != EOF) && (result != EOT)
             && ((buffer_content_size + 1) < buffer_size))
        {
          *c = result & 0xff;
          c++;
          buffer_content_size++;
          result = fgetc(f);
        }
      // Add zero termination character
      *c = 0;
      buffer_content_size++;
    }

  puts("\n");

  if (filename != NULL)
    {
      fclose(f);
    }

  return buffer_content_size;
}

static int readFromFileAndConvert(FAR const char *prompt, FAR char *filename,
                                  FAR uint8_t *buffer, size_t buffer_size,
                                  FAR size_t *buffer_content_size)
{
  char file_buffer[DEFAULT_BUFFER_SIZE];
  size_t file_buffer_content_size;
  int result;

  file_buffer_content_size = readFromFile(
      prompt, filename, (FAR char *)file_buffer, sizeof(file_buffer));
  result = convert_array_hex_to_bin(file_buffer, file_buffer_content_size,
                                    buffer, buffer_size, buffer_content_size);

  return result;
}

static size_t addZeroTerminationCharacter(char *data, size_t content_size,
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

static int process(int se05x_fd, FAR struct SSettings *settings)
{
  int result = 0;
  Controlse::CSecureElement se(se05x_fd);
  if (settings->operation == KEYSTORE_GET_INFO)
    {
      struct se05x_info_s info;
      result = se.GetInfo(info) ? 0 : -EPERM;

      if (result == 0)
        {
          printf("OEF ID: %04x\n", info.oef_id);
        }
    }
  else if (settings->operation == KEYSTORE_GET_UID)
    {
      struct se05x_uid_s uid;
      result = se.GetUid(uid) ? 0 : -EPERM;
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
      struct se05x_generate_keypair_s args
          = { .id = settings->key_id,
              .cipher = SE05X_ASYM_CIPHER_EC_NIST_P_256 };

      result = se.GenerateKey(args) ? 0 : -EPERM;

      if (result == 0)
        {
          printf("Keypair generated successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_WRITE)
    {
      char data[DEFAULT_BUFFER_SIZE];
      FAR const char *prompt = settings->data_type == KEYSTORE_DATA_TYPE_STRING
                                   ? enter_data_raw
                                   : enter_data_pem;

      size_t data_size
          = readFromFile(prompt, settings->input_filename, data, sizeof(data));

      result = -EINVAL;
      if (data_size != 0)
        {
          Controlse::ISecureElementObject *object = nullptr;
          if (settings->data_type == KEYSTORE_DATA_TYPE_STRING)
            {
              object = new Controlse::CString(
                  data,
                  data_size - 1); // -1 because no zero termination character
                                  // required
            }
          else if (settings->data_type == KEYSTORE_DATA_TYPE_KEY)
            {
              object = new Controlse::CPublicKey(data);
            }
          else if (settings->data_type == KEYSTORE_DATA_TYPE_CERT_OR_CSR)
            {
              object = new Controlse::CCertificate(
                  reinterpret_cast<uint8_t *>(data), data_size);
              if (object)
                {
                  if (!object->IsLoaded())
                    {
                      delete object;
                      object = new Controlse::CCsr(
                          reinterpret_cast<uint8_t *>(data), data_size);
                    }
                }
            }

          result = -EPERM;
          if (object)
            {
              if (object->IsLoaded())
                {
                  result = object->StoreOnSecureElement(se, settings->key_id)
                               ? 0
                               : -EPERM;
                }
              delete object;
            }
        }

      if (result == 0)
        {
          printf("Data stored successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_READ)
    {
      char *data = nullptr;
      if (settings->data_type == KEYSTORE_DATA_TYPE_STRING)
        {
          Controlse::CString string(se, settings->key_id);
          if (string.IsLoaded())
            {
              data = string.c_str();
            }
        }
      else if (settings->data_type == KEYSTORE_DATA_TYPE_KEY)
        {
          Controlse::CPublicKey key(se, settings->key_id);
          if (key.IsLoaded())
            {
              data = key.GetPem();
            }
        }
      else if (settings->data_type == KEYSTORE_DATA_TYPE_CERT_OR_CSR)
        {
          Controlse::CCertificate cert(se, settings->key_id);
          if (cert.IsLoaded())
            {
              data = cert.GetPem();
            }
          else
            {
              Controlse::CCsr csr(se, settings->key_id);
              if (csr.IsLoaded())
                {
                  data = csr.GetPem();
                }
            }
        }
      if (data)
        {
          puts(data);
          delete[] data;
        }
      else
        {
          result = -EPERM;
        }
    }
  else if (settings->operation == KEYSTORE_DELETE)
    {
      result = se.DeleteKey(settings->key_id) ? 0 : -EPERM;
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
        .content = { .buffer = buffer, .buffer_size = sizeof(buffer) },
      };

      result = se.DeriveSymmetricalKey(args) ? 0 : -EPERM;

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
      readFromFileAndConvert("Enter tbs(hex):", settings->input_filename,
                             tbs_buffer, sizeof(tbs_buffer),
                             &tbs_content_size);
      uint8_t signature_buffer[SIGNATURE_BUFFER_SIZE];
      size_t signature_content_len = 0;
      if (result == 0)
        {
          struct se05x_signature_s args = {
            .key_id = settings->key_id,
            .algorithm = SE05X_ALGORITHM_SHA256,
            .tbs = { .buffer = tbs_buffer,
                     .buffer_size = tbs_content_size,
                     .buffer_content_size = tbs_content_size },
            .signature = { .buffer = signature_buffer,
                           .buffer_size = sizeof(signature_buffer) },
          };

          result = se.CreateSignature(args) ? 0 : -EPERM;
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
      result = readFromFileAndConvert(
          "Enter tbs(hex):", settings->input_filename, tbs_buffer,
          sizeof(tbs_buffer), &tbs_content_size);

      uint8_t signature_buffer[SIGNATURE_BUFFER_SIZE];
      size_t signature_content_size;
      if (result == 0)
        {
          result = readFromFileAndConvert(
              "Enter signature(hex):", settings->signature_filename,
              signature_buffer, sizeof(signature_buffer),
              &signature_content_size);
        }

      if (result == 0)
        {
          struct se05x_signature_s args = {
            .key_id = settings->key_id,
            .algorithm = SE05X_ALGORITHM_SHA256,
            .tbs = { .buffer = tbs_buffer,
                     .buffer_size = tbs_content_size,
                     .buffer_content_size = tbs_content_size },
            .signature = { .buffer = signature_buffer,
                           .buffer_size = signature_content_size,
                           .buffer_content_size = signature_content_size },
          };

          result = se.Verify(args) ? 0 : -EPERM;
        }

      if (result == 0)
        {
          printf("Signature verified successfully\n");
        }
    }
  else if (settings->operation == KEYSTORE_SIGN_CSR)
    {
      char csr_pem_buf[DEFAULT_BUFFER_SIZE];
      size_t csr_pem_buf_content_size
          = readFromFile("enter csr(pem)", settings->input_filename,
                         csr_pem_buf, sizeof(csr_pem_buf));

      csr_pem_buf_content_size = addZeroTerminationCharacter(
          csr_pem_buf, csr_pem_buf_content_size, sizeof(csr_pem_buf));

      auto certificate = Controlse::CCertificate(
          se, reinterpret_cast<uint8_t *>(csr_pem_buf),
          csr_pem_buf_content_size, settings->key_id);

      auto crt_pem = certificate.GetPem();
      if (crt_pem)
        {
          puts(crt_pem);
          delete[] crt_pem;
        }
      else
        {
          result = -EPERM;
        }
    }
  else if (settings->operation == KEYSTORE_VERIFY_CERTIFICATE)
    {
      char pem_buf[DEFAULT_BUFFER_SIZE];
      size_t pem_content_size
          = readFromFile("enter crt(pem)", settings->input_filename, pem_buf,
                         sizeof(pem_buf));

      pem_content_size = addZeroTerminationCharacter(pem_buf, pem_content_size,
                                                     sizeof(pem_buf));

      auto certificate = Controlse::CCertificate(
          reinterpret_cast<uint8_t *>(pem_buf), pem_content_size);

      result = certificate.VerifyAgainst(se, settings->key_id) ? 0 : -EPERM;

      if (result == 0)
        {
          printf("Signature verified successfully\n");
        }
    }

  return result;
}

//***************************************************************************
// Public Functions
//**************************************************************************

int main(int argc, FAR char *argv[])
{
  struct SSettings settings = DEFAULT_SETTINGS;
  int result = parseArguments(argc, argv, &settings);

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
      fprintf(stderr, "\nStack used: %zu / %zu\n", up_check_tcbstack(tcb),
              tcb->adj_stack_size);
#else
      fprintf(stderr, "\nStack used: unknown"
                      " (STACK_COLORATION must be enabled)\n");
#endif
    }

  return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
