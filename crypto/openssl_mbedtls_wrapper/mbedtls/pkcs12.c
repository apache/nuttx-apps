/****************************************************************************
 * apps/crypto/openssl_mbedtls_wrapper/mbedtls/pkcs12.c
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <openssl/pkcs12.h>
#include <openssl/obj.h>
#include <openssl/ssl_dbg.h>

#include <mbedtls/asn1.h>
#include <mbedtls/asn1write.h>
#include <mbedtls/cipher.h>
#include <mbedtls/constant_time.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/pkcs12.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform.h>
#include <mbedtls/x509_crt.h>

#include "ssl_port.h"
#include "ssl_pm.h"

#if defined(MBEDTLS_PKCS12_C)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SSL_PKCS12_ERROR_LEVEL       1
#define PKCS12_MAX_PWDLEN            128

/* PKCS#12 BagTypes: {pkcs-12 10 1 x} */

#define MBEDTLS_OID_PKCS12_BAG_TYPE  MBEDTLS_OID_PKCS12 "\x0a\x01"

/* PKCS#9 certTypes: {pkcs-9 22 x} */

#define MBEDTLS_OID_PKCS9_CERT_TYPE  MBEDTLS_OID_PKCS9 "\x16"

#define OID_ENTRY(oid_macro, nid_val) \
  { oid_macro, sizeof(oid_macro) - 1, nid_val }

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct PKCS12_st
{
  unsigned char *data;
  size_t len;
};

