/****************************************************************************
 * apps/examples/mqttc/mqttc_pub.c
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <mqtt.h>

#ifdef MQTT_USE_MBEDTLS
#  include <inttypes.h>

#  include <mbedtls/error.h>
#  include <mbedtls/entropy.h>
#  include <mbedtls/ctr_drbg.h>
#  include <mbedtls/net_sockets.h>
#  include <mbedtls/ssl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* This is taken from mbedtls/tests/data_files/test-ca-sha256.crt. */

/* BEGIN FILE string macro TEST_CA_CRT_RSA_SHA256_PEM
 * mbedtls/tests/data_files/test-ca-sha256.crt
 */

#  define TEST_CA_CRT_RSA_SHA256_PEM                                       \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDQTCCAimgAwIBAgIBAzANBgkqhkiG9w0BAQsFADA7MQswCQYDVQQGEwJOTDER\r\n" \
    "MA8GA1UECgwIUG9sYXJTU0wxGTAXBgNVBAMMEFBvbGFyU1NMIFRlc3QgQ0EwHhcN\r\n" \
    "MTkwMjEwMTQ0NDAwWhcNMjkwMjEwMTQ0NDAwWjA7MQswCQYDVQQGEwJOTDERMA8G\r\n" \
    "A1UECgwIUG9sYXJTU0wxGTAXBgNVBAMMEFBvbGFyU1NMIFRlc3QgQ0EwggEiMA0G\r\n" \
    "CSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDA3zf8F7vglp0/ht6WMn1EpRagzSHx\r\n" \
    "mdTs6st8GFgIlKXsm8WL3xoemTiZhx57wI053zhdcHgH057Zk+i5clHFzqMwUqny\r\n" \
    "50BwFMtEonILwuVA+T7lpg6z+exKY8C4KQB0nFc7qKUEkHHxvYPZP9al4jwqj+8n\r\n" \
    "YMPGn8u67GB9t+aEMr5P+1gmIgNb1LTV+/Xjli5wwOQuvfwu7uJBVcA0Ln0kcmnL\r\n" \
    "R7EUQIN9Z/SG9jGr8XmksrUuEvmEF/Bibyc+E1ixVA0hmnM3oTDPb5Lc9un8rNsu\r\n" \
    "KNF+AksjoBXyOGVkCeoMbo4bF6BxyLObyavpw/LPh5aPgAIynplYb6LVAgMBAAGj\r\n" \
    "UDBOMAwGA1UdEwQFMAMBAf8wHQYDVR0OBBYEFLRa5KWz3tJS9rnVppUP6z68x/3/\r\n" \
    "MB8GA1UdIwQYMBaAFLRa5KWz3tJS9rnVppUP6z68x/3/MA0GCSqGSIb3DQEBCwUA\r\n" \
    "A4IBAQA4qFSCth2q22uJIdE4KGHJsJjVEfw2/xn+MkTvCMfxVrvmRvqCtjE4tKDl\r\n" \
    "oK4MxFOek07oDZwvtAT9ijn1hHftTNS7RH9zd/fxNpfcHnMZXVC4w4DNA1fSANtW\r\n" \
    "5sY1JB5Je9jScrsLSS+mAjyv0Ow3Hb2Bix8wu7xNNrV5fIf7Ubm+wt6SqEBxu3Kb\r\n" \
    "+EfObAT4huf3czznhH3C17ed6NSbXwoXfby7stWUDeRJv08RaFOykf/Aae7bY5PL\r\n" \
    "yTVrkAnikMntJ9YI+hNNYt3inqq11A5cN0+rVTst8UKCxzQ4GpvroSwPKTFkbMw4\r\n" \
    "/anT1dVxr/BtwJfiESoK3/4CeXR1\r\n"                                     \
    "-----END CERTIFICATE-----\r\n"
/* END FILE */

#  define EXTRA_OPT "c:"
#else
#  define EXTRA_OPT ""
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mqttc_cfg_s
{
  FAR const char *host;
  FAR const char *port;
  FAR const char *topic;
  FAR const char *msg;
  FAR const char *id;
  FAR const char *user;
  FAR const char *pass;
#ifdef MQTT_USE_MBEDTLS
  FAR const char *ca_file;
#endif
  uint32_t tmo;
  uint8_t flags;
  uint8_t qos;
};

