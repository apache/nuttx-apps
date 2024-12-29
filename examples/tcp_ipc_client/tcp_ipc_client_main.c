/****************************************************************************
 * apps/examples/tcp_ipc_client/tcp_ipc_client_main.c
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

/* This program consists of a client socket & custom messages that send
 * data (hex-string formatted data) to a server (server_tcp). Then,
 * server_tcp send this data over LoraWAN (using Radioenge LoRaWAn module)
 * Both client and server work on local network.
 * IMPORTANT NOTE:
 * In order to test client_tcp & server_tcp together, there are two
 * ways to proceed:
 * 1) Init server manually (command: SERVER &), and after successfull
 * server init, also init client manually (CLIENT 127.0.0.1)
 * 2) Init server automatically after boot using NuttShell start up scripts.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "protocol.h"

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

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void show_usage(FAR const char *progname)
{
  fprintf(stderr, "USAGE: %s <Server-IP>\n", progname);
  fprintf(stderr, "       %s -h\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<<Server-IP>: IP of server TCP/IP socket for IPC.\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

#define SOCKET_PORT                     5000
#define TCP_DATA_RCV_WITHOUT_FLAGS      0
#define RCV_BUFFER_SIZE                 520
#define SEND_BUFFER_SIZE                500
#define TIME_SECONDS_TO_SEND_NEXT_DATA  15

/****************************************************************************
 * Client_tcp_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int socket_client = 0;
  char rcv_buffer[RCV_BUFFER_SIZE];
  char buffer_to_send[SEND_BUFFER_SIZE];
  int bytes_read_from_server = 0;
  struct sockaddr_in serv_addr;
  protocolo_ipc tprotocol;

  /* Check if there are sufficient arguments passed to this program */

  if (argc != 2)
    {
      printf("\nNot enough parameters: %s.\n", argv[0]);
      return 1;
    }

  if (strcmp(argv[1], "-h") == 0)
    {
      show_usage(argv[0]);
      return EXIT_FAILURE;
    }

  /* Create client socket */

  memset(rcv_buffer, 0x00, sizeof(rcv_buffer));
  socket_client = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_client < 0)
    {
      perror("Failed to create client socket");
      exit(EXIT_FAILURE);
    }

  /* Connect to server socket */

  memset(&serv_addr, 0x00, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SOCKET_PORT);

  if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
      perror("Failed when executing inet_pton()");
      exit(EXIT_FAILURE);
    }

  if (connect(socket_client, (struct sockaddr *)&serv_addr,
              sizeof(serv_addr)) < 0)
    {
      perror("Failed to connect to server socket");
      exit(EXIT_FAILURE);
    }

  /* Countinuosly send server data to be forwarded to LPWAN transceiver */

  while (1)
    {
      /* Formats message to be sent to server (opcode, message size
       * and message content)
       */

      tprotocol.opcode = 'U';
      tprotocol.msg_size = 4;
      snprintf((char *)tprotocol.msg, sizeof(tprotocol.msg), "0102");

      /* Send message to server */

      memcpy(buffer_to_send, (unsigned char *)&tprotocol,
             sizeof(protocolo_ipc));
      write(socket_client, buffer_to_send, strlen(buffer_to_send));
      printf("Message sent to server!\n\n");

      /* Waits for server response */

      bytes_read_from_server = recv(socket_client, rcv_buffer,
                               sizeof(protocolo_ipc),
                               TCP_DATA_RCV_WITHOUT_FLAGS);

      if (bytes_read_from_server < 0)
        {
          perror("Failed to get server response");
          exit(EXIT_FAILURE);
        }
      else
        {
          /* Server response successfully received. Print it on the screen */

          memcpy((unsigned char *)&tprotocol, rcv_buffer,
                 sizeof(protocolo_ipc));
          printf("Protocol: opcode: %c\n", tprotocol.opcode);
          printf("Protocol: message size: %d\n", tprotocol.msg_size);
          printf("Protocol: message: %s\n", tprotocol.msg);
        }

      /* Wait to send again */

      sleep(TIME_SECONDS_TO_SEND_NEXT_DATA);
    }

  return 0;
}