struct oid_nid_entry
{
  const char *oid;
  size_t      oid_len;
  int         nid;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oid_nid_entry g_oid_table[] =
{
  /* PKCS#7 ContentInfo types */

  OID_ENTRY(MBEDTLS_OID_PKCS7_DATA,           NID_pkcs7_data),
  OID_ENTRY(MBEDTLS_OID_PKCS7_SIGNED_DATA,    NID_pkcs7_signed),
  OID_ENTRY(MBEDTLS_OID_PKCS7_ENVELOPED_DATA, NID_pkcs7_enveloped),
  OID_ENTRY(MBEDTLS_OID_PKCS7_ENCRYPTED_DATA, NID_pkcs7_encrypted),

  /* PKCS#12 BagTypes */

  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x01", NID_keyBag),
  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x02", NID_pkcs8ShroudedKeyBag),
  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x03", NID_certBag),
  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x04", NID_crlBag),
  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x05", NID_secretBag),
  OID_ENTRY(MBEDTLS_OID_PKCS12_BAG_TYPE "\x06", NID_safeContentsBag),

  /* PKCS#9 certTypes */

  OID_ENTRY(MBEDTLS_OID_PKCS9_CERT_TYPE "\x01", NID_x509Certificate),

  /* PKCS#5 */

  OID_ENTRY(MBEDTLS_OID_PKCS5 "\x0c", NID_pbkdf2),
  OID_ENTRY(MBEDTLS_OID_PKCS5 "\x0d", NID_pbes2),

  /* PKCS#12 PBE */

  OID_ENTRY(MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC,
            NID_pbe_WithSHA1And3_Key_TripleDES_CBC),
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pkcs12_oid2nid(const mbedtls_asn1_buf *oid)
{
  size_t i;

  if (oid->p == NULL || oid->len == 0)
    {
      return NID_undef;
    }

  for (i = 0; i < sizeof(g_oid_table) / sizeof(g_oid_table[0]); i++)
    {
      if (g_oid_table[i].oid_len == oid->len &&
          memcmp(g_oid_table[i].oid, oid->p, oid->len) == 0)
        {
          return g_oid_table[i].nid;
        }
    }

  return NID_undef;
}

/****************************************************************************
 * Name: pkcs12_asc2uni
 *
 * Description:
 *   Convert UTF-8 password to BMPString (UTF-16BE with null terminator)
 *   as required by PKCS#12 specification (RFC 7292, B.1).
 *
 * Returns:
 *   0 on success, -1 on failure. bmp_len includes the 2-byte null term.
 ****************************************************************************/

static int pkcs12_asc2uni(const char *pass, size_t pass_len,
                          unsigned char *bmp, size_t *bmp_len)
{
  size_t out_len = (pass_len + 1) * 2;
  size_t i;

  if (out_len > PKCS12_MAX_PWDLEN * 2 + 2)
    {
      return -1;
    }

  for (i = 0; i < pass_len; i++)
    {
      bmp[i * 2]     = 0x00;
      bmp[i * 2 + 1] = (unsigned char)pass[i];
    }

  /* Null terminator in UTF-16BE */

  bmp[pass_len * 2]     = 0x00;
  bmp[pass_len * 2 + 1] = 0x00;

  *bmp_len = out_len;
  return 0;
}

/****************************************************************************
 * Name: pkcs12_verify_mac
 *
 * Description:
 *   Verify the MacData integrity check on the authSafe content.
 *   MacData ::= SEQUENCE {
 *     mac        DigestInfo,
 *     macSalt    OCTET STRING,
 *     iterations INTEGER DEFAULT 1
 *   }
 ****************************************************************************/

static int pkcs12_verify_mac(unsigned char *mac_data,
                             unsigned char *mac_end,
                             const unsigned char *authsafe,
                             size_t authsafe_len,
                             const unsigned char *bmp_pwd,
                             size_t bmp_pwd_len)
{
  unsigned char computed_mac[MBEDTLS_MD_MAX_SIZE];
  unsigned char key[MBEDTLS_MD_MAX_SIZE];
  const mbedtls_md_info_t *md_info;
  mbedtls_asn1_buf alg_params;
  mbedtls_asn1_buf mac_digest;
  mbedtls_asn1_buf mac_salt;
  mbedtls_asn1_buf alg_oid;
  mbedtls_md_type_t md_type;
  unsigned char *p = mac_data;
  size_t key_len;
  size_t mac_len;
  size_t len;
  int iterations = 1;
  int ret;

  /* MacData ::= SEQUENCE */

  ret = mbedtls_asn1_get_tag(&p, mac_end, &len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return ret;
    }

  mac_end = p + len;

  /* DigestInfo ::= SEQUENCE { algorithm, digest } */

  ret = mbedtls_asn1_get_tag(&p, mac_end, &len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return ret;
    }

  /* AlgorithmIdentifier */

  ret = mbedtls_asn1_get_alg(&p, p + len, &alg_oid, &alg_params);
  if (ret != 0)
    {
      return ret;
    }

  ret = mbedtls_oid_get_md_alg(&alg_oid, &md_type);
  if (ret != 0)
    {
      return ret;
    }

  md_info = mbedtls_md_info_from_type(md_type);
  if (md_info == NULL)
    {
      return -1;
    }

  mac_len = mbedtls_md_get_size(md_info);
  key_len = mac_len;

  /* Guard the fixed-size key/computed_mac buffers: the MAC digest algorithm
   * comes from the (untrusted) MacData AlgorithmIdentifier, so reject any
   * digest whose output would overflow the local buffers.
   */

  if (mac_len == 0 || mac_len > sizeof(key))
    {
      return -1;
    }

  /* digest OCTET STRING */

  ret = mbedtls_asn1_get_tag(&p, mac_end, &mac_digest.len,
                             MBEDTLS_ASN1_OCTET_STRING);
  if (ret != 0)
    {
      return ret;
    }

  mac_digest.p = p;
  p += mac_digest.len;

  /* macSalt OCTET STRING */

  ret = mbedtls_asn1_get_tag(&p, mac_end, &mac_salt.len,
                             MBEDTLS_ASN1_OCTET_STRING);
  if (ret != 0)
    {
      return ret;
    }

  mac_salt.p = p;
  p += mac_salt.len;

  /* iterations INTEGER (optional, default 1) */

  if (p < mac_end)
    {
      ret = mbedtls_asn1_get_int(&p, mac_end, &iterations);
      if (ret != 0)
        {
          return ret;
        }
    }

  /* Derive MAC key using PKCS#12 key derivation (id=3 for MAC key) */

  ret = mbedtls_pkcs12_derivation(key, key_len,
                                  bmp_pwd, bmp_pwd_len,
                                  mac_salt.p, mac_salt.len,
                                  md_type,
                                  MBEDTLS_PKCS12_DERIVE_MAC_KEY,
                                  iterations);
  if (ret != 0)
    {
      return ret;
    }

  /* Compute HMAC over authSafe content */

  ret = mbedtls_md_hmac(md_info, key, key_len,
                        authsafe, authsafe_len,
                        computed_mac);
  if (ret != 0)
    {
      return ret;
    }

  /* Compare */

  if (mac_digest.len != mac_len ||
      mbedtls_ct_memcmp(computed_mac, mac_digest.p, mac_len) != 0)
    {
      return -1;
    }

  return 0;
}

/****************************************************************************
 * Name: pkcs12_pbe_crypt
 *
 * Description:
 *   Decrypt encrypted content using either PKCS#12 PBE or PKCS#5 PBES2.
 *   Handles AlgorithmIdentifier dispatch internally.
 ****************************************************************************/

static int pkcs12_pbe_crypt(const unsigned char *data, size_t data_len,
                            const unsigned char *pwd, size_t pwd_len,
                            const unsigned char *bmp_pwd,
                            size_t bmp_pwd_len,
                            unsigned char **out, size_t *out_len)
{
  mbedtls_cipher_type_t cipher_alg;
  mbedtls_md_type_t md_alg;
  mbedtls_asn1_buf alg_params;
  mbedtls_asn1_buf alg_oid;
  unsigned char *dec_buf = NULL;
  unsigned char *end = (unsigned char *)data + data_len;
  unsigned char *p = (unsigned char *)data;
  size_t dec_len = 0;
  size_t enc_len;
  int ret;

  /* Parse AlgorithmIdentifier */

  ret = mbedtls_asn1_get_alg(&p, end, &alg_oid, &alg_params);
  if (ret != 0)
    {
      return ret;
    }

  /* encryptedContent [0] IMPLICIT OCTET STRING — strip the tag+length */

  ret = mbedtls_asn1_get_tag(&p, end, &enc_len,
                             MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0);
  if (ret != 0)
    {
      return ret;
    }

  dec_buf = mbedtls_calloc(1, enc_len);
  if (!dec_buf)
    {
      return -1;
    }

  /* Try PKCS#12 PBE first */

  ret = mbedtls_oid_get_pkcs12_pbe_alg(&alg_oid, &md_alg, &cipher_alg);
  if (ret == 0)
    {
      ret = mbedtls_pkcs12_pbe_ext(&alg_params,
                                   MBEDTLS_PKCS12_PBE_DECRYPT,
                                   cipher_alg, md_alg,
                                   pwd, pwd_len,
                                   p, enc_len,
                                   dec_buf, enc_len, &dec_len);
      if (ret != 0)
        {
          mbedtls_free(dec_buf);
          return ret;
        }

      *out = dec_buf;
      *out_len = dec_len;
      return 0;
    }

  /* Try PKCS#5 PBES2 */

  if (pkcs12_oid2nid(&alg_oid) == NID_pbes2)
    {
      ret = mbedtls_pkcs5_pbes2_ext(&alg_params, MBEDTLS_PKCS5_DECRYPT,
                                    pwd, pwd_len,
                                    p, enc_len,
                                    dec_buf, enc_len, &dec_len);
      if (ret != 0)
        {
          mbedtls_free(dec_buf);
          return ret;
        }

      *out = dec_buf;
      *out_len = dec_len;
      return 0;
    }

  mbedtls_free(dec_buf);
  return -1;
}

/****************************************************************************
 * Name: parse_bags
 *
 * Description:
 *   Parse SafeContents (SEQUENCE OF SafeBag) and extract certs/keys.
 ****************************************************************************/

static int parse_bags(const unsigned char *data, size_t data_len,
                      const unsigned char *pwd, size_t pwd_len,
                      const unsigned char *bmp_pwd,
                      size_t bmp_pwd_len,
                      EVP_PKEY **pkey, X509 **cert,
                      OPENSSL_STACK **ca)
{
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_asn1_buf bag_oid;
  mbedtls_asn1_buf cert_oid;
  struct pkey_pm *pm;
  unsigned char *end = (unsigned char *)data + data_len;
  unsigned char *p = (unsigned char *)data;
  unsigned char *bag_end;
  unsigned char *cb_end;
  unsigned char *cb_p;
  OPENSSL_STACK *sk;
  EVP_PKEY *k;
  X509 *ca_cert;
  X509 *c;
  char **new_data;
  size_t bag_len;
  size_t seq_len;
  size_t len;
  int new_alloc;
  int ret;

  /* SafeContents ::= SEQUENCE OF SafeBag */

  ret = mbedtls_asn1_get_tag(&p, end, &len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return ret;
    }

  end = p + len;

  while (p < end)
    {
      /* SafeBag ::= SEQUENCE { bagId, bagValue [0] EXPLICIT, bagAttr } */

      ret = mbedtls_asn1_get_tag(&p, end, &bag_len,
                                 MBEDTLS_ASN1_CONSTRUCTED |
                                 MBEDTLS_ASN1_SEQUENCE);
      if (ret != 0)
        {
          return ret;
        }

      bag_end = p + bag_len;

      /* bagId OID */

      ret = mbedtls_asn1_get_tag(&p, bag_end, &bag_oid.len,
                                 MBEDTLS_ASN1_OID);
      if (ret != 0)
        {
          return ret;
        }

      bag_oid.p = p;
      p += bag_oid.len;

      /* bagValue [0] EXPLICIT */

      ret = mbedtls_asn1_get_tag(&p, bag_end, &len,
                                 MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                                 MBEDTLS_ASN1_CONSTRUCTED | 0);
      if (ret != 0)
        {
          return ret;
        }

      if (pkcs12_oid2nid(&bag_oid) == NID_certBag)
        {
          /* CertBag ::= SEQUENCE { certId, certValue [0] EXPLICIT } */

          cb_end = p + len;
          cb_p = p;

          ret = mbedtls_asn1_get_tag(&cb_p, cb_end, &seq_len,
                                     MBEDTLS_ASN1_CONSTRUCTED |
                                     MBEDTLS_ASN1_SEQUENCE);
          if (ret != 0)
            {
              goto skip_bag;
            }

          /* certId OID */

          ret = mbedtls_asn1_get_tag(&cb_p, cb_end, &cert_oid.len,
                                     MBEDTLS_ASN1_OID);
          if (ret != 0)
            {
              goto skip_bag;
            }

          cert_oid.p = cb_p;
          cb_p += cert_oid.len;

          if (pkcs12_oid2nid(&cert_oid) != NID_x509Certificate)
            {
              goto skip_bag;
            }

          /* certValue [0] EXPLICIT -> OCTET STRING containing DER cert */

          ret = mbedtls_asn1_get_tag(&cb_p, cb_end, &len,
                                     MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                                     MBEDTLS_ASN1_CONSTRUCTED | 0);
          if (ret != 0)
            {
              goto skip_bag;
            }

          ret = mbedtls_asn1_get_tag(&cb_p, cb_end, &len,
                                     MBEDTLS_ASN1_OCTET_STRING);
          if (ret != 0)
            {
              goto skip_bag;
            }

          if (cert != NULL && *cert == NULL)
            {
              c = X509_new();
              if (c == NULL)
                {
                  return -1;
                }

              ret = X509_METHOD_CALL(load, c, cb_p, (int)len);
              if (ret != 0)
                {
                  X509_free(c);
                  goto skip_bag;
                }

              *cert = c;
            }
          else if (ca != NULL)
            {
              ca_cert = X509_new();
              if (ca_cert == NULL)
                {
                  return -1;
                }

              ret = X509_METHOD_CALL(load, ca_cert, cb_p, (int)len);
              if (ret != 0)
                {
                  X509_free(ca_cert);
                  goto skip_bag;
                }

              if (*ca == NULL)
                {
                  *ca = OPENSSL_sk_new_null();
                  if (*ca == NULL)
                    {
                      X509_free(ca_cert);
                      return -1;
                    }
                }

              sk = *ca;

              /* Grow the backing array when the preallocated slots
               * (num_alloc) are exhausted.  Without this bounds check a
               * PKCS#12 file carrying more CA certificates than the initial
               * capacity would write past sk->data[] (heap overflow).
               */

              if (sk->num >= sk->num_alloc)
                {
                  new_alloc = sk->num_alloc * 2;
                  new_data = ssl_mem_zalloc(new_alloc * sizeof(*new_data));
                  if (new_data == NULL)
                    {
                      X509_free(ca_cert);
                      return -1;
                    }

                  memcpy(new_data, sk->data,
                         sk->num * sizeof(*new_data));
                  ssl_mem_free(sk->data);
                  sk->data = new_data;
                  sk->num_alloc = new_alloc;
                }

              sk->data[sk->num] = (char *)ca_cert;
              sk->num++;
            }
        }
      else if (pkcs12_oid2nid(&bag_oid) == NID_pkcs8ShroudedKeyBag)
        {
          /* PKCS8ShroudedKeyBag: EncryptedPrivateKeyInfo */

          if (pkey != NULL && *pkey == NULL)
            {
              *pkey = EVP_PKEY_new();
              if (*pkey == NULL)
                {
                  return -1;
                }

              pm = (struct pkey_pm *)(*pkey)->pkey_pm;
              if (!pm->pkey)
                {
                  pm->pkey = mbedtls_calloc(1, sizeof(mbedtls_pk_context));
                  if (!pm->pkey)
                    {
                      EVP_PKEY_free(*pkey);
                      *pkey = NULL;
                      return -1;
                    }
                }

              mbedtls_pk_init(pm->pkey);
              mbedtls_ctr_drbg_init(&ctr_drbg);

              ret = mbedtls_pk_parse_key(pm->pkey, p, len,
                                         pwd, pwd_len,
                                         mbedtls_ctr_drbg_random,
                                         &ctr_drbg);
              mbedtls_ctr_drbg_free(&ctr_drbg);
              if (ret != 0)
                {
                  SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL,
                            "pk_parse_key failed: -0x%x", -ret);
                  mbedtls_pk_free(pm->pkey);
                  mbedtls_free(pm->pkey);
                  pm->pkey = NULL;
                  EVP_PKEY_free(*pkey);
                  *pkey = NULL;
                }
            }
        }
      else if (pkcs12_oid2nid(&bag_oid) == NID_keyBag)
        {
          /* KeyBag: unencrypted PrivateKeyInfo */

          if (pkey != NULL && *pkey == NULL)
            {
              k = EVP_PKEY_new();
              if (k == NULL)
                {
                  return -1;
                }

              EVP_PKEY_METHOD_CALL(load, k, p, (int)len);
              *pkey = k;
            }
        }

skip_bag:
      p = bag_end;
    }

  return 0;
}

/****************************************************************************
 * Name: parse_pk12
 *
 * Description:
 *   Parse a single ContentInfo from AuthenticatedSafe.
 *   Handles both plain data and encryptedData content types.
 ****************************************************************************/

static int parse_pk12(const unsigned char *data, size_t len,
                      const unsigned char *pwd, size_t pwd_len,
                      const unsigned char *bmp_pwd,
                      size_t bmp_pwd_len,
                      EVP_PKEY **pkey, X509 **cert,
                      OPENSSL_STACK **ca)
{
  mbedtls_asn1_buf ci_oid;
  mbedtls_asn1_buf ct_oid;
  unsigned char *dec_data = NULL;
  unsigned char *end = (unsigned char *)data + len;
  unsigned char *p = (unsigned char *)data;
  unsigned char *ci_end;
  unsigned char *eci_end;
  unsigned char *ed_end;
  size_t dec_len = 0;
  size_t oct_len;
  size_t seq_len;
  size_t eci_len;
  size_t ci_len;
  int version;
  int ret;

  /* ContentInfo ::= SEQUENCE { contentType, content [0] EXPLICIT } */

  ret = mbedtls_asn1_get_tag(&p, end, &ci_len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return ret;
    }

  ci_end = p + ci_len;

  /* contentType OID */

  ret = mbedtls_asn1_get_tag(&p, ci_end, &ci_oid.len, MBEDTLS_ASN1_OID);
  if (ret != 0)
    {
      return ret;
    }

  ci_oid.p = p;
  p += ci_oid.len;

  /* content [0] EXPLICIT */

  ret = mbedtls_asn1_get_tag(&p, ci_end, &ci_len,
                             MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                             MBEDTLS_ASN1_CONSTRUCTED | 0);
  if (ret != 0)
    {
      return ret;
    }

  if (pkcs12_oid2nid(&ci_oid) == NID_pkcs7_data)
    {
      /* data content: OCTET STRING containing SafeContents */

      ret = mbedtls_asn1_get_tag(&p, ci_end, &oct_len,
                                 MBEDTLS_ASN1_OCTET_STRING);
      if (ret != 0)
        {
          return ret;
        }

      ret = parse_bags(p, oct_len, pwd, pwd_len,
                       bmp_pwd, bmp_pwd_len,
                       pkey, cert, ca);
    }
  else if (pkcs12_oid2nid(&ci_oid) == NID_pkcs7_encrypted)
    {
      /* encryptedData: decrypt then parse SafeContents */

      ret = mbedtls_asn1_get_tag(&p, ci_end, &seq_len,
                                 MBEDTLS_ASN1_CONSTRUCTED |
                                 MBEDTLS_ASN1_SEQUENCE);
      if (ret != 0)
        {
          return ret;
        }

      ed_end = p + seq_len;

      /* version INTEGER */

      ret = mbedtls_asn1_get_int(&p, ed_end, &version);
      if (ret != 0)
        {
          return ret;
        }

      /* EncryptedContentInfo ::= SEQUENCE */

      ret = mbedtls_asn1_get_tag(&p, ed_end, &eci_len,
                                 MBEDTLS_ASN1_CONSTRUCTED |
                                 MBEDTLS_ASN1_SEQUENCE);
      if (ret != 0)
        {
          return ret;
        }

      eci_end = p + eci_len;

      /* contentType OID (skip) */

      ret = mbedtls_asn1_get_tag(&p, eci_end, &ct_oid.len,
                                 MBEDTLS_ASN1_OID);
      if (ret != 0)
        {
          return ret;
        }

      p += ct_oid.len;

      /* contentEncryptionAlgorithm + [0] encryptedContent */

      ret = pkcs12_pbe_crypt(p, (size_t)(eci_end - p),
                             pwd, pwd_len,
                             bmp_pwd, bmp_pwd_len,
                             &dec_data, &dec_len);
      if (ret != 0)
        {
          return ret;
        }

      ret = parse_bags(dec_data, dec_len, pwd, pwd_len,
                       bmp_pwd, bmp_pwd_len,
                       pkey, cert, ca);
      mbedtls_free(dec_data);
    }

  return ret;
}

/****************************************************************************
 * Name: pkcs12_select_leaf_cert
 *
 * Description:
 *   OpenSSL's PKCS12_parse designates the certificate matching the private
 *   key as the leaf (*cert) and puts the rest into the CA stack, regardless
 *   of the order the bags appear in the file.  parse_bags fills *cert with
 *   the first certificate it encounters, which is wrong when a CA
 *   certificate precedes the end-entity certificate in the PKCS#12 file.
 *
 *   This helper corrects that once both the key and all certificates have
 *   been parsed: if *cert does not match the private key, scan the CA stack
 *   for a certificate that does and swap it into *cert.  When there is no
 *   private key the first-seen certificate is kept (same as OpenSSL).
 ****************************************************************************/

static void pkcs12_select_leaf_cert(EVP_PKEY *pkey, X509 **cert,
                                    OPENSSL_STACK *ca)
{
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_pk_context *prv;
  struct x509_pm *cert_pm;
  struct x509_pm *ca_pm;
  struct pkey_pm *pm;
  X509 *cur;
  int i;

  if (pkey == NULL || cert == NULL || *cert == NULL || ca == NULL)
    {
      return;
    }

  pm = (struct pkey_pm *)pkey->pkey_pm;
  if (pm == NULL || pm->pkey == NULL)
    {
      return;
    }

  prv = pm->pkey;

  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func,
                            &entropy, NULL, 0) != 0)
    {
      mbedtls_ctr_drbg_free(&ctr_drbg);
      mbedtls_entropy_free(&entropy);
      return;
    }

  /* If the current leaf already matches the private key, nothing to do. */

  cert_pm = (struct x509_pm *)(*cert)->x509_pm;
  if (cert_pm != NULL &&
      mbedtls_pk_check_pair(&cert_pm->x509_crt.pk, prv,
                            mbedtls_ctr_drbg_random, &ctr_drbg) == 0)
    {
      goto done;
    }

  /* Otherwise scan the CA stack for the certificate that matches. */

  for (i = 0; i < ca->num; i++)
    {
      cur = (X509 *)ca->data[i];
      if (cur == NULL)
        {
          continue;
        }

      ca_pm = (struct x509_pm *)cur->x509_pm;
      if (ca_pm == NULL)
        {
          continue;
        }

      if (mbedtls_pk_check_pair(&ca_pm->x509_crt.pk, prv,
                                mbedtls_ctr_drbg_random, &ctr_drbg) == 0)
        {
          /* Swap: matching CA entry becomes the leaf, old leaf joins CA. */

          ca->data[i] = (char *)(*cert);
          *cert = cur;
          break;
        }
    }

