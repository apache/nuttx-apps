/****************************************************************************
 * apps/examples/mqttc/mqttc_sub.c
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

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <mqtt.h>
#include <netutils/netlib.h>

#ifdef MQTT_USE_MBEDTLS
#  include <inttypes.h>

#  include <mbedtls/error.h>
#  include <mbedtls/entropy.h>
#  include <mbedtls/ctr_drbg.h>
#  include <mbedtls/net_sockets.h>
#  include <mbedtls/ssl.h>

#  include "cert.inc"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#  define EXTRA_OPT "c:"
#else
#  define EXTRA_OPT ""
#endif

/* Seconds to wait between failed connection attempts in the reconnect
 * callback.
 */

#define MQTTC_RECONNECT_DELAY_S 2

/* Network interface to wait on before the first connection attempt, and the
 * polling parameters for that wait.  Change MQTTC_NETIF to match the board's
 * network device (e.g. "eth0" for wired Ethernet).
 */

#define MQTTC_NETIF             "wlan0"
#define MQTTC_NET_POLL_MS       200
#define MQTTC_NET_WAIT_MS       30000

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

/* State handed to the reconnect callback so it can rebuild the transport
 * connection and reconfigure the MQTT session from scratch.
 */

struct mqttc_reconnect_s
{
  FAR const struct mqttc_cfg_s *cfg;
  FAR struct mqtt_conn_context_s *conn;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *client_refresher(FAR void *data);
static void parsearg(int argc, FAR char *argv[],
                    FAR struct mqttc_cfg_s *cfg);
static int init_conn(FAR const struct mqttc_cfg_s *cfg,
                     FAR struct mqtt_conn_context_s *ctx,
                     FAR mqtt_pal_socket_handle *handle);
static void close_conn(FAR struct mqtt_conn_context_s *ctx);
static void reconnect_client(FAR struct mqtt_client *client,
                             FAR void **state);
static void wait_for_network(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef MQTT_USE_MBEDTLS

/****************************************************************************
 * Name: mqttc_net_recv
 *
 * Description:
 *   mbedTLS BIO receive callback for non-blocking sockets.  The stock
 *   mbedtls_net_recv() relies on fcntl(F_GETFL) reporting O_NONBLOCK to
 *   classify EAGAIN as "would block".  On NuttX, F_SETFL applies the
 *   non-blocking mode via FIONBIO but does not retain O_NONBLOCK in the
 *   file status flags, so F_GETFL never reports it.  The stock callback
 *   therefore turns a benign EAGAIN into MBEDTLS_ERR_NET_RECV_FAILED.  This
 *   wrapper inspects errno directly and returns MBEDTLS_ERR_SSL_WANT_READ
 *   so callers (e.g. MQTT-C's mqtt_sync) keep retrying instead of aborting.
 *
 ****************************************************************************/

static int mqttc_net_recv(FAR void *ctx, FAR unsigned char *buf, size_t len)
{
  int fd = ((FAR mbedtls_net_context *)ctx)->fd;
  int ret;

  ret = recv(fd, buf, len, 0);
  if (ret < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
          return MBEDTLS_ERR_SSL_WANT_READ;
        }

      if (errno == EPIPE || errno == ECONNRESET)
        {
          return MBEDTLS_ERR_NET_CONN_RESET;
        }

      return MBEDTLS_ERR_NET_RECV_FAILED;
    }

  return ret;
}

/****************************************************************************
 * Name: mqttc_net_send
 *
 * Description:
 *   mbedTLS BIO send callback, mirroring mqttc_net_recv() for the same
 *   non-blocking/EAGAIN reason on the transmit side.
 *
 ****************************************************************************/

static int mqttc_net_send(FAR void *ctx, FAR const unsigned char *buf,
                          size_t len)
{
  int fd = ((FAR mbedtls_net_context *)ctx)->fd;
  int ret;

  ret = send(fd, buf, len, 0);
  if (ret < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
          return MBEDTLS_ERR_SSL_WANT_WRITE;
        }

      if (errno == EPIPE || errno == ECONNRESET)
        {
          return MBEDTLS_ERR_NET_CONN_RESET;
        }

      return MBEDTLS_ERR_NET_SEND_FAILED;
    }

