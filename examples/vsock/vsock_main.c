/****************************************************************************
 * apps/examples/vsock/vsock_main.c
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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __NuttX__
#include <nuttx/config.h>
#include <sys/vm_sockets.h>
#else
#include <linux/vm_sockets.h>
#endif

#define vsockdbg(fmt, ...)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int check_buf(char *buf, size_t size)
{
  ssize_t i;

  for (i = 0; i < size; i++)
    {
      if (buf[i] != (i & 0xff))
        {
          printf("Error at %zu buf=%x exp=%lx\n", i, buf[i], i & 0xff);
          return -1;
        }
    }

  return 0;
}

static int safe_send(int s, char *buf, size_t len, int flags)
{
  int sent = 0;
  int ret;

  while (sent < len)
    {
      ret = send(s, buf + sent, len - sent, flags);
      if (ret < 0)
        {
          ret = -errno;
          printf("Send failed, ret=%u\n", ret);
          return ret;
        }
      else if (ret == 0)
        {
          return sent;
        }

      sent += ret;
    }

  return sent;
}

static int safe_recv(int s, char *buf, size_t len, int flags)
{
  int recvd = 0;
  int ret;

  while (recvd < len)
    {
      ret = recv(s, buf + recvd, len - recvd, 0);
      if (ret < 0)
        {
          ret = -errno;
          printf("Recv failed, ret=%u\n", ret);
          return ret;
        }
      else if (ret == 0)
        {
          return recvd;
        }

      recvd += ret;
    }

  return recvd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static int vsock_client(unsigned int port, size_t size,
                        size_t count, int cid)
{
  struct sockaddr_vm addr;
  size_t sendlen = 0;
  ssize_t ret;
  ssize_t i;
  char *buf;
  int s;

  printf("Vsock client test, port=%u, size=%zu, count=%zu, cid=%u\n",
         port, size, count, cid);
  s = socket(AF_VSOCK, SOCK_STREAM, 0);
  if (s < 0)
    {
      printf("Create socket failed, s=%d, err=%d\n", s, errno);
      return -errno;
    }

  memset(&addr, 0, sizeof(struct sockaddr_vm));
  addr.svm_family = AF_VSOCK;
  addr.svm_port = port;
  addr.svm_cid = cid;

  printf("Connect, s=%d\n", s);
  ret = connect(s, (const struct sockaddr *)&addr,
                sizeof(struct sockaddr_vm));
  if (ret < 0)
    {
      ret = -errno;
      printf("Connect failed, ret=%zu\n", ret);
      goto out;
    }

  buf = malloc(size);
  if (!buf)
    {
      printf("Malloc failed\n");
      ret = -ENOMEM;
      goto out;
    }

  printf("Send\n");
  for (i = 0; i < size; i++)
    {
      buf[i] = i & 0xff;
    }

  while (count-- > 0)
    {
      ret = safe_send(s, buf, size, 0);
      if (ret <= 0)
        {
          printf("Send failed, ret=%zu\n", ret);
          goto err;
        }

      vsockdbg("Round %zu send=%zu\n", count, ret);
      sendlen += ret;

      ret = safe_recv(s, buf, size, 0);
      if (ret <= 0)
        {
          printf("Recv failed, ret=%zu\n", ret);
          goto err;
        }

      ret = check_buf(buf, ret);
      assert(ret == 0);
    }

  printf("Send finish, sendlen=%zd\n", sendlen);

err:
  free(buf);
out:
  printf("Close\n");
  close(s);
  return ret;
}

static int vsock_server(unsigned int port, size_t size, int cid)
{
  struct sockaddr_vm peer_addr;
  struct sockaddr_vm addr;
  socklen_t peer_addr_size;
  size_t received = 0;
  ssize_t ret;
  char *buf;
  int peer_fd;
  int s;

  printf("Vsock server test, port=%u, size=%zu, cid=%u\n", port, size, cid);
  s = socket(AF_VSOCK, SOCK_STREAM, 0);
  if (s < 0)
    {
      printf("Create socket failed, s=%d, err=%d\n", s, errno);
      return -errno;
    }

  memset(&addr, 0, sizeof(struct sockaddr_vm));
  addr.svm_family = AF_VSOCK;
  addr.svm_port = port;
  addr.svm_cid = cid;

  ret = bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_vm));
  if (ret < 0)
    {
      ret = -errno;
      printf("Bind failed, ret=%zu\n", ret);
      goto out;
    }

  ret = listen(s, 8);
  if (ret < 0)
    {
      ret = -errno;
      printf("Listen failed, ret=%zu\n", ret);
      goto out;
    }

  peer_addr_size = sizeof(struct sockaddr_vm);
  peer_fd = accept(s, (struct sockaddr *)&peer_addr, &peer_addr_size);
  if (peer_fd < 0)
    {
      ret = -errno;
      printf("Accept failed, ret=%zu\n", ret);
      goto out;
    }

  printf("Accept peer_fd=%d\n", peer_fd);

  buf = malloc(size);
  if (!buf)
    {
      printf("Malloc failed\n");
      ret = -ENOMEM;
      goto out;
    }

  while ((ret = safe_recv(peer_fd, buf, size, 0)) > 0)
    {
      received += ret;
      ret = check_buf(buf, ret);
      assert(ret == 0);
      vsockdbg("Recv=%zd Received=%zu\n", ret, received);

      ret = safe_send(peer_fd, buf, size, 0);
      if (ret <= 0)
        {
          printf("Send failed, ret=%zu\n", ret);
          goto err;
        }
    }

  printf("Received finish, received=%zd\n", received);

err:
  free(buf);
out:
  printf("Close\n");
  close(s);
  return ret;
}

static int vsock_client_conn(unsigned int port, int count, int cid)
{
  struct sockaddr_vm addr;
  char buf[32];
  ssize_t ret;
  int success = 0;
  int fail = 0;
  int s;
  int i;

  memset(&addr, 0, sizeof(struct sockaddr_vm));
  addr.svm_family = AF_VSOCK;
  addr.svm_cid = cid;
  addr.svm_port = port;

  printf("Vsock server conn test: port=%u, count=%u, cid=%u\n",
         port, count, cid);
  for (i = 0; i < count; i++)
    {
      s = socket(AF_VSOCK, SOCK_STREAM, 0);
      if (s < 0)
        {
          printf("Create socket failed, s=%d, err=%d\n", s, errno);
          fail++;
          break;
        }

      ret = connect(s, (const struct sockaddr *)&addr, sizeof(addr));
      if (ret == 0)
        {
          snprintf(buf, sizeof(buf), "test_%u", i);
          if (i == count - 1)
            {
              snprintf(buf, sizeof(buf), "end");
            }

          ret = safe_send(s, buf, sizeof(buf), 0);
          if (ret <= 0)
            {
              printf("send() failed, ret=%zu\n", ret);
              fail++;
            }
          else
            {
              success++;
            }

          vsockdbg("Connection %u established\n", i + 1);
        }
      else
        {
          printf("connect() failed, ret = %zu\n", ret);
          fail++;
        }

      close(s);
    }

  printf("Connection attempts: %u (Success: %d, Failed: %d)\n",
         count, success, fail);
  return fail > 0 ? -ETIMEDOUT : 0;
}

static int vsock_server_conn(unsigned int port, int cid)
{
  struct sockaddr_vm addr;
  char buf[32];
  ssize_t ret;
  int success = 0;
  int fail = 0;
  int newfd;
  int s;

  s = socket(AF_VSOCK, SOCK_STREAM, 0);
  if (s < 0)
    {
      printf("Create socket failed, s=%d, err=%d\n", s, errno);
      return -errno;
    }

  memset(&addr, 0, sizeof(struct sockaddr_vm));
  addr.svm_family = AF_VSOCK;
  addr.svm_cid = cid;
  addr.svm_port = port;
  if (bind(s, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      printf("bind() failed");
      close(s);
      return -errno;
    }

  if (listen(s, SOMAXCONN) < 0)
    {
      printf("listen() failed");
      close(s);
      return -errno;
    }

  printf("Vsock client conn test: port=%u, cid=%u\n", port, cid);
  while (1)
    {
      newfd = accept(s, NULL, NULL);
      if (newfd < 0)
        {
          if (errno == EINTR)
            {
              printf("accept() interrupted\n");
              continue;
            }

          printf("accept() failed");
          fail++;
          break;
        }

      ret = safe_recv(newfd, buf, sizeof(buf), 0);
      if (ret <= 0)
        {
          printf("recv failed, ret = %zu\n", ret);
          fail++;
        }
      else
        {
          success++;
        }

      close(newfd);
      vsockdbg("Accepted connection success:%d\n", success);
      if (strcmp(buf, "end") == 0)
        {
          printf("Test conn end\n");
          break;
        }
    }

  close(s);
  printf("Total connections: %d (Success: %d, Failed: %d)\n",
         success + fail, success, fail);

  return fail > 0 ? -ETIMEDOUT : 0;
}

/****************************************************************************
 * vsock_main
 ****************************************************************************/

