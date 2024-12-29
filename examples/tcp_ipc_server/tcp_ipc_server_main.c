/****************************************************************************
 * apps/examples/tcp_ipc_server/tcp_ipc_server_main.c
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

/* This program consists of a server socket & custom messages to establish
 * IPC for multiple applications (clients) and one process that controls
 * LoRaWAN connectivity (server). Both client and server work on local
 * network.
 * For more details about client side, see client-tcp example.
 *
 * IMPORTANT NOTE:
 * In order to test client_tcp & server_tcp together, there are two
 * ways to proceed:
 * 1) Init server manually (command: SERVER &), and after successfull
 * server init, also init client manually (CLIENT 127.0.0.1)
 * 2) init server automatically after boot using NuttShell start up scripts.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "protocol.h"
#include "lorawan/uart_lorawan_layer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int socket_clients_counter;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void *thread_socket_client(void *arg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define SOCKET_PORT                         5000
#define TX_BUFFER_SIZE                      200
#define RX_BUFFER_SIZE                      150
#define TCP_NO_FLAGS_WHEN_SENDING_DATA      0
#define IP_SERVER                           "127.0.0.1"
#define MAX_PENDING_SOCKET_CONNECTIONS      20
#define TIME_TO_CHECK_FOR_NEW_CLIENTS       500000

/****************************************************************************
 * Name: thread_socket_client
 * Description: socket client thread. Each sonnected cleint socket will
 *              instantiate a thread, which handles all data traffic between
 *              this client and server.
 * Parameters: thread arguments (in this case, file descriptor of client
 *             socket)
 * Return: nothing
 ****************************************************************************/

static void *thread_socket_client(void *arg)
{
  int socket_client_fd = *((int *)arg);
  char tx_buffer[TX_BUFFER_SIZE];
  unsigned char rx_buffer[RX_BUFFER_SIZE];
  int bytes_read_from_server = 0;
  int ret_send = 0;
  protocolo_ipc tprotocol;

  memset(tx_buffer, 0x00, sizeof(tx_buffer));
  memset(rx_buffer, 0x00, sizeof(rx_buffer));

  while (1)
    {
      bytes_read_from_server = read(socket_client_fd,
                                    rx_buffer,
                                    RX_BUFFER_SIZE);

      if (bytes_read_from_server == 0)
        {
          /* Socket disconnection has been detected.
           * This thread will be terminated.
           */

          printf("\n\rDisconnection has been detected.\n\r");
          break;
        }
      else
        {
          printf("Client request received! Comm. with client...\n");
          send_msg_to_lpwan (rx_buffer, &tprotocol);
          if (tprotocol.msg_size >= 0)
            {
              memcpy(tx_buffer,
                     (unsigned char *)&tprotocol,
                     sizeof(protocolo_ipc));
              ret_send = send(socket_client_fd,
                              tx_buffer,
                              sizeof(protocolo_ipc),
                              TCP_NO_FLAGS_WHEN_SENDING_DATA);

              if (ret_send > 0)
                {
                  printf("\r\nSucess: %d bytes sent to client\r\n",
                         ret_send);
                }
              else
                {
                  printf("\r\nError: fail to send %d bytes to client\r\n",
                         strlen(tx_buffer));
                }
            }
          else
            {
                printf("ERROR: invalid message size (<0)\n");
            }
        }
    }

  /* Terminate this thread */

  close(socket_client_fd);
  socket_clients_counter--;
  pthread_exit(NULL);
}

/****************************************************************************
 * Server_tcp_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int socket_server_fd = 0;
  int new_socket_client_fd = 0;
  int ret_bind = 0;
  struct sockaddr_in server_add;
  struct sockaddr_storage server_storage;
  socklen_t addr_size;
  pthread_t tid[MAX_PENDING_SOCKET_CONNECTIONS];
  config_lorawan_radioenge_t lora_config;

  socket_clients_counter = 0;

  /* Configuring LoRaWAN credentials */

  snprintf(lora_config.application_session_key, APP_SESSION_KEY_SIZE,
           "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00");
  snprintf(lora_config.network_session_key, NW_SESSION_KEY_SIZE,
           "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00");
  snprintf(lora_config.application_eui, APP_EUI_SIZE,
           "00:00:00:00:00:00:00:00");
  snprintf(lora_config.device_address, DEVICE_ADDRESS_SIZE,
           "00:00:00:00");
  snprintf(lora_config.channel_mask, CHANNEL_MASK_SIZE,
           "00FF:0000:0000:0000:0000:0000");
  lorawan_radioenge_init(lora_config);

  /* Create socket (server) */

  socket_server_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (socket_server_fd == 0)
    {
      perror("Failed to create server socket");
      exit(EXIT_FAILURE);
    }

  /* Fill addr structure and do socket bind operation */

  server_add.sin_family = AF_INET;
  server_add.sin_port = htons(SOCKET_PORT);
  server_add.sin_addr.s_addr = inet_addr(IP_SERVER);
  memset(server_add.sin_zero, '\0', sizeof server_add.sin_zero);

  ret_bind = bind(socket_server_fd, (struct sockaddr *)&server_add,
                  sizeof(server_add));
  if (ret_bind < 0)
    {
      perror("Failed to do socket bind operation");
      exit(EXIT_FAILURE);
    }

  /* Initiate listening process for client sockets connection requests.
   * The maximum number client sockets pending connection is
   * defined by MAX_PENDING_SOCKET_CONNECTIONS
   */

  if (listen(socket_server_fd, MAX_PENDING_SOCKET_CONNECTIONS) == 0)
    {
      printf("Listening for clients connections...\n");
    }
  else
    {
      perror("Failed to listen for clients connections");
    }

  while (1)
    {
      /* Wait for a new client socket connection */

      addr_size = sizeof server_storage;
      new_socket_client_fd = accept4(socket_server_fd,
                                     (struct sockaddr *)&server_storage,
                                     &addr_size, SOCK_CLOEXEC);

      if (new_socket_client_fd < 0)
        {
          perror("Failed to accept new client socket connection");
          exit(EXIT_FAILURE);
        }

      /* For each connected client socket, a new client thread
       * is instantiated.
       */

      if (pthread_create(&tid[socket_clients_counter++], NULL,
                        thread_socket_client, &new_socket_client_fd) != 0)
        {
          perror("Failed to instantiate a thread for new client socket\n");
        }

      if (socket_clients_counter <= MAX_PENDING_SOCKET_CONNECTIONS)
        {
          pthread_join(tid[socket_clients_counter++], NULL);
          socket_clients_counter++;
        }

      usleep(TIME_TO_CHECK_FOR_NEW_CLIENTS);
    }

  return 0;
}