  return ret;
}

#endif /* MQTT_USE_MBEDTLS */

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
 * Name: publish_callback
 *
 * Description:
 *   Print received publish message.
 *
 ****************************************************************************/

void publish_callback(void** unused, struct mqtt_response_publish *published)
{
  /* this is a byte stream, we have to convert it to C string */

  char message[CONFIG_EXAMPLES_MQTTC_RXSIZE];

  /* convert to C string (null terminated) */

  size_t message_size = published->application_message_size;
  if (message_size >= CONFIG_EXAMPLES_MQTTC_RXSIZE)
    {
      message_size = CONFIG_EXAMPLES_MQTTC_RXSIZE - 1;
    }

  memcpy(message, published->application_message, message_size);
  message[message_size] = '\0';

  printf("Received: %s\n", message);
}

/****************************************************************************
 * Name: parsearg
 *
 * Description:
 *   Parse command line arguments.
 *
 ****************************************************************************/

static void parsearg(int argc, FAR char *argv[],
                     FAR struct mqttc_cfg_s *cfg)
{
  int opt;

  while ((opt = getopt(argc, argv, "h:p:t:q:" EXTRA_OPT)) != ERROR)
    {
      switch (opt)
        {
          case 'h':
            cfg->host = optarg;
            break;

          case 'p':
            cfg->port = optarg;
            break;

          case 't':
            cfg->topic = optarg;
            break;

          case 'q':
            switch (strtol(optarg, NULL, 10))
              {
                case 0L:
                  cfg->qos = 0;
                  break;
                case 1L:
                  cfg->qos = 1;
                  break;
                case 2L:
                  cfg->qos = 2;
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
                      mqttc_net_send, mqttc_net_recv, NULL);

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
 * Name: wait_for_network
 *
 * Description:
 *   Block until the network interface has a usable IPv4 address (i.e. the
 *   link is up and DHCP has completed) or a timeout elapses.  This avoids
 *   the spurious first-attempt connect() failure that happens when the
 *   example starts before Wi-Fi association/DHCP have finished.
 *
 ****************************************************************************/

static void wait_for_network(void)
{
  struct in_addr addr;
  int waited = 0;

  for (; ; )
    {
      if (netlib_get_ipv4addr(MQTTC_NETIF, &addr) == 0 &&
          addr.s_addr != INADDR_ANY &&
          addr.s_addr != htonl(INADDR_LOOPBACK))
        {
          return;
        }

      if (waited >= MQTTC_NET_WAIT_MS)
        {
          return;
        }

      usleep(MQTTC_NET_POLL_MS * 1000);
      waited += MQTTC_NET_POLL_MS;
    }
}

/****************************************************************************
 * Name: reconnect_client
 *
 * Description:
 *   Reconnect callback registered with mqtt_init_reconnect().  MQTT-C calls
 *   this whenever the client enters an error state, and once at start-up
 *   (with client->error == MQTT_ERROR_INITIAL_RECONNECT) to set up the
 *   initial session.  It tears down the previous connection, re-establishes
 *   the transport (TCP + optional TLS), reinitializes the client and then
 *   reconfigures the session (CONNECT + SUBSCRIBE).
 *
 ****************************************************************************/

static void reconnect_client(FAR struct mqtt_client *client,
                             FAR void **state)
{
  FAR struct mqttc_reconnect_s *rc =
    *((FAR struct mqttc_reconnect_s **)state);
  FAR const struct mqttc_cfg_s *cfg = rc->cfg;
  FAR struct mqtt_conn_context_s *conn = rc->conn;
  mqtt_pal_socket_handle socketfd;
  enum MQTTErrors mqtterr;

  /* Tear down the previous connection, unless this is the initial set-up
   * call (in which case there is nothing to clean up yet).
   */

  if (client->error != MQTT_ERROR_INITIAL_RECONNECT)
    {
      printf("reconnect_client: reconnecting after error \"%s\"\n",
             mqtt_error_str(client->error));
      close_conn(conn);
    }

  /* (Re)establish the transport connection.
   *
   * mqtt_sync() unconditionally proceeds to recv/send on client->socketfd
   * after this callback returns.  Until mqtt_reinit() runs, socketfd is the
   * sentinel value -1 set by mqtt_init_reconnect(); in the mbedTLS build
   * that is a pointer (0xffffffff) that mbedtls_ssl_read() would dereference
   * and crash on.  We must therefore not return until a connection has
   * actually been established, so retry with a short back-off on failure.
   */

  while (init_conn(cfg, conn, &socketfd) < 0)
    {
      printf("ERROR! init_conn() failed; retrying in %d s\n",
             MQTTC_RECONNECT_DELAY_S);
      sleep(MQTTC_RECONNECT_DELAY_S);
    }

  printf("Success: Connected to broker!\n");

  /* Reinitialize the client with the fresh socket/SSL handle. */

  mqtt_reinit(client, socketfd,
              conn->sendbuf, sizeof(conn->sendbuf),
              conn->recvbuf, sizeof(conn->recvbuf));

  /* Send the connection request to the broker. */

  mqtterr = mqtt_connect(client, cfg->id, NULL, NULL, 0, NULL, NULL,
                         cfg->flags, cfg->tmo);
  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_connect() failed: %s\n", mqtt_error_str(mqtterr));
      return;
    }

  /* (Re)subscribe to the configured topic. */

  mqtterr = mqtt_subscribe(client, cfg->topic, cfg->qos);
  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_subscribe() failed: %s\n",
             mqtt_error_str(mqtterr));
    }
}

