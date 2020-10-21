/****************************************************************************
 * netutils/netcat/netcat_main.c
 * netcat networking application
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

#include <nuttx/config.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifndef NETCAT_PORT
# define NETCAT_PORT 31337
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int netcat_server(int argc, char * argv[])
{
  FILE * fout = stdout;
  struct sockaddr_in server;
  struct sockaddr_in client;
  int port = NETCAT_PORT;

  if ((1 < argc) && (0 == strcmp("-l", argv[1])))
    {
      if (2 < argc)
        {
          port = atoi(argv[2]);
        }

      if (3 < argc)
        {
          fout = fopen(argv[3], "w");
          if (0 > fout)
            {
              perror("error: io: Failed to create file");
              return 1;
            }
        }
    }

  int id;
  id = socket(AF_INET , SOCK_STREAM , 0);
  if (0 > id)
    {
      perror("error: net: Failed to create socket");
      return 2;
    }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if (0 > bind(id, (struct sockaddr *)&server , sizeof(server)))
    {
      perror("error: net: Failed to bind");
      return 3;
    }

  fprintf(stderr, "log: net: listening on :%d\n", port);
  listen(id , 3);
  int capacity = 256;
  char buf[capacity];
  socklen_t addrlen;
  int conn;
  while ((conn = accept(id, (struct sockaddr *)&client, &addrlen)))
    {
      int avail = 1;
      while (0 < avail)
        {
          avail = recv(conn, buf, capacity, 0);
          buf[avail] = 0;
          fprintf(fout, "%s", buf);
          int status = fflush(fout);
          if (0 != status)
            {
              perror("error: io: Failed to flush");
            }
        }
    }

  if (0 > conn)
    {
      perror("accept failed");
      return 4;
    }

  if (stdout != fout)
    {
      fclose(fout);
    }

  return EXIT_SUCCESS;
}

int netcat_client(int argc, char * argv[])
{
  FILE *fin = stdin;
  char *host = "127.0.0.1";
  int port = NETCAT_PORT;

  if (argc > 1)
    {
      host = argv[1];
    }

  if (argc > 2)
    {
      port = atoi(argv[2]);
    }

  if (argc > 3)
    {
      fin = fopen(argv[3], "r");
      if (0 > fin)
        {
          perror("error: io: Failed to create file");
          return 1;
        }
    }

  int id;
  id = socket(AF_INET , SOCK_STREAM , 0);
  if (0 > id)
    {
      perror("error: net: Failed to create socket");
      return 2;
    }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (1 != inet_pton(AF_INET, host, &server.sin_addr))
    {
      perror("error: net: Invalid host");
      return 3;
    }

  if (connect(id, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
      perror("error: net: Failed to connect");
      return 4;
    }

  int capacity = 256;
  char buf[capacity];
  int avail;
  while (true)
    {
      avail = -1;
      if (fgets(buf, capacity, fin))
        {
          avail = strnlen(buf, capacity);
        }

      if (avail < 0)
        {
          exit(EXIT_SUCCESS);
        }

      buf[avail] = 0;
      avail = write(id, buf, avail);
      printf("%s", buf);
      if (avail < 0)
        {
          perror("error: net: writing to socket");
          exit(1);
        }
    }

  if (stdout != fin)
    {
      fclose(fin);
    }

  return EXIT_SUCCESS;
}

/****************************************************************************
 * netcat_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int status = EXIT_SUCCESS;
  if (2 > argc)
    {
      fprintf(stderr,
              "Usage: netcat [-l] [destination] [port] [file]\n");
    }
  else if ((1 < argc) && (0 == strcmp("-l", argv[1])))
    {
      status = netcat_server(argc, argv);
    }
  else
    {
      status = netcat_client(argc, argv);
    }

  return status;
}