done:
  mbedtls_ctr_drbg_free(&ctr_drbg);
  mbedtls_entropy_free(&entropy);
}

/****************************************************************************
 * Name: pkcs12_create_pbe_params
 *
 * Description:
 *   Build DER-encoded pkcs-12PbeParams: SEQUENCE { salt, iterations }
 *   Written backwards into buf. Returns total length written.
 ****************************************************************************/

static int pkcs12_create_pbe_params(unsigned char *buf, size_t buflen,
                                    const unsigned char *salt,
                                    size_t salt_len,
                                    int iterations)
{
  unsigned char *p = buf + buflen;
  const unsigned char *start = buf;
  int len = 0;
  int ret;

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(&p, start, iterations));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_octet_string(&p, start,
                                                            salt, salt_len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(&p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(&p, start,
                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  memmove(buf, p, (size_t)len);
  return len;
}

/****************************************************************************
 * Name: pkcs12_pbe_crypt_enc
 *
 * Description:
 *   Encrypt data using pbeWithSHA1And3-KeyTripleDES-CBC.
 *   Generates random salt, builds PBE params, encrypts.
 *   Returns encrypted data (caller frees). pbe_params_out receives the
 *   DER-encoded AlgorithmIdentifier parameters (SEQUENCE {salt, iter}).
 ****************************************************************************/

static int pkcs12_pbe_crypt_enc(const unsigned char *data, size_t data_len,
                                const unsigned char *pwd, size_t pwd_len,
                                int iterations,
                                unsigned char **enc_out, size_t *enc_out_len,
                                unsigned char *pbe_params_buf,
                                size_t *pbe_params_len)
{
  mbedtls_entropy_context *entropy;
  mbedtls_ctr_drbg_context *ctr_drbg;
  mbedtls_asn1_buf params;
  unsigned char *pp = pbe_params_buf;
  unsigned char salt[8];
  size_t inner_len;
  size_t out_len = 0;
  size_t out_size;
  int ret;

  entropy = mbedtls_calloc(1, sizeof(*entropy));
  ctr_drbg = mbedtls_calloc(1, sizeof(*ctr_drbg));
  if (entropy == NULL || ctr_drbg == NULL)
    {
      mbedtls_free(entropy);
      mbedtls_free(ctr_drbg);
      return -1;
    }

  mbedtls_entropy_init(entropy);
  mbedtls_ctr_drbg_init(ctr_drbg);
  ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func,
                              entropy, NULL, 0);
  if (ret != 0)
    {
      mbedtls_ctr_drbg_free(ctr_drbg);
      mbedtls_entropy_free(entropy);
      mbedtls_free(ctr_drbg);
      mbedtls_free(entropy);
      return ret;
    }

  ret = mbedtls_ctr_drbg_random(ctr_drbg, salt, sizeof(salt));
  mbedtls_ctr_drbg_free(ctr_drbg);
  mbedtls_entropy_free(entropy);
  mbedtls_free(ctr_drbg);
  mbedtls_free(entropy);
  if (ret != 0)
    {
      return ret;
    }

  /* Build PBE params DER */

  ret = pkcs12_create_pbe_params(pbe_params_buf, 64, salt,
                                 sizeof(salt), iterations);
  if (ret < 0)
    {
      goto cleanup;
    }

  *pbe_params_len = (size_t)ret;

  /* Re-parse the SEQUENCE header to point params at the inner content */

  ret = mbedtls_asn1_get_tag(&pp, pbe_params_buf + *pbe_params_len,
                             &inner_len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      goto cleanup;
    }

  params.tag = MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE;
  params.p = pp;
  params.len = inner_len;

  /* Allocate output: input + up to one block of padding (8 for 3DES) */

  out_size = data_len + 8;
  *enc_out = mbedtls_calloc(1, out_size);
  if (*enc_out == NULL)
    {
      ret = -1;
      goto cleanup;
    }

  ret = mbedtls_pkcs12_pbe_ext(&params, MBEDTLS_PKCS12_PBE_ENCRYPT,
                               MBEDTLS_CIPHER_DES_EDE3_CBC,
                               MBEDTLS_MD_SHA1,
                               pwd, pwd_len,
                               data, data_len,
                               *enc_out, out_size, &out_len);
  if (ret != 0)
    {
      mbedtls_free(*enc_out);
      *enc_out = NULL;
      goto cleanup;
    }

  *enc_out_len = out_len;

cleanup:
  return ret;
}

/****************************************************************************
 * Name: pkcs12_write_key_ci
 *
 * Description:
 *   Write the "data" ContentInfo holding a pkcs8ShroudedKeyBag (DER,
 *   backwards): ContentInfo { pkcs7-data, [0] OCTET STRING { SafeContents
 *   { SafeBag { pkcs8ShroudedKeyBag, [0] EncryptedPrivateKeyInfo } } } }.
 *   enc_key is the PBE-encrypted PKCS8 key, pbe_params the DER-encoded PBE
 *   AlgorithmIdentifier parameters.  Returns bytes written or negative.
 ****************************************************************************/

static int pkcs12_write_key_ci(unsigned char **p, const unsigned char *start,
                               const unsigned char *enc_key,
                               size_t enc_key_len,
                               const unsigned char *pbe_params,
                               size_t pbe_params_len)
{
  int alg_len = 0;
  int len = 0;
  int ret;

  /* encryptedData OCTET STRING */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_raw_buffer(p, start, enc_key, enc_key_len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, enc_key_len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_OCTET_STRING));

  /* AlgorithmIdentifier SEQUENCE { OID, params } */

  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_raw_buffer(p, start, pbe_params, pbe_params_len));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC,
      sizeof(MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC) - 1));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_len(p, start, (size_t)alg_len));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  len += alg_len;

  /* EncryptedPrivateKeyInfo SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* bagValue [0] EXPLICIT */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* bagId: pkcs8ShroudedKeyBag */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS12 "\x0a\x01\x02",
      sizeof(MBEDTLS_OID_PKCS12 "\x0a\x01\x02") - 1));

  /* SafeBag SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* SafeContents SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* OCTET STRING wrapping SafeContents */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_OCTET_STRING));

  /* content [0] EXPLICIT */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* contentType: pkcs7-data */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS7_DATA, sizeof(MBEDTLS_OID_PKCS7_DATA) - 1));

  /* ContentInfo SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_certs_ci
 *
 * Description:
 *   Write the "encryptedData" ContentInfo holding the encrypted cert
 *   SafeContents (DER, backwards): ContentInfo { pkcs7-encryptedData, [0]
 *   EncryptedData { version, EncryptedContentInfo { pkcs7-data, AlgId,
 *   [0] encryptedContent } } }.  Returns bytes written or negative.
 ****************************************************************************/

