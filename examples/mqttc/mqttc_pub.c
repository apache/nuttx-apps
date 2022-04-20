/****************************************************************************
 * apps/examples/mqttc/mqttc_pub.c
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

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct mqttc_cfg_s
{
  struct mqtt_client client;
  FAR const char *host;
  FAR const char *port;
  FAR const char *topic;
  FAR const char *msg;
  FAR const char *id;
  uint8_t sendbuf[CONFIG_EXAMPLES_MQTTC_TXSIZE];
  uint8_t recvbuf[CONFIG_EXAMPLES_MQTTC_RXSIZE];
  uint32_t tmo;
  uint8_t flags;
  uint8_t qos;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *client_refresher(FAR void *data);
static void parsearg(int argc, FAR char *argv[], FAR struct mqttc_cfg_s *cfg,
                     FAR int *n);
static int initserver(FAR const struct mqttc_cfg_s *cfg);

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

  while ((opt = getopt(argc, argv, "h:p:m:t:n:")) != ERROR)
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

          default:
            fprintf(stderr, "ERROR: Unrecognized option\n");
            break;
        }
    }
}

/****************************************************************************
 * Name: initserver
 *
 * Description:
 *   Resolve server's name and try to establish a connection.
 *
 ****************************************************************************/

static int initserver(FAR const struct mqttc_cfg_s *cfg)
{
  struct addrinfo hints;
  FAR struct addrinfo *servinfo;
  FAR struct addrinfo *itr;
  int fd;
  int ret;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family  = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  printf("Connecting to %s:%s...\n", cfg->host, cfg->port);

  ret = getaddrinfo(cfg->host, cfg->port, &hints, &servinfo);
  if (ret != OK)
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
      printf("ERROR! fcntl() F_GETFL failed, errno: %d\n", errno);
      return -1;
    }

  ret = fcntl(fd, F_SETFL, ret | O_NONBLOCK);
  if (ret < 0)
    {
      printf("ERROR! fcntl() F_SETFL failed, errno: %d\n", errno);
      return -1;
    }

  return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int sockfd;
  enum MQTTErrors mqtterr;
  pthread_t thrdid;
  int n = 1;
  struct mqttc_cfg_s mqtt_cfg =
    {
      .host = "broker.hivemq.com",
      .port = "1883",
      .topic = "test",
      .msg = "test",
      .flags = MQTT_CONNECT_CLEAN_SESSION,
      .tmo = 400,
      .id = NULL,
      .qos = MQTT_PUBLISH_QOS_0,
    };

  parsearg(argc, argv, &mqtt_cfg, &n);
  sockfd = initserver(&mqtt_cfg);
  if (sockfd < 0)
    {
      return -1;
    }

  mqtterr = mqtt_init(&mqtt_cfg.client, sockfd,
                      mqtt_cfg.sendbuf, sizeof(mqtt_cfg.sendbuf),
                      mqtt_cfg.recvbuf, sizeof(mqtt_cfg.recvbuf),
                      NULL);
  if (mqtterr != MQTT_OK)
    {
      printf("ERRPR! mqtt_init() failed.\n");
      goto err_with_socket;
    }

  mqtterr = mqtt_connect(&mqtt_cfg.client, mqtt_cfg.id,
                         NULL, /* Will topic */
                         NULL, /* Will message */
                         0,    /* Will message size */
                         NULL, /* User name */
                         NULL, /* Password */
                         mqtt_cfg.flags, mqtt_cfg.tmo);

  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_connect() failed\n");
      goto err_with_socket;
    }

  if (mqtt_cfg.client.error != MQTT_OK)
    {
      printf("error: %s\n", mqtt_error_str(mqtt_cfg.client.error));
      goto err_with_socket;
    }
  else
    {
      printf("Success: Connected to broker!\n");
    }

  /* Start a thread to refresh the client (handle egress and ingree client
   * traffic)
   */

  if (pthread_create(&thrdid, NULL, client_refresher, &mqtt_cfg.client))
    {
      printf("ERROR! pthread_create() failed.\n");
      goto err_with_socket;
    }

  while (n--)
    {
      mqtterr = mqtt_publish(&mqtt_cfg.client, mqtt_cfg.topic,
                             mqtt_cfg.msg, strlen(mqtt_cfg.msg) + 1,
                             mqtt_cfg.qos);
      if (mqtterr != MQTT_OK)
        {
          printf("ERROR! mqtt_publish() failed\n");
          goto err_with_thrd;
        }

      if (mqtt_cfg.client.error != MQTT_OK)
        {
          printf("error: %s\n", mqtt_error_str(mqtt_cfg.client.error));
          goto err_with_thrd;
        }
      else
        {
          printf("Success: Published to broker!\n");
        }

      sleep(5);
    }

  printf("\nDisconnecting from %s\n\n", mqtt_cfg.host);
  mqtterr = mqtt_disconnect(&mqtt_cfg.client);
  if (mqtterr != MQTT_OK)
    {
      printf("ERROR! mqtt_disconnect() failed\n");
    }

  /* Force sending the DISCONNECT, the thread will be canceled before getting
   * the chance to sync this last packet.
   * Note however that close() would cleanly close the connection but only
   * through TCP (i.e. no MQTT DISCONNECT packet).
   */

  mqtt_sync(&mqtt_cfg.client);

err_with_thrd:
  pthread_cancel(thrdid);
err_with_socket:
  close(sockfd);

  return 0;
}