int main(int argc, char *argv[])
{
  int cid = VMADDR_CID_ANY;
  unsigned int port = 9999;
  const char *type = NULL;
  size_t count = 100;
  size_t size = 64;
  int is_server = 1;
  int ret = -EINVAL;
  int opt;

  if (argc < 4)
    {
      printf("Usage: vsock [-c <cid>] [-t <data/conn>] [-p <port>] "
             "[-i <size>] [-n <count>]\n\n"
             "       -c: means client, else is server\n"
             "       -t: means test type, data or conn\n"
             "         data: means test the data transfer process\n"
             "         conn: means test the connection establish and "
             "disconnect process\n"
             "       -p: PORT -- the port number to listen or connect\n"
             "       -i: SIZE -- the size of the data to send\n"
             "       -n: COUNT -- the number of the data to send\n"
          );

      return -EINVAL;
    }

  optind = 0;
  while ((opt = getopt(argc, argv, "c:t:p:i:n:?")) != -1)
    {
      switch (opt)
        {
          case 'c':
            is_server = 0;
            cid = strtoul(optarg, NULL, 0);
            break;

          case 't':
            type = optarg;
            break;

          case 'p':
            port = strtoul(optarg, NULL, 0);
            break;

          case 'i':
            size = strtoul(optarg, NULL, 0);
            break;

          case 'n':
            count = strtoul(optarg, NULL, 0);
            break;

          default:
            printf("Unknown option: %c\n", opt);
            break;
        }
    }

  if (is_server)
    {
      if (strcmp(type, "data") == 0)
        {
          ret = vsock_server(port, size, cid);
        }
      else if (strcmp(type, "conn") == 0)
        {
          ret = vsock_server_conn(port, cid);
        }
    }
  else
    {
      if (strcmp(type, "data") == 0)
        {
          ret = vsock_client(port, size, count, cid);
        }
      else if (strcmp(type, "conn") == 0)
        {
          ret = vsock_client_conn(port, count, cid);
        }
    }

  return ret;
}