static int pkcs12_write_certs_ci(unsigned char **p,
                                 const unsigned char *start,
                                 const unsigned char *enc_certs,
                                 size_t enc_certs_len,
                                 const unsigned char *pbe_params,
                                 size_t pbe_params_len)
{
  int alg_len = 0;
  int len = 0;
  int ret;

  /* encryptedContent [0] IMPLICIT OCTET STRING */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_raw_buffer(p, start, enc_certs, enc_certs_len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, enc_certs_len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_CONTEXT_SPECIFIC | 0));

  /* contentEncryptionAlgorithm AlgorithmIdentifier */

  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_raw_buffer(p, start, pbe_params, pbe_params_len));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC,
      sizeof(MBEDTLS_OID_PKCS12_PBE_SHA1_DES3_EDE_CBC) - 1));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_len(p, start, (size_t)alg_len));
  MBEDTLS_ASN1_CHK_ADD(alg_len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  len += alg_len;

  /* contentType: pkcs7-data */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS7_DATA, sizeof(MBEDTLS_OID_PKCS7_DATA) - 1));

  /* EncryptedContentInfo SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* version INTEGER 0 */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, 0));

  /* EncryptedData SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* content [0] EXPLICIT */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* contentType: pkcs7-encryptedData */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS7_ENCRYPTED_DATA,
      sizeof(MBEDTLS_OID_PKCS7_ENCRYPTED_DATA) - 1));

  /* ContentInfo SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_pkcs8
 *
 * Description:
 *   Wrap a raw private key DER in a PKCS8 PrivateKeyInfo (DER, backwards):
 *   SEQUENCE { version INTEGER(0), algorithm AlgId,
 *               privateKey OCTET STRING }
 *   Writes into the caller-provided buffer; returns bytes written or a
 *   negative error code.
 ****************************************************************************/

