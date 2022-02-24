/****************************************************************************
 * apps/netutils/netcat/netcat_main.c
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
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

int do_io(int infd,
          int outfd,
          char *buf,
          size_t buf_size)
{
  ssize_t avail;
  ssize_t written;

  while (true)
    {
      avail = read(infd, buf, buf_size);
      if (avail == 0)
        {
          break;
        }

      if (avail == -1)
        {
          perror("do_io: read error");
          return 5;
        }

      written = write(outfd, buf, avail);
      if (written == -1)
        {
          perror("do_io: write error");
          return 6;
        }
    }

  return EXIT_SUCCESS;
}

#ifdef CONFIG_NETUTILS_NETCAT_SENDFILE
int do_io_over_sendfile(int infd, int outfd, ssize_t len)
{
  off_t offset = 0;
  ssize_t written;

  while (len > 0)
    {
      written = sendfile(outfd, infd, &offset, len);

      if (written == -1 && errno == EAGAIN)
        {
          continue;
        }
      else if (written == -1)
        {
          perror("do_io: sendfile error");
          return 5;
        }

      len -= written;
    }

  return EXIT_SUCCESS;
}
#endif

int netcat_server(int argc, char * argv[])
{
  int id = -1;
  int outfd = STDOUT_FILENO;
  struct sockaddr_in server;
  struct sockaddr_in client;
  int port = NETCAT_PORT;
  int result = EXIT_SUCCESS;
  int conn;
  socklen_t addrlen;
  char *preallocated_iobuf = NULL;

  if ((1 < argc) && (0 == strcmp("-l", argv[1])))
    {
      if (2 < argc)
        {
          port = atoi(argv[2]);
        }

      if (3 < argc)
        {
          outfd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0777);
          if (outfd == -1)
            {
              perror("error: io: Failed to create file");
              outfd = STDOUT_FILENO;
              result = 1;
              goto out;
            }
        }
    }

  preallocated_iobuf = (char *)malloc(CONFIG_NETUTILS_NETCAT_BUFSIZE);
  if (preallocated_iobuf == NULL)
    {
      perror("error: malloc: Failed to allocate I/O buffer\n");
      result = 2;
      goto out;
    }

  id = socket(AF_INET , SOCK_STREAM , 0);
  if (0 > id)
    {
      perror("error: net: Failed to create socket");
      result = 2;
      goto out;
    }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  if (0 > bind(id, (struct sockaddr *)&server , sizeof(server)))
    {
      perror("error: net: Failed to bind");
      result = 3;
      goto out;
    }

  fprintf(stderr, "log: net: listening on :%d\n", port);
  if (listen(id , 3) == -1)
    {
      perror("error: net: Failed to listen");
      result = 7;
      goto out;
    }

  addrlen = sizeof(struct sockaddr_in);
  if ((conn = accept(id, (struct sockaddr *)&client, &addrlen)) != -1)
    {
      result = do_io(conn, outfd,
                     preallocated_iobuf, CONFIG_NETUTILS_NETCAT_BUFSIZE);
    }

  if (0 > conn)
    {
      perror("accept failed");
      result = 4;
      goto out;
    }

out:
  if (id != -1)
    {
      close(id);
    }

  if (preallocated_iobuf != NULL)
    {
      free(preallocated_iobuf);
    }

  if (outfd != STDOUT_FILENO)
    {
      close(outfd);
    }

  return result;
}

int netcat_client(int argc, char * argv[])
{
  int id = -1;
  int infd = STDIN_FILENO;
  char *host = "127.0.0.1";
  int port = NETCAT_PORT;
  int result = EXIT_SUCCESS;
  struct sockaddr_in server;
  char *preallocated_iobuf = NULL;
#ifdef CONFIG_NETUTILS_NETCAT_SENDFILE
  struct stat stat_buf;
#endif

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
      infd = open(argv[3], O_RDONLY);
      if (infd == -1)
        {
          perror("error: io: Failed to open file");
          infd = STDIN_FILENO;
          result = 1;
          goto out;
        }

#ifdef CONFIG_NETUTILS_NETCAT_SENDFILE
      if (fstat(infd, &stat_buf) == -1)
        {
          perror("error: fstat: Could not get the input file size");
          infd = STDIN_FILENO;
          result = 1;
          goto out;
        }
#endif
    }

  id = socket(AF_INET , SOCK_STREAM , 0);
  if (0 > id)
    {
      perror("error: net: Failed to create socket");
      result = 2;
      goto out;
    }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  if (1 != inet_pton(AF_INET, host, &server.sin_addr))
    {
      perror("error: net: Invalid host");
      result = 3;
      goto out;
    }

  if (connect(id, (struct sockaddr *) &server, sizeof(server)) < 0)
    {
      perror("error: net: Failed to connect");
      result = 4;
      goto out;
    }

#ifdef CONFIG_NETUTILS_NETCAT_SENDFILE
  if (argc > 3)
    {
      result = do_io_over_sendfile(infd, id, stat_buf.st_size);
    }
  else
#endif
    {
      preallocated_iobuf = (char *)malloc(CONFIG_NETUTILS_NETCAT_BUFSIZE);

      if (preallocated_iobuf == NULL)
        {
          perror("error: malloc: Failed to allocate I/O buffer\n");
          result = 2;
          goto out;
        }

      result = do_io(infd, id,
                     preallocated_iobuf, CONFIG_NETUTILS_NETCAT_BUFSIZE);
    }

out:
  if (id != -1)
    {
      close(id);
    }

  if (preallocated_iobuf != NULL)
    {
      free(preallocated_iobuf);
    }

  if (infd != STDIN_FILENO)
    {
      close(infd);
    }

  return result;
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
              "Usage: netcat <destination> [port] [file]\n"
              "Usage: netcat -l [port] [file]\n");
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