struct mqtt_conn_context_s
{
  struct mqtt_client client;
#ifdef MQTT_USE_MBEDTLS
  mbedtls_net_context net_ctx;
  mbedtls_ssl_context ssl_ctx;
  mbedtls_ssl_config ssl_conf;
  mbedtls_x509_crt ca_crt;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
#endif
  uint8_t sendbuf[CONFIG_EXAMPLES_MQTTC_TXSIZE];
  uint8_t recvbuf[CONFIG_EXAMPLES_MQTTC_RXSIZE];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *client_refresher(FAR void *data);
static void parsearg(int argc, FAR char *argv[], FAR struct mqttc_cfg_s *cfg,
                     FAR int *n);
static int init_conn(FAR const struct mqttc_cfg_s *cfg,
                     FAR struct mqtt_conn_context_s *ctx,
                     FAR mqtt_pal_socket_handle *handle);
static void close_conn(FAR struct mqtt_conn_context_s *ctx);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: client_refresher
 *
 * Description:
 *   The client's refresher. This function triggers back-end routines to
 *   handle ingress/egress traffic to the broker.
 *
 ****************************************************************************/

static FAR void *client_refresher(FAR void *data)
{
  while (1)
    {
      mqtt_sync((FAR struct mqtt_client *)data);
      usleep(100000U);
    }

  return NULL;
}

/****************************************************************************
 * Name: parsearg
 *
 * Description:
 *   Parse command line arguments.
 *
 ****************************************************************************/

static void parsearg(int argc, FAR char *argv[],
                     FAR struct mqttc_cfg_s *cfg, FAR int *n)
{
  int opt;

  while ((opt = getopt(argc, argv, "h:p:m:t:n:q:" EXTRA_OPT)) != ERROR)
    {
      switch (opt)
        {
          case 'h':
            cfg->host = optarg;
            break;

          case 'p':
            cfg->port = optarg;
            break;

          case 'm':
            cfg->msg = optarg;
            break;

          case 't':
            cfg->topic = optarg;
            break;

          case 'n':
            *n = strtol(optarg, NULL, 10);
            break;

          case 'q':
            switch (strtol(optarg, NULL, 10))
              {
                case '0':
                  cfg->qos = MQTT_PUBLISH_QOS_0;
                  break;
                case '1':
                  cfg->qos = MQTT_PUBLISH_QOS_1;
                  break;
                case '2':
                  cfg->qos = MQTT_PUBLISH_QOS_2;
                  break;
                }
            break;

#ifdef MQTT_USE_MBEDTLS
          case 'c':
            cfg->ca_file = optarg;
            break;
#endif

          default:
            fprintf(stderr, "ERROR: Unrecognized option\n");
            break;
        }
    }
}

/****************************************************************************
 * Name: init_conn
 *
 * Description:
 *   Resolve server's name and try to establish a connection.
 *
 ****************************************************************************/

static int init_conn(FAR const struct mqttc_cfg_s *cfg,
                     FAR struct mqtt_conn_context_s *conn,
                     FAR mqtt_pal_socket_handle *socketfd)
{
#ifdef MQTT_USE_MBEDTLS
  FAR mbedtls_net_context *net_ctx = &conn->net_ctx;
  FAR mbedtls_ssl_context *ssl_ctx = &conn->ssl_ctx;
  FAR mbedtls_ssl_config *ssl_conf = &conn->ssl_conf;
  FAR mbedtls_x509_crt *ca_crt = &conn->ca_crt;
  FAR mbedtls_entropy_context *entropy = &conn->entropy;
  FAR mbedtls_ctr_drbg_context *ctr_drbg = &conn->ctr_drbg;
  uint32_t result;

  FAR const unsigned char *additional = (FAR const unsigned char *)"MQTT-C";
  size_t additional_len = 6;
#else
  struct addrinfo hints;
  FAR struct addrinfo *servinfo;
  FAR struct addrinfo *itr;
  int fd;
#endif
  int ret;

  printf("Connecting to %s:%s...\n", cfg->host, cfg->port);

#ifdef MQTT_USE_MBEDTLS
  mbedtls_entropy_init(entropy);
  mbedtls_ctr_drbg_init(ctr_drbg);
  ret = mbedtls_ctr_drbg_seed(ctr_drbg, mbedtls_entropy_func, entropy,
                              additional, additional_len);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_ctr_drbg_seed() failed: %d\n", ret);
      goto err_with_ctr_drbg;
    }

  mbedtls_x509_crt_init(ca_crt);
  if (cfg->ca_file != NULL)
    {
      ret = mbedtls_x509_crt_parse_file(ca_crt, cfg->ca_file);
    }
  else
    {
      ret = mbedtls_x509_crt_parse(ca_crt,
                (FAR const unsigned char *)TEST_CA_CRT_RSA_SHA256_PEM,
                sizeof(TEST_CA_CRT_RSA_SHA256_PEM));
    }

  if (ret != 0)
    {
      printf("ERROR! mbedtls_x509_crt_parse_file() failed: %d\n", ret);
      goto err_with_x509_crt;
    }

  mbedtls_ssl_config_init(ssl_conf);
  ret = mbedtls_ssl_config_defaults(ssl_conf, MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_ssl_config_defaults() failed: %d\n", ret);
      goto err_with_ssl_conf;
    }