static int pkcs12_write_pkcs8(unsigned char **p, const unsigned char *start,
                              const unsigned char *raw_key, size_t raw_len)
{
  int len = 0;
  int ret;

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_octet_string(p, start, raw_key, raw_len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_algorithm_identifier(p, start,
      MBEDTLS_OID_PKCS1_RSA, sizeof(MBEDTLS_OID_PKCS1_RSA) - 1, 0));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, 0));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_cert_bag
 *
 * Description:
 *   Write a single certBag SafeBag (DER, backwards) into the buffer:
 *   SafeBag { bagId=certBag, bagValue [0] { CertBag { certType, [0] cert }}}
 *   Returns the number of bytes written, or a negative error code.
 ****************************************************************************/

static int pkcs12_write_cert_bag(unsigned char **p,
                                 const unsigned char *start,
                                 const mbedtls_x509_crt *crt)
{
  int len = 0;
  int ret;

  /* certValue [0] EXPLICIT OCTET STRING containing DER cert */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_raw_buffer(p, start, crt->raw.p, crt->raw.len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, crt->raw.len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start, MBEDTLS_ASN1_OCTET_STRING));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* certType OID: x509Certificate */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS9 "\x16\x01",
      sizeof(MBEDTLS_OID_PKCS9 "\x16\x01") - 1));

  /* CertBag SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* bagValue [0] EXPLICIT */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONTEXT_SPECIFIC | MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* bagId OID: certBag */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_oid(p, start,
      MBEDTLS_OID_PKCS12 "\x0a\x01\x03",
      sizeof(MBEDTLS_OID_PKCS12 "\x0a\x01\x03") - 1));

  /* SafeBag SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len,
    mbedtls_asn1_write_tag(p, start,
      MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_seq_hdr
 *
 * Description:
 *   Prepend a SEQUENCE tag+length header over already-written content
 *   (reverse DER).  Returns the number of header bytes written.
 ****************************************************************************/

static int pkcs12_write_seq_hdr(unsigned char **p,
                                const unsigned char *start,
                                size_t content_len)
{
  int len = 0;
  int ret;

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, content_len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_authsafe
 *
 * Description:
 *   Write the authSafe portion of a PFX (reverse DER): the two ContentInfos
 *   (shrouded key bag + encrypted certs), the AuthenticatedSafe SEQUENCE,
 *   and the enclosing pkcs7-data ContentInfo plus version INTEGER(3).
 *   Reports the AuthenticatedSafe content span so the caller can MAC it.
 *   Returns the total number of bytes written.
 ****************************************************************************/

static int pkcs12_write_authsafe(unsigned char **p,
                                 const unsigned char *start,
                                 const unsigned char *enc_key,
                                 size_t enc_key_len,
                                 const unsigned char *pbe_params_key,
                                 size_t pbe_params_key_len,
                                 const unsigned char *enc_certs,
                                 size_t enc_certs_len,
                                 const unsigned char *pbe_params_cert,
                                 size_t pbe_params_cert_len,
                                 unsigned char **authsafe_content,
                                 size_t *authsafe_content_len)
{
  int len = 0;
  int ret;

  /* ContentInfo for key (data type, contains shrouded key bag) */

  MBEDTLS_ASN1_CHK_ADD(len,
    pkcs12_write_key_ci(p, start, enc_key, enc_key_len,
                        pbe_params_key, pbe_params_key_len));

  /* ContentInfo for certs (encryptedData type) */

  MBEDTLS_ASN1_CHK_ADD(len,
    pkcs12_write_certs_ci(p, start, enc_certs, enc_certs_len,
                          pbe_params_cert, pbe_params_cert_len));

  /* AuthenticatedSafe SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* Save authSafe content span for MAC computation */

  *authsafe_content = *p;
  *authsafe_content_len = (size_t)len;

  /* Wrap in OCTET STRING */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                            MBEDTLS_ASN1_OCTET_STRING));

  /* content [0] EXPLICIT */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                            MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                            MBEDTLS_ASN1_CONSTRUCTED | 0));

  /* contentType: pkcs7-data */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_oid(p, start,
                            MBEDTLS_OID_PKCS7_DATA,
                            sizeof(MBEDTLS_OID_PKCS7_DATA) - 1));

  /* authSafe ContentInfo SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  /* version INTEGER 3 */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, 3));

  return len;
}

/****************************************************************************
 * Name: pkcs12_write_macdata
 *
 * Description:
 *   Write a MacData structure (reverse DER): SEQUENCE { DigestInfo,
 *   macSalt OCTET STRING, iterations INTEGER (omitted when 1) }.
 *   Returns the number of bytes written.
 ****************************************************************************/

static int pkcs12_write_macdata(unsigned char **p,
                                const unsigned char *start,
                                const unsigned char *mac_digest,
                                size_t mac_digest_len,
                                const unsigned char *mac_salt,
                                size_t mac_salt_len,
                                int mac_iter)
{
  int di_len = 0;
  int len = 0;
  int ret;

  if (mac_iter != 1)
    {
      MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_int(p, start, mac_iter));
    }

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_octet_string(p, start,
                            mac_salt, mac_salt_len));

  /* DigestInfo SEQUENCE { AlgorithmIdentifier(SHA1), digest
   * OCTET STRING
   */

  MBEDTLS_ASN1_CHK_ADD(di_len, mbedtls_asn1_write_octet_string(p, start,
                               mac_digest, mac_digest_len));
  MBEDTLS_ASN1_CHK_ADD(di_len,
                       mbedtls_asn1_write_algorithm_identifier(p, start,
                               MBEDTLS_OID_DIGEST_ALG_SHA1,
                               sizeof(MBEDTLS_OID_DIGEST_ALG_SHA1) - 1, 0));
  MBEDTLS_ASN1_CHK_ADD(di_len, mbedtls_asn1_write_len(p, start,
                               (size_t)di_len));
  MBEDTLS_ASN1_CHK_ADD(di_len, mbedtls_asn1_write_tag(p, start,
                               MBEDTLS_ASN1_CONSTRUCTED |
                               MBEDTLS_ASN1_SEQUENCE));

  len += di_len;

  /* MacData SEQUENCE */

  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_len(p, start, (size_t)len));
  MBEDTLS_ASN1_CHK_ADD(len, mbedtls_asn1_write_tag(p, start,
                        MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE));

  return len;
}

