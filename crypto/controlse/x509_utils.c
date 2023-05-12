/****************************************************************************
 * apps/crypto/controlse/x509_utils.c
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
#include <string.h>
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pem.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/x509_csr.h"
#include "mbedtls_extension.h"
#include <time.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SECONDS_IN_DAY (60 * 60 * 24)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char certificate_header[] = "-----BEGIN CERTIFICATE-----\n";
static const char certificate_footer[] = "-----END CERTIFICATE-----\n";

static const char certificate_request_header[] =
    "-----BEGIN CERTIFICATE REQUEST-----\n";
static const char certificate_request_footer[] =
    "-----END CERTIFICATE REQUEST-----\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int convert_der_certificate_to_pem(FAR char *pem_buf,
                                          size_t pem_buf_size,
                                          FAR size_t *pem_content_size,
                                          FAR uint8_t *der_buf,
                                          size_t der_buf_size)
{
  int result;

    {
      mbedtls_x509_crt crt;
      mbedtls_x509_crt_init(&crt);
      result = mbedtls_x509_crt_parse(&crt, der_buf, der_buf_size);
      mbedtls_x509_crt_free(&crt);
    }

  if (result == 0)
    {
      result = mbedtls_pem_write_buffer(
          certificate_header, certificate_footer, der_buf, der_buf_size,
          (FAR uint8_t *)pem_buf, pem_buf_size, pem_content_size);
    }

  return result;
}

static int convert_der_csr_to_pem(FAR char *pem_buf, size_t pem_buf_size,
                                  FAR size_t *pem_content_size,
                                  FAR uint8_t *der_buf, size_t der_buf_size)
{
  int result;

    {
      mbedtls_x509_csr csr;
      mbedtls_x509_csr_init(&csr);
      result =
          mbedtls_x509_csr_parse(&csr, (FAR uint8_t *)der_buf, der_buf_size);
      mbedtls_x509_csr_free(&csr);
    }

  if (result == 0)
    {
      result = mbedtls_pem_write_buffer(certificate_request_header,
                                        certificate_request_footer, der_buf,
                                        der_buf_size, (FAR uint8_t *)pem_buf,
                                        pem_buf_size, pem_content_size);
    }

  return result;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int convert_public_key_raw_to_pem(FAR char *pem_buf, size_t pem_buf_size,
                                  FAR uint8_t *key_buf, size_t key_buf_size)
{
  mbedtls_pk_context key =
    {
      0
    };

  mbedtls_pk_init(&key);
  FAR const mbedtls_pk_info_t *info =
      mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY);

  int result = -1;
  if (info != NULL)
    {
      result = mbedtls_pk_setup(&key, info);
    }

  FAR mbedtls_ecp_keypair *keypair = (mbedtls_ecp_keypair *)key.pk_ctx;
  if (result == 0)
    {
      result =
          mbedtls_ecp_group_load(&keypair->grp, MBEDTLS_ECP_DP_SECP256R1);
    }

  if (result == 0)
    {
      result = mbedtls_ecp_point_read_binary(&keypair->grp, &keypair->Q,
                                             key_buf, key_buf_size);
    }

  if (result == 0)
    {
      result = mbedtls_pk_write_pubkey_pem(&key, (FAR uint8_t *)pem_buf,
                                           pem_buf_size);
    }

  mbedtls_pk_free(&key);
  return result < 0 ? -EINVAL : 0;
}

int convert_public_key_pem_to_raw(FAR uint8_t *key_buf, size_t key_buf_size,
                                  FAR size_t *key_size, FAR char *pem_buf)
{
  int result = -1;
  mbedtls_pk_context key =
    {
      0
    };

  mbedtls_pk_init(&key);

  result = mbedtls_pk_parse_public_key(&key, (FAR uint8_t *)pem_buf,
                                       strlen(pem_buf) + 1);

  if (result == 0)
    {
      result = mbedtls_pk_can_do(&key, MBEDTLS_PK_ECKEY) == 1 ? 0 : -1;
    }

  if (result == 0)
    {
      FAR mbedtls_ecp_keypair *keypair = (mbedtls_ecp_keypair *)key.pk_ctx;
      result = mbedtls_ecp_point_write_binary(
          &keypair->grp, &keypair->Q, MBEDTLS_ECP_PF_UNCOMPRESSED, key_size,
          key_buf, key_buf_size);
    }

  mbedtls_pk_free(&key);
  return result < 0 ? -EINVAL : 0;
}

int convert_pem_certificate_or_csr_to_der(FAR uint8_t *der_buf,
                                          size_t der_buf_size,
                                          FAR size_t *der_content_size,
                                          FAR char *pem_buf,
                                          size_t pem_buf_size)
{
  int result;

    {
      mbedtls_x509_crt crt;
      mbedtls_x509_crt_init(&crt);
      result =
          mbedtls_x509_crt_parse(&crt, (FAR uint8_t *)pem_buf, pem_buf_size);
      if ((result == 0) && (der_buf_size < crt.raw.len))
        {
          result = -EINVAL;
        }

      if (result == 0)
        {
          memcpy(der_buf, crt.raw.p, crt.raw.len);
          *der_content_size = crt.raw.len;
        }

      mbedtls_x509_crt_free(&crt);
    }

  /* if bad input data then try parsing CSR */

  if (result != 0)
    {
      mbedtls_x509_csr csr;
      mbedtls_x509_csr_init(&csr);
      result =
          mbedtls_x509_csr_parse(&csr, (FAR uint8_t *)pem_buf, pem_buf_size);
      if ((result == 0) && (der_buf_size < csr.raw.len))
        {
          result = -EINVAL;
        }

      if (result == 0)
        {
          memcpy(der_buf, csr.raw.p, csr.raw.len);
          *der_content_size = csr.raw.len;
        }

      mbedtls_x509_csr_free(&csr);
    }

  return result;
}