  mbedtls_ssl_conf_ca_chain(ssl_conf, ca_crt, NULL);
  mbedtls_ssl_conf_authmode(ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
  mbedtls_ssl_conf_rng(ssl_conf, mbedtls_ctr_drbg_random, ctr_drbg);

  mbedtls_net_init(net_ctx);
  ret = mbedtls_net_connect(net_ctx, cfg->host, cfg->port,
                            MBEDTLS_NET_PROTO_TCP);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_net_connect() failed: %d\n", ret);
      goto err_with_net;
    }

  ret = mbedtls_net_set_nonblock(net_ctx);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_net_set_nonblock() failed: %d\n", ret);
      goto err_with_net;
    }

  mbedtls_ssl_init(ssl_ctx);
  ret = mbedtls_ssl_setup(ssl_ctx, ssl_conf);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_ssl_setup() failed: %d\n", ret);
      goto err_with_ssl;
    }

  ret = mbedtls_ssl_set_hostname(ssl_ctx, cfg->host);
  if (ret != 0)
    {
      printf("ERROR! mbedtls_ssl_set_hostname() failed: %d\n", ret);
      goto err_with_ssl;
    }

  mbedtls_ssl_set_bio(ssl_ctx, net_ctx,
                      mbedtls_net_send, mbedtls_net_recv, NULL);

  for (; ; )
    {
      uint32_t want = 0;
      ret = mbedtls_ssl_handshake(ssl_ctx);
      if (ret == MBEDTLS_ERR_SSL_WANT_READ)
        {
          want |= MBEDTLS_NET_POLL_READ;
        }
      else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE)
        {
          want |= MBEDTLS_NET_POLL_WRITE;
        }
      else
        {
          break;
        }

      ret = mbedtls_net_poll(net_ctx, want, -1);
      if (ret < 0)
        {
          printf("ERROR! mbedtls_net_poll() failed: %d\n", ret);
          goto err_with_ssl;
        }
    }

  if (ret != 0)
    {
      printf("ERROR! mbedtls_ssl_handshake() failed: %d\n", ret);
      goto err_with_ssl;
    }

  result = mbedtls_ssl_get_verify_result(ssl_ctx);
  if (result != 0)
    {
      if (result == 0xffffffff)
        {
          printf("ERROR! mbedtls_ssl_get_verify_result() failed\n");
          goto err_with_ssl;
        }
      else
        {
#if defined(MBEDTLS_X509_REMOVE_INFO)
          const char *buf = "";
#else
          char buf[512];
          mbedtls_x509_crt_verify_info(buf, sizeof(buf), "\t", result);
#endif
          printf("Certificate verification failed (%0" PRIx32 ")\n%s\n",
                 result, buf);

#ifndef CONFIG_EXAMPLES_MQTTC_ALLOW_UNVERIFIED_TLS
          goto err_with_ssl;
#endif
        }
    }

  *socketfd = ssl_ctx;

  return 0;

err_with_ssl:
  mbedtls_ssl_free(ssl_ctx);
err_with_net:
  mbedtls_net_free(net_ctx);
err_with_ssl_conf:
  mbedtls_ssl_config_free(ssl_conf);
err_with_x509_crt:
  mbedtls_x509_crt_free(ca_crt);
err_with_ctr_drbg:
  mbedtls_ctr_drbg_free(ctr_drbg);
  mbedtls_entropy_free(entropy);

  return -1;
#else
  memset(&hints, 0, sizeof(hints));
  hints.ai_family  = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  ret = getaddrinfo(cfg->host, cfg->port, &hints, &servinfo);
  if (ret != 0)
    {
      printf("ERROR! getaddrinfo() failed: %s\n", gai_strerror(ret));
      return -1;
    }

  itr = servinfo;
  do
    {
      fd = socket(itr->ai_family, itr->ai_socktype, itr->ai_protocol);
      if (fd < 0)
        {
          continue;
        }

      ret = connect(fd, itr->ai_addr, itr->ai_addrlen);
      if (ret == 0)
        {
          break;
        }

      close(fd);
      fd = -1;
    }
  while ((itr = itr->ai_next) != NULL);

  freeaddrinfo(servinfo);

  if (fd < 0)
    {
      printf("ERROR! Couldn't create socket\n");
      return -1;
    }

  ret = fcntl(fd, F_GETFL, 0);
  if (ret < 0)
    {
      close(fd);
      printf("ERROR! fcntl() F_GETFL failed, errno: %d\n", errno);
      return -1;
    }

  ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
  if (ret < 0)
    {
      close(fd);
      printf("ERROR! fcntl() F_SETFL failed, errno: %d\n", errno);
      return -1;
    }

  *socketfd = fd;

  return 0;
#endif
}