/****************************************************************************
 * Name: pkcs12_compute_mac
 *
 * Description:
 *   Generate a random MAC salt, derive the PKCS#12 MAC key from the BMP
 *   password, and compute HMAC-SHA1 over the AuthenticatedSafe content.
 *   Fills mac_salt and mac_digest.  Returns 0 on success.
 ****************************************************************************/

static int pkcs12_compute_mac(const unsigned char *authsafe,
                              size_t authsafe_len,
                              const unsigned char *bmp_pwd,
                              size_t bmp_pwd_len,
                              int mac_iter,
                              unsigned char *mac_salt, size_t mac_salt_len,
                              unsigned char *mac_digest)
{
  mbedtls_entropy_context *entropy;
  mbedtls_ctr_drbg_context *ctr_drbg;
  unsigned char mac_key[20];
  int ret;

  entropy = mbedtls_calloc(1, sizeof(*entropy));
  ctr_drbg = mbedtls_calloc(1, sizeof(*ctr_drbg));
  if (entropy == NULL || ctr_drbg == NULL)
    {
      mbedtls_free(entropy);
      mbedtls_free(ctr_drbg);
      return -1;
    }

  mbedtls_entropy_init(entropy);
  mbedtls_ctr_drbg_init(ctr_drbg);

  ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func,
                              entropy, NULL, 0);
  if (ret == 0)
    {
      ret = mbedtls_ctr_drbg_random(ctr_drbg, mac_salt, mac_salt_len);
    }

  mbedtls_ctr_drbg_free(ctr_drbg);
  mbedtls_entropy_free(entropy);
  mbedtls_free(ctr_drbg);
  mbedtls_free(entropy);
  if (ret != 0)
    {
      return ret;
    }

  /* Derive MAC key */

  ret = mbedtls_pkcs12_derivation(mac_key, sizeof(mac_key),
                                  bmp_pwd, bmp_pwd_len,
                                  mac_salt, mac_salt_len,
                                  MBEDTLS_MD_SHA1,
                                  MBEDTLS_PKCS12_DERIVE_MAC_KEY,
                                  mac_iter);
  if (ret != 0)
    {
      return ret;
    }

  /* Compute HMAC-SHA1 over authSafe content */

  ret = mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1),
                        mac_key, sizeof(mac_key),
                        authsafe, authsafe_len,
                        mac_digest);
  mbedtls_platform_zeroize(mac_key, sizeof(mac_key));

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

PKCS12 *d2i_PKCS12(PKCS12 **a, const unsigned char **in, long len)
{
  PKCS12 *p12;

  SSL_ASSERT2(in);
  SSL_ASSERT2(*in);
  SSL_ASSERT2(len > 0);

  p12 = mbedtls_calloc(1, sizeof(PKCS12));
  if (!p12)
    {
      SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "no enough memory > (p12)");
      return NULL;
    }

  p12->data = mbedtls_calloc(1, (size_t)len);
  if (!p12->data)
    {
      SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "no enough memory > (p12->data)");
      mbedtls_free(p12);
      return NULL;
    }

  memcpy(p12->data, *in, (size_t)len);
  p12->len = (size_t)len;

  *in += len;

  if (a != NULL)
    {
      *a = p12;
    }

  return p12;
}

PKCS12 *d2i_PKCS12_fp(FILE *fp, PKCS12 **a)
{
  const unsigned char *p;
  unsigned char *buf = NULL;
  PKCS12 *p12;
  long len;

  SSL_ASSERT2(fp);

  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  if (len <= 0)
    {
      return NULL;
    }

  buf = mbedtls_calloc(1, (size_t)len);
  if (!buf)
    {
      return NULL;
    }

  if (fread(buf, 1, (size_t)len, fp) != (size_t)len)
    {
      mbedtls_free(buf);
      return NULL;
    }

  p = buf;
  p12 = d2i_PKCS12(a, &p, len);
  mbedtls_free(buf);
  return p12;
}

int PKCS12_parse(PKCS12 *p12, const char *pass,
                 EVP_PKEY **pkey, X509 **cert,
                 STACK_OF(X509) **ca)
{
  unsigned char bmp_pwd[PKCS12_MAX_PWDLEN * 2 + 2];
  mbedtls_asn1_buf ci_oid;
  const unsigned char *pwd = NULL;
  unsigned char *authsafe_content;
  unsigned char *elem_start;
  unsigned char *ci_end;
  unsigned char *as_end;
  unsigned char *end;
  unsigned char *p;
  size_t authsafe_content_len;
  size_t bmp_pwd_len = 0;
  size_t pwd_len = 0;
  size_t elem_len;
  size_t as_len;
  size_t ci_len;
  size_t len;
  int version;
  int ret;
  int i;

  if (p12 == NULL || p12->data == NULL)
    {
      return 0;
    }

  if (pkey)
    {
      *pkey = NULL;
    }

  if (cert)
    {
      *cert = NULL;
    }

  if (ca)
    {
      *ca = NULL;
    }

  /* Prepare password in both raw and BMPString forms */

  if (pass != NULL && pass[0] != '\0')
    {
      pwd_len = strlen(pass);
      pwd = (const unsigned char *)pass;

      if (pkcs12_asc2uni(pass, pwd_len,
                         bmp_pwd, &bmp_pwd_len) != 0)
        {
          SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "password too long");
          return 0;
        }
    }
  else
    {
      /* Empty password: BMPString is just null terminator (2 bytes) */

      bmp_pwd[0] = 0x00;
      bmp_pwd[1] = 0x00;
      bmp_pwd_len = 2;
    }

  p = p12->data;
  end = p + p12->len;

  /* PFX ::= SEQUENCE { version, authSafe, macData } */

  ret = mbedtls_asn1_get_tag(&p, end, &len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "PFX SEQUENCE parse failed");
      return 0;
    }

  end = p + len;

  /* version INTEGER (must be 3) */

  ret = mbedtls_asn1_get_int(&p, end, &version);
  if (ret != 0 || version != 3)
    {
      SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "PFX version != 3");
      return 0;
    }

  /* authSafe: ContentInfo */

  ret = mbedtls_asn1_get_tag(&p, end, &ci_len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return 0;
    }

  ci_end = p + ci_len;

  /* contentType OID */

  ret = mbedtls_asn1_get_tag(&p, ci_end, &ci_oid.len, MBEDTLS_ASN1_OID);
  if (ret != 0)
    {
      return 0;
    }

  ci_oid.p = p;
  p += ci_oid.len;

  /* content [0] EXPLICIT -> OCTET STRING -> AuthenticatedSafe */

  ret = mbedtls_asn1_get_tag(&p, ci_end, &len,
                             MBEDTLS_ASN1_CONTEXT_SPECIFIC |
                             MBEDTLS_ASN1_CONSTRUCTED | 0);
  if (ret != 0)
    {
      return 0;
    }

  ret = mbedtls_asn1_get_tag(&p, ci_end, &len,
                             MBEDTLS_ASN1_OCTET_STRING);
  if (ret != 0)
    {
      return 0;
    }

  /* Save authSafe content range for MAC verification */

  authsafe_content = p;
  authsafe_content_len = len;

  /* AuthenticatedSafe ::= SEQUENCE OF ContentInfo */

  ret = mbedtls_asn1_get_tag(&p, p + len, &as_len,
                             MBEDTLS_ASN1_CONSTRUCTED |
                             MBEDTLS_ASN1_SEQUENCE);
  if (ret != 0)
    {
      return 0;
    }

  as_end = p + as_len;

  while (p < as_end)
    {
      elem_start = p;

      ret = mbedtls_asn1_get_tag(&p, as_end, &elem_len,
                                 MBEDTLS_ASN1_CONSTRUCTED |
                                 MBEDTLS_ASN1_SEQUENCE);
      if (ret != 0)
        {
          return 0;
        }

      ret = parse_pk12(elem_start,
                       (size_t)(p + elem_len - elem_start),
                       pwd, pwd_len,
                       bmp_pwd, bmp_pwd_len,
                       pkey, cert, ca);
      if (ret != 0)
        {
          SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL,
                    "ContentInfo parse error: -0x%x", -ret);
        }

      p += elem_len;
    }

  /* Verify MAC if macData is present */

  p = ci_end;
  if (p < end)
    {
      ret = pkcs12_verify_mac(p, end,
                              authsafe_content, authsafe_content_len,
                              bmp_pwd, bmp_pwd_len);
      if (ret != 0)
        {
          SSL_DEBUG(SSL_PKCS12_ERROR_LEVEL, "MAC verification failed");

          /* Clean up on MAC failure */

          if (pkey && *pkey)
            {
              EVP_PKEY_free(*pkey);
              *pkey = NULL;
            }

          if (cert && *cert)
            {
              X509_free(*cert);
              *cert = NULL;
            }

          if (ca && *ca)
            {
              for (i = 0; i < (*ca)->num; i++)
                {
                  X509_free((X509 *)(*ca)->data[i]);
                }

              OPENSSL_sk_free(*ca);
              *ca = NULL;
            }

          return 0;
        }
    }

  /* Align with OpenSSL: the certificate matching the private key is the
   * leaf; correct *cert if a CA-stack entry is the real match.
   */

  if (pkey != NULL && cert != NULL && ca != NULL)
    {
      pkcs12_select_leaf_cert(*pkey, cert, *ca);
    }

  mbedtls_platform_zeroize(bmp_pwd, sizeof(bmp_pwd));
  return 1;
}