int convert_der_certificate_or_csr_to_pem(FAR char *pem_buf,
                                          size_t pem_buf_size,
                                          FAR size_t *pem_content_size,
                                          FAR uint8_t *der_buf,
                                          size_t der_buf_size)
{
  int result = convert_der_certificate_to_pem(
      pem_buf, pem_buf_size, pem_content_size, der_buf, der_buf_size);

  if (result != 0)
    {
      result = convert_der_csr_to_pem(pem_buf, pem_buf_size,
                                      pem_content_size,
                                      der_buf, der_buf_size);
    }

  return result;
}

int sign_csr(int se05x_fd, uint32_t private_key_id, FAR char *crt_pem_buf,
             size_t crt_pem_buf_size, FAR char *csr_pem_buf,
             size_t csr_pem_buf_content_size)
{
  mbedtls_x509_csr csr;
  mbedtls_x509_csr_init(&csr);
  int result = mbedtls_x509_csr_parse(&csr, (FAR uint8_t *)csr_pem_buf,
                                      csr_pem_buf_content_size);

  mbedtls_x509write_cert crt;
  mbedtls_x509write_crt_init(&crt);
  char subject_name[200];
  if (result == 0)
    {
      mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
      result = mbedtls_x509_dn_gets(subject_name, sizeof(subject_name),
                                    &csr.subject);
    }

  mbedtls_pk_context private_key;
  mbedtls_pk_init(&private_key);
  if (result >= 0)
    {
      mbedtls_x509write_crt_set_subject_key(&crt, &csr.pk);
      result = mbedtls_pk_setup(
          &private_key,
          mbedtls_pk_info_from_type((mbedtls_pk_type_t)MBEDTLS_PK_ECDSA));
    }

  if (result == 0)
    {
      mbedtls_x509write_crt_set_issuer_key(&crt, &private_key);
      result = mbedtls_x509write_crt_set_subject_name(&crt, subject_name);
    }

  if (result == 0)
    {
      result =
          mbedtls_x509write_crt_set_issuer_name(&crt, "CN=CA,O=NXP,C=NL");
    }

  mbedtls_mpi serial;
  mbedtls_mpi_init(&serial);
  if (result == 0)
    {
      mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
      result = mbedtls_mpi_read_string(&serial, 10, "1");
    }

  if (result == 0)
    {
      result = mbedtls_x509write_crt_set_serial(&crt, &serial);
    }

  if (result == 0)
    {
      time_t rawtime;
      struct tm tm_info;
      char from_datetime[20];
      char to_datetime[20];
      time(&rawtime);
      strftime(from_datetime, sizeof(from_datetime), "%Y%m%d%H%M%S",
               gmtime_r(&rawtime, &tm_info));
      rawtime += SECONDS_IN_DAY;
      strftime(to_datetime, sizeof(to_datetime), "%Y%m%d%H%M%S",
               gmtime_r(&rawtime, &tm_info));
      result = mbedtls_x509write_crt_set_validity(&crt, from_datetime,
                                                  to_datetime);
    }

  if (result == 0)
    {
      result = mbedtls_x509write_crt_der_se05x(
          &crt, (FAR uint8_t *)crt_pem_buf, crt_pem_buf_size, se05x_fd,
          private_key_id);
    }

  if (result >= 0)
    {
      size_t olen;
      result = mbedtls_pem_write_buffer(
          certificate_header, certificate_footer,
          (FAR uint8_t *)(crt_pem_buf + crt_pem_buf_size - result), result,
          (FAR uint8_t *)crt_pem_buf, crt_pem_buf_size, &olen);
    }

  mbedtls_mpi_free(&serial);
  mbedtls_pk_free(&private_key);
  mbedtls_x509write_crt_free(&crt);
  mbedtls_x509_csr_free(&csr);
  return result;
}