/****************************************************************************
 * Name: close_conn
 *
 * Description:
 *   Shut down connection to server established by init_conn.
 *
 ****************************************************************************/

static void close_conn(FAR struct mqtt_conn_context_s *conn)
{
#ifndef MQTT_USE_MBEDTLS
  close(conn->client.socketfd);
#else
  mbedtls_net_free(&conn->net_ctx);
  mbedtls_ssl_free(&conn->ssl_ctx);
  mbedtls_ssl_config_free(&conn->ssl_conf);
  mbedtls_x509_crt_free(&conn->ca_crt);
  mbedtls_ctr_drbg_free(&conn->ctr_drbg);
  mbedtls_entropy_free(&conn->entropy);
#endif
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct mqtt_conn_context_s mqtt_conn;
  mqtt_pal_socket_handle socketfd;
  int timeout = 100;
  enum MQTTErrors mqtterr;
  pthread_attr_t attr;
  pthread_t thrdid;
  int n = 1;
  struct mqttc_cfg_s mqtt_cfg =
    {
      .host = "broker.hivemq.com",
#ifndef MQTT_USE_MBEDTLS
      .port = "1883",
#else
      .port = "8883",
#endif
      .topic = "test",
      .msg = "test",
      .flags = MQTT_CONNECT_CLEAN_SESSION,
      .tmo = 400,
      .id = NULL,
      .user = NULL,
      .pass = NULL,
      .qos = MQTT_PUBLISH_QOS_0
    };

  parsearg(argc, argv, &mqtt_cfg, &n);

  if (init_conn(&mqtt_cfg, &mqtt_conn, &socketfd) < 0)
    {
      return -1;
    }

  mqtterr = mqtt_init(&mqtt_conn.client, socketfd,
                      mqtt_conn.sendbuf, sizeof(mqtt_conn.sendbuf),
                      mqtt_conn.recvbuf, sizeof(mqtt_conn.recvbuf),
                      NULL);
  if (mqtterr != MQTT_OK)
    {
      printf("ERRPR! mqtt_init() failed.\n");
      goto err_with_conn;
    }

  mqtterr = mqtt_connect(&mqtt_conn.client, mqtt_cfg.id,
                         NULL,          /* Will topic */
                         NULL,          /* Will message */
                         0,             /* Will message size */
                         mqtt_cfg.user, /* User name */
                         mqtt_cfg.pass, /* Password */
                         mqtt_cfg.flags, mqtt_cfg.tmo);

  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_connect() failed\n");
      goto err_with_conn;
    }

  if (mqtt_conn.client.error != MQTT_OK)
    {
      printf("error: %s\n", mqtt_error_str(mqtt_conn.client.error));
      goto err_with_conn;
    }
  else
    {
      printf("Success: Connected to broker!\n");
    }

  /* Start a thread to refresh the client (handle egress and ingress client
   * traffic)
   */

  if (pthread_attr_init(&attr) != 0)
    {
      printf("ERROR! pthread_attr_init() failed.\n");
      goto err_with_conn;
    }

  pthread_attr_setstacksize(&attr, CONFIG_EXAMPLES_MQTTC_STACKSIZE);

  if (pthread_create(&thrdid, &attr, client_refresher, &mqtt_conn.client))
    {
      printf("ERROR! pthread_create() failed.\n");
      goto err_with_conn;
    }

  /* Wait for MQTT ACK or time-out */

  while (!mqtt_conn.client.event_connect && --timeout > 0)
    {
      usleep(10000);
    }

  if (timeout == 0)
    {
      goto err_with_thrd;
    }

  while (n--)
    {
      mqtterr = mqtt_publish(&mqtt_conn.client, mqtt_cfg.topic,
                             mqtt_cfg.msg, strlen(mqtt_cfg.msg),
                             mqtt_cfg.qos);
      if (mqtterr != MQTT_OK)
        {
          printf("ERROR! mqtt_publish() failed\n");
          goto err_with_thrd;
        }

      if (mqtt_conn.client.error != MQTT_OK)
        {
          printf("error: %s\n", mqtt_error_str(mqtt_conn.client.error));
          goto err_with_thrd;
        }
      else
        {
          printf("Success: Published to broker!\n");
        }

      sleep(5);
    }

  printf("\nDisconnecting from %s\n\n", mqtt_cfg.host);
  mqtterr = mqtt_disconnect(&mqtt_conn.client);
  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_disconnect() failed\n");
    }

  /* Force sending the DISCONNECT, the thread will be canceled before getting
   * the chance to sync this last packet.
   * Note however that close() would cleanly close the connection but only
   * through TCP (i.e. no MQTT DISCONNECT packet).
   */

  mqtt_sync(&mqtt_conn.client);

err_with_thrd:
  pthread_cancel(thrdid);

err_with_conn:
  close_conn(&mqtt_conn);

  return 0;
}