int i2d_PKCS12(const PKCS12 *p12, unsigned char **out)
{
  if (p12 == NULL || p12->data == NULL)
    {
      return 0;
    }

  if (out != NULL)
    {
      if (*out == NULL)
        {
          *out = mbedtls_calloc(1, p12->len);
          if (*out == NULL)
            {
              return -1;
            }

          memcpy(*out, p12->data, p12->len);
        }
      else
        {
          memcpy(*out, p12->data, p12->len);
          *out += p12->len;
        }
    }

  return (int)p12->len;
}

void PKCS12_free(PKCS12 *p12)
{
  SSL_ASSERT3(p12);

  if (p12->data)
    {
      mbedtls_platform_zeroize(p12->data, p12->len);
      mbedtls_free(p12->data);
    }

  mbedtls_free(p12);
}

PKCS12 *PKCS12_create(const char *pass, const char *name,
                      EVP_PKEY *pkey, X509 *cert,
                      STACK_OF(X509) *ca,
                      int nid_key, int nid_cert,
                      int iter, int mac_iter, int keytype)
{
  struct x509_pm *cert_pm;
  struct x509_pm *ca_pm;
  struct pkey_pm *pm;
  unsigned char bmp_pwd[PKCS12_MAX_PWDLEN * 2 + 2];
  unsigned char pbe_params_key[64];
  unsigned char pbe_params_cert[64];
  unsigned char mac_digest[20];
  unsigned char mac_salt[8];
  unsigned char mac_der[128];
  unsigned char seq_hdr[8];
  const unsigned char *start;
  const unsigned char *ms = mac_der;
  const unsigned char *ss = seq_hdr;
  const char *pwd = pass ? pass : "";
  unsigned char *authsafe_content;
  unsigned char *cert_bags = NULL;
  unsigned char *enc_certs = NULL;
  unsigned char *enc_key = NULL;
  unsigned char *final_buf;
  unsigned char *key_der = NULL;
  unsigned char *pkcs8_buf;
  unsigned char *buf = NULL;
  unsigned char *mp = mac_der + sizeof(mac_der);
  unsigned char *sp = seq_hdr + sizeof(seq_hdr);
  unsigned char *pk8p;
  unsigned char *cp;
  unsigned char *p;
  const unsigned char *cs;
  X509 *ca_cert;
  PKCS12 *p12;
  size_t authsafe_content_len;
  size_t cert_bags_size = 4096;
  size_t pbe_params_cert_len = 0;
  size_t pbe_params_key_len = 0;
  size_t key_der_size = 2048;
  size_t enc_certs_len = 0;
  size_t enc_key_len = 0;
  size_t bmp_pwd_len = 0;
  size_t pwd_len = strlen(pwd);
  size_t pkcs8_size;
  size_t buflen = 8192;
  size_t cur_len;
  size_t total;
  int key_der_len;
  int hdr_len = 0;
  int mac_len = 0;
  int cont_len = 0;
  int ret;
  int i;

  /* Iteration defaults */

  if (iter <= 0)
    {
      iter = 2048;
    }

  if (mac_iter <= 0)
    {
      mac_iter = 1;
    }

  if (pkey == NULL || cert == NULL)
    {
      return NULL;
    }

  /* Convert password to BMP */

  ret = pkcs12_asc2uni(pwd, pwd_len, bmp_pwd, &bmp_pwd_len);
  if (ret != 0)
    {
      return NULL;
    }

  /* Serialize the private key to PKCS8 DER */

  pm = (struct pkey_pm *)pkey->pkey_pm;
  if (pm == NULL || pm->pkey == NULL)
    {
      return NULL;
    }

  key_der = mbedtls_calloc(1, key_der_size);
  if (key_der == NULL)
    {
      return NULL;
    }

  key_der_len = mbedtls_pk_write_key_der(pm->pkey, key_der, key_der_size);
  if (key_der_len < 0)
    {
      ret = key_der_len;
      goto fail;
    }

  /* Wrap raw key in PKCS8 PrivateKeyInfo:
   * SEQUENCE { version INTEGER(0), algorithm AlgId,
   *            privateKey OCTET STRING }
   */

  pkcs8_size = (size_t)key_der_len + 64;
  pkcs8_buf = mbedtls_calloc(1, pkcs8_size);
  if (pkcs8_buf == NULL)
    {
      ret = -1;
      goto fail;
    }

  pk8p = pkcs8_buf + pkcs8_size;
  ret = pkcs12_write_pkcs8(&pk8p, pkcs8_buf,
                           key_der + key_der_size - key_der_len,
                           (size_t)key_der_len);
  if (ret < 0)
    {
      mbedtls_platform_zeroize(key_der, key_der_size);
      mbedtls_free(pkcs8_buf);
      goto fail;
    }

  mbedtls_platform_zeroize(key_der, key_der_size);
  mbedtls_free(key_der);
  key_der = pkcs8_buf;
  key_der_size = pkcs8_size;
  key_der_len = ret;

  /* Encrypt the private key */

  ret = pkcs12_pbe_crypt_enc(key_der + key_der_size - key_der_len,
                             (size_t)key_der_len,
                             (const unsigned char *)pwd, pwd_len, iter,
                             &enc_key, &enc_key_len,
                             pbe_params_key, &pbe_params_key_len);
  mbedtls_platform_zeroize(key_der, key_der_size);
  if (ret != 0)
    {
      goto fail;
    }

  /* Build cert SafeContents DER (unencrypted), then encrypt it */

  cert_pm = (struct x509_pm *)cert->x509_pm;
  if (cert_pm == NULL)
    {
      ret = -1;
      goto fail;
    }

  cert_bags = mbedtls_calloc(1, cert_bags_size);
  if (cert_bags == NULL)
    {
      ret = -1;
      goto fail;
    }

  cp = cert_bags + cert_bags_size;
  cs = cert_bags;

  /* Write cert bags backwards: CA certs first (they end up last) */

  if (ca != NULL)
    {
      for (i = ca->num - 1; i >= 0; i--)
        {
          ca_cert = (X509 *)ca->data[i];
          if (ca_cert == NULL)
            {
              continue;
            }

          ca_pm = (struct x509_pm *)ca_cert->x509_pm;
          if (ca_pm == NULL)
            {
              continue;
            }

          ret = pkcs12_write_cert_bag(&cp, cs, &ca_pm->x509_crt);
          if (ret < 0)
            {
              goto fail;
            }

          cont_len += ret;
        }
    }

  /* Write the main cert (end entity) */

  ret = pkcs12_write_cert_bag(&cp, cs, &cert_pm->x509_crt);
  if (ret < 0)
    {
      goto fail;
    }

  cont_len += ret;

  /* Wrap in SafeContents SEQUENCE */

  ret = pkcs12_write_seq_hdr(&cp, cs, (size_t)cont_len);
  if (ret < 0)
    {
      goto fail;
    }

  cont_len += ret;

  /* Encrypt the cert SafeContents */

  ret = pkcs12_pbe_crypt_enc(cp, (size_t)cont_len,
                             (const unsigned char *)pwd, pwd_len, iter,
                             &enc_certs, &enc_certs_len,
                             pbe_params_cert, &pbe_params_cert_len);
  mbedtls_free(cert_bags);
  cert_bags = NULL;
  if (ret != 0)
    {
      goto fail;
    }

  /* Now build the final PFX DER (backwards) */

  buf = mbedtls_calloc(1, buflen);
  if (buf == NULL)
    {
      goto fail;
    }

  p = buf + buflen;
  start = buf;

  /* We build: PFX ::= SEQUENCE { version, authSafe, macData }.
   * Build authSafe first (backwards), then prepend MacData at the end.
   * AuthenticatedSafe ::= SEQUENCE OF ContentInfo
   *   ContentInfo 1: encryptedData (certs)
   *   ContentInfo 2: data (shrouded key bag)
   */

  ret = pkcs12_write_authsafe(&p, start,
                              enc_key, enc_key_len,
                              pbe_params_key, pbe_params_key_len,
                              enc_certs, enc_certs_len,
                              pbe_params_cert, pbe_params_cert_len,
                              &authsafe_content, &authsafe_content_len);
  if (ret < 0)
    {
      goto fail;
    }

  /* Compute the MAC (random salt + HMAC-SHA1) over the saved
   * AuthenticatedSafe content.  Since ASN.1 is written backwards, the
   * MacData is built into a separate buffer and concatenated afterwards.
   */

  ret = pkcs12_compute_mac(authsafe_content, authsafe_content_len,
                           bmp_pwd, bmp_pwd_len, mac_iter,
                           mac_salt, sizeof(mac_salt),
                           mac_digest);
  if (ret != 0)
    {
      goto fail;
    }

  /* Build MacData DER backwards:
   * MacData ::= SEQUENCE {
   *   mac DigestInfo ::= SEQUENCE { algorithm, digest },
   *   macSalt OCTET STRING,
   *   iterations INTEGER
   * }
   */

  ret = pkcs12_write_macdata(&mp, ms, mac_digest, 20,
                             mac_salt, sizeof(mac_salt), mac_iter);
  if (ret < 0)
    {
      goto fail;
    }

  mac_len = ret;

  /* Combine: PFX SEQUENCE = current content (version + authSafe) + MacData.
   * Current content is at p..buf+buflen; MacData at mp..mac_der+sizeof.
   */

  cur_len = (size_t)(buf + buflen - p);
  final_buf = mbedtls_calloc(1, cur_len + (size_t)mac_len + 16);
  if (final_buf == NULL)
    {
      goto fail;
    }

  /* Content: version + authSafe + macData */

  memcpy(final_buf, p, cur_len);
  memcpy(final_buf + cur_len, mp, (size_t)mac_len);

  /* Wrap in PFX outer SEQUENCE */

  total = cur_len + (size_t)mac_len;

  ret = pkcs12_write_seq_hdr(&sp, ss, total);
  if (ret < 0)
    {
      goto fail;
    }

  hdr_len = ret;

  /* Build final PKCS12 */

  p12 = mbedtls_calloc(1, sizeof(PKCS12));
  if (p12 == NULL)
    {
      mbedtls_free(final_buf);
      goto fail;
    }

  p12->len = (size_t)hdr_len + total;
  p12->data = mbedtls_calloc(1, p12->len);
  if (p12->data == NULL)
    {
      mbedtls_free(p12);
      mbedtls_free(final_buf);
      goto fail;
    }

  memcpy(p12->data, sp, (size_t)hdr_len);
  memcpy(p12->data + hdr_len, final_buf, total);
  mbedtls_free(final_buf);
  mbedtls_free(buf);
  mbedtls_free(key_der);
  mbedtls_free(enc_key);
  mbedtls_free(enc_certs);
  mbedtls_platform_zeroize(bmp_pwd, sizeof(bmp_pwd));
  return p12;

fail:
  mbedtls_free(buf);
  mbedtls_free(key_der);
  mbedtls_free(cert_bags);
  mbedtls_free(enc_key);
  mbedtls_free(enc_certs);
  mbedtls_platform_zeroize(bmp_pwd, sizeof(bmp_pwd));
  return NULL;
}