int main(int argc, FAR char * argv[])
{
  struct mqtt_conn_context_s mqtt_conn =
    {
      0
    };

  struct mqttc_reconnect_s reconnect_state;
  int timeout = 100;
  enum MQTTErrors mqtterr;
  pthread_attr_t attr;
  pthread_t thrdid;
  struct mqttc_cfg_s mqtt_cfg =
    {
        .host = "broker.hivemq.com",
    #ifndef MQTT_USE_MBEDTLS
        .port = "1883",
    #else
        .port = "8883",
    #endif
        .topic = "test",
        .msg = "NULL",
        .flags = MQTT_CONNECT_CLEAN_SESSION,
        .tmo = 400,
        .id = NULL,
        .user = NULL,
        .pass = NULL,
        .qos = MQTT_PUBLISH_QOS_0
    };

  parsearg(argc, argv, &mqtt_cfg);

  /* Wait for the network to come up so the first connection attempt does not
   * fail with ENETUNREACH
   */

  wait_for_network();

  /* Bundle the configuration and connection context for the reconnect
   * callback.  mqtt_init_reconnect() leaves the client in the
   * MQTT_ERROR_INITIAL_RECONNECT state; the first mqtt_sync() (driven by the
   * refresher thread) invokes reconnect_client(), which establishes the
   * transport, calls mqtt_reinit() and then mqtt_connect()/mqtt_subscribe().
   */

  reconnect_state.cfg = &mqtt_cfg;
  reconnect_state.conn = &mqtt_conn;

  mqtt_init_reconnect(&mqtt_conn.client,
                      reconnect_client, &reconnect_state,
                      publish_callback);

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

  /* Wait for the initial MQTT connection to be acknowledged or time-out.
   * On time-out we keep going anyway: the reconnect callback will continue
   * retrying in the background.
   */

  while (!mqtt_conn.client.event_connect && --timeout > 0)
    {
      usleep(10000);
    }

  if (timeout == 0)
    {
      printf("WARNING! Initial connection timed out; "
             "reconnect will keep retrying.\n");
    }
  else
    {
      printf("Success: Connected to broker!\n");
    }

  printf("Listening for %s.\nType q to exit.\n\n", mqtt_cfg.topic);

  /* Wait for messages */

  while (fgetc(stdin) != 'q');

  /* Disconnect */

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

  pthread_cancel(thrdid);

err_with_conn:
  close_conn(&mqtt_conn);

  return 0;
}
