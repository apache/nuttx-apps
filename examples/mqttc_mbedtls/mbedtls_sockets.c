/****************************************************************************
 * apps/examples/mqttc_mbedtls/mbedtls_sockets.c
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mbedtls/error.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mbedtls_context
{
  mbedtls_net_context net_ctx;
  mbedtls_ssl_context ssl_ctx;
  mbedtls_ssl_config ssl_conf;
  mbedtls_x509_crt ca_crt;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void failed(const char *fn, int rv);
static void cert_verify_failed(uint32_t rv);
static void open_nb_socket(struct mbedtls_context *ctx,
                           const char *hostname,
                           const char *port,
                           const char *ca_file);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: failed
 *
 * Description:
 *   Facilitates error display for mbedtls functions
 *
 ****************************************************************************/

static void failed(const char *fn, int rv)
{
  char buf[100];
  mbedtls_strerror(rv, buf, sizeof(buf));
  printf("%s failed with %x (%s)\n", fn, -rv, buf);
  exit(1);
}

/****************************************************************************
 * Name: cert_verify_failed
 *
 * Description:
 *   Facilitates display of the reason why a certificate verification
 *   failed
 *
 ****************************************************************************/

static void cert_verify_failed(uint32_t rv)
{
  char buf[512];
  mbedtls_x509_crt_verify_info(buf, sizeof(buf), "\t", rv);
  printf("Certificate verification failed (%0" PRIx32 ")\n%s\n"
          "Continuing without a valid certificate\n",
          rv, buf);
}

/****************************************************************************
 * Name: open_nb_socket
 *
 * Description:
 *   Opens a non-blocking mbed TLS connection
 *
 ****************************************************************************/

static void open_nb_socket(struct mbedtls_context *ctx,
                           const char *hostname,
                           const char *port,
                           const char *ca_file)
{
  const unsigned char *additional = (const unsigned char *)"RANDOM";
  size_t additional_len = 6;
  int rv;

  mbedtls_net_context *net_ctx = &ctx->net_ctx;
  mbedtls_ssl_context *ssl_ctx = &ctx->ssl_ctx;
  mbedtls_ssl_config *ssl_conf = &ctx->ssl_conf;
  mbedtls_x509_crt *ca_crt = &ctx->ca_crt;
  mbedtls_entropy_context *entropy = &ctx->entropy;
  mbedtls_ctr_drbg_context *ctr_drbg = &ctx->ctr_drbg;

  mbedtls_entropy_init(entropy);
  mbedtls_ctr_drbg_init(ctr_drbg);

  rv = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                             additional, additional_len);
  if (rv != 0)
    {
      failed("mbedtls_ctr_drbg_seed", rv);
    }

  mbedtls_x509_crt_init(ca_crt);
  rv = mbedtls_x509_crt_parse(ca_crt,
                              (const unsigned char *)ca_file,
                              strlen(ca_file) + 1);
  rv = mbedtls_x509_crt_parse(ca_crt,
                              (const unsigned char *)ca_file,
                              strlen(ca_file) + 1);
  if (rv != 0)
    {
      failed("mbedtls_x509_crt_parse", rv);
    }

  mbedtls_ssl_config_init(ssl_conf);
  rv = mbedtls_ssl_config_defaults(ssl_conf, MBEDTLS_SSL_IS_CLIENT,
                                   MBEDTLS_SSL_TRANSPORT_STREAM,
                                   MBEDTLS_SSL_PRESET_DEFAULT);
  if (rv != 0)
    {
      failed("mbedtls_ssl_config_defaults", rv);
    }

  mbedtls_ssl_conf_ca_chain(ssl_conf, ca_crt, NULL);
  mbedtls_ssl_conf_authmode(ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_rng(ssl_conf, mbedtls_ctr_drbg_random, ctr_drbg);

  mbedtls_net_init(net_ctx);
  rv = mbedtls_net_connect(net_ctx, hostname, port, MBEDTLS_NET_PROTO_TCP);
  if (rv != 0)
    {
      failed("mbedtls_net_connect", rv);
    }

  rv = mbedtls_net_set_nonblock(net_ctx);
  if (rv != 0)
    {
      failed("mbedtls_net_set_nonblock", rv);
    }

  mbedtls_ssl_init(ssl_ctx);
  rv = mbedtls_ssl_setup(ssl_ctx, ssl_conf);
  if (rv != 0)
    {
      failed("mbedtls_ssl_setup", rv);
    }

  rv = mbedtls_ssl_set_hostname(ssl_ctx, hostname);
  if (rv != 0)
    {
      failed("mbedtls_ssl_set_hostname", rv);
    }

  mbedtls_ssl_set_bio(ssl_ctx, net_ctx,
                      mbedtls_net_send, mbedtls_net_recv, NULL);

  for (; ; )
    {
      rv = mbedtls_ssl_handshake(ssl_ctx);
      uint32_t want = 0;

      if (rv == MBEDTLS_ERR_SSL_WANT_READ)
        {
          want |= MBEDTLS_NET_POLL_READ;
        }
      else if (rv == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          want |= MBEDTLS_NET_POLL_WRITE;
        }
      else
        {
          break;
        }
    }

  if (rv != 0)
    {
      failed("mbedtls_ssl_handshake", rv);
    }

  uint32_t result = mbedtls_ssl_get_verify_result(ssl_ctx);
  if (result != 0)
    {
      if (result == (uint32_t)-1)
        {
          failed("mbedtls_ssl_get_verify_result", (int)result);
        }
      else
        {
          cert_verify_failed(result);
        }
    }
}