int PKCS12_newpass(PKCS12 *p12, const char *oldpass, const char *newpass)
{
  STACK_OF(X509) *ca = NULL;
  EVP_PKEY *pkey = NULL;
  X509 *cert = NULL;
  PKCS12 *new_p12;
  int ret;
  int i;

  if (p12 == NULL)
    {
      return 0;
    }

  ret = PKCS12_parse(p12, oldpass, &pkey, &cert, &ca);
  if (ret != 1)
    {
      return 0;
    }

  new_p12 = PKCS12_create(newpass, NULL, pkey, cert, ca,
                          0, 0, 0, 0, 0);

  EVP_PKEY_free(pkey);
  X509_free(cert);
  if (ca)
    {
      for (i = 0; i < ca->num; i++)
        {
          X509_free((X509 *)ca->data[i]);
        }

      OPENSSL_sk_free(ca);
    }

  if (new_p12 == NULL)
    {
      return 0;
    }

  /* Swap the DER data */

  if (p12->data)
    {
      mbedtls_platform_zeroize(p12->data, p12->len);
      mbedtls_free(p12->data);
    }

  p12->data = new_p12->data;
  p12->len = new_p12->len;
  new_p12->data = NULL;
  mbedtls_free(new_p12);

  return 1;
}

int i2d_PKCS12_bio(BIO *bp, const PKCS12 *p12)
{
  unsigned char *b = NULL;
  int ret = 1;
  int j = 0;
  int n;
  int i;

  if (bp == NULL || p12 == NULL)
    {
      return 0;
    }

  n = i2d_PKCS12(p12, &b);
  if (n < 0 || b == NULL)
    {
      return 0;
    }

  while (1)
    {
      i = BIO_write(bp, &b[j], n);
      if (i == n)
        {
          break;
        }

      if (i <= 0)
        {
          ret = 0;
          break;
        }

      j += i;
      n -= i;
    }

  mbedtls_free(b);
  return ret;
}

PKCS12 *d2i_PKCS12_bio(BIO *bp, PKCS12 **p12)
{
  const unsigned char *p;
  unsigned char *buf = NULL;
  unsigned char *tmp;
  size_t total = 0;
  size_t cap = 4096;
  PKCS12 *ret;
  int n;

  SSL_ASSERT2(bp);

  buf = mbedtls_calloc(1, cap);
  if (buf == NULL)
    {
      return NULL;
    }

  while ((n = BIO_read(bp, buf + total, (int)(cap - total))) > 0)
    {
      total += (size_t)n;
      if (total == cap)
        {
          if (cap > SIZE_MAX / 2)
            {
              mbedtls_free(buf);
              return NULL;
            }

          cap *= 2;
          tmp = mbedtls_calloc(1, cap);
          if (tmp == NULL)
            {
              mbedtls_free(buf);
              return NULL;
            }

          memcpy(tmp, buf, total);
          mbedtls_free(buf);
          buf = tmp;
        }
    }

  if (total == 0)
    {
      mbedtls_free(buf);
      return NULL;
    }

  p = buf;
  ret = d2i_PKCS12(p12, &p, (long)total);
  mbedtls_free(buf);
  return ret;
}

#endif /* MBEDTLS_PKCS12_C */
