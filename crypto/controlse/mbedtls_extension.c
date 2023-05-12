/****************************************************************************
 * apps/crypto/controlse/mbedtls_extension.c
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

/* Copyright The Mbed TLS Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * The source code in this file is based on
 * mbedtls_x509write_crt_der() method from x509write_crt.c in Mbed TLS
 */

/* Copyright 2023 NXP */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include <mbedtls/asn1write.h>
#include <mbedtls/error.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <nuttx/crypto/se05x.h>
#include <sys/ioctl.h>
#include <string.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int x509_write_time(FAR unsigned char **p, FAR unsigned char *start,
                           FAR const char *t, size_t size)
{
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
  size_t len = 0;

  /* write MBEDTLS_ASN1_UTC_TIME if year < 2050 (2 bytes shorter) */

  if (t[0] == '2' && t[1] == '0' && t[2] < '5')
    {
      MBEDTLS_ASN1_CHK_ADD(
          len, mbedtls_asn1_write_raw_buffer(
                   p, start, (FAR const unsigned char *)t + 2, size - 2));
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
      MBEDTLS_ASN1_CHK_ADD(
          len, mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_UTC_TIME));
    }
  else
    {
      MBEDTLS_ASN1_CHK_ADD(len,
                           mbedtls_asn1_write_raw_buffer(
                             p, start, (FAR const unsigned char *)t, size));
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, len));
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(
                                  p, start, MBEDTLS_ASN1_GENERALIZED_TIME));
    }

  return ((int)len);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_x509write_crt_der_se05x(FAR mbedtls_x509write_cert *ctx,
                                    FAR unsigned char *buf, size_t size,
                                    int se05x_fd, uint32_t private_key_id)
{
  int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
  FAR const char *sig_oid;
  size_t sig_oid_len = 0;
  FAR unsigned char *c;
  FAR unsigned char *c2;
  unsigned char hash[64];
  unsigned char sig[MBEDTLS_PK_SIGNATURE_MAX_SIZE];
  size_t sub_len = 0;
  size_t pub_len = 0;
  size_t sig_and_oid_len = 0;
  size_t sig_len;
  size_t len = 0;
  mbedtls_pk_type_t pk_alg;

  /* Prepare data to be signed at the end of the target buffer */

  c = buf + size;

  /* Signature algorithm needed in TBS, and later for actual signature */

  /* There's no direct way of extracting a signature algorithm
   * (represented as an element of mbedtls_pk_type_t) from a PK instance.
   */

  if (mbedtls_pk_can_do(ctx->issuer_key, MBEDTLS_PK_RSA))
    {
      pk_alg = MBEDTLS_PK_RSA;
    }
  else if (mbedtls_pk_can_do(ctx->issuer_key, MBEDTLS_PK_ECDSA))
    {
      pk_alg = MBEDTLS_PK_ECDSA;
    }
  else
    {
      return (MBEDTLS_ERR_X509_INVALID_ALG);
    }

  if ((ret = mbedtls_oid_get_oid_by_sig_alg(pk_alg, ctx->md_alg, &sig_oid,
                                            &sig_oid_len)) != 0)
    {
      return (ret);
    }

  /* Extensions  ::=  SEQUENCE SIZE (1..MAX) OF Extension */

  /* Only for v3 */

  if (ctx->version == MBEDTLS_X509_CRT_VERSION_3)
    {
      MBEDTLS_ASN1_CHK_ADD(
          len, mbedtls_x509_write_extensions(&c, buf, ctx->extensions));
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
      MBEDTLS_ASN1_CHK_ADD(len,
                           mbedtls_asn1_write_tag(&c, buf,
                                                  MBEDTLS_ASN1_CONSTRUCTED |
                                                    MBEDTLS_ASN1_SEQUENCE));
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
      MBEDTLS_ASN1_CHK_ADD(
          len, mbedtls_asn1_write_tag(&c, buf,
                                      MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                                          MBEDTLS_ASN1_CONSTRUCTED | 3));
    }

  /*  SubjectPublicKeyInfo  */

  MBEDTLS_ASN1_CHK_ADD(
      pub_len, mbedtls_pk_write_pubkey_der(ctx->subject_key, buf, c - buf));
  c -= pub_len;
  len += pub_len;

  /*  Subject  ::=  Name  */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_x509_write_names(&c, buf, ctx->subject));

  /*  Validity ::= SEQUENCE {
   *       notBefore      Time,
   *       notAfter       Time }
   */

  sub_len = 0;

  MBEDTLS_ASN1_CHK_ADD(sub_len,
                       x509_write_time(&c, buf, ctx->not_after,
                                       MBEDTLS_X509_RFC5280_UTC_TIME_LEN));

  MBEDTLS_ASN1_CHK_ADD(sub_len,
                       x509_write_time(&c, buf, ctx->not_before,
                                       MBEDTLS_X509_RFC5280_UTC_TIME_LEN));

  len += sub_len;
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, sub_len));
  MBEDTLS_ASN1_CHK_ADD(
      len, mbedtls_asn1_write_tag(
               &c, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /*  Issuer  ::=  Name  */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_x509_write_names(&c, buf, ctx->issuer));

  /*  Signature   ::=  AlgorithmIdentifier  */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_algorithm_identifier(
                                &c, buf, sig_oid, strlen(sig_oid), 0));

  /*  Serial   ::=  INTEGER
   *
   * Written data is:
   * - "ctx->serial_len" bytes for the raw serial buffer
   *   - if MSb of "serial" is 1, then prepend an extra 0x00 byte
   * - 1 byte for the length
   * - 1 byte for the TAG
   */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_raw_buffer(&c, buf,
                                            ctx->serial, ctx->serial_len));
  if (*c & 0x80)
    {
      if (c - buf < 1)
        {
          return MBEDTLS_ERR_X509_BUFFER_TOO_SMALL;
        }

      *(--c) = 0x0;
      len++;
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf,
                                                       ctx->serial_len + 1));
    }
  else
    {
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf,
                                                       ctx->serial_len));
    }

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&c, buf,
                                                   MBEDTLS_ASN1_INTEGER));

  /*  Version  ::=  INTEGER  {  v1(0), v2(1), v3(2)  }  */

  /* Can be omitted for v1 */

  if (ctx->version != MBEDTLS_X509_CRT_VERSION_1)
    {
      sub_len = 0;
      MBEDTLS_ASN1_CHK_ADD(sub_len,
                           mbedtls_asn1_write_int(&c, buf, ctx->version));
      len += sub_len;
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, sub_len));
      MBEDTLS_ASN1_CHK_ADD(
          len, mbedtls_asn1_write_tag(&c, buf,
                                      MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                                          MBEDTLS_ASN1_CONSTRUCTED | 0));
    }

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
  MBEDTLS_ASN1_CHK_ADD(
      len, mbedtls_asn1_write_tag(
               &c, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* Make signature
   */

  /* Compute hash of CRT. */

  if ((ret = mbedtls_md(mbedtls_md_info_from_type(ctx->md_alg), c, len,
                        hash)) != 0)
    {
      return (ret);
    }

    {
      struct se05x_signature_s args = {
          .key_id = private_key_id,
          .algorithm = SE05X_ALGORITHM_SHA256,
          .tbs = {
            .buffer = hash,
            .buffer_size = 32,
            .buffer_content_size = 32
          },
          .signature = {.buffer = sig, .buffer_size = sizeof(sig)},
      };

      ret = ioctl(se05x_fd, SEIOC_CREATE_SIGNATURE, &args);
      if (ret != 0)
        {
          return ret;
        }

      sig_len = args.signature.buffer_content_size;
    }

  /* Move CRT to the front of the buffer to have space
   * for the signature.
   */

  memmove(buf, c, len);
  c = buf + len;

  /* Add signature at the end of the buffer,
   * making sure that it doesn't underflow
   * into the CRT buffer.
   */

  c2 = buf + size;
  MBEDTLS_ASN1_CHK_ADD(
      sig_and_oid_len,
      mbedtls_x509_write_sig(&c2, c, sig_oid, sig_oid_len, sig, sig_len));

  /* Memory layout after this step:
   *
   * buf       c=buf+len                c2            buf+size
   * [CRT0,...,CRTn, UNUSED, ..., UNUSED, SIG0, ..., SIGm]
   */

  /* Move raw CRT to just before the signature. */

  c = c2 - len;
  memmove(c, buf, len);

  len += sig_and_oid_len;
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&c, buf, len));
  MBEDTLS_ASN1_CHK_ADD(
      len, mbedtls_asn1_write_tag(
               &c, buf, MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return ((int)len);
}
