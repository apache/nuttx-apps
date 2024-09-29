/****************************************************************************
 * apps/examples/mqttc_mbedtls/mqttc_mbedtls_pub.c
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

#include <mqtt.h>
#include "mbedtls_sockets.c"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *client_refresher(FAR void *client);
static void safe_exit(int status,
                      mqtt_pal_socket_handle sockfd,
                      pthread_t *client_daemon);

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

static FAR void *client_refresher(FAR void *client)
{
  while (1)
    {
      mqtt_sync((FAR struct mqtt_client *) client);
      usleep(100000U);
    }

  return NULL;
}

/****************************************************************************
 * Name: safe_exit
 *
 * Description:
 *   Safely closes the sockfd and cancels the client_daemon before exit.
 *
 ****************************************************************************/

static void safe_exit(int status,
                      mqtt_pal_socket_handle sockfd,
                      pthread_t *client_daemon)
{
  if (client_daemon != NULL)
    {
      pthread_cancel(*client_daemon);
    }

  mbedtls_ssl_free(sockfd);

  exit(status); /* XXX free the rest of contexts */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, const char *argv[])
{
  enum MQTTErrors mqtterr;
  const char *addr;
  const char *port;
  const char *topic;
  char msg[256] = "test_message";

  const char *ca_file =
  (char *)"-----BEGIN CERTIFICATE-----\n\
      MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\
      BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\
      A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\
      BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\
      by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\
      BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\
      MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\
      dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\
      KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\
      UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\
      Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\
      s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\
      3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\
      E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\
      MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\
      6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\
      BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\
      6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\
      +pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\
      sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\
      LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\
      m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n\
      -----END CERTIFICATE-----\n";

  struct mbedtls_context ctx;
  mqtt_pal_socket_handle sockfd;

  if (argc > 1)
    {
      ca_file = argv[1];
    }

  if (argc > 2) /* Get address (if present) */
    {
      addr = argv[2];
    }
  else
    {
      addr = "test.mosquitto.org";
    }

  if (argc > 3) /* Get port number (if present) */
    {
      port = argv[3];
    }
  else
    {
      port = "8883";
    }

  if (argc > 4) /* Get the topic name to publish */
    {
      topic = argv[4];
    }
  else
    {
      topic = "test_topic";
    }

  /* Open the non-blocking TCP socket (connecting to the broker) */

  open_nb_socket(&ctx, addr, port, ca_file);
  sockfd = &ctx.ssl_ctx;

  if (sockfd == NULL)
    {
      safe_exit(EXIT_FAILURE, sockfd, NULL);
    }

  /* Setup a client */

  struct mqtt_client client;
  uint8_t sendbuf[CONFIG_EXAMPLES_MQTTC_MBEDTLS_TXSIZE];
  uint8_t recvbuf[CONFIG_EXAMPLES_MQTTC_MBEDTLS_RXSIZE];
  mqtterr = mqtt_init(&client,
                      sockfd,
                      sendbuf,
                      sizeof(sendbuf),
                      recvbuf,
                      sizeof(recvbuf),
                      NULL);

  if (mqtterr != MQTT_OK)
    {
      fprintf(stderr, "ERROR! mqtt_init() failed.\n");
      safe_exit(EXIT_FAILURE, sockfd, NULL);
    }

  mqtterr = mqtt_connect(&client,
                          "publishing_client",
                          NULL,
                          NULL,
                          0,
                          NULL,
                          NULL,
                          0,
                          400);

  if (mqtterr != MQTT_OK)
    {
      fprintf(stderr, "ERROR! mqtt_connect() failed.\n");
      safe_exit(EXIT_FAILURE, sockfd, NULL);
    }

  /* Check for successful broker connection */

  if (client.error != MQTT_OK)
    {
      fprintf(stderr, "Error: %s\n", mqtt_error_str(client.error));
      safe_exit(EXIT_FAILURE, sockfd, NULL);
    }
  else
    {
      fprintf(stdout, "Success: Connected to broker!\n");
    }

  /* Start a thread to refresh the client (handle egress and ingress client
   * traffic)
   */

  pthread_t client_daemon;
  if (pthread_create(&client_daemon, NULL, client_refresher, &client))
    {
      fprintf(stderr, "Failed to start client daemon.\n");
      safe_exit(EXIT_FAILURE, sockfd, NULL);
    }

  fprintf(stdout, "%s is ready to begin publishing.\n", argv[0]);

  /* Print and publish a message */

  fprintf(stdout, "%s published : \"%s\"", argv[0], msg);

  mqtt_publish(&client, topic, msg, strlen(msg) + 1, MQTT_PUBLISH_QOS_2);

  /* Check for errors */

  if (client.error != MQTT_OK)
    {
      fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
      safe_exit(EXIT_FAILURE, sockfd, &client_daemon);
    }

  /* Disconnect */

  fprintf(stdout, "\n%s disconnecting from %s\n", argv[0], addr);
  sleep(1);

  /* Exit */

  safe_exit(EXIT_SUCCESS, sockfd, &client_daemon);
  return EXIT_SUCCESS;
